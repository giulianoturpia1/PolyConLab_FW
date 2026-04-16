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

#include <stdint.h>

#include <array>
#include <map>
#include <string>

namespace definitions {

///
/// @brief Maximum number of solenoid valves supported by the system.
///
constexpr uint8_t kMaxSolenoidValves = 8;

///
/// @brief System states for the Finite State Machine. Refer to the design documentation for details.
///
///
enum class SystemState {
    kStartup,      ///< Initial state: Initialize HW/RTOS, Init TCP/BLE, Init lights
    kCheckup,      ///< Automatic checkup: Health-check sensors, Handshake SCADA/BLE, Analogic checks
    kCalibration,  ///< Calibration: Ordered stop, Execute calibration protocol, Return to Automatic Checkups
    kManual,       ///< Manual/Wait state: Commands 1 and 1, Active interfaces, User waiting
    kPause,        ///< Pause: Freeze timers, safe partial exit, allow resume/abort
    kSoftError,    ///< Soft error: Partial stop, HMI warning + LED/Buzzer, Retry/Ack
    kHardError,    ///< Critical error (latched): Full stop, Control locked, Requires reset
    kShutdown      ///< Safe stop: Ordered stop, save state, return to Startup
};

///
/// @brief System events for the Finite State Machine that could trigger state transitions.
///
/// @note These are arbitrary events and may not define all possible events in the system.
///
enum class SystemEvent {
    // Power and initialization
    kPowerOn,
    kInitOk,

    // Checkup events
    kCheckupOk,
    kScadaLinkOk,
    kSensorsOk,

    // Calibration events
    kStartCalibration,
    kFinishCalibration,

    // Manual mode events
    kCmdStartAuto,
    kSeqLoaded,
    kCmdStartDemo,
    kDemoEnabled,

    // Command events
    kCmdPause,
    kCmdResume,
    kCmdAbort,
    kCmdShutdown,

    // Error events
    kFaultRecoverable,
    kFaultCritical,

    // Retry/check events
    kCmdRetryCheck,
    kCmdAckFault,
    kCmdSeqReset,
    kConditionCleared,
    kSeqDone,

    // Reset event
    kCmdResetFault,
    kShutdownComplete,
    kShutdownButton,
    kInitFatal,
    kWatchdogFatal,
    kTooManyRetries,
    kStop
};

///
/// @brief Mapping of event strings to SystemEvent enums for parsing incoming messages.
///
///
struct EventMap {
    const char* name;   //< Event string (e.g., "POWER_ON")
    SystemEvent event;  //< Corresponding SystemEvent enum
};

///
/// @brief Device commands that can be sent over communication transports
///
enum class CommandType {
    // Heater/Resistor commands
    kHeaterOn,
    kHeaterOff,
    kHeaterState,

    // Pump commands
    kPumpOn,
    kPumpOff,
    kPumpState,

    // Chiller commands
    kChillerOn,
    kChillerOff,
    kChillerState,

    // Valve commands (parameterized by valve number)
    kValveOpen,
    kValveClose,
    kValveState,

    // Temperature sensor commands
    kTempInRead,
    kTempOutRead,
    kTempBathRead,
    kTempMixRead,

    // Flow meter command
    kFlowRead,

