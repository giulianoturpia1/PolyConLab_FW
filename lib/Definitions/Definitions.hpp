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
///@brief Flowmeter calibration constant: sensor pulses per liter. Adjust this value based on the specific flowmeter used and its
/// calibration data.
///
///
constexpr float kFlowmeterPulsesPerLiter = 28.75f;  // 489.125f;

///
///@brief Reference flow rate in liters per second for the flowmeter. Adjust based on the specific flowmeter used.
///
///
constexpr float kReferenceFlow =
    0.015f;  ///< Reference flow rate in liters per second for the flowmeter. Adjust based on the specific flowmeter used.

///
///@brief Frequency of pulses per second for the flowmeter. Adjust based on the specific flowmeter used. Refers to kReferenceFlow
///
///
constexpr float kReferenceFrequencyofPulsesPerSecond =
    12.5f;  ///< Frequency of pulses per second for the flowmeter. Adjust based on the specific flowmeter used.

///
/// @brief System states for the Finite State Machine. Refer to the design documentation for details.
///
///
enum class SystemState {
    kStartup,    ///< Initial state: Initialize HW/RTOS, Init TCP/BLE, Init lights
    kCheckup,    ///< Automatic checkup: Health-check sensors, Handshake SCADA/BLE, Analogic checks
    kManual,     ///< Manual/Wait state: Commands 1 and 1, Active interfaces, User waiting
    kPause,      ///< Pause: Freeze timers, safe partial exit, allow resume/abort
    kSoftError,  ///< Soft error: Partial stop, HMI warning + LED/Buzzer, Retry/Ack
    kHardError,  ///< Critical error (latched): Full stop, Control locked, Requires reset
    kShutdown    ///< Safe stop: Ordered stop, save state, return to Startup
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
    kTooManyRetries,

    // Hard error reset event
    kResetFault,
    kConditionsOk,
    kPhysicalReset,

    // Communications events
    kCommunicationLost
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

    // Manual setpoint from SCADA: pack IEEE-754 float into Command::extra
    kTempSetIn,

    // Level switch command
    kLevelSwitchState,

    // Diagnostics/Status
    kPing,
    kStateQuery,

    // System commands
    kLogOn,
    kLogOff,
    kLogQuery,
    kFaultQuery,  ///< SYST:FAULT? — returns comma-separated list of devices that failed init, or "NONE"

    // Internal sentinel — never appears in kCommandMap or over the wire.
    // Used to route command errors through the unified response callback.
    // extra holds the ResponseType cast to uint32_t.
    kCommandError
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
    kStatusLow,
    kErrorCommandRejected,  ///< Command not accepted in the current FSM state
    kErrorCommandTimeout,   ///< Device did not confirm command execution within timeout
    kErrorInvalidParam,     ///< Command parameter is out of range (e.g. valve number = 0)
    kErrorNotOperator,      ///< Actuation command from a client that does not hold the operator role
    kErrorSensorFault,      ///< Sensor returned NaN (disconnected or CRC failure)
    kMeasTemp,              ///< Runtime-formatted temperature reading in Celsius — no static wire string
    kMeasFlow,              ///< Runtime-formatted flow rate in L/min — no static wire string
    kStateValue,            ///< Runtime-formatted FSM state name — no static wire string
};

///
/// @brief Communication transport that originated a command or response.
///
enum class Transport {
    kNone,   ///< No active transport (idle / after response delivery)
    kWifi,   ///< Command received over WiFi TCP
    kSerial  ///< Command received over Serial
};

///
/// @brief Identity of a connected client (its IP packed as uint32_t); 0 = none/unknown.
/// Used by the authorization layer to tell clients apart, independent of the transport.
///
using ClientId = uint32_t;

///
/// @brief Command structure used for representing a command in the system.
///
struct Command {
    CommandType type;  ///< Command type identifier
    uint32_t extra;    ///< Generic payload — interpretation depends on CommandType:
                       ///<   actuator/sensor pin or I2C address for device commands;
                       ///<   IEEE-754 float bits for temperature/flow reads;
                       ///<   SystemState cast to uint32_t for kStateQuery;
                       ///<   ResponseType cast to uint32_t for kCommandError.
};

