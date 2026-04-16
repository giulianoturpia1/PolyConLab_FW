#include "ManualState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

constexpr uint32_t kAcceptableTimeWindowMs = 100;

definitions::SystemState modules::ManualState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kCmdPause:
            return definitions::SystemState::kPause;
        case definitions::SystemEvent::kCmdShutdown:
            return definitions::SystemState::kShutdown;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
        case definitions::SystemEvent::kStop:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kStartCalibration:
            return definitions::SystemState::kCalibration;
        default:
            return definitions::SystemState::kManual;
    }
}

bool modules::ManualState::dispatch_command(StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;

    switch (request.command) {
        case definitions::CommandType::kHeaterOn:
            return modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kHeaterOff:
            return modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOn:
            return modules::Pump::instance().queue_command(definitions::CommandType::kPumpOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOff:
            return modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOn:
            return modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOff:
            return modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveOpen:
            return modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveOpen, request.pin,
                                                                    kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveClose:
            return modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, request.pin,
                                                                    kAcceptableTimeWindowMs);
        default:
            return false;
    }
}

void modules::ManualState::on_enter(StateMachine* context) {
    (void) context;
    modules::hal_general().log("Iniciando Estado: MANUAL - Esperando comandos del usuario");
}

const char* modules::ManualState::name() const {
    return "MANUAL";
}

definitions::SystemState modules::ManualState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}
