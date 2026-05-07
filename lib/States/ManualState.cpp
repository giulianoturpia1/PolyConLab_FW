#include "ManualState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

constexpr uint32_t kAcceptableTimeWindowMs = 100;

namespace modules {

definitions::SystemState ManualState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kPause:
            return definitions::SystemState::kPause;
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
            return definitions::SystemState::kHardError;
        default:
            return definitions::SystemState::kManual;
    }
}

bool ManualState::dispatch_command(StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;

    switch (request.command) {
        case definitions::CommandType::kHeaterOn:
            return Heater::instance().queue_command(definitions::CommandType::kHeaterOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kHeaterOff:
            return Heater::instance().queue_command(definitions::CommandType::kHeaterOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOn:
            return Pump::instance().queue_command(definitions::CommandType::kPumpOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOff:
            return Pump::instance().queue_command(definitions::CommandType::kPumpOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOn:
            return Chiller::instance().queue_command(definitions::CommandType::kChillerOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOff:
            return Chiller::instance().queue_command(definitions::CommandType::kChillerOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveOpen:
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveOpen, request.pin, kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveClose:
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, request.pin, kAcceptableTimeWindowMs);
        default:
            return false;
    }
}

void ManualState::on_enter(StateMachine* context) {
    (void) context;
    hal_general().log("Iniciando Estado: MANUAL - Esperando comandos del usuario");
}

const char* ManualState::name() const {
    return "MANUAL";
}

}  // namespace modules
