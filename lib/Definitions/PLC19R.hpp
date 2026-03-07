///
/// @file PLC19R.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief File for PLC19R-specific definitions, such as pin assignments and hardware configurations.
/// @version 0.1
/// @date 25-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace definitions {

///
/// @brief Pin definitions for the system. These are arbitrary and should be defined according to the actual hardware
/// design. They can be used in the HAL and device drivers to refer to specific pins in a more abstract way.
/// @note Check the "ESP32_PLC_UserGuide.pdf" Section 7 for the actual pinout and update these definitions accordingly.
/// Nomenclature is "Device_PinNumber", where "PinNumber" is the quantity of the pints that device needs.
///
enum class Pins : uint32_t {
    // Actuators
    kHeater = Q0_1,   ///< Heating Resistor
    kPump = Q0_0,     ///< Circulation Pump
    kChiller = Q0_2,  ///< Chiller
    kValve1 = R1_1,   ///< Valve 1
    kValve2 = R1_2,   ///< Valve 2

    // Sensors
    kTempSensor1 = I0_2,  ///< Temperature Sensor 1
    kTempSensor2 = I0_3,  ///< Temperature Sensor 2
    kTempSensor3 = I0_4,  ///< Temperature Sensor 3
    kTempSensor4 = I0_5,  ///< Temperature Sensor 4
};

}  // namespace definitions
