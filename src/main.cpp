#include <Ethernet.h>

#include "Communications.hpp"
#include "Devices.hpp"
#include "HALGeneral.hpp"
#include "LocalServer.hpp"
#include "StateMachine.hpp"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // MAC address of the PLC
IPAddress ip(192, 168, 1, 1);                       // IP address of the PLC

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server_(80);  // Means the pc is listening for standard HTTP web traffic, acting as a website host

// Singleton References
auto& hal = modules::hal_general();
auto& local_server = modules::LocalServer::instance();
auto& communications = modules::Communications::instance();
auto& state_machine = modules::StateMachine::instance();

void configure_serial_response_callback() {
    communications.set_serial_response_callback([](const std::string& response) { hal.log(response.c_str()); });
}

void initialize_network_server() {
    local_server.initialize(server_);
    local_server.begin(mac, ip);
}

void initialize_modules() {
    setupDevices();
    state_machine.begin();
    initialize_network_server();
    configure_serial_response_callback();
}

void poll_serial_messages() {
    if (!Serial.available()) {
        return;
    }

    String arduino_msg = Serial.readStringUntil('\n');
    arduino_msg.trim();
    if (arduino_msg.length() == 0) {
        return;
    }

    std::string msg(arduino_msg.c_str());
    communications.handle_serial_message(msg);
}

void run_modules() {
    local_server.poll([](const std::string& msg, const modules::LocalServer::ResponseCallback& respond) {
        communications.set_ethernet_response_callback(respond);
        communications.handle_ethernet_message(msg);
    });
    poll_serial_messages();
}

void setup() {
    hal.serial_begin(115200);
    hal.log("Iniciando setup...");
    hal.delay(1000);

    initialize_modules();

    hal.log("Setup completado");
}

void loop() {
    run_modules();
}
