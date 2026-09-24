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
    // Partial safe output (state diagram), as in SOFT ERROR: every relay output to its configured safe
    // state (heater and pump OFF, mixer ON), valves left as they are. The temperature interlocks only
    // run in MANUAL, so without this a heater left ON would heat unguarded for as long as the pause
    // lasts -- including a pause forced by a lost SCADA link.
    apply_safe_outputs();
    kLog.info("Iniciando Estado: PAUSE - Timers congelados y salida segura parcial");
}

const char* PauseState::name() const {
    return "PAUSE";
}

}  // namespace modules