///
/// @brief Single entry in the SCPI command lookup table: maps a command string to its CommandType.
///
struct CommandEntry {
    const char* name;     ///< SCPI command string (e.g., "OUTP:HEAT ON")
    CommandType command;  ///< Corresponding CommandType enum value
    bool actuates;        ///< True if the command drives a physical output (requires the operator role)
};

///
/// @brief Single entry in the command-response lookup table: maps a CommandType to its ResponseType.
///
struct CommandResponseEntry {
    CommandType command;    ///< Command type used as the lookup key
    ResponseType response;  ///< Canonical response to send after this command executes
};

///
/// @brief Single entry in the SCPI event lookup table: maps an event string to its SystemEvent.
///
struct EventEntry {
    const char* name;   ///< SCPI event string (e.g., "EVT:PAUSE")
    SystemEvent event;  ///< Corresponding SystemEvent enum value
};

///
/// @brief Single entry in the SCPI response lookup table: maps a ResponseType to its wire string.
///
struct ResponseEntry {
    const char* name;   ///< Canonical response string sent over the wire (e.g., "ON", "-100,'Command error'")
    ResponseType type;  ///< Response type identifier
};

///
/// @brief Single entry in the state name lookup table: maps a SystemState to its display name.
///
struct StateNameEntry {
    SystemState state;  ///< System state enum value
    const char* name;   ///< Human-readable state name (e.g., "STARTUP")
};

constexpr std::array<EventEntry, 21> kEventMap = {{
    {("EVT:POWER ON"), SystemEvent::kPowerOn},
    {("EVT:INIT OK"), SystemEvent::kInitOk},
    {("EVT:SCADA OK"), SystemEvent::kScadaOk},
    {("EVT:SENSORS OK"), SystemEvent::kSensorOk},
    {("EVT:PAUSE"), SystemEvent::kPause},
    {("EVT:UNPAUSE"), SystemEvent::kUnpause},
    {("EVT:STOP"), SystemEvent::kEStop},
    {("EVT:SHUTDOWN"), SystemEvent::kShutdown},
    {("EVT:SHUTDOWN BUTTON"), SystemEvent::kShutdownButton},
    {("EVT:SHUTDOWN COMPLETE"), SystemEvent::kShutdownComplete},
    {("EVT:FAULT RECOVERABLE"), SystemEvent::kFaultRecoverable},
    {("EVT:FAULT CRITICAL"), SystemEvent::kFaultCritical},
    {("EVT:RETRY CHECKUP"), SystemEvent::kRetryCheckup},
    {("EVT:ACK FAULT"), SystemEvent::kAckFault},
    {("EVT:RESET FAULT"), SystemEvent::kResetFault},
    {("EVT:CONDITION CLEARED"), SystemEvent::kConditionCleared},
    {("EVT:CONDITIONS OK"), SystemEvent::kConditionsOk},
    {("EVT:WATCHDOG FATAL"), SystemEvent::kWatchdogFatal},
    {("EVT:TOO MANY RETRIES"), SystemEvent::kTooManyRetries},
    {("EVT:PHYSICAL RESET"), SystemEvent::kPhysicalReset},
    {("EVT:COMM LOST"), SystemEvent::kCommunicationLost},
}};

constexpr std::array<CommandEntry, 25> kCommandMap = {{
    {("OUTP:HEAT ON"), CommandType::kHeaterOn, true},
    {("OUTP:HEAT OFF"), CommandType::kHeaterOff, true},
    {("OUTP:HEAT?"), CommandType::kHeaterState, false},
    {("OUTP:PUMP ON"), CommandType::kPumpOn, true},
    {("OUTP:PUMP OFF"), CommandType::kPumpOff, true},
    {("OUTP:PUMP?"), CommandType::kPumpState, false},
    {("OUTP:CHILL ON"), CommandType::kChillerOn, true},
    {("OUTP:CHILL OFF"), CommandType::kChillerOff, true},
    {("OUTP:CHILL?"), CommandType::kChillerState, false},
    {("OUTP:VALV OPEN"), CommandType::kValveOpen, true},
    {("OUTP:VALV CLOSE"), CommandType::kValveClose, true},
    {("OUTP:VALV?"), CommandType::kValveState, false},
    {("MEAS:TEMP:IN?"), CommandType::kTempInRead, false},
    {("MEAS:TEMP:OUT?"), CommandType::kTempOutRead, false},
    {("MEAS:TEMP:CAM?"), CommandType::kTempBathRead, false},
    {("MEAS:TEMP:MEZ?"), CommandType::kTempMixRead, false},
    {("TEMP:SET:IN"), CommandType::kTempSetIn, true},
    {("MEAS:FLOW?"), CommandType::kFlowRead, false},
    {("MEAS:LEVEL?"), CommandType::kLevelSwitchState, false},
    {("PING"), CommandType::kPing, false},
    {("STATE?"), CommandType::kStateQuery, false},
    {("SYST:LOG ON"), CommandType::kLogOn, false},
    {("SYST:LOG OFF"), CommandType::kLogOff, false},
    {("SYST:LOG?"), CommandType::kLogQuery, false},
    {("SYST:FAULT?"), CommandType::kFaultQuery, false},
}};

