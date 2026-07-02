#include "ShutdownState.hpp"

#include "HALGeneral.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"

namespace modules {

static const TaggedLogger kLog("ShutdownState");

definitions::SystemState ShutdownState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kShutdownComplete:
            completed_ = true;
            return definitions::SystemState::kStartup;
        default:
            return definitions::SystemState::kShutdown;
    }
}

definitions::SystemState ShutdownState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    if (completed_) {
        return definitions::SystemState::kStartup;
    }

    const unsigned long now_ms = hal_general().millis();
    if ((now_ms - enteredAtMs_) >= 20UL) {
        completed_ = true;
        return definitions::SystemState::kStartup;
    }

    return context->get_current_state();
}

void ShutdownState::on_enter(StateMachine* context) {
    (void) context;
    enteredAtMs_ = hal_general().millis();
    completed_ = false;
    kLog.info("Iniciando Estado: SHUTDOWN - Parada ordenada y guardado de estado");
    apply_safe_outputs();
    close_all_valves();
}

const char* ShutdownState::name() const {
    return "SHUTDOWN";
}

}  // namespace modules
