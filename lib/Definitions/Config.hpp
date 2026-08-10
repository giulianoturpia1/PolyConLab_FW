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
// WiFi access point with a built-in DHCP server, so operator/observer devices
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
/// The ESP32 SoftAP supports up to ~10, but this is capped at 1 until the
/// operator/observer role layer enforces read-only observers (ADR-0001). Raise
/// it only once actuation is gated, so additional clients can watch but not act.
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

// ── Task Periods ─────────────────────────────────────────────────────────────

///
/// @brief App task wakeup period in milliseconds.
///
/// The App task polls WiFi and serial. SCADA commands arrive at ~1 Hz, so
/// 100 ms is more than responsive enough while reducing traffic on the scheduler.
///
constexpr uint32_t kAppTaskPeriodMs = 100;

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
/// @brief FreeRTOS priority shared by all non-thermocouple device tasks.
///
constexpr UBaseType_t kDeviceTaskPriority = 2;

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
/// @brief ESP32 core ID for I2C and GPIO device tasks.
///
/// All device tasks run on Core 1 (APP_CPU) so their I2C and GPIO operations
/// are never preempted by SPI traffic from Core 0.
///
constexpr BaseType_t kI2cCoreId = 1;

// ── Task Stack Sizes (Words, FreeRTOS defined) ──────────────────────────────────────────────────
// Note that words are not bytes: the actual stack size in bytes is configSTACK_DEPTH_TYPE (uint16_t or uint32_t) multiplied by the number
// of words defined here.

/// @brief Stack size in words for the App task.
constexpr uint32_t kAppTaskStackSize = 4096;

/// @brief Stack size in words for the StateMachine task.
constexpr uint32_t kStateMachineTaskStackSize = 4096;

/// @brief Stack size in words for the Thermocouple task.
constexpr uint32_t kThermocoupleTaskStackSize = 3072;

/// @brief Stack size in words for the Flowmeter task.
constexpr uint32_t kFlowmeterTaskStackSize = 3072;

/// @brief Stack size in words for the SolenoidValve task.
constexpr uint32_t kSolenoidValveTaskStackSize = 3072;

/// @brief Stack size in words for the Heater task.
constexpr uint32_t kHeaterTaskStackSize = 2048;

/// @brief Stack size in words for the Pump task.
constexpr uint32_t kPumpTaskStackSize = 2048;

/// @brief Stack size in words for the Chiller task.
constexpr uint32_t kChillerTaskStackSize = 2048;

/// @brief Stack size in words for the LevelSwitch task.
constexpr uint32_t kLevelSwitchTaskStackSize = 2048;

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
constexpr uint32_t kAppStartupDelayMs = 1000;

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
/// @brief Maximum time in milliseconds a device task waits to publish its
///        response back to the StateMachine response queue.
///
constexpr uint32_t kDeviceResponseTimeoutMs = 150;

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
constexpr uint8_t kMaxCommandsPerTick = 4;

///
/// @brief Maximum number of responses drained from the response queue in a single SM tick.
///
constexpr uint8_t kMaxResponsesPerTick = 4;

}  // namespace config