constexpr std::array<ResponseEntry, 15> kResponseMap = {{
    {"-100,’UNKNOWN COMMAND’", ResponseType::kErrorUnknownCommand},
    {"-200,’EMPTY EVENT’", ResponseType::kErrorEmptyEvent},
    {"-300,’UNKNOWN EVENT’", ResponseType::kErrorUnknownEvent},
    {"-101,’COMMAND REJECTED’", ResponseType::kErrorCommandRejected},
    {"-102,’COMMAND TIMEOUT’", ResponseType::kErrorCommandTimeout},
    {"-220,’INVALID PARAMETER’", ResponseType::kErrorInvalidParam},
    {"-103,’SENSOR FAULT’", ResponseType::kErrorSensorFault},
    {"-104,’NOT OPERATOR’", ResponseType::kErrorNotOperator},
    {"PONG", ResponseType::kPingOk},
    {"ON", ResponseType::kStatusOn},
    {"OFF", ResponseType::kStatusOff},
    {"OPEN", ResponseType::kStatusOpen},
    {"CLOSED", ResponseType::kStatusClosed},
    {"HIGH", ResponseType::kStatusHigh},
    {"LOW", ResponseType::kStatusLow},
}};

constexpr std::array<CommandResponseEntry, 24> kCommandResponseMap = {{
    {CommandType::kHeaterOn, ResponseType::kStatusOn},
    {CommandType::kHeaterOff, ResponseType::kStatusOff},
    {CommandType::kHeaterState, ResponseType::kStatusOn},
    {CommandType::kPumpOn, ResponseType::kStatusOn},
    {CommandType::kPumpOff, ResponseType::kStatusOff},
    {CommandType::kPumpState, ResponseType::kStatusOn},
    {CommandType::kChillerOn, ResponseType::kStatusOn},
    {CommandType::kChillerOff, ResponseType::kStatusOff},
    {CommandType::kChillerState, ResponseType::kStatusOn},
    {CommandType::kValveOpen, ResponseType::kStatusOpen},
    {CommandType::kValveClose, ResponseType::kStatusClosed},
    {CommandType::kValveState, ResponseType::kStatusOpen},
    {CommandType::kTempInRead, ResponseType::kMeasTemp},
    {CommandType::kTempOutRead, ResponseType::kMeasTemp},
    {CommandType::kTempBathRead, ResponseType::kMeasTemp},
    {CommandType::kTempMixRead, ResponseType::kMeasTemp},
    {CommandType::kFlowRead, ResponseType::kMeasFlow},
    {CommandType::kLevelSwitchState, ResponseType::kStatusHigh},
    {CommandType::kPing, ResponseType::kPingOk},
    {CommandType::kStateQuery, ResponseType::kStateValue},
    {CommandType::kLogOn, ResponseType::kStatusOn},
    {CommandType::kLogOff, ResponseType::kStatusOff},
    {CommandType::kLogQuery, ResponseType::kStatusOn},
    {CommandType::kTempSetIn, ResponseType::kStatusOn},
}};

constexpr std::array<StateNameEntry, 7> kStateNameMap = {{
    {SystemState::kStartup, "STARTUP"},
    {SystemState::kCheckup, "CHECKUP"},
    {SystemState::kManual, "MANUAL"},
    {SystemState::kPause, "PAUSE"},
    {SystemState::kSoftError, "SOFT ERROR"},
    {SystemState::kHardError, "HARD ERROR"},
    {SystemState::kShutdown, "SHUTDOWN"},
}};

}  // namespace definitions
