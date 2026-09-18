#include "IState.hpp"

#include "Definitions.hpp"
#include "DeviceDispatcher.hpp"
#include "StateMachine.hpp"
#include "TargetDefinitions.hpp"

namespace interfaces {

definitions::CommandOutcome IState::handle_command(modules::StateMachine* context, const definitions::Command& request,
                                                   definitions::Command& out_response) {
    (void) context;
    (void) request;
    (void) out_response;
    return definitions::CommandOutcome::kRejected;
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

void IState::apply_safe_outputs() {
    // Safety actuation, not a client command: enforce it through the dispatcher so no
    // confirmation report is injected into the StateMachine response queue. The timeout is
    // ignored — every actuator is a SyncDevice that executes inline.
    modules::DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kHeaterOff, 0u}, 0);
    modules::DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kPumpOff, 0u}, 0);
    modules::DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kChillerOff, 0u}, 0);
    modules::DeviceDispatcher::instance().enforce_to_device({definitions::CommandType::kMixerOff, 0u}, 0);
}

void IState::close_all_valves() {
    // Safety actuation, not a client command: see apply_safe_outputs().
    modules::DeviceDispatcher::instance().enforce_to_device(
        {definitions::CommandType::kValveClose, static_cast<uint32_t>(definitions::Pins::kValve1)}, 0);
    modules::DeviceDispatcher::instance().enforce_to_device(
        {definitions::CommandType::kValveClose, static_cast<uint32_t>(definitions::Pins::kValve2)}, 0);
}

}  // namespace interfaces
