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
#include <Ethernet.h>
#include <SPI.h>
#include <stdint.h>

// Pins definitions
// ------- Actuators pins -------
#define RELAYHEATER Q0_0  // A1_0
#define RELAYPUMP Q0_1    // A1_0
// Valves pins
#define RELAY_AUS_1 R1_1  // R1_4
#define RELAY_AUS_2 R1_2  // R1_5
#define RELAY_AUS_3 R1_3  // R1_6

//------- Sensors pins -------
#define RES_ST_1 I0_0  // T1
#define RES_ST_2 I0_1  // T2
#define RES_ST_3 I0_2  // T3
#define RES_ST_4 I0_3  // T4

#define TEMP_INTERVAL 2000
#define ERR_TEMP 111

void setupDevices();

void heaterON(uint32_t relayHeater_);
void heaterOFF(uint32_t relayHeater_);
bool heaterStatus();

float readTempSensor(uint32_t sensor_);

void pumpON(uint32_t relayPump_);
void pumpOFF(uint32_t relayPump_);
bool statusPump();

void valveON(uint32_t relayValve_);
void valveOFF(uint32_t relayValve_);
bool valveStatus(uint32_t relayValve_);

void IRAM_ATTR isrFlowMeter();
void IRAM_ATTR isrTimerFlow();
void initFlowMeter(uint32_t pinFlow_);
void stopFlowMeter(uint32_t pinFlow_);
float readFlowMeter();
