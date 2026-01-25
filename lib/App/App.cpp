#include "App.hpp"

int initTCPServer(byte _mac[], IPAddress ip_, EthernetServer& server_) {
    // start the Ethernet connection and the server:
    Ethernet.begin(_mac, ip_);

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
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
    return 1;
}

// -------- COMMAND PROCESSING ---------

void processCommand(String msg, EthernetClient client) {
    String response = "";

    // -------- RESISTENCIA / HEATER ---------
    if (msg == "RES_ON") {
        heaterON(RELAYHEATER);
        response = "RES:ON";
    } else if (msg == "RES_OFF") {
        heaterOFF(RELAYHEATER);
        response = "RES:OFF";
    } else if (msg == "RES_STATE") {
        bool state = heaterStatus();
        response = "RES:STATE," + String(state ? "ON" : "OFF");
    }

    // -------- PUMP ---------
    else if (msg == "BBA_ON") {
        pumpON(RELAYPUMP);
        response = "BBA:ON";
    } else if (msg == "BBA_OFF") {
        pumpOFF(RELAYPUMP);
        response = "BBA:OFF";
    } else if (msg == "BBA_STATE") {
        bool state = statusPump();
        response = "BBA:STATE," + String(state ? "ON" : "OFF");
    }

    // -------- VALVE ---------
    else if (msg == "VALVE_OPN") {
        valveON(RELAY_AUS_1);
        response = "VALVE:OPEN";
    } else if (msg == "VALVE_CLS") {
        valveOFF(RELAY_AUS_1);
        response = "VALVE:CLOSED";
    } else if (msg == "VALVE_STATE") {
        bool state = valveStatus(RELAY_AUS_1);
        response = "VALVE:STATE," + String(state ? "OPEN" : "CLOSED");
    }

    // -------- TEMPERATURE READINGS ---------
    else if (msg == "LEER_TEMP_INCAMISA") {
        float temp = readTempSensor(RES_ST_1);
        response = "TEMP_IN:" + String(temp, 2);
    } else if (msg == "LEER_TEMP_OUTCAMISA") {
        float temp = readTempSensor(RES_ST_2);
        response = "TEMP_OUT:" + String(temp, 2);
    } else if (msg == "LEER_TEMP_CAMISA") {
        float temp = readTempSensor(RES_ST_3);
        response = "TEMP_BATH:" + String(temp, 2);
    } else if (msg == "LEER_TEMP_MEZCLA") {
        float temp = readTempSensor(RES_ST_4);
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
