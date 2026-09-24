#include "ManualState.hpp"

#include <Arduino.h>

#include <cmath>

#include "Config.hpp"
#include "DeviceDispatcher.hpp"
#include "FaultRegistry.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"
#include "TargetDefinitions.hpp"

static volatile bool manual_control_tick_ready = false;
static hw_timer_t* manualControlTimer = nullptr;

namespace modules {

static const TaggedLogger kLog("ManualState");

void IRAM_ATTR isrManualControlTimer() {
    manual_control_tick_ready = true;
}

definitions::SystemState ManualState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kPause:
            return definitions::SystemState::kPause;
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        default:
            return definitions::SystemState::kManual;
    }
}

definitions::CommandOutcome ManualState::handle_command(StateMachine* context, const definitions::Command& request,
                                                        definitions::Command& out_response) {
    (void) context;
    // Handle temperature setpoint command locally (toggle mode)
    if (request.type == definitions::CommandType::kTempSetIn) {
        const float sp = utils::bits_to_float(request.extra);

        // Toggle control state
        if (!enabled_) {
            // Enable temperature control
            enabled_ = true;
            setpoint_ = sp;
            manual_control_tick_ready = true;
            kLog.info("ManualState: temperature control ENABLED, setpoint " + std::to_string(setpoint_) + "°C");
        } else {
            // Disable temperature control and turn off the heater. Autonomous control
            // actuation: enforce through the dispatcher (no confirmation report), so it does
            // not masquerade as a client command in the StateMachine.
            enabled_ = false;
            DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOff, 0u}, 0);
            kLog.info("ManualState: temperature control DISABLED, heater OFF");
        }
        // No device confirms a setpoint, so the command is closed here and reports the resulting
        // control state. Leaving it pending would hold up every reply behind it until it timed out.
        out_response = {definitions::CommandType::kTempSetIn, enabled_ ? 1u : 0u, request.token};
        return definitions::CommandOutcome::kCompleted;
    }

    // Client-originated command: route through the single device command API so its
    // confirmation flows back through the StateMachine.
    return DeviceDispatcher::instance().issue_to_device(request, config::kManualCommandTimeoutMs) ? definitions::CommandOutcome::kPending
                                                                                                  : definitions::CommandOutcome::kRejected;
}

