#include "PauseState.hpp"

#include "Logger.hpp"
#include "StateMachine.hpp"

namespace modules {

static const TaggedLogger kLog("PauseState");

definitions::SystemState PauseState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    switch (event) {
        case definitions::SystemEvent::kUnpause:
            // Resume into MANUAL only when the pause interrupted MANUAL. A pause taken from any other
            // state re-enters CHECKUP on purpose: the sensors are re-verified before any actuator becomes available again.
            // HARD ERROR and SHUTDOWN are latched -- resuming must not wash them out.
            switch (context->get_previous_state()) {
                case definitions::SystemState::kManual:
                    return definitions::SystemState::kManual;
                case definitions::SystemState::kHardError:
                case definitions::SystemState::kShutdown:
                    return context->get_previous_state();
                default:
                    return definitions::SystemState::kCheckup;
            }
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
