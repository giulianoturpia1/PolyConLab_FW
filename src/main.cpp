#include <Ethernet.h>

#include "Communications.hpp"
#include "Devices.hpp"
#include "HALGeneral.hpp"
#include "LocalServer.hpp"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // MAC address of the PLC
IPAddress ip(192, 168, 1, 1);                       // IP address of the PLC

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server_(80);  // Means the pc is listening for standard HTTP web traffic, acting as a website host

void setup() {
    modules::hal_general().serial_begin(115200);
    modules::hal_general().log("Iniciando setup...");
    // setupDevices();
    modules::hal_general().delay(1000);

    // Initialize LocalServer with the EthernetServer
    modules::LocalServer::instance().initialize(server_);
    modules::LocalServer::instance().begin(mac, ip);

    // Set up Serial response callback
    modules::Communications::instance().set_serial_response_callback(
        [](const std::string& response) {
            Serial.println(response.c_str());
        });

    modules::hal_general().log("Setup completado");
}

void loop() {
    // Poll Ethernet
    modules::LocalServer::instance().poll(modules::Communications::instance());

    // Poll Serial
    if (Serial.available()) {
        String arduino_msg = Serial.readStringUntil('\n');
        arduino_msg.trim();
        if (arduino_msg.length() > 0) {
            std::string msg(arduino_msg.c_str());
            modules::Communications::instance().handle_serial_message(msg);
        }
    }
}
