#include "App.hpp"

#include <Ethernet.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Chiller.hpp"
#include "Communications.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "LocalServer.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 1);
EthernetServer server_(80);
constexpr uint32_t kAppTaskPeriodMs = 20;
constexpr UBaseType_t kAppTaskPriority = 2;

namespace modules {

App& App::instance() {
    static App instance;
    return instance;
}

App::App()
    : hal_(modules::hal_general()),
      local_server_(modules::LocalServer::instance()),
      communications_(modules::Communications::instance()),
      heater_(modules::Heater::instance()),
      pump_(modules::Pump::instance()),
      chiller_(modules::Chiller::instance()),
      valves_(modules::SolenoidValve::instance()),
      state_machine_(modules::StateMachine::instance()) {}

void App::configure_serial_response_callback() {
    communications_.set_serial_response_callback([this](const std::string& response) { hal_.log(response.c_str()); });
}

void App::initialize_network_server() {
    local_server_.initialize(server_);
    local_server_.begin(mac, ip);
}

void App::initialize_modules() {
    initialize_devices();
    state_machine_.initialize();

    heater_.start_task();
    pump_.start_task();
    chiller_.start_task();
    valves_.start_task();
    state_machine_.start_task();

    initialize_network_server();
    configure_serial_response_callback();
}

void App::initialize_devices() {
    pump_.begin(static_cast<uint32_t>(definitions::Pins::kPump));
    chiller_.begin(static_cast<uint32_t>(definitions::Pins::kChiller));

    uint32_t valve_pins[] = {static_cast<uint32_t>(definitions::Pins::kValve1), static_cast<uint32_t>(definitions::Pins::kValve2)};
    valves_.begin(valve_pins, 2);
}

void App::poll_serial_messages() {
    if (!Serial.available()) {
        return;
    }

    String arduino_msg = Serial.readStringUntil('\n');
    arduino_msg.trim();
    if (arduino_msg.length() == 0) {
        return;
    }

    std::string msg(arduino_msg.c_str());
    communications_.handle_serial_message(msg);
}

void App::begin() {
    hal_.serial_begin(115200);
    hal_.log("Iniciando configuracion...");
    hal_.delay(1000);

    initialize_modules();
    start_task();

    hal_.log("Configuracion completada");
}

void App::start_task() {
    static bool task_started = false;
    if (task_started) {
        hal_.log("La tarea de App ya esta en ejecucion.");
        return;
    }

    if (xTaskCreate(App::task_entry, "app_task", 4096, nullptr, kAppTaskPriority, nullptr) != pdPASS) {
        hal_.log("No se pudo crear la tarea de App.");
        return;
    }

    task_started = true;
}

void App::task_entry(void* /*param*/) {
    while (true) {
        App::instance().tick();
        vTaskDelay(pdMS_TO_TICKS(kAppTaskPeriodMs));
    }
}

void App::tick() {
    local_server_.poll([this](const std::string& msg, const modules::LocalServer::ResponseCallback& respond) {
        communications_.set_ethernet_response_callback(respond);
        communications_.handle_ethernet_message(msg);
    });

    poll_serial_messages();
}

}  // namespace modules
