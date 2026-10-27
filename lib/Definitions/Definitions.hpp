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

    // Mixer/emulsifier commands
    kMixerOn,
    kMixerOff,
    kMixerState,

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
    kTempLimitQuery,  ///< TEMP:LIM? — answers the setpoint range and bath limits from config::

    // Level switch command
    kLevelSwitchState,

    // Sensor re-initialisation (re-run one sensor's hardware begin())
    kTempInReinit,
    kTempOutReinit,
    kTempBathReinit,
    kTempMixReinit,
    kFlowReinit,
    kLevelSwitchReinit,

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
    kReinitOk,              ///< A sensor re-initialisation succeeded ("OK")
    kEventOk,               ///< An event was queued for the state machine ("OK")
    kErrorCommandRejected,  ///< Command not accepted in the current FSM state
    kErrorCommandTimeout,   ///< Device did not confirm command execution within timeout
    kErrorInvalidParam,     ///< Command parameter is out of range (e.g. valve number = 0)
    kErrorSensorFault,      ///< Sensor returned NaN (disconnected or CRC failure)
    kErrorReinitFailed,     ///< A sensor re-initialisation (re-run begin()) did not succeed
    kErrorBusy,             ///< No free in-flight slot: the command was not dispatched and may be retried
    kMeasTemp,              ///< Runtime-formatted temperature reading in Celsius — no static wire string
    kMeasFlow,              ///< Runtime-formatted flow rate in L/min — no static wire string
    kStateValue,            ///< Runtime-formatted FSM state name — no static wire string
};

///
/// @brief Result of offering a command to the active state's handler.
///
enum class CommandOutcome {
    kRejected,   ///< Not valid in this state; the client is answered kErrorCommandRejected
    kPending,    ///< Dispatched to a device; that device's reply will confirm it
    kCompleted,  ///< Handled by the state itself, which filled the response to send back
};

///
/// @brief Command structure used for representing a command in the system.
///
struct Command {
    CommandType type;  ///< Command type identifier
    uint32_t extra;    ///< Generic payload — interpretation depends on CommandType:
                       ///<   actuator/sensor pin or I2C address for device commands;
                       ///<   IEEE-754 float bits for temperature/flow reads;
                       ///<   SystemState cast to uint32_t for kStateQuery;
                       ///<   ResponseType cast to uint32_t for kCommandError;
                       ///<   valve pin in the low bits plus kValveStatusOpenMask for a kValveState response.
    uint16_t token;    ///< Correlation id: assigned by Communications when the command is queued, held
                       ///< against the client that issued it, carried through the StateMachine and
                       ///< echoed by the device on its response. It is what pairs a reply with the
                       ///< request that caused it — and with the client waiting for it — rather than
                       ///< with any command of the same type. Declared last so existing
                       ///< {type, extra} initializers keep their meaning and leave it zero.
                       ///< 0 marks untagged traffic (autonomous actuations, inline reads).
};

///
/// @brief Bit set in Command::extra of a kValveState response when the valve is open.
///
/// A kValveState response must carry both the valve identity (so StateMachine can match
/// the response to the pending command) and the reading (so Communications can format
/// OPEN/CLOSED). The pin occupies the low bits; this flag occupies the top bit.
///
constexpr uint32_t kValveStatusOpenMask = 1u << 31;

///
/// @brief Mask that recovers the valve pin from a kValveState payload.
///
constexpr uint32_t kValvePinMask = ~kValveStatusOpenMask;

