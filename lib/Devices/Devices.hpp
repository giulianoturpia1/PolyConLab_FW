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

void IRAM_ATTR isrFlowMeter();
void IRAM_ATTR isrTimerFlow();
void initFlowMeter(uint32_t pinFlow_);
void stopFlowMeter(uint32_t pinFlow_);
float readFlowMeter();
