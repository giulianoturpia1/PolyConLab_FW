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
/// @brief Maximum number of Thermocouple Amplifiers supported by the system.
///
constexpr uint8_t kMaxThermocouples = 5;

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
    // Startup and shutdown events
    kPowerOn,
    kInitOk,
    kInitFatal,
    kWatchdogFatal,
    kShutdown,
    kShutdownComplete,

    // Runtime control events
    kPause,
    kUnpause,
    kScadaOk,
    kSensorOk,
    kFaultRecoverable,
    kRetryCheckup,
    kAckFault,
    kConditionCleared,

    // Fault handling events
    kFaultCritical,
    kShutdownButton,
    kEStop,
    kFaultEscalates,
    kTooManyRetries,

    // Hard error reset event
    kResetFault,
    kConditionsOk,
    kPhysicalReset
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
    kFlowBegin,
    kFlowStop,
    kFlowRead,
    kFlowStatus,

    // Level switch command
    kLevelSwitchState,

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
    kPingOk,
    kStatusOn,
    kStatusOff,
    kStatusOpen,
    kStatusClosed,
    kStatusHigh,
    kStatusLow
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

constexpr std::array<CommandMap, 23> kCommandMap = {{
    {("OUTP:HEAT ON"), CommandType::kHeaterOn},
    {("OUTP:HEAT OFF"), CommandType::kHeaterOff},
    {("OUTP:HEAT?"), CommandType::kHeaterState},
    {("OUTP:PUMP ON"), CommandType::kPumpOn},
    {("OUTP:PUMP OFF"), CommandType::kPumpOff},
    {("OUTP:PUMP?"), CommandType::kPumpState},
    {("OUTP:CHILL ON"), CommandType::kChillerOn},
    {("OUTP:CHILL OFF"), CommandType::kChillerOff},
    {("OUTP:CHILL?"), CommandType::kChillerState},
    {("OUTP:VALV OPEN"), CommandType::kValveOpen},
    {("OUTP:VALV CLOSE"), CommandType::kValveClose},
    {("OUTP:VALV?"), CommandType::kValveState},
    {("MEAS:TEMP:IN?"), CommandType::kTempInRead},
    {("MEAS:TEMP:OUT?"), CommandType::kTempOutRead},
    {("MEAS:TEMP:CAM?"), CommandType::kTempBathRead},
    {("MEAS:TEMP:MEZ?"), CommandType::kTempMixRead},
    {("OUTP:FLOW ON"), CommandType::kFlowBegin},
    {("OUTP:FLOW OFF"), CommandType::kFlowStop},
    {("OUTP:FLOW?"), CommandType::kFlowStatus},
    {("MEAS:FLOW?"), CommandType::kFlowRead},
    {("MEAS:LEVEL?"), CommandType::kLevelSwitchState},
    {("PING"), CommandType::kPing},
    {("STATE?"), CommandType::kStateQuery},
}};

constexpr std::array<EventMap, 22> kEventMap = {{
    {("POWER ON"), SystemEvent::kPowerOn},
    {("INIT OK"), SystemEvent::kInitOk},
    {("INIT FATAL"), SystemEvent::kInitFatal},
    {("WATCHDOG FATAL"), SystemEvent::kWatchdogFatal},
    {("SHUTDOWN"), SystemEvent::kShutdown},
    {("SHUTDOWN COMPLETE"), SystemEvent::kShutdownComplete},
    {("PAUSE"), SystemEvent::kPause},
    {("UNPAUSE"), SystemEvent::kUnpause},
    {("SCADA OK"), SystemEvent::kScadaOk},
    {("SENSOR OK"), SystemEvent::kSensorOk},
    {("FAULT RECOVERABLE"), SystemEvent::kFaultRecoverable},
    {("RETRY CHECKUP"), SystemEvent::kRetryCheckup},
    {("ACK FAULT"), SystemEvent::kAckFault},
    {("CONDITION CLEARED"), SystemEvent::kConditionCleared},
    {("FAULT CRITICAL"), SystemEvent::kFaultCritical},
    {("SHUTDOWN BUTTON"), SystemEvent::kShutdownButton},
    {("E STOP"), SystemEvent::kEStop},
    {("FAULT ESCALATES"), SystemEvent::kFaultEscalates},
    {("TOO MANY RETRIES"), SystemEvent::kTooManyRetries},
    {("RESET FAULT"), SystemEvent::kResetFault},
    {("CONDITIONS OK"), SystemEvent::kConditionsOk},
    {("RESET FISICO"), SystemEvent::kPhysicalReset},
}};

constexpr std::array<ResponseMap, 27> kResponseMap = {{
    {"-100,’COMANDO DESCONOCIDO’", ResponseType::kErrorUnknownCommand},
    {"-200,’EVENTO VACIO’", ResponseType::kErrorEmptyEvent},
    {"-300,’EVENTO DESCONOCIDO’", ResponseType::kErrorUnknownEvent},
    {"PONG", ResponseType::kPingOk},
    {"ON", ResponseType::kStatusOn},
    {"OFF", ResponseType::kStatusOff},
    {"OPEN", ResponseType::kStatusOpen},
    {"CLOSED ", ResponseType::kStatusClosed},
    {"HIGH", ResponseType::kStatusHigh},
    {"LOW", ResponseType::kStatusLow},
}};

constexpr std::array<CommandResponseMap, 8> kCommandResponseMap = {{
    {CommandType::kHeaterOn, ResponseType::kStatusOn},
    {CommandType::kHeaterOff, ResponseType::kStatusOff},
    {CommandType::kPumpOn, ResponseType::kStatusOn},
    {CommandType::kPumpOff, ResponseType::kStatusOff},
    {CommandType::kChillerOn, ResponseType::kStatusOn},
    {CommandType::kChillerOff, ResponseType::kStatusOff},
    {CommandType::kValveOpen, ResponseType::kStatusOpen},
    {CommandType::kValveClose, ResponseType::kStatusClosed},
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
