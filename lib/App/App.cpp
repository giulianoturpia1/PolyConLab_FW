#include "App.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <array>

#include "Chiller.hpp"
#include "Communications.hpp"
#include "Flowmeter.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "LevelSwitch.hpp"
#include "Logger.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"
#include "StateMachineResponseQueue.hpp"
#include "Thermocouple.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

#include "Config.hpp"
#include "FaultRegistry.hpp"

constexpr uint8_t kEthernetMacLength = 6;
constexpr uint8_t kValveCount = 2;
constexpr uint8_t kTemperatureSensorCount = 4;
constexpr char kAppTaskName[] = "app_task";

std::array<uint8_t, kEthernetMacLength> mac = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
std::array<uint8_t, 4> ip_addr = {192, 168, 1, 1};

namespace modules {

static const TaggedLogger kLog("App");

////////////////////////////////////// Public Methods //////////////////////////////////////

App& App::instance() {
    static App instance;
    return instance;
}

void App::begin() {
    hal_.serial_begin(115200);
    kLog.info("Iniciando Configuracion...");
    hal_.delay(config::kAppStartupDelayMs);

    // Initialize all system modules
    initialize_system();

    // Start the App task which will run the main loop
    create_task();

    kLog.info("Configuracion Completada");
}

////////////////////////////////////// Private Methods //////////////////////////////////////

App::App()
    : state_machine_(modules::StateMachine::instance()),
      hal_(modules::hal_general()),
      communications_(modules::Communications::instance()),
      heater_(modules::Heater::instance()),
      pump_(modules::Pump::instance()),
      chiller_(modules::Chiller::instance()),
      valves_(modules::SolenoidValve::instance()),
      thermocouples_(modules::Thermocouple::instance()),
      flowmeter_(modules::Flowmeter::instance()),
      level_switch_(modules::LevelSwitch::instance()) {}

void App::initialize_network_server() {
    communications_.initialize_server(mac, ip_addr, [this](const std::string& response) { hal_.serial_println(response); });
}

void App::initialize_devices() {
    // Reset the fault registry before each initialisation pass so that a
    // physical reset produces a fresh fault list.
    FaultRegistry::instance().clear();

    if (!pump_.begin(static_cast<uint32_t>(definitions::Pins::kPump))) {
        kLog.error("Fallo al inicializar PUMP.");
        FaultRegistry::instance().add("PUMP");
    }
    if (!chiller_.begin(static_cast<uint32_t>(definitions::Pins::kChiller))) {
        kLog.error("Fallo al inicializar CHILLER.");
        FaultRegistry::instance().add("CHILLER");
    }
    if (!heater_.begin(static_cast<uint32_t>(definitions::Pins::kHeater))) {
        kLog.error("Fallo al inicializar HEATER.");
        FaultRegistry::instance().add("HEATER");
    }

    const std::array<uint32_t, kValveCount> valve_pins = {static_cast<uint32_t>(definitions::Pins::kValve1),
                                                          static_cast<uint32_t>(definitions::Pins::kValve2)};
    // SolenoidValve::begin() returns void (GPIO setup never fails on ESP32).
    valves_.begin(valve_pins.data(), valve_pins.size());

    const std::array<uint32_t, kTemperatureSensorCount> i2c_addresses = {
        static_cast<uint32_t>(definitions::Addresses::kTempIn), static_cast<uint32_t>(definitions::Addresses::kTempOut),
        static_cast<uint32_t>(definitions::Addresses::kTempBath), static_cast<uint32_t>(definitions::Addresses::kTempMix)};
    if (!thermocouples_.begin(i2c_addresses.data(), i2c_addresses.size())) {
        kLog.error("Fallo al inicializar THERMOCOUPLES.");
        FaultRegistry::instance().add("THERMOCOUPLES");
    }
    if (!flowmeter_.begin(static_cast<uint32_t>(definitions::Pins::kFlowmeter))) {
        kLog.error("Fallo al inicializar FLOWMETER.");
        FaultRegistry::instance().add("FLOWMETER");
    }
    if (!level_switch_.begin(static_cast<uint32_t>(definitions::Pins::kLevelSwitch))) {
        kLog.error("Fallo al inicializar LEVEL_SWITCH.");
        FaultRegistry::instance().add("LEVEL_SWITCH");
    }

    if (FaultRegistry::instance().has_faults()) {
        kLog.warning("Dispositivos con fallo de inicializacion: " + FaultRegistry::instance().get());
    } else {
        kLog.info("Todos los dispositivos inicializados correctamente.");
    }
}

void App::create_tasks() {
    heater_.create_task();
    pump_.create_task();
    chiller_.create_task();
    valves_.create_task();
    thermocouples_.create_task();
    flowmeter_.create_task();
    level_switch_.create_task();
}

void App::set_response_queues() {
    // Set the response queue for each module to the State Machine's response queue
    heater_.set_response_queue(&modules::StateMachineResponseQueue::instance());
    pump_.set_response_queue(&modules::StateMachineResponseQueue::instance());
    chiller_.set_response_queue(&modules::StateMachineResponseQueue::instance());
    valves_.set_response_queue(&modules::StateMachineResponseQueue::instance());
    thermocouples_.set_response_queue(&modules::StateMachineResponseQueue::instance());
    flowmeter_.set_response_queue(&modules::StateMachineResponseQueue::instance());
    level_switch_.set_response_queue(&modules::StateMachineResponseQueue::instance());
}

void App::initialize_system() {
    // Initialize Ethernet server
    initialize_network_server();

    // Initialize Devices
    initialize_devices();

    // Create and Initialize State Machine
    state_machine_.initialize();
    state_machine_.create_task();
    // Wire deferred responses: for every command outcome the SM fires handle_response(),
    // which sends the formatted SCPI string to whichever transport originated the command.
    state_machine_.set_response_callback([](const definitions::Command& cmd) { Communications::instance().handle_response(cmd); });

    // Create Tasks for all modules except State Machine
    create_tasks();  // Initialized in main.cpp

    // Response Queue for each module.
    set_response_queues();
}

void App::create_task() {
    static bool task_started = false;
    if (task_started) {
        kLog.warning("La tarea de App ya esta en ejecucion.");
        return;
    }

    if (xTaskCreatePinnedToCore(App::task_entry, kAppTaskName, config::kAppTaskStackSize, nullptr, config::kAppTaskPriority, nullptr,
                                config::kSpiCoreId) != pdPASS) {
        kLog.error("No se pudo crear la tarea de App.");
        return;
    }
}

void App::tick() {
    // Delegate all communication polling to Communications module
    communications_.poll_ethernet();
    communications_.poll_serial();
}

void App::task_entry(void* param) {
    while (true) {
        App::instance().tick();
        vTaskDelay(pdMS_TO_TICKS(config::kAppTaskPeriodMs));
    }
}

}  // namespace modules
