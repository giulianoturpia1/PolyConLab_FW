///
///@file Devices.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Device control module for the reactor system. Manages actuators (heating resistor,
///       circulation pump, valves) and sensor readings (temperature, flow rate).
///@version 0.1
///@date 20-01-2026
///
///@copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

// Pins definitions
// ------- Actuators pins -------
#define RELAYHEATER A1_0
#define RELAYBOMBA A1_0 
// Valves pins
#define RELAY_AUS_1 R1_4
#define RELAY_AUS_2 R1_5
#define RELAY_AUS_3 R1_6

//------- Sensors pins -------
#define RES_ST_1 T1
#define RES_ST_2 T2
#define RES_ST_3 T3
#define RES_ST_4 T4

#define INTERVALO_TEMP 2000
#define ERR_TEMP 111

void setupDevices();

void heaterON(int relayHeater_);
void heaterOFF(int relayHeater_);
bool heaterStatus();

float leerSensorTemp(uint8_t sensor_);

void bombaON(int relayBomba_);
void bombaOFF(int relayBomba_);
bool statusPump();

void valveON(int relayValvula_);
void valveOFF(int relayValvula_);
bool valveStatus(int relayValvula_);

void IRAM_ATTR isrCaudalimetro();
void IRAM_ATTR isrTimerCaudal();
void initCaudalimetro(int pinQ_);
void detenerCaudalimetro(int pinQ_);
float leerCaudalimetro();

