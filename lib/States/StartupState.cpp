#include "StartupState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Logger.hpp"
#include "Pump.hpp"
#include "StateMachine.hpp"

namespace modules {

static const TaggedLogger kLog("StartupState");

definitions::SystemState StartupState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kInitOk:
            return definitions::SystemState::kCheckup;
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
    kLog.info("Iniciando Estado: STARTUP");
    apply_safe_outputs();
}

const char* StartupState::name() const {
    return "STARTUP";
}

void StartupState::apply_safe_outputs() {
    Heater::instance().queue_command({definitions::CommandType::kHeaterOff, 0u}, 10);
    Pump::instance().queue_command({definitions::CommandType::kPumpOff, 0u}, 10);
    Chiller::instance().queue_command({definitions::CommandType::kChillerOff, 0u}, 10);
}

}  // namespace modules
