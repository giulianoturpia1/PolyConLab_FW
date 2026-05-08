#include "ManualState.hpp"

#include "Chiller.hpp"
#include "FlowMeter.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "LevelSwitch.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"
#include "Thermocouple.hpp"

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
        case definitions::CommandType::kHeaterState:
            return Heater::instance().queue_command(definitions::CommandType::kHeaterState, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOn:
            return Pump::instance().queue_command(definitions::CommandType::kPumpOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOff:
            return Pump::instance().queue_command(definitions::CommandType::kPumpOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpState:
            return Pump::instance().queue_command(definitions::CommandType::kPumpState, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOn:
            return Chiller::instance().queue_command(definitions::CommandType::kChillerOn, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOff:
            return Chiller::instance().queue_command(definitions::CommandType::kChillerOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerState:
            return Chiller::instance().queue_command(definitions::CommandType::kChillerState, kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveOpen:
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveOpen, request.pin, kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveClose:
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, request.pin, kAcceptableTimeWindowMs);
        case definitions::CommandType::kTempInRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempInRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempOutRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempOutRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempBathRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempBathRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempMixRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempMixRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowRead:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowStatus:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowStatus, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowBegin:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowBegin, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowStop:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowStop, kAcceptableTimeWindowMs);

        case definitions::CommandType::kLevelSwitchState:
            return LevelSwitch::instance().queue_command(definitions::CommandType::kLevelSwitchState, kAcceptableTimeWindowMs);
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
