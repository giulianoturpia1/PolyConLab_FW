#include <Ethernet.h>

#include "App.hpp"
#include "Devices.hpp"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // MAC address of the PLC
IPAddress ip(192, 168, 1, 1);                       // IP address of the PLC

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server_(80);  // Means the pc is listening for standard HTTP web traffic, acting as a website host

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando setup...");
    setupDevices();
    delay(1000);
    int comunicacion = initTCPServer(mac, ip, server_);
    Serial.println("Setup completado");
}

void loop() {
    EthernetClient client = server_.available();
    if (client) {
        while (client.connected()) {
            if (client.available()) {
                Serial.println("New client");
                String msg = client.readStringUntil('\n');
                msg.trim();
                Serial.println("Message recieved: " + msg);
                Serial.println("Processing command...");
                processCommand(msg, client);
            }
        }
    }
    // Serial.println("Esta pasando el tiempo..." + String(millis()));
}
