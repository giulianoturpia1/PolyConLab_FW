#include "ManualState.hpp"

#include <Arduino.h>

#include <cmath>

#include "DeviceDispatcher.hpp"
#include "FaultRegistry.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"
#include "TargetDefinitions.hpp"
#include "Thermocouple.hpp"

constexpr uint32_t kAcceptableTimeWindowMs = 100;
constexpr unsigned long kManualControlIntervalUs = 1000000UL;
constexpr int kManualControlTimerIndex = 1;

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

bool ManualState::dispatch_command(StateMachine* context, const definitions::Command& request) {
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
            // Disable temperature control and turn off heater/chiller
            enabled_ = false;
            definitions::Command h_off{definitions::CommandType::kHeaterOff, 0};
            definitions::Command c_off{definitions::CommandType::kChillerOff, 0};
            DeviceDispatcher::instance().forward_to_device(h_off, kAcceptableTimeWindowMs);
            DeviceDispatcher::instance().forward_to_device(c_off, kAcceptableTimeWindowMs);
            kLog.info("ManualState: temperature control DISABLED, heater/chiller OFF");
        }
        return true;
    }

    return DeviceDispatcher::instance().forward_to_device(request, kAcceptableTimeWindowMs);
}

definitions::SystemState ManualState::tick(StateMachine* context) {
    (void) context;
    // Safety: if faults present, disable outputs
    if (modules::FaultRegistry::instance().has_faults()) {
        interfaces::IState::apply_safe_outputs();
        return definitions::SystemState::kManual;
    }

    // Only apply hysteresis control if temperature control is enabled
    if (!enabled_) {
        return definitions::SystemState::kManual;
    }

    if (!manual_control_tick_ready) {
        return definitions::SystemState::kManual;
    }
    manual_control_tick_ready = false;

    // Read mix temperature
    float temp = modules::Thermocouple::instance().readCelcius(static_cast<uint32_t>(definitions::Addresses::kTempMix));
    if (std::isnan(temp)) {
        interfaces::IState::apply_safe_outputs();
        kLog.warning("ManualState: temperature sensor fault (NaN), safe outputs applied");
        return definitions::SystemState::kManual;
    }

    // Hysteresis control (only when enabled)
    if (temp < setpoint_ - deadband_) {
        // heater ON, chiller OFF
        definitions::Command h_on{definitions::CommandType::kHeaterOn, 0};
        definitions::Command c_off{definitions::CommandType::kChillerOff, 0};
        DeviceDispatcher::instance().forward_to_device(h_on, kAcceptableTimeWindowMs);
        DeviceDispatcher::instance().forward_to_device(c_off, kAcceptableTimeWindowMs);
    } else if (temp > setpoint_ + deadband_) {
        // heater OFF, chiller ON
        definitions::Command h_off{definitions::CommandType::kHeaterOff, 0};
        definitions::Command c_on{definitions::CommandType::kChillerOn, 0};
        DeviceDispatcher::instance().forward_to_device(h_off, kAcceptableTimeWindowMs);
        DeviceDispatcher::instance().forward_to_device(c_on, kAcceptableTimeWindowMs);
    }

    return definitions::SystemState::kManual;
}

void ManualState::on_enter(StateMachine* context) {
    (void) context;
    enabled_ = false;  // Reset control state on entry
    manual_control_tick_ready = true;

    if (manualControlTimer == nullptr) {
        manualControlTimer = timerBegin(kManualControlTimerIndex, 80, true);  // 1 tick = 1 µs
        if (manualControlTimer != nullptr) {
            timerAttachInterrupt(manualControlTimer, &isrManualControlTimer, true);
            timerAlarmWrite(manualControlTimer, kManualControlIntervalUs, true);
            timerAlarmEnable(manualControlTimer);
        } else {
            kLog.error("ManualState: failed to initialize hardware timer for control gating.");
        }
    }

    kLog.info("Iniciando Estado: MANUAL - Esperando comandos del usuario");
}

void ManualState::on_exit(StateMachine* context) {
    (void) context;
    if (manualControlTimer != nullptr) {
        timerEnd(manualControlTimer);
        manualControlTimer = nullptr;
    }
    manual_control_tick_ready = false;
}

const char* ManualState::name() const {
    return "MANUAL";
}

}  // namespace modules
