#include "PauseState.hpp"

#include "HALGeneral.hpp"
#include "StateMachine.hpp"

definitions::SystemState modules::PauseState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    switch (event) {
        case definitions::SystemEvent::kCmdResume:
        case definitions::SystemEvent::kCmdAbort:
            return context->get_previous_state() == definitions::SystemState::kPause ? definitions::SystemState::kManual
                                                                                     : context->get_previous_state();
        case definitions::SystemEvent::kCmdShutdown:
            return definitions::SystemState::kShutdown;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
        case definitions::SystemEvent::kStop:
            return definitions::SystemState::kHardError;
        default:
            return definitions::SystemState::kPause;
    }
}

void modules::PauseState::on_enter(StateMachine* context) {
    (void) context;
    modules::hal_general().log("Iniciando Estado: PAUSE - Timers congelados y salida segura parcial");
}

const char* modules::PauseState::name() const {
    return "PAUSE";
}

definitions::SystemState modules::PauseState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}
