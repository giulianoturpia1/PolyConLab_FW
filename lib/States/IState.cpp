#include "IState.hpp"

#include "StateMachine.hpp"

bool modules::IState::handle_command(StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;
    (void) request;
    return false;
}

void modules::IState::on_enter(StateMachine* context) {
    (void) context;
}

void modules::IState::on_exit(StateMachine* context) {
    (void) context;
}

definitions::SystemState modules::IState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}
