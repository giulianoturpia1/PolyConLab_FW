#include "ShutdownState.hpp"

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

definitions::SystemState modules::ShutdownState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kShutdownComplete:
            completed_ = true;
            return definitions::SystemState::kStartup;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
            return definitions::SystemState::kHardError;
        default:
            return definitions::SystemState::kShutdown;
    }
}

definitions::SystemState modules::ShutdownState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    if (completed_) {
        return definitions::SystemState::kStartup;
    }

    const unsigned long now_ms = modules::hal_general().millis();
    if ((now_ms - enteredAtMs_) >= 20UL) {
        completed_ = true;
        return definitions::SystemState::kStartup;
    }

    return context->get_current_state();
}

void modules::ShutdownState::on_enter(StateMachine* context) {
    (void) context;
    enteredAtMs_ = modules::hal_general().millis();
    completed_ = false;
    modules::hal_general().log("Iniciando Estado: SHUTDOWN - Parada ordenada y guardado de estado");
    apply_safe_outputs();
    close_all_valves_if_supported();
}

const char* modules::ShutdownState::name() const {
    return "SHUTDOWN";
}

void modules::ShutdownState::apply_safe_outputs() {
    modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}

void modules::ShutdownState::close_all_valves_if_supported() {
#if defined(PLC_38AR) || defined(PLC_19R)
    modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose,
                                                     static_cast<uint32_t>(definitions::Pins::kValve1), 10);
    modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose,
                                                     static_cast<uint32_t>(definitions::Pins::kValve2), 10);
#endif
}
