#include "App.hpp"
#include "Definitions.hpp"
#include "Devices.hpp"
#include "Pump.hpp"
#include "Heater.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

modules::Pump& pump = modules::Pump::instance(); // Global instance of the pump controller.
modules::Heater& heater = modules::Heater::instance();

int initTCPServer(byte mac[], IPAddress ip, EthernetServer& server_) {
    // start the Ethernet connection and the server:
    Ethernet.begin(mac, ip);

    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        return 0;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
        return 0;
    }

    // start the server
    server_.begin();
    Serial.print("Server is at: ");
    Serial.println(Ethernet.localIP());
    return 1;
}

// -------- COMMAND PROCESSING ---------

void processCommand(String msg, EthernetClient client) {
    String response = "";

    // -------- RESISTENCIA / HEATER ---------
    if (msg == "RES_ON") {
        heater.on();
        response = "RES:ON";
    } else if (msg == "RES_OFF") {
        heater.off();
        response = "RES:OFF";
    } else if (msg == "RES_STATE") {
        bool state = heater.status();
        response = "RES:STATE," + String(state ? "ON" : "OFF");
    }

    // -------- PUMP ---------
    else if (msg == "BBA_ON") {
        pump.on();
        response = "BBA:ON";
    } else if (msg == "BBA_OFF") {
        pump.off();
        response = "BBA:OFF";
    } else if (msg == "BBA_STATE") {
        bool state = pump.status();
        response = "BBA:STATE," + String(state ? "ON" : "OFF");
    }

    // -------- VALVE ---------
    else if (msg == "VALVE_OPN") {
        valveON(static_cast<uint32_t>(definitions::Pins::RelayAus1));
        response = "VALVE:OPEN";
    } else if (msg == "VALVE_CLS") {
        valveOFF(static_cast<uint32_t>(definitions::Pins::RelayAus1));
        response = "VALVE:CLOSED";
    } else if (msg == "VALVE_STATE") {
        bool state = valveStatus(static_cast<uint32_t>(definitions::Pins::RelayAus1));
        response = "VALVE:STATE," + String(state ? "OPEN" : "CLOSED");
    }

    // -------- TEMPERATURE READINGS ---------
    else if (msg == "LEER_TEMP_INCAMISA") {
        float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt1));
        response = "TEMP_IN:" + String(temp, 2);
    } else if (msg == "LEER_TEMP_OUTCAMISA") {
        float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt2));
        response = "TEMP_OUT:" + String(temp, 2);
    } else if (msg == "LEER_TEMP_CAMISA") {
        float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt3));
        response = "TEMP_BATH:" + String(temp, 2);
    } else if (msg == "LEER_TEMP_MEZCLA") {
        float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt4));
        response = "TEMP_MIX:" + String(temp, 2);
    }

    // -------- FLOW RATE READING ---------
    else if (msg == "LEER_CAUDAL") {
        float flow_rate = readFlowMeter();
        response = "CAUDAL:" + String(flow_rate, 2);
    }

    // -------- UNKNOWN COMMAND ---------
    else {
        response = "ERROR:UNKNOWN_COMMAND";
    }

    // Send response to client
    if (client.connected()) {
        client.println(response);
        Serial.println("Response sent: " + response);
    }
}
