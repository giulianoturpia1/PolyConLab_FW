#include "App.hpp"

int initServidorTCP(byte _mac[], IPAddress ip_, EthernetServer& server_) {
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

void procesarComando(String msg, EthernetClient client) {

  String respuesta = "";

  // -------- RESISTENCIA / HEATER ---------
  if (msg == "RES_ON") {
    heaterON(RELAYHEATER);
    respuesta = "RES:ON";
  }
  else if (msg == "RES_OFF") {
    heaterOFF(RELAYHEATER);
    respuesta = "RES:OFF";
  }
  else if (msg == "RES_STATE") {
    bool estado = heaterStatus();
    respuesta = "RES:STATE," + String(estado ? "ON" : "OFF");
  }

  // -------- BOMBA ---------
  else if (msg == "BBA_ON") {
    bombaON(RELAYBOMBA);
    respuesta = "BBA:ON";
  }
  else if (msg == "BBA_OFF") {
    bombaOFF(RELAYBOMBA);
    respuesta = "BBA:OFF";
  }
  else if (msg == "BBA_STATE") {
    bool estado = statusPump();
    respuesta = "BBA:STATE," + String(estado ? "ON" : "OFF");
  }

  // -------- VÁLVULA ---------
  else if (msg == "VALVE_OPN") {
    valveON(RELAY_AUS_1);
    respuesta = "VALVE:OPEN";
  }
  else if (msg == "VALVE_CLS") {
    valveOFF(RELAY_AUS_1);
    respuesta = "VALVE:CLOSED";
  }
  else if (msg == "VALVE_STATE") {
    bool estado = valveStatus(RELAY_AUS_1);
    respuesta = "VALVE:STATE," + String(estado ? "OPEN" : "CLOSED");
  }

  // -------- LECTURAS DE TEMPERATURA ---------
  else if (msg == "LEER_TEMP_INCAMISA") {
    float temp = leerSensorTemp(RES_ST_1);
    respuesta = "TEMP_IN:" + String(temp, 2);
  }
  else if (msg == "LEER_TEMP_OUTCAMISA") {
    float temp = leerSensorTemp(RES_ST_2);
    respuesta = "TEMP_OUT:" + String(temp, 2);
  }
  else if (msg == "LEER_TEMP_CAMISA") {
    float temp = leerSensorTemp(RES_ST_3);
    respuesta = "TEMP_BATH:" + String(temp, 2);
  }
  else if (msg == "LEER_TEMP_MEZCLA") {
    float temp = leerSensorTemp(RES_ST_4);
    respuesta = "TEMP_MIX:" + String(temp, 2);
  }

  // -------- LECTURA DE CAUDAL ---------
  else if (msg == "LEER_CAUDAL") {
    float caudal = leerCaudalimetro();
    respuesta = "CAUDAL:" + String(caudal, 2);
  }

  // -------- COMANDO NO RECONOCIDO ---------
  else {
    respuesta = "ERROR:COMANDO_DESCONOCIDO";
  }

  // Enviar respuesta al cliente
  if (client.connected()) {
    client.println(respuesta);
    Serial.println("Respuesta enviada: " + respuesta);
  }
}