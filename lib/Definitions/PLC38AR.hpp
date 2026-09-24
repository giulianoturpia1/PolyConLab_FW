///
/// @file PLC38AR.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief File for PLC38AR-specific definitions, such as pin assignments and hardware configurations. This file should
/// be included in the main program and any module that needs to reference PLC38AR-specific hardware details.
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
    kHeater = Q0_4,        ///< Heating Resistor - Output Digital Pin
    kPump = Q0_5,          ///< Circulation Pump - Output Digital Pin
    kChiller = Q0_3,       ///< Chiller - Output Digital Pin (Enables PSU to it)
    kValve1 = Q0_1,        ///< Valve 1 -  Output Digital Pin
    kMixer = Q0_2,         ///< Mixer/Emulsifier - Output Digital Pin (Enables PSU to it)
    kFlowmeter = I0_5,     ///< Flowmeter Sensor - Input  Digital Pin (Interrupt Capable)
    kLevelSwitch = I0_12,  ///< Level Switch - Input Digital Pin
};

enum class Addresses : uint32_t {
    kTempIn = 0x64,    ///< I2C Address - Inlet Temperature Sensor (kTempInRead)
    kTempOut = 0x65,   ///< I2C Address - Outlet Temperature Sensor (kTempOutRead)
    kTempBath = 0x66,  ///< I2C Address - Bath Temperature Sensor (kTempBathRead)
    kTempMix = 0x67    ///< I2C Address - Mix Temperature Sensor (kTempMixRead)
};
}  // namespace definitions
