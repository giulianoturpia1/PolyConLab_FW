#include "HardErrorState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

definitions::SystemState modules::HardErrorState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kResetFault:
            resetFaultRequested_ = true;
            break;
        case definitions::SystemEvent::kConditionsOk:
            conditionsOk_ = true;
            break;
        case definitions::SystemEvent::kPhysicalReset:
            return definitions::SystemState::kStartup;
        default:
            break;
    }

    if (resetFaultRequested_ && conditionsOk_) {
        return definitions::SystemState::kStartup;
    }

    return definitions::SystemState::kHardError;
}

void modules::HardErrorState::on_enter(StateMachine* context) {
    (void) context;
    resetFaultRequested_ = false;
    conditionsOk_ = false;
    modules::hal_general().log("Iniciando Estado: HARD_ERROR - Sistema bloqueado");
    apply_safe_outputs();
    close_all_valves_if_supported();
}

const char* modules::HardErrorState::name() const {
    return "HARD_ERROR";
}

void modules::HardErrorState::apply_safe_outputs() {
    modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}

void modules::HardErrorState::close_all_valves_if_supported() {
#if defined(PLC_38AR) || defined(PLC_19R)
    modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose,
                                                     static_cast<uint32_t>(definitions::Pins::kValve1), 10);
    modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose,
                                                     static_cast<uint32_t>(definitions::Pins::kValve2), 10);
#endif
}
