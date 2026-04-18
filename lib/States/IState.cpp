#include "IState.hpp"

#include "StateMachine.hpp"

bool interfaces::IState::dispatch_command(modules::StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;
    (void) request;
    return false;
}

void interfaces::IState::on_enter(modules::StateMachine* context) {
    (void) context;
}

void interfaces::IState::on_exit(modules::StateMachine* context) {
    (void) context;
}

definitions::SystemState interfaces::IState::tick(modules::StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}
