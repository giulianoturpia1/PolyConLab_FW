///
/// @file Config.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Central runtime-tuning constants for the PolyConLab firmware.
/// @version 0.1
/// @date 03-06-2026
///
/// @copyright Copyright (c) 2026
///
/// All task periods, priorities, stack sizes (in FreeRTOS words), queue depths
/// and timeouts live here so they can be adjusted without hunting through
/// individual .cpp files. No other source file should introduce new magic
/// numbers for these concerns.
///

#pragma once

#include <freertos/FreeRTOS.h>

#include <array>
#include <cstdint>

namespace config {

// ── Network / WiFi Access Point (SoftAP) ──────────────────────────────────────
//
// The PLC hosts its own WiFi network from inside the enclosure: it runs as a
// WiFi access point with a built-in DHCP server, so client devices
// connect directly to the PLC with no external router. The PLC always sits at
// kApIp, and the SCADA application targets that fixed address.
//
// Credentials default to the placeholders below but may be overridden per
// deployment through PlatformIO build_flags (e.g.
// -DWIFI_AP_SSID='"MyRig"' -DWIFI_AP_PASSWORD='"secret12"') so they need not be
// committed to source control.

#ifndef WIFI_AP_SSID
#define WIFI_AP_SSID "PolyConLab"  ///< TODO: override via build_flags per deployment.
#endif
#ifndef WIFI_AP_PASSWORD
#define WIFI_AP_PASSWORD "polyconlab"  ///< WPA2 needs >= 8 chars; "" for an open network.
#endif

/// @brief SSID the PLC access point broadcasts.
constexpr char kApSsid[] = WIFI_AP_SSID;

/// @brief WPA2 passphrase for the access point (>= 8 chars, or "" for open).
constexpr char kApPassword[] = WIFI_AP_PASSWORD;

/// @brief 2.4 GHz channel (1-13). 1, 6 or 11 are the non-overlapping choices.
constexpr uint8_t kApChannel = 1;

/// @brief Broadcast the SSID (false) or hide it (true).
constexpr bool kApHidden = false;

///
/// @brief Maximum WiFi stations allowed to associate with the AP.
///
/// The ESP32 SoftAP supports up to 4 clients, but the current firmware is single-client only.
///
constexpr uint8_t kApMaxClients = 1;

/// @brief TCP port the SCPI server listens on (SCADA default is 80).
constexpr uint16_t kWifiServerPort = 80;

/// @brief Static IPv4 address of the PLC access point. SCADA connects here.
constexpr std::array<uint8_t, 4> kApIp = {192, 168, 4, 1};

/// @brief Gateway advertised by the AP's DHCP server (the PLC itself).
constexpr std::array<uint8_t, 4> kApGateway = {192, 168, 4, 1};

/// @brief Subnet mask handed out by the AP's DHCP server.
constexpr std::array<uint8_t, 4> kApSubnet = {255, 255, 255, 0};

///
/// @brief Maximum time in milliseconds WifiServer::poll() waits to acquire the
///        client mutex before skipping the current tick.
///
constexpr uint32_t kClientMutexPollTimeoutMs = 5;

///
/// @brief Maximum time in milliseconds the response callback waits to acquire
///        the client mutex before discarding the response.
///
constexpr uint32_t kClientMutexCallbackTimeoutMs = 10;

///
/// @brief Maximum time in milliseconds EthernetServer::poll() waits to acquire the
///        SPI mutex before skipping the current tick.
///
constexpr uint32_t kSpiMutexPollTimeoutMs = 5;

///
/// @brief Maximum time in milliseconds the response callback waits to acquire
///        the SPI mutex before discarding the response.
///
constexpr uint32_t kSpiMutexCallbackTimeoutMs = 10;

// ── Ethernet transport (used only when -DTRANSPORT_ETHERNET selects EthernetServer) ──

/// @brief MAC address for the Ethernet interface.
constexpr std::array<uint8_t, 6> kEthernetMac = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

/// @brief Static IPv4 address for the Ethernet interface. SCADA connects here.
constexpr std::array<uint8_t, 4> kEthernetIp = {192, 168, 1, 100};

/// @brief TCP port the Ethernet SCPI server listens on (SCADA default is 80).
constexpr uint16_t kEthernetPort = 80;

// ── Task Periods ─────────────────────────────────────────────────────────────

///
/// @brief App task wakeup period in milliseconds.
///
/// The App task polls WiFi and serial. Every SCADA query waits on two App ticks (reading the line
/// and writing the reply), so this period is the largest fixed delay in a reading's round trip:
/// 20 ms keeps a full sensor round within ~150 ms. Polling is non-blocking, so the extra wakeups
/// cost little CPU.
///
constexpr uint32_t kAppTaskPeriodMs = 20;

///
/// @brief StateMachine task wakeup period in milliseconds.
///
/// The SM drains event, command and response queues on every tick. 50 ms keeps
/// fault-event response within an acceptable window; all dependent timeouts
/// (e.g. @c kCommandFeedbackTimeoutMs) have ample margin at this period.
///
constexpr uint32_t kStateMachineTaskPeriodMs = 50;

// ── Task Priorities ───────────────────────────────────────────────────────────

///
/// @brief FreeRTOS priority for the App task.
///
constexpr UBaseType_t kAppTaskPriority = 2;

///
/// @brief FreeRTOS priority for the StateMachine task.
///
constexpr UBaseType_t kStateMachineTaskPriority = 3;

///
/// @brief FreeRTOS priority for the Thermocouple task.
///
/// Matches the StateMachine priority  so that no lower-priority
/// task can time-slice into its I2C-transaction on Core 1. The
/// Thermocouple task is pinned to @c kI2cCoreId and the StateMachine to
/// @c kSpiCoreId, so, they never share a scheduler.
///
constexpr UBaseType_t kThermocoupleTaskPriority = 3;

// ── Core Responsibilities (ESP32 dual-core) ──────────────────────────────────────────

///
/// @brief ESP32 core ID for network/control tasks (App, StateMachine).
///
/// Keeping network and control work on Core 0 (PRO_CPU) isolates it from the I2C
/// device tasks on Core 1.
///
constexpr BaseType_t kSpiCoreId = 0;

///
/// @brief ESP32 core ID for the remaining device task (Thermocouple I2C).
///
/// The Thermocouple task runs on Core 1 (APP_CPU) so its blocking I2C
/// transactions are never preempted by SPI/network traffic from Core 0. GPIO
/// devices are now actuated synchronously on the caller's task (SyncDevice), so
/// they no longer need a Core 1 task.
///
constexpr BaseType_t kI2cCoreId = 1;

// ── Task Stack Sizes (Words, FreeRTOS defined) ──────────────────────────────────────────────────
// Note that words are not bytes: the actual stack size in bytes is configSTACK_DEPTH_TYPE (uint16_t or uint32_t) multiplied by the number
// of words defined here.

/// @brief Stack size in words for the App task.
constexpr uint32_t kAppTaskStackSize = 4096;

/// @brief Stack size in words for the StateMachine task.
constexpr uint32_t kStateMachineTaskStackSize = 4096;

/// @brief Stack size in words for the Thermocouple task (the only remaining device task).
constexpr uint32_t kThermocoupleTaskStackSize = 3072;

// ── Queue Lengths (items) ─────────────────────────────────────────────────────

///
/// @brief Maximum number of items in each StateMachine queue (events, commands, responses).
///
constexpr UBaseType_t kStateMachineQueueLength = 16;

///
/// @brief Maximum number of items in each device task command queue.
///
constexpr UBaseType_t kDeviceQueueLength = 8;

///
/// @brief Queue block time for all device tasks.
///
/// How long a device task suspends itself waiting for a command to arrive in
/// its queue. Set to @c portMAX_DELAY so the task blocks indefinitely and
/// consumes no CPU while idle. To add timeout handling (e.g. log a warning if
/// no command arrives within N ms), change this value and add the corresponding
/// branch in each task loop.
///
constexpr TickType_t kDeviceWaitTicks = portMAX_DELAY;

// ── Timeout Constants (ms) ────────────────────────────────────────────────────

///
/// @brief Delay in milliseconds before peripheral initialisation begins at startup.
///
/// Settling time after power-up: lets the supplies, the I2C bus and its external cabling, and the
/// thermocouple amplifiers stabilise (each MCP9600 also completes its first conversions) before any
/// device is probed, so a slow-starting sensor is not recorded as an init fault. The Wi-Fi server
/// only starts afterwards, so the SCADA cannot connect during this window.
///
constexpr uint32_t kAppStartupDelayMs = 5000;

///
/// @brief Maximum time in milliseconds the StateMachine waits for a device task
///        to confirm a dispatched command before declaring a timeout.
///
constexpr unsigned long kCommandFeedbackTimeoutMs = 1000;

///
/// @brief Maximum time in milliseconds Communications waits to enqueue a command
///        or event into the StateMachine queues.
///
constexpr uint32_t kCommsQueueTimeoutMs = 100;

///
/// @brief Maximum time in milliseconds the StateMachine waits when re-queuing
///        a command internally.
///
constexpr uint32_t kStateMachineCommandTimeoutMs = 150;

///
/// @brief Maximum time (ms) SOFT ERROR waits to dispatch a SYST:REINIT:* sensor
///        re-initialisation command to its device.
///
constexpr uint32_t kReinitCommandTimeoutMs = 200;

///
/// @brief Maximum time in milliseconds a device task waits to publish its
///        response back to the StateMachine response queue.
///
constexpr uint32_t kDeviceResponseTimeoutMs = 150;

///
/// @brief Maximum time in milliseconds a caller waits to acquire the shared I2C
///        bus mutex (Thermocouple) before abandoning the transaction.
///
/// Serializes access to the bus between the Thermocouple task (Core 1)
/// and synchronous reads issued from control states (Core 0). Kept short so a
/// wedged transaction cannot stall the caller indefinitely, yet well above a read that
/// uses every retry at kI2cClockHz (~2 ms per attempt at 50 kHz), plus a queued one.
///
constexpr uint32_t kI2cBusTimeoutMs = 100;

///
/// @brief Clock of the shared I2C bus, in Hz.
///
/// The bus carries both the PLC's own I/O expanders (relay outputs Q0_x, inputs such as I0_12) and
/// the thermocouple amplifiers on their external cabling. The PLC core starts it at 100 kHz; a slower
/// clock gives the long cable time to settle between edges and makes transactions less sensitive to
/// interference. At 50 kHz a temperature read takes ~2 ms and a relay write ~1 ms.
///
constexpr uint32_t kI2cClockHz = 50000;

///
/// @brief Queue-send timeout in milliseconds for a SyncDevice publishing its response.
///
/// A SyncDevice runs inline on the StateMachine task, which is also the sole
/// drainer of the response queue. Blocking here would stall the very task that
/// empties the queue, so the send is non-blocking (0): if the queue is
/// momentarily full the response is dropped and kCommandFeedbackTimeoutMs handles
/// it, exactly as when a device task's bounded send failed.
///
constexpr uint32_t kSyncResponsePublishTimeoutMs = 0;

// ── SM Queue-Drain Limits per Tick ────────────────────────────────────────────

///
/// @brief Maximum number of events drained from the event queue in a single SM tick.
///
/// Caps per-tick processing so that a flood of events cannot starve the command
/// and response queues within the same tick.
///
constexpr uint8_t kMaxEventsPerTick = 4;

///
/// @brief Maximum number of commands drained from the command queue in a single SM tick.
///
constexpr uint8_t kMaxCommandsPerTick = 8;

///
/// @brief Maximum number of responses drained from the response queue in a single SM tick.
///
constexpr uint8_t kMaxResponsesPerTick = 8;

///
/// @brief Commands that may await device confirmation at the same time.
///
/// Bounds both the StateMachine in-flight table and the Communications responder table, which are
/// kept the same depth so a command that Communications accepts always finds a slot downstream.
/// A command arriving with every slot occupied is answered kErrorBusy instead of displacing an
/// older one. Sized so a whole SCADA sampling round (6 sensor queries) plus a STATE? poll goes out
/// as one batch instead of waiting for earlier replies to free slots, and kept at
/// kMaxCommandsPerTick so a full intake tick never has to reject.
///
constexpr uint8_t kMaxInFlightCommands = 8;
static_assert(kMaxInFlightCommands <= kMaxCommandsPerTick && kMaxInFlightCommands <= kMaxResponsesPerTick,
              "One SM tick must be able to take in and answer every command in flight");

///
/// @brief Lines read from the connected client in a single communications poll tick.
///
/// At kAppTaskPeriodMs this sets the ingest rate: one line per tick was below what a burst of
/// sensor queries needs, so a round of them took several ticks just to be read. Bounds the work one
/// busy client can impose on the App task, and is kept at kMaxInFlightCommands so a full tick of
/// commands never has to be refused for want of a slot.
///
constexpr uint8_t kMaxLinesPerPollTick = 8;
static_assert(kMaxLinesPerPollTick <= kMaxInFlightCommands, "A full poll tick of commands must find in-flight slots");

///
/// @brief Replies Communications can hold before they are written to the client.
///
/// Every accepted message that owes a reply takes a slot, in arrival order: one answered on the
/// spot (SYST:LOG*, a rejected parameter) and one still waiting on a device both occupy one. Sized
/// above kMaxInFlightCommands so inline answers queued behind a slow command do not exhaust the
/// ring and start refusing traffic the StateMachine could still have accepted.
///
constexpr uint8_t kOutboundRingLength = 12;
static_assert(kOutboundRingLength > kMaxInFlightCommands, "The outbound ring must leave room for inline replies");

// ── Control Setpoint Limits ───────────────────────────────────────────────────

///
/// @brief Inclusive bounds (°C) accepted for temperature-setpoint commands (e.g. TEMP:SET:IN).
///
/// Setpoints outside this range — and non-finite values — are rejected with kErrorInvalidParam
/// before reaching the StateMachine. Tune to the physical envelope of the bath/chiller; the
/// ManualState default setpoint (kManualDefaultSetpointC) sits inside it.
///
constexpr float kTempSetpointMinC = 0.0f;
constexpr float kTempSetpointMaxC = 100.0f;

// ── Thermocouple Amplifiers (MCP9600) ────────────────────────────────────────

///
/// @brief ADC resolution of every MCP9600, in bits (18, 16, 14 or 12).
///
/// Trades step size against conversion time (type K, ~41 uV/°C): 18 -> ~0.05 °C every 320 ms,
/// 16 -> ~0.2 °C every 80 ms, 14 -> ~0.8 °C every 20 ms, 12 -> ~3 °C every 5 ms. Accuracy is the same
/// at every setting. 16 bits keeps the step under the 0.5 °C control deadband and refreshes the
/// reading 4x faster than 18 bits; 14 and 12 bits would switch the heater on quantization noise.
///
constexpr uint8_t kThermocoupleAdcResolutionBits = 16;
static_assert(kThermocoupleAdcResolutionBits == 18 || kThermocoupleAdcResolutionBits == 16 || kThermocoupleAdcResolutionBits == 14 ||
                  kThermocoupleAdcResolutionBits == 12,
              "The MCP9600 ADC supports only 18, 16, 14 or 12 bits");

///
/// @brief On-chip digital filter coefficient n of every MCP9600 (0 = off, 7 = strongest).
///
/// First-order filter applied per conversion with k = 2 / (2^n + 1); its time constant is
/// -1 / ln(1 - k) conversions. n = 4 (k ~ 0.118) gives ~8 conversions and cuts conversion noise ~4x.
/// A step reaches 95 % after ~24 conversions: ~1.9 s at 16 bits if the cold-junction conversion runs
/// in parallel (80 ms per update), up to ~8 s if the chip runs it in series with the ADC (~330 ms).
///
constexpr uint8_t kThermocoupleFilterCoefficient = 4;
static_assert(kThermocoupleFilterCoefficient <= 7, "The MCP9600 filter coefficient ranges 0-7");

///
/// @brief Cold-junction resolution: true = 0.0625 °C (250 ms per conversion), false = 0.25 °C (63 ms).
///
/// The MCP9600 offers only these two. 0.0625 °C is the original configuration; 0.25 °C would add up
/// to ±0.125 °C of quantization (still inside the ±1.5 °C type-K accuracy) in exchange for a shorter
/// conversion, which only matters if the chip runs it in series with the thermocouple ADC.
///
constexpr bool kThermocoupleColdJunctionFine = true;

///
/// @brief Plausibility window (°C): a reading outside it is reported as missing (NaN).
///
/// The lower bound is exclusive. Under interference the amplifiers report junk between 0 and ~3 °C (a bus
/// held low reads exactly 0 °C, a failed read -0.0625 °C); the water-based process never runs that cold.
///
constexpr float kThermocoupleMinValidC = 5.0f;
constexpr float kThermocoupleMaxValidC = 100.0f;

///
/// @brief Largest difference (°C) between two readings for them to count as agreeing.
///
/// Outlier rejection on top of the on-chip filter: a reading further than this from the last accepted
/// one is dropped unless the next attempt confirms it. Between reads (~0.5 s apart) the filtered
/// temperature moves a fraction of a degree, so anything beyond this is a corrupted transaction.
///
constexpr float kThermocoupleMaxJumpC = 5.0f;

///
/// @brief Attempts per temperature read before the reading is reported as missing.
///
/// Interference shows up as a failed or corrupted transaction, so a rejected reading is retried at
/// once; a good first read costs nothing extra, and a genuine large change needs two agreeing
/// attempts. Each attempt holds the bus ~2 ms at 50 kHz, so the worst case stays well inside
/// kI2cBusTimeoutMs.
///
constexpr uint8_t kThermocoupleReadAttempts = 3;
static_assert(kThermocoupleReadAttempts >= 2, "Confirming a large change needs two agreeing attempts");

// ── Manual Temperature Control (ManualState) ──────────────────────────────────

/// @brief Inlet setpoint (°C) MANUAL starts from, until the operator sends TEMP:SET:IN.
constexpr float kManualDefaultSetpointC = 20.0f;

/// @brief Initial hysteresis half-width (°C): heater ON below setpoint - deadband, OFF above setpoint + deadband.
/// ManualState copies it into its own deadband_ member, which is what the loop uses.
constexpr float kManualDeadbandC = 0.5f;

///
/// @brief Bath (jacket) temperature (°C) at or above which MANUAL forces the heater OFF, whatever the setpoint.
///
/// MANUAL only turns the heater off on a measured threshold, never because a reading is missing. When the
/// bath reading is missing, this limit is checked against the mix reading instead; with neither reading
/// nothing is measured, so no action is taken. Once reached, the heater stays off until the reading drops
/// below kBathTempClearC.
///
constexpr float kMaxBathTempC = 65.0f;

/// @brief Temperature (°C) the limit reading must drop below before MANUAL lets the heater back on.
constexpr float kBathTempClearC = 60.0f;

static_assert(kBathTempClearC < kMaxBathTempC, "The clear threshold must sit below the limit");

///
/// @brief Plausible range (°C) of the mix reading that drives the hysteresis loop.
///
/// A reading outside it (or NaN) is treated as a sensor fault and every output goes to its safe state.
///
constexpr float kMixTempPlausibleMinC = 0.0f;
constexpr float kMixTempPlausibleMaxC = 85.0f;

/// @brief Period (µs) of the hardware timer that allows one hysteresis decision per tick.
constexpr unsigned long kManualControlIntervalUs = 1000000UL;

/// @brief ESP32 hardware timer used for that gate.
constexpr uint8_t kManualControlTimerIndex = 1;

/// @brief Prescaler of that timer: 80 MHz APB clock / 80 = one tick per µs.
constexpr uint16_t kManualControlTimerPrescaler = 80;

/// @brief Timeout (ms) for a client command MANUAL forwards to a device.
constexpr uint32_t kManualCommandTimeoutMs = 100;

/// @brief Timeout (ms) for a read-only query PAUSE forwards to a device.
constexpr uint32_t kPauseQueryTimeoutMs = 100;

// ── Logging ───────────────────────────────────────────────────────────────────

///
/// @brief Whether debug/info/warning logs are printed from boot. Off keeps the Serial port and the
/// tasks that log free; SYST:LOG ON turns them on at runtime. Errors are printed regardless, so a
/// device that fails at boot is always visible.
///
constexpr bool kLogEnabledAtBoot = false;

// ── Relay Outputs ─────────────────────────────────────────────────────────────

///
/// @brief Wiring and fallback state of one relay-driven output (modules::RelayOutput).
///
/// on/off always keep their process meaning (ON = load powered); normally_closed only decides which
/// pin level achieves it. safe_on is the state RelayOutput::begin() leaves the load in and the one
/// IState::apply_safe_outputs() forces in every safe/error state.
///
struct RelayConfig {
    bool normally_closed;  ///< true if the load hangs off the relay's NC contact (pin LOW = load powered)
    bool safe_on;          ///< Safe/default state of the load: true = powered, false = cut
};

/// @brief Heater: NO contact, safe state OFF.
constexpr RelayConfig kHeaterRelay = {false, false};

/// @brief Circulation pump: NO contact, safe state OFF.
constexpr RelayConfig kPumpRelay = {false, false};

///
/// @brief Mixer/emulsifier: NC contact, safe state ON.
///
/// The mixer must keep running unless the operator explicitly stops it, so it is ON by default and
/// in every safe/error state (including HARD ERROR and SHUTDOWN). The NC contact makes that hold
/// even while the PLC is unpowered, booting or hung: a de-energised coil keeps the mixer powered.
///
constexpr RelayConfig kMixerRelay = {true, true};

}  // namespace config