definitions::SystemState ManualState::tick(StateMachine* context) {
    (void) context;
    // Safety: if faults present, disable outputs
    if (modules::FaultRegistry::instance().has_faults()) {
        interfaces::IState::apply_safe_outputs();
        return definitions::SystemState::kManual;
    }

    // Read bath temperature synchronously through the dispatcher. This sensor is the safety
    // gate for the manual heating loop; if the bath reaches the overtemperature threshold,
    // the heater must be forced off regardless of the configured setpoint.
    const definitions::Command bath_temp_resp =
        DeviceDispatcher::instance().read_from_device({definitions::CommandType::kTempBathRead, 0u});
    const float bath_temp = utils::bits_to_float(bath_temp_resp.extra);
    if (std::isnan(bath_temp)) {
        // A single missing reading is usually a bus timeout, not a dead sensor: hold the outputs and
        // skip this tick until it has been missing for the whole grace period.
        if (!reading_lost(bath_misses_)) {
            return definitions::SystemState::kManual;
        }
        if (bath_misses_ == config::kManualSensorFaultTicks) {
            kLog.warning("ManualState: bath temperature sensor fault (NaN)");
        }
        interfaces::IState::apply_safe_outputs();
        return definitions::SystemState::kManual;
    }
    bath_misses_ = 0;

    if (bath_temp >= config::kMaxBathTempC) {
        DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOff, 0u}, 0);
        kLog.warning("ManualState: bath temperature limit reached (" + std::to_string(bath_temp) + "°C), heater disabled");
        return definitions::SystemState::kManual;
    }

    // Read mix temperature synchronously through the dispatcher — no concrete device dependency.
    // The thermocouple resolves kTempMixRead to its address and packs the reading (IEEE-754 bits,
    // NaN on fault) into extra.
    const definitions::Command temp_resp = DeviceDispatcher::instance().read_from_device({definitions::CommandType::kTempMixRead, 0u});
    const float temp = utils::bits_to_float(temp_resp.extra);
    if (std::isnan(temp) || temp < config::kMixTempPlausibleMinC || temp > config::kMixTempPlausibleMaxC) {
        // Same grace period as the bath reading (see above).
        if (!reading_lost(mix_misses_)) {
            return definitions::SystemState::kManual;
        }
        if (mix_misses_ == config::kManualSensorFaultTicks) {
            kLog.warning("ManualState: temperature sensor fault (NaN)");
        }
        interfaces::IState::apply_safe_outputs();
        return definitions::SystemState::kManual;
    }
    mix_misses_ = 0;

    if (!manual_control_tick_ready) {
        return definitions::SystemState::kManual;
    }
    manual_control_tick_ready = false;

    ////////// Temperature control logic: hysteresis loop with ±deadband around setpoint.
    // Only apply hysteresis control if temperature control is enabled
    if (!enabled_) {
        return definitions::SystemState::kManual;
    }

    // Hysteresis control (only when enabled). Autonomous control actuation: enforce through
    // the dispatcher — these are not client commands, so they must not publish confirmation
    // reports into the StateMachine response queue.
    // Heater-only control: the chiller is hard-wired to mains and runs continuously, outside the
    // PLC's reach, so cooling is not actuated here.
    if (temp < setpoint_ - deadband_) {
        DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOn, 0u}, 0);
    } else if (temp > setpoint_ + deadband_) {
        DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOff, 0u}, 0);
    }

    return definitions::SystemState::kManual;
}

static_assert(config::kManualSensorFaultTicks < UINT8_MAX, "kManualSensorFaultTicks must leave room for the one-past saturation");

bool ManualState::reading_lost(uint8_t& misses) {
    // Saturate one past the threshold, so `misses == kManualSensorFaultTicks` is true on exactly one
    // tick: the one that confirms the fault (callers log it there, not on every tick after).
    if (misses <= config::kManualSensorFaultTicks) {
        ++misses;
    }
    return misses >= config::kManualSensorFaultTicks;
}

void ManualState::on_enter(StateMachine* context) {
    (void) context;
    enabled_ = false;  // Reset control state on entry
    manual_control_tick_ready = true;
    bath_misses_ = 0;
    mix_misses_ = 0;

    // The timer is created once for the life of the board and only re-armed on entry. It used to be
    // torn down on exit and rebuilt here, so every MANUAL round trip destroyed and recreated hardware
    // timer 1 with its ISR attached — a plausible cause of the restarts seen when leaving and
    // re-entering MANUAL, and needless work regardless.
    if (manualControlTimer == nullptr) {
        manualControlTimer = timerBegin(config::kManualControlTimerIndex, config::kManualControlTimerPrescaler, true);
        if (manualControlTimer == nullptr) {
            kLog.error("ManualState: failed to initialize hardware timer for control gating.");
            kLog.info("Iniciando Estado: MANUAL - Esperando comandos del usuario");
            return;
        }
        timerAttachInterrupt(manualControlTimer, &isrManualControlTimer, true);
        timerAlarmWrite(manualControlTimer, config::kManualControlIntervalUs, true);
    }

    timerAlarmEnable(manualControlTimer);

    kLog.info("Iniciando Estado: MANUAL - Esperando comandos del usuario");
}

void ManualState::on_exit(StateMachine* context) {
    (void) context;
    // Silence the timer instead of destroying it: the ISR stops firing, the hardware stays set up,
    // and nothing outside MANUAL sees a stale tick.
    if (manualControlTimer != nullptr) {
        timerAlarmDisable(manualControlTimer);
    }
    manual_control_tick_ready = false;
}

const char* ManualState::name() const {
    return "MANUAL";
}

}  // namespace modules
