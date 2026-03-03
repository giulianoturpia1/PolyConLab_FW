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

#include <array>
#include <string>

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

struct EventMap {
	const char* name;
	SystemEvent event;
};

///
/// @brief Device commands that can be sent over communication transports
///
enum class CommandType {
    // Heater/Resistor commands
    HEATER_ON,
    HEATER_OFF,
    HEATER_STATE,

    // Pump commands
    PUMP_ON,
    PUMP_OFF,
    PUMP_STATE,

    // Valve commands
    VALVE_OPEN,
    VALVE_CLOSE,
    VALVE_STATE,

    // Temperature sensor commands
    TEMP_IN_READ,
    TEMP_OUT_READ,
    TEMP_BATH_READ,
    TEMP_MIX_READ,

    // Flow meter command
    FLOW_READ,

    // Diagnostics/Status
    PING,
    STATE_QUERY
};

struct CommandMap {
	const char* name;
	CommandType command;
};

constexpr std::array<CommandMap, 13> kCommandMap = {{
	{("RES_ON"), CommandType::HEATER_ON},
	{("RES_OFF"), CommandType::HEATER_OFF},
	{("RES_STATE"), CommandType::HEATER_STATE},
	{("BBA_ON"), CommandType::PUMP_ON},
	{("BBA_OFF"), CommandType::PUMP_OFF},
	{("BBA_STATE"), CommandType::PUMP_STATE},
	{("VALVE_OPN"), CommandType::VALVE_OPEN},
	{("VALVE_CLS"), CommandType::VALVE_CLOSE},
	{("VALVE_STATE"), CommandType::VALVE_STATE},
	{("LEER_CAUDAL"), CommandType::FLOW_READ},
	{("PING"), CommandType::PING},
	{("STATE"), CommandType::STATE_QUERY},
	{("STATE?"), CommandType::STATE_QUERY},
}};

constexpr std::array<EventMap, 28> kEventMap = {{
	{("POWER_ON"), SystemEvent::POWER_ON},
	{("INIT_OK"), SystemEvent::INIT_OK},
	{("CHECKUP_OK"), SystemEvent::CHECKUP_OK},
	{("SCADA_LINK_OK"), SystemEvent::SCADA_LINK_OK},
	{("SENSORS_OK"), SystemEvent::SENSORS_OK},
	{("START_CALIBRATION"), SystemEvent::START_CALIBRATION},
	{("FINISH_CALIBRATION"), SystemEvent::FINISH_CALIBRATION},
	{("CMD_START_AUTO"), SystemEvent::CMD_START_AUTO},
	{("SEQ_LOADED"), SystemEvent::SEQ_LOADED},
	{("CMD_START_DEMO"), SystemEvent::CMD_START_DEMO},
	{("DEMO_ENABLED"), SystemEvent::DEMO_ENABLED},
	{("CMD_PAUSE"), SystemEvent::CMD_PAUSE},
	{("CMD_RESUME"), SystemEvent::CMD_RESUME},
	{("CMD_ABORT"), SystemEvent::CMD_ABORT},
	{("CMD_SHUTDOWN"), SystemEvent::CMD_SHUTDOWN},
	{("FAULT_RECOVERABLE"), SystemEvent::FAULT_RECOVERABLE},
	{("FAULT_CRITICAL"), SystemEvent::FAULT_CRITICAL},
	{("CMD_RETRY_CHECK"), SystemEvent::CMD_RETRY_CHECK},
	{("CMD_SEQ_RESET"), SystemEvent::CMD_SEQ_RESET},
	{("CONDITION_CLEARED"), SystemEvent::CONDITION_CLEARED},
	{("SEQ_DONE"), SystemEvent::SEQ_DONE},
	{("CMD_RESET_FAULT"), SystemEvent::CMD_RESET_FAULT},
	{("SHUTDOWN_COMPLETE"), SystemEvent::SHUTDOWN_COMPLETE},
	{("SHUTDOWN_BUTTON"), SystemEvent::SHUTDOWN_BUTTON},
	{("INIT_FATAL"), SystemEvent::INIT_FATAL},
	{("WATCHDOG_FATAL"), SystemEvent::WATCHDOG_FATAL},
	{("TOO_MANY_RETRIES"), SystemEvent::TOO_MANY_RETRIES},
	{("STOP"), SystemEvent::STOP},
}};

}  // namespace definitions