///
/// @brief Single entry in the SCPI command lookup table: maps a command string to its CommandType.
///
struct CommandEntry {
    const char* name;     ///< SCPI command string (e.g., "OUTP:HEAT ON")
    CommandType command;  ///< Corresponding CommandType enum value
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
    const char* name;   ///< Canonical response string sent over the wire (e.g., "ON", "-100,\"UNKNOWN COMMAND\"")
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

constexpr std::array<CommandEntry, 32> kCommandMap = {{
    {("OUTP:HEAT ON"), CommandType::kHeaterOn},
    {("OUTP:HEAT OFF"), CommandType::kHeaterOff},
    {("OUTP:HEAT?"), CommandType::kHeaterState},
    {("OUTP:PUMP ON"), CommandType::kPumpOn},
    {("OUTP:PUMP OFF"), CommandType::kPumpOff},
    {("OUTP:PUMP?"), CommandType::kPumpState},
    {("OUTP:MIX ON"), CommandType::kMixerOn},
    {("OUTP:MIX OFF"), CommandType::kMixerOff},
    {("OUTP:MIX?"), CommandType::kMixerState},
    {("OUTP:VALV OPEN"), CommandType::kValveOpen},
    {("OUTP:VALV CLOSE"), CommandType::kValveClose},
    {("OUTP:VALV?"), CommandType::kValveState},
    {("MEAS:TEMP:IN?"), CommandType::kTempInRead},
    {("MEAS:TEMP:OUT?"), CommandType::kTempOutRead},
    {("MEAS:TEMP:CAM?"), CommandType::kTempBathRead},
    {("MEAS:TEMP:MEZ?"), CommandType::kTempMixRead},
    {("TEMP:SET:IN"), CommandType::kTempSetIn},
    {("TEMP:LIM?"), CommandType::kTempLimitQuery},
    {("MEAS:FLOW?"), CommandType::kFlowRead},
    {("MEAS:LEVEL?"), CommandType::kLevelSwitchState},
    {("SYST:REINIT:TEMP:IN"), CommandType::kTempInReinit},
    {("SYST:REINIT:TEMP:OUT"), CommandType::kTempOutReinit},
    {("SYST:REINIT:TEMP:CAM"), CommandType::kTempBathReinit},
    {("SYST:REINIT:TEMP:MEZ"), CommandType::kTempMixReinit},
    {("SYST:REINIT:FLOW"), CommandType::kFlowReinit},
    {("SYST:REINIT:LEVEL"), CommandType::kLevelSwitchReinit},
    {("PING"), CommandType::kPing},
    {("STATE?"), CommandType::kStateQuery},
    {("SYST:LOG ON"), CommandType::kLogOn},
    {("SYST:LOG OFF"), CommandType::kLogOff},
    {("SYST:LOG?"), CommandType::kLogQuery},
    {("SYST:FAULT?"), CommandType::kFaultQuery},
}};

constexpr std::array<ResponseEntry, 18> kResponseMap = {{
    {"-100,\"UNKNOWN COMMAND\"", ResponseType::kErrorUnknownCommand},
    {"-200,\"EMPTY EVENT\"", ResponseType::kErrorEmptyEvent},
    {"-300,\"UNKNOWN EVENT\"", ResponseType::kErrorUnknownEvent},
    {"-101,\"COMMAND REJECTED\"", ResponseType::kErrorCommandRejected},
    {"-102,\"COMMAND TIMEOUT\"", ResponseType::kErrorCommandTimeout},
    {"-220,\"INVALID PARAMETER\"", ResponseType::kErrorInvalidParam},
    {"-103,\"SENSOR FAULT\"", ResponseType::kErrorSensorFault},
    {"PONG", ResponseType::kPingOk},
    {"ON", ResponseType::kStatusOn},
    {"OFF", ResponseType::kStatusOff},
    {"OPEN", ResponseType::kStatusOpen},
    {"CLOSED", ResponseType::kStatusClosed},
    {"HIGH", ResponseType::kStatusHigh},
    {"LOW", ResponseType::kStatusLow},
    {"OK", ResponseType::kReinitOk},
    {"OK", ResponseType::kEventOk},
    {"-105,\"REINIT FAILED\"", ResponseType::kErrorReinitFailed},
    {"-106,\"BUSY\"", ResponseType::kErrorBusy},
}};

constexpr std::array<CommandResponseEntry, 29> kCommandResponseMap = {{
    {CommandType::kHeaterOn, ResponseType::kStatusOn},
    {CommandType::kHeaterOff, ResponseType::kStatusOff},
    {CommandType::kHeaterState, ResponseType::kStatusOn},
    {CommandType::kPumpOn, ResponseType::kStatusOn},
    {CommandType::kPumpOff, ResponseType::kStatusOff},
    {CommandType::kPumpState, ResponseType::kStatusOn},
    {CommandType::kMixerOn, ResponseType::kStatusOn},
    {CommandType::kMixerOff, ResponseType::kStatusOff},
    {CommandType::kMixerState, ResponseType::kStatusOn},
    {CommandType::kValveOpen, ResponseType::kStatusOpen},
    {CommandType::kValveClose, ResponseType::kStatusClosed},
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
    {CommandType::kTempInReinit, ResponseType::kReinitOk},
    {CommandType::kTempOutReinit, ResponseType::kReinitOk},
    {CommandType::kTempBathReinit, ResponseType::kReinitOk},
    {CommandType::kTempMixReinit, ResponseType::kReinitOk},
    {CommandType::kFlowReinit, ResponseType::kReinitOk},
    {CommandType::kLevelSwitchReinit, ResponseType::kReinitOk},
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

///
/// @brief Identity of a device that can fail hardware initialisation, used as the bit index into
/// modules::FaultRegistry's bitmask (bit N = this FaultId's numeric value). Keep this enum under
/// 32 entries: the registry stores it as a single uint32_t bitmask, one bit per FaultId.
///
/// @note SYST:FAULT? now answers with per-sensor names (e.g. "TEMP_IN,FLOWMETER") instead of the
/// old coarse "THERMOCOUPLES" entry that lumped all four I2C amplifiers together, so a single dead
/// amplifier is now distinguishable from four. The SCADA does not currently send SYST:FAULT?, so
/// this wire-format change breaks nothing today.
///
enum class FaultId : uint8_t {
    kPump,
    kMixer,
    kHeater,
    kTempIn,
    kTempOut,
    kTempBath,
    kTempMix,
    kFlowmeter,
    kLevelSwitch,
    kCount  ///< Sentinel: also gives the registry its bit width (must stay <= 32).
};

static_assert(static_cast<uint8_t>(FaultId::kCount) <= 32, "FaultId must fit in FaultRegistry's uint32_t bitmask");

///
/// @brief Single entry in the fault name lookup table: maps a FaultId to its wire name.
///
struct FaultNameEntry {
    FaultId id;        ///< Fault identity
    const char* name;  ///< Wire name used in the SYST:FAULT? response (e.g. "TEMP_IN")
};

constexpr std::array<FaultNameEntry, 9> kFaultNameMap = {{
    {FaultId::kPump, "PUMP"},
    {FaultId::kMixer, "MIXER"},
    {FaultId::kHeater, "HEATER"},
    {FaultId::kTempIn, "TEMP_IN"},
    {FaultId::kTempOut, "TEMP_OUT"},
    {FaultId::kTempBath, "TEMP_CAM"},
    {FaultId::kTempMix, "TEMP_MEZ"},
    {FaultId::kFlowmeter, "FLOWMETER"},
    {FaultId::kLevelSwitch, "LEVEL_SWITCH"},
}};

///
/// @brief Single entry in the re-init lookup table: maps a SYST:REINIT:* command to the FaultId
/// it clears when that device confirms the re-initialisation succeeded (kReinitOk).
///
struct ReinitFaultEntry {
    CommandType command;  ///< Re-initialisation command
    FaultId fault;        ///< Fault cleared on that command's successful response
};

constexpr std::array<ReinitFaultEntry, 6> kReinitFaultMap = {{
    {CommandType::kTempInReinit, FaultId::kTempIn},
    {CommandType::kTempOutReinit, FaultId::kTempOut},
    {CommandType::kTempBathReinit, FaultId::kTempBath},
    {CommandType::kTempMixReinit, FaultId::kTempMix},
    {CommandType::kFlowReinit, FaultId::kFlowmeter},
    {CommandType::kLevelSwitchReinit, FaultId::kLevelSwitch},
}};

}  // namespace definitions
