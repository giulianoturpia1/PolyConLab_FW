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

///
/// @brief Mapping of event strings to SystemEvent enums for parsing incoming messages.
///
///
struct EventMap {
	const char* name; //< Event string (e.g., "POWER_ON")
	SystemEvent event; //< Corresponding SystemEvent enum
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

///
/// @brief Mapping of command strings to CommandType enums for parsing incoming messages.
///
///
struct CommandMap {
	const char* name; //< Command string (e.g., "HEATER_ON")
	CommandType command; //< Corresponding CommandType enum
};

///
/// @brief Canonical response messages used by communications handlers.
///
enum class ResponseType {
    ERROR_UNKNOWN_COMMAND,
    ERROR_EMPTY_EVENT,
    ERROR_UNKNOWN_EVENT,
    EVENT_OK_PREFIX,
    TEMP_IN_PREFIX,
    TEMP_OUT_PREFIX,
    TEMP_BATH_PREFIX,
    TEMP_MIX_PREFIX,
    HEATER_ON,
    HEATER_OFF,
    HEATER_STATE_PREFIX,
    PUMP_ON,
    PUMP_OFF,
    PUMP_STATE_PREFIX,
    VALVE_OPEN,
    VALVE_CLOSED,
    VALVE_STATE_PREFIX,
    FLOW_PREFIX,
    PING_OK,
    STATE_PREFIX,
    STATUS_ON,
    STATUS_OFF,
    STATUS_OPEN,
    STATUS_CLOSED
};

///
/// @brief Canonical response messages used by communications handlers.
///
///
struct ResponseMap {
    const char* name; //< Canonical response name (e.g., "ERROR:UNKNOWN_COMMAND")
    ResponseType type; //< Response identifier
};


constexpr std::array<CommandMap, 13> kCommandMap = {{
	{("HEATER_ON"), CommandType::HEATER_ON},
	{("HEATER_OFF"), CommandType::HEATER_OFF},
	{("HEATER_STATE"), CommandType::HEATER_STATE},
	{("PUMP_ON"), CommandType::PUMP_ON},
	{("PUMP_OFF"), CommandType::PUMP_OFF},
	{("PUMP_STATE"), CommandType::PUMP_STATE},
	{("VALVE_OPN"), CommandType::VALVE_OPEN},
	{("VALVE_CLS"), CommandType::VALVE_CLOSE},
	{("VALVE_STATE"), CommandType::VALVE_STATE},
	{("READ_FLOW"), CommandType::FLOW_READ},
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

constexpr std::array<ResponseMap, 24> kResponseMap = {{
    {"ERROR:UNKNOWN_COMMAND", ResponseType::ERROR_UNKNOWN_COMMAND},
    {"ERROR:EMPTY_EVENT", ResponseType::ERROR_EMPTY_EVENT},
    {"ERROR:UNKNOWN_EVENT", ResponseType::ERROR_UNKNOWN_EVENT},
    {"EVENT:OK,", ResponseType::EVENT_OK_PREFIX},
    {"TEMP_IN:", ResponseType::TEMP_IN_PREFIX},
    {"TEMP_OUT:", ResponseType::TEMP_OUT_PREFIX},
    {"TEMP_BATH:", ResponseType::TEMP_BATH_PREFIX},
    {"TEMP_MIX:", ResponseType::TEMP_MIX_PREFIX},
    {"RES:ON", ResponseType::HEATER_ON},
    {"RES:OFF", ResponseType::HEATER_OFF},
    {"RES:STATE,", ResponseType::HEATER_STATE_PREFIX},
    {"BBA:ON", ResponseType::PUMP_ON},
    {"BBA:OFF", ResponseType::PUMP_OFF},
    {"BBA:STATE,", ResponseType::PUMP_STATE_PREFIX},
    {"VALVE:OPEN", ResponseType::VALVE_OPEN},
    {"VALVE:CLOSED", ResponseType::VALVE_CLOSED},
    {"VALVE:STATE,", ResponseType::VALVE_STATE_PREFIX},
    {"CAUDAL:", ResponseType::FLOW_PREFIX},
    {"PONG", ResponseType::PING_OK},
    {"STATE:", ResponseType::STATE_PREFIX},
    {"ON", ResponseType::STATUS_ON},
    {"OFF", ResponseType::STATUS_OFF},
    {"OPEN", ResponseType::STATUS_OPEN},
    {"CLOSED", ResponseType::STATUS_CLOSED},
}};

}  // namespace definitions
