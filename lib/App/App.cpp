#include "App.hpp"

#include <Ethernet.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <array>

#include "Chiller.hpp"
#include "Communications.hpp"
#include "Flowmeter.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "LevelSwitch.hpp"
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

constexpr uint8_t kEthernetMacLength = 6;
constexpr uint16_t kAppServerPort = 80;
constexpr uint8_t kValveCount = 2;
constexpr uint8_t kTemperatureSensorCount = 4;
constexpr uint32_t kAppStartupDelayMs = 1000;
constexpr uint32_t kAppTaskPeriodMs = 20;
constexpr UBaseType_t kAppTaskPriority = 2;
constexpr uint32_t kAppTaskStackSize = 4096;
constexpr char kAppTaskName[] = "app_task";

std::array<byte, kEthernetMacLength> mac = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 1);
EthernetServer server_(kAppServerPort);

namespace modules {

////////////////////////////////////// Public Methods //////////////////////////////////////

App& App::instance() {
    static App instance;
    return instance;
}

void App::begin() {
    hal_.serial_begin(115200);
    hal_.log("Iniciando Configuracion...");
    hal_.delay(kAppStartupDelayMs);

    // Initialize all system modules
    initialize_system();

    // Start the App task which will run the main loop
    create_task();

    hal_.log("Configuracion Completada");
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
    // Initialize Ethernet server and related settings
    communications_.initialize_server(server_, mac, ip);
    communications_.set_serial_response_callback([this](const std::string& response) { hal_.log(response.c_str()); });
}

void App::initialize_devices() {
    pump_.begin(static_cast<uint32_t>(definitions::Pins::kPump));
    chiller_.begin(static_cast<uint32_t>(definitions::Pins::kChiller));
    heater_.begin(static_cast<uint32_t>(definitions::Pins::kHeater));

    const std::array<uint32_t, kValveCount> valve_pins = {static_cast<uint32_t>(definitions::Pins::kValve1),
                                                          static_cast<uint32_t>(definitions::Pins::kValve2)};
    valves_.begin(valve_pins.data(), valve_pins.size());

    const std::array<uint32_t, kTemperatureSensorCount> i2c_addresses = {
        static_cast<uint32_t>(definitions::Addresses::kTempIn), static_cast<uint32_t>(definitions::Addresses::kTempOut),
        static_cast<uint32_t>(definitions::Addresses::kTempBath), static_cast<uint32_t>(definitions::Addresses::kTempMix)};
    thermocouples_.begin(i2c_addresses.data(), i2c_addresses.size());
    flowmeter_.begin(static_cast<uint32_t>(definitions::Pins::kFlowmeter));
    level_switch_.begin(static_cast<uint32_t>(definitions::Pins::kLevelSwitch));
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
    // Initialize Network Server
    initialize_network_server();

    // Initialize Devices
    initialize_devices();

    // Create and Initialize State Machine
    state_machine_.initialize();
    state_machine_.create_task();

    // Create Tasks for all modules except State Machine
    create_tasks();  // Initialized in main.cpp

    // Response Queue for each module.
    set_response_queues();
}

void App::create_task() {
    static bool task_started = false;
    if (task_started) {
        hal_.log("La tarea de App ya esta en ejecucion.");
        return;
    }

    if (xTaskCreate(App::task_entry, kAppTaskName, kAppTaskStackSize, nullptr, kAppTaskPriority, nullptr) != pdPASS) {
        hal_.log("No se pudo crear la tarea de App.");
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
        vTaskDelay(pdMS_TO_TICKS(kAppTaskPeriodMs));
    }
}

}  // namespace modules
