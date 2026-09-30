#include "SoftErrorState.hpp"

#include "Config.hpp"
#include "DeviceDispatcher.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"

namespace modules {

static const TaggedLogger kLog("SoftErrorState");

definitions::SystemState SoftErrorState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    switch (event) {
        case definitions::SystemEvent::kRetryCheckup:
            retryRequested_ = true;
            break;
        case definitions::SystemEvent::kAckFault:
            ackRequested_ = true;
            break;
        case definitions::SystemEvent::kConditionCleared:
            conditionCleared_ = true;
            break;
        case definitions::SystemEvent::kEStop:
        case definitions::SystemEvent::kTooManyRetries:
            return definitions::SystemState::kHardError;
        default:
            break;
    }

    if (retryRequested_) {
        return definitions::SystemState::kCheckup;
    }

    if (ackRequested_ && conditionCleared_) {
        const definitions::SystemState previousState = context->get_previous_state();
        if (previousState == definitions::SystemState::kManual || previousState == definitions::SystemState::kPause) {
            return definitions::SystemState::kManual;
        }

        return definitions::SystemState::kCheckup;
    }

    return definitions::SystemState::kSoftError;
}

void SoftErrorState::on_enter(StateMachine* context) {
    (void) context;
    retryRequested_ = false;
    ackRequested_ = false;
    conditionCleared_ = false;
    kLog.info("Iniciando Estado: SOFT_ERROR - Error recuperable");
    apply_safe_outputs();
}

const char* SoftErrorState::name() const {
    return "SOFT_ERROR";
}

definitions::CommandOutcome SoftErrorState::handle_command(StateMachine* context, const definitions::Command& request,
                                                           definitions::Command& out_response) {
    (void) context;
    (void) out_response;
    // Only sensor re-initialisation is accepted while in SOFT ERROR — it lets the operator recover
    // a device that failed hardware init without leaving the recovery flow.  Everything else is rejected.
    switch (request.type) {
        case definitions::CommandType::kTempInReinit:
        case definitions::CommandType::kTempOutReinit:
        case definitions::CommandType::kTempBathReinit:
        case definitions::CommandType::kTempMixReinit:
        case definitions::CommandType::kFlowReinit:
        case definitions::CommandType::kLevelSwitchReinit:
            return DeviceDispatcher::instance().issue_to_device(request, config::kReinitCommandTimeoutMs)
                       ? definitions::CommandOutcome::kPending
                       : definitions::CommandOutcome::kRejected;
        default:
            return definitions::CommandOutcome::kRejected;
    }
}

}  // namespace modules
