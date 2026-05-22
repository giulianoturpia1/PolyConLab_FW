#include "SoftErrorState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "StateMachine.hpp"

namespace modules {

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
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
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
    hal_general().log("Iniciando Estado: SOFT_ERROR - Error recuperable");
    apply_safe_outputs();
}

const char* SoftErrorState::name() const {
    return "SOFT_ERROR";
}

void SoftErrorState::apply_safe_outputs() {
    Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}

}  // namespace modules
