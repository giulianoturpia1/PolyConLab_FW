#include "IState.hpp"

#include "StateMachine.hpp"

namespace interfaces {

bool IState::dispatch_command(modules::StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;
    (void) request;
    return false;
}

void IState::on_enter(modules::StateMachine* context) {
    (void) context;
}

void IState::on_exit(modules::StateMachine* context) {
    (void) context;
}

definitions::SystemState IState::tick(modules::StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}

}  // namespace interfaces
