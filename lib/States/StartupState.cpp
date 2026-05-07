#include "StartupState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "StateMachine.hpp"

namespace modules {

definitions::SystemState StartupState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kInitOk:
            return definitions::SystemState::kCheckup;
        case definitions::SystemEvent::kInitFatal:
        case definitions::SystemEvent::kWatchdogFatal:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        default:
            return definitions::SystemState::kStartup;
    }
}

void StartupState::on_enter(StateMachine* context) {
    (void) context;
    hal_general().log("Iniciando Estado: STARTUP");
    apply_safe_outputs();
}

const char* StartupState::name() const {
    return "STARTUP";
}

void StartupState::apply_safe_outputs() {
    Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}

}  // namespace modules