    // Diagnostics/Status
    kPing,
    kStateQuery
};

///
/// @brief Payload to request a command dispatch from the StateMachine.
///
struct CommandRequest {
    CommandType command;
    uint32_t pin;
};

///
/// @brief Mapping of command strings to CommandType enums for parsing incoming messages.
///
///
struct CommandMap {
    const char* name;     //< Command string (e.g., "HEATER_ON")
    CommandType command;  //< Corresponding CommandType enum
};

///
/// @brief Canonical response messages used by communications handlers.
///
enum class ResponseType {
    kErrorUnknownCommand,
    kErrorEmptyEvent,
    kErrorUnknownEvent,
    kEventOkPrefix,
    kTempInPrefix,
    kTempOutPrefix,
    kTempBathPrefix,
    kTempMixPrefix,
    kHeaterOn,
    kHeaterOff,
    kHeaterStatePrefix,
    kPumpOn,
    kPumpOff,
    kPumpStatePrefix,
    kChillerOn,
    kChillerOff,
    kChillerStatePrefix,
    kValveOpen,
    kValveClosed,
    kValveStatePrefix,
    kFlowPrefix,
    kPingOk,
    kStatePrefix,
    kStatusOn,
    kStatusOff,
    kStatusOpen,
    kStatusClosed
};

///
/// @brief Canonical response messages used by communications handlers.
///
///
struct ResponseMap {
    const char* name;   //< Canonical response name (e.g., "ERROR:UNKNOWN_COMMAND")
    ResponseType type;  //< Response identifier
};

///
/// @brief Mapping of command enums to canonical response identifiers.
///
struct CommandResponseMap {
    CommandType command;
    ResponseType response;
};

constexpr std::array<CommandMap, 16> kCommandMap = {{
    {("HEATER_ON"), CommandType::kHeaterOn},
    {("HEATER_OFF"), CommandType::kHeaterOff},
    {("HEATER_STATE"), CommandType::kHeaterState},
    {("PUMP_ON"), CommandType::kPumpOn},
    {("PUMP_OFF"), CommandType::kPumpOff},
    {("PUMP_STATE"), CommandType::kPumpState},
    {("CHILLER_ON"), CommandType::kChillerOn},
    {("CHILLER_OFF"), CommandType::kChillerOff},
    {("CHILLER_STATE"), CommandType::kChillerState},
    {("VALVE_OPEN"), CommandType::kValveOpen},
    {("VALVE_CLOSE"), CommandType::kValveClose},
    {("VALVE_STATE"), CommandType::kValveState},
    {("READ_FLOW"), CommandType::kFlowRead},
    {("PING"), CommandType::kPing},
    {("STATE"), CommandType::kStateQuery},
    {("STATE?"), CommandType::kStateQuery},
}};

constexpr std::array<EventMap, 29> kEventMap = {{
    {("POWER_ON"), SystemEvent::kPowerOn},
    {("INIT_OK"), SystemEvent::kInitOk},
    {("CHECKUP_OK"), SystemEvent::kCheckupOk},
    {("SCADA_LINK_OK"), SystemEvent::kScadaLinkOk},
    {("SENSORS_OK"), SystemEvent::kSensorsOk},
    {("START_CALIBRATION"), SystemEvent::kStartCalibration},
    {("FINISH_CALIBRATION"), SystemEvent::kFinishCalibration},
    {("CMD_START_AUTO"), SystemEvent::kCmdStartAuto},
    {("SEQ_LOADED"), SystemEvent::kSeqLoaded},
    {("CMD_START_DEMO"), SystemEvent::kCmdStartDemo},
    {("DEMO_ENABLED"), SystemEvent::kDemoEnabled},
    {("CMD_PAUSE"), SystemEvent::kCmdPause},
    {("CMD_RESUME"), SystemEvent::kCmdResume},
    {("CMD_ABORT"), SystemEvent::kCmdAbort},
    {("CMD_SHUTDOWN"), SystemEvent::kCmdShutdown},
    {("FAULT_RECOVERABLE"), SystemEvent::kFaultRecoverable},
    {("FAULT_CRITICAL"), SystemEvent::kFaultCritical},
    {("CMD_RETRY_CHECK"), SystemEvent::kCmdRetryCheck},
    {("CMD_ACK_FAULT"), SystemEvent::kCmdAckFault},
    {("CMD_SEQ_RESET"), SystemEvent::kCmdSeqReset},
    {("CONDITION_CLEARED"), SystemEvent::kConditionCleared},
    {("SEQ_DONE"), SystemEvent::kSeqDone},
    {("CMD_RESET_FAULT"), SystemEvent::kCmdResetFault},
    {("SHUTDOWN_COMPLETE"), SystemEvent::kShutdownComplete},
    {("SHUTDOWN_BUTTON"), SystemEvent::kShutdownButton},
    {("INIT_FATAL"), SystemEvent::kInitFatal},
    {("WATCHDOG_FATAL"), SystemEvent::kWatchdogFatal},
    {("TOO_MANY_RETRIES"), SystemEvent::kTooManyRetries},
    {("STOP"), SystemEvent::kStop},
}};

constexpr std::array<ResponseMap, 27> kResponseMap = {{
    {"ERROR:UNKNOWN_COMMAND", ResponseType::kErrorUnknownCommand},
    {"ERROR:EMPTY_EVENT", ResponseType::kErrorEmptyEvent},
    {"ERROR:UNKNOWN_EVENT", ResponseType::kErrorUnknownEvent},
    {"EVENT:OK,", ResponseType::kEventOkPrefix},
    {"TEMP_IN: ", ResponseType::kTempInPrefix},
    {"TEMP_OUT: ", ResponseType::kTempOutPrefix},
    {"TEMP_BATH: ", ResponseType::kTempBathPrefix},
    {"TEMP_MIX: ", ResponseType::kTempMixPrefix},
    {"HEATER: ON", ResponseType::kHeaterOn},
    {"HEATER: OFF", ResponseType::kHeaterOff},
    {"HEATER: STATE,", ResponseType::kHeaterStatePrefix},
    {"PUMP: ON", ResponseType::kPumpOn},
    {"PUMP: OFF", ResponseType::kPumpOff},
    {"PUMP: STATE,", ResponseType::kPumpStatePrefix},
    {"CHILLER: ON", ResponseType::kChillerOn},
    {"CHILLER: OFF", ResponseType::kChillerOff},
    {"CHILLER: STATE,", ResponseType::kChillerStatePrefix},
    {"VALVE: OPEN ", ResponseType::kValveOpen},
    {"VALVE: CLOSE ", ResponseType::kValveClosed},
    {"VALVE: STATE ,", ResponseType::kValveStatePrefix},
    {"FLOW: ", ResponseType::kFlowPrefix},
    {"PONG", ResponseType::kPingOk},
    {"STATE: ", ResponseType::kStatePrefix},
    {"ON", ResponseType::kStatusOn},
    {"OFF", ResponseType::kStatusOff},
    {"OPEN", ResponseType::kStatusOpen},
    {"CLOSE", ResponseType::kStatusClosed},
}};

constexpr std::array<CommandResponseMap, 8> kCommandResponseMap = {{
    {CommandType::kHeaterOn, ResponseType::kHeaterOn},
    {CommandType::kHeaterOff, ResponseType::kHeaterOff},
    {CommandType::kPumpOn, ResponseType::kPumpOn},
    {CommandType::kPumpOff, ResponseType::kPumpOff},
    {CommandType::kChillerOn, ResponseType::kChillerOn},
    {CommandType::kChillerOff, ResponseType::kChillerOff},
    {CommandType::kValveOpen, ResponseType::kValveOpen},
    {CommandType::kValveClose, ResponseType::kValveClosed},
}};

enum class ErrorCode {
    kErrTemp = 111,  ///< Error code for temperature sensor failure
};

enum class Constants {
    kTempIntervalMs = 2000,  ///< Interval for temperature readings in milliseconds
};

///
/// @brief Mapping of SystemState enums to human-readable state names.
///
struct StateNameMap {
    SystemState state;  ///< System state enum value
    const char* name;   ///< Human-readable state name (e.g., "STARTUP")
};

constexpr std::array<StateNameMap, 8> kStateNameMap = {{
    {SystemState::kStartup, "STARTUP"},
    {SystemState::kCheckup, "CHECKUP"},
    {SystemState::kCalibration, "CALIBRATION"},
    {SystemState::kManual, "MANUAL"},
    {SystemState::kPause, "PAUSE"},
    {SystemState::kSoftError, "SOFT_ERROR"},
    {SystemState::kHardError, "HARD_ERROR"},
    {SystemState::kShutdown, "SHUTDOWN"},
}};

}  // namespace definitions
