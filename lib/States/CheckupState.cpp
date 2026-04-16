#include "CheckupState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "StateMachine.hpp"

definitions::SystemState modules::CheckupState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kScadaLinkOk:
            scadaLinkOk_ = true;
            break;
        case definitions::SystemEvent::kSensorsOk:
            sensorsOk_ = true;
            break;
        case definitions::SystemEvent::kStartCalibration:
            return definitions::SystemState::kCalibration;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
        case definitions::SystemEvent::kStop:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kCmdShutdown:
            return definitions::SystemState::kShutdown;
        default:
            break;
    }

    if (scadaLinkOk_ && sensorsOk_) {
        return definitions::SystemState::kManual;
    }

    return definitions::SystemState::kCheckup;
}

void modules::CheckupState::on_enter(StateMachine* context) {
    (void) context;
    scadaLinkOk_ = false;
    sensorsOk_ = false;
    modules::hal_general().log("Iniciando Estado: CHECKUP");
    apply_safe_outputs();
}

const char* modules::CheckupState::name() const {
    return "CHECKUP";
}

definitions::SystemState modules::CheckupState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}

void modules::CheckupState::apply_safe_outputs() {
    modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}
