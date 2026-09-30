#include "ManualState.hpp"

#include <Arduino.h>

#include <cmath>

#include "Config.hpp"
#include "DeviceDispatcher.hpp"
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
            kLog.info("ManualState: control de temperatura ACTIVADO, consigna " + std::to_string(setpoint_) + "°C");
        } else {
            // Disable temperature control and turn off the heater. Autonomous control
            // actuation: enforce through the dispatcher (no confirmation report), so it does
            // not masquerade as a client command in the StateMachine.
            enabled_ = false;
            DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOff, 0u}, 0);
            kLog.info("ManualState: control de temperatura DESACTIVADO, calentador apagado");
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
    // A device that failed its startup check no longer forces safe outputs here: the operator
    // accepted it to reach MANUAL, and outputs are only changed on a measured threshold (below).

    // Everything below runs once per control period (hardware timer, kManualControlIntervalUs), not
    // on every StateMachine tick: each reading is a multi-sample average that holds the shared I2C bus,
    // and reading both sensors every 50 ms tick kept the bus saturated -- the SCADA's own reads (and
    // these) timed out, and the StateMachine task overran its period. The bath limit at 1 Hz is still
    // far faster than the bath can heat.
    if (!manual_control_tick_ready) {
        return definitions::SystemState::kManual;
    }
    manual_control_tick_ready = false;

    // Read both temperatures synchronously through the dispatcher -- no concrete device dependency. The
    // thermocouple packs each reading (IEEE-754 bits, NaN on fault) into extra.
    const float bath_temp =
        utils::bits_to_float(DeviceDispatcher::instance().read_from_device({definitions::CommandType::kTempBathRead, 0u}).extra);
    const float temp =
        utils::bits_to_float(DeviceDispatcher::instance().read_from_device({definitions::CommandType::kTempMixRead, 0u}).extra);
    const bool bath_ok = !std::isnan(bath_temp);
    const bool mix_measured = !std::isnan(temp);
    // The control loop only trusts a mix reading inside the plausible range.
    const bool mix_ok = mix_measured && temp >= config::kMixTempPlausibleMinC && temp <= config::kMixTempPlausibleMaxC;
    track_reading(bath_ok, bath_missing_, "camisa");
    track_reading(mix_ok, mix_missing_, "mezcla");

    // The heater is turned off only on a measured threshold, never because a reading is missing. The
    // over-temperature limit uses the bath reading; if that is missing, the same limit is checked against
    // the mix reading (any measured value, even outside the plausible range: a high reading must still cut
    // the heater). With neither reading nothing is measured, so the outputs are left as they are. Once the
    // limit is reached the heater stays off until the reading drops below kBathTempClearC: without that gap
    // the loop would switch it back on as soon as the reading dipped under the limit.
    const bool limit_measured = bath_ok || mix_measured;
    const float limit_temp = bath_ok ? bath_temp : temp;
    if (limit_measured && limit_temp >= config::kMaxBathTempC && !over_limit_) {
        over_limit_ = true;
        kLog.warning(std::string("ManualState: limite de temperatura alcanzado en la lectura de ") + (bath_ok ? "camisa" : "mezcla") +
                     " (" + std::to_string(limit_temp) + "°C), calentador apagado");
    } else if (limit_measured && limit_temp < config::kBathTempClearC && over_limit_) {
        over_limit_ = false;
        kLog.info("ManualState: temperatura bajo el umbral de liberacion, el control puede volver a encender el calentador");
    }
    if (over_limit_) {
        DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOff, 0u}, 0);
        return definitions::SystemState::kManual;
    }

    // The control loop needs the mix reading; without it this period makes no decision (outputs unchanged).
    if (!mix_ok) {
        return definitions::SystemState::kManual;
    }

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

void ManualState::track_reading(bool available, bool& missing, const char* sensor) {
    if (!available && !missing) {
        kLog.warning(std::string("ManualState: lectura de temperatura de ") + sensor + " no disponible; no se actua sobre ella");
    } else if (available && missing) {
        kLog.info(std::string("ManualState: lectura de temperatura de ") + sensor + " disponible nuevamente");
    }
    missing = !available;
}

void ManualState::on_enter(StateMachine* context) {
    (void) context;
    enabled_ = false;  // Reset control state on entry
    manual_control_tick_ready = true;
    bath_missing_ = false;
    mix_missing_ = false;
    over_limit_ = false;

    // The timer is created once for the life of the board and only re-armed on entry. It used to be
    // torn down on exit and rebuilt here, so every MANUAL round trip destroyed and recreated hardware
    // timer 1 with its ISR attached — a plausible cause of the restarts seen when leaving and
    // re-entering MANUAL, and needless work regardless.
    if (manualControlTimer == nullptr) {
        manualControlTimer = timerBegin(config::kManualControlTimerIndex, config::kManualControlTimerPrescaler, true);
        if (manualControlTimer == nullptr) {
            kLog.error("ManualState: no se pudo inicializar el timer de hardware del lazo de control.");
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
