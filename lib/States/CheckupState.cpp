#include "CheckupState.hpp"

#include "DeviceDispatcher.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"

constexpr uint32_t kCheckupTimeWindowMs = 200;

namespace modules {

static const TaggedLogger kLog("CheckupState");

definitions::SystemState CheckupState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kScadaOk:
            scadaOk_ = true;
            break;
        case definitions::SystemEvent::kSensorOk:
            sensorOk_ = true;
            break;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        default:
            break;
    }

    if (scadaOk_ && sensorOk_) {
        return definitions::SystemState::kManual;
    }

    return definitions::SystemState::kCheckup;
}

void CheckupState::on_enter(StateMachine* context) {
    (void) context;
    scadaOk_ = false;
    sensorOk_ = false;
    kLog.info("Iniciando Estado: CHECKUP");
    apply_safe_outputs();
    close_all_valves();
}

const char* CheckupState::name() const {
    return "CHECKUP";
}

bool CheckupState::handle_command(StateMachine* context, const definitions::Command& request) {
    (void) context;
    // Only read-only sensor queries are accepted during CHECKUP so the SCADA
    // can probe each device.  Actuator commands are always rejected here.
    switch (request.type) {
        case definitions::CommandType::kTempInRead:
        case definitions::CommandType::kTempOutRead:
        case definitions::CommandType::kTempBathRead:
        case definitions::CommandType::kTempMixRead:
        case definitions::CommandType::kFlowRead:
        case definitions::CommandType::kLevelSwitchState:
            return DeviceDispatcher::instance().issue_to_device(request, kCheckupTimeWindowMs);
        default:
            return false;
    }
}

}  // namespace modules
