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
#include <stdint.h>

void setupDevices();

void heaterON(uint32_t relayHeater_);
void heaterOFF(uint32_t relayHeater_);
bool heaterStatus();

float readTempSensor(uint32_t sensor_);

void valveON(uint32_t relayValve_);
void valveOFF(uint32_t relayValve_);
bool valveStatus(uint32_t relayValve_);

void IRAM_ATTR isrFlowMeter();
void IRAM_ATTR isrTimerFlow();
void initFlowMeter(uint32_t pinFlow_);
void stopFlowMeter(uint32_t pinFlow_);
float readFlowMeter();
