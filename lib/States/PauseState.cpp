#include "PauseState.hpp"

#include "Logger.hpp"
#include "StateMachine.hpp"

namespace modules {

static const TaggedLogger kLog("PauseState");

definitions::SystemState PauseState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    switch (event) {
        case definitions::SystemEvent::kUnpause:
            return context->get_previous_state() == definitions::SystemState::kPause ? definitions::SystemState::kManual
                                                                                     : context->get_previous_state();
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        default:
            return definitions::SystemState::kPause;
    }
}

void PauseState::on_enter(StateMachine* context) {
    (void) context;
    kLog.info("Iniciando Estado: PAUSE - Timers congelados y salida segura parcial");
}

const char* PauseState::name() const {
    return "PAUSE";
}

}  // namespace modules
