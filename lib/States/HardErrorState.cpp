#include "HardErrorState.hpp"

#include "Logger.hpp"
#include "StateMachine.hpp"

namespace modules {

static const TaggedLogger kLog("HardErrorState");

definitions::SystemState HardErrorState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kResetFault:
            resetFaultRequested_ = true;
            break;
        case definitions::SystemEvent::kConditionsOk:
            conditionsOk_ = true;
            break;
        case definitions::SystemEvent::kPhysicalReset:
            return definitions::SystemState::kStartup;
        default:
            break;
    }

    if (resetFaultRequested_ && conditionsOk_) {
        return definitions::SystemState::kStartup;
    }

    return definitions::SystemState::kHardError;
}

void HardErrorState::on_enter(StateMachine* context) {
    (void) context;
    resetFaultRequested_ = false;
    conditionsOk_ = false;
    kLog.error("Iniciando Estado: HARD_ERROR - Sistema bloqueado");
    apply_safe_outputs();
    close_all_valves();
}

const char* HardErrorState::name() const {
    return "HARD_ERROR";
}

}  // namespace modules
