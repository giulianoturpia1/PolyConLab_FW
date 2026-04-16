#include "StartupState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "StateMachine.hpp"

definitions::SystemState modules::StartupState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kInitOk:
            return definitions::SystemState::kCheckup;
        case definitions::SystemEvent::kInitFatal:
        case definitions::SystemEvent::kWatchdogFatal:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kCmdShutdown:
            return definitions::SystemState::kShutdown;
        default:
            return definitions::SystemState::kStartup;
    }
}

void modules::StartupState::on_enter(StateMachine* context) {
    (void) context;
    modules::hal_general().log("Iniciando Estado: STARTUP");
    apply_safe_outputs();
}

const char* modules::StartupState::name() const {
    return "STARTUP";
}

definitions::SystemState modules::StartupState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}

void modules::StartupState::apply_safe_outputs() {
    modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}
