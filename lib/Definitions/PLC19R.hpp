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
/// @brief Pin definitions for the system. These are arbitrary and should be defined according to the actual hardware design. They can be used in the HAL and device drivers to refer to specific pins in a more abstract way.
/// @note Check the "ESP32_PLC_UserGuide.pdf" Section 7 for the actual pinout and update these definitions accordingly.///
///
enum class Pins: uint32_t {
    // Actuators
    RelayHeater = Q0_1,  ///< Heating resistor relay
    RelayPump = Q0_0,    ///< Circulation pump relay
    RelayAus1 = R1_1,    ///< Valve 1 relay
    RelayAus2 = R1_2,    ///< Valve 2 relay
    RelayAus3 = R1_3,    ///< Valve 3 relay

    // Sensors
    ResSt1 = I0_2,  ///< Temperature sensor 1
    ResSt2 = I0_3,  ///< Temperature sensor 2
    ResSt3 = I0_4,  ///< Temperature sensor 3
    ResSt4 = I0_5,  ///< Temperature sensor 4
};

}  // namespace definitions
