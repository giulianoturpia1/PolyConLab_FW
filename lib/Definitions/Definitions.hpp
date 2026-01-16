///
/// @file Definitions.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief State definitions and system-wide constants for PolyConLab
/// @version 0.1
/// @date 16-01-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

namespace definitions {

///
/// @brief System states for the Finite State Machine. Refer to the design documentation for details.
///
///
enum class SystemState {
    STARTUP,        ///< Initial state: Initialize HW/RTOS, Init TCP/BLE, Init lights
    CHECKUP,        ///< Automatic checkup: Health-check sensors, Handshake SCADA/BLE, Analogic checks
    CALIBRATION,    ///< Calibration: Ordered stop, Execute calibration protocol, Return to Automatic Checkups
    MANUAL,         ///< Manual/Wait state: Commands 1 and 1, Active interfaces, User waiting
    SOFT_ERROR,     ///< Soft error: Partial stop, HMI warning + LED/Buzzer, Retry/Ack
    CRITICAL_ERROR  ///< Critical error (latched): Full stop, Control locked, Requires reset
};

///
/// @brief System events for the Finite State Machine that could trigger state transitions.
///
/// @note These are arbitrary events and may not define all possible events in the system.
///
enum class SystemEvent {
    // Power and initialization
    POWER_ON,
    INIT_OK,

    // Checkup events
    CHECKUP_OK,
    SCADA_LINK_OK,
    SENSORS_OK,

    // Calibration events
    START_CALIBRATION,
    FINISH_CALIBRATION,

    // Manual mode events
    CMD_START_AUTO,
    SEQ_LOADED,
    CMD_START_DEMO,
    DEMO_ENABLED,

    // Command events
    CMD_PAUSE,
    CMD_RESUME,
    CMD_ABORT,
    CMD_SHUTDOWN,

    // Error events
    FAULT_RECOVERABLE,
    FAULT_CRITICAL,

    // Retry/check events
    CMD_RETRY_CHECK,
    CMD_SEQ_RESET,
    CONDITION_CLEARED,
    SEQ_DONE,

    // Reset event
    CMD_RESET_FAULT,
    SHUTDOWN_COMPLETE,
    SHUTDOWN_BUTTON,
    INIT_FATAL,
    WATCHDOG_FATAL,
    TOO_MANY_RETRIES,
    STOP
};

}  // namespace definitions
