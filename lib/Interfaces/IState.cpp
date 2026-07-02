#include "IState.hpp"

#include "Chiller.hpp"
#include "Definitions.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"
#include "TargetDefinitions.hpp"

namespace interfaces {

bool IState::dispatch_command(modules::StateMachine* context, const definitions::Command& request) {
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

void IState::apply_safe_outputs() {
    modules::Heater::instance().queue_command({definitions::CommandType::kHeaterOff, 0u}, 10);
    modules::Pump::instance().queue_command({definitions::CommandType::kPumpOff, 0u}, 10);
    modules::Chiller::instance().queue_command({definitions::CommandType::kChillerOff, 0u}, 10);
}

void IState::close_all_valves() {
    modules::SolenoidValve::instance().queue_command(
        {definitions::CommandType::kValveClose, static_cast<uint32_t>(definitions::Pins::kValve1)}, 10);
    modules::SolenoidValve::instance().queue_command(
        {definitions::CommandType::kValveClose, static_cast<uint32_t>(definitions::Pins::kValve2)}, 10);
}

}  // namespace interfaces
