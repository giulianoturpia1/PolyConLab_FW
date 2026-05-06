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

definitions::SystemState modules::ManualState::handle_event(StateMachine* context, definitions::SystemEvent event) {
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

bool modules::ManualState::dispatch_command(StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;

    switch (request.command) {
        case definitions::CommandType::kHeaterOn:
            return modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOn, kAcceptableTimeWindowMs);

        case definitions::CommandType::kHeaterOff:
            return modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kHeaterState:
            return modules::Heater::instance().queue_command(definitions::CommandType::kHeaterState, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpOn:
            return modules::Pump::instance().queue_command(definitions::CommandType::kPumpOn, kAcceptableTimeWindowMs);

        case definitions::CommandType::kPumpOff:
            return modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kPumpState:
            return modules::Pump::instance().queue_command(definitions::CommandType::kPumpState, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerOn:
            return modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOn, kAcceptableTimeWindowMs);

        case definitions::CommandType::kChillerOff:
            return modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, kAcceptableTimeWindowMs);
        case definitions::CommandType::kChillerState:
            return modules::Chiller::instance().queue_command(definitions::CommandType::kChillerState, kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveOpen:
            return modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveOpen, request.pin,
                                                                    kAcceptableTimeWindowMs);
        case definitions::CommandType::kValveClose:
            return modules::SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, request.pin,
                                                                    kAcceptableTimeWindowMs);
        case definitions::CommandType::kTempInRead:
            return modules::Thermocouple::instance().queue_command(definitions::CommandType::kTempInRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempOutRead:
            return modules::Thermocouple::instance().queue_command(definitions::CommandType::kTempOutRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempBathRead:
            return modules::Thermocouple::instance().queue_command(definitions::CommandType::kTempBathRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempMixRead:
            return modules::Thermocouple::instance().queue_command(definitions::CommandType::kTempMixRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowRead:
            return modules::Flowmeter::instance().queue_command(definitions::CommandType::kFlowRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowStatus:
            return modules::Flowmeter::instance().queue_command(definitions::CommandType::kFlowStatus, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowBegin:
            return modules::Flowmeter::instance().queue_command(definitions::CommandType::kFlowBegin, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowStop:
            return modules::Flowmeter::instance().queue_command(definitions::CommandType::kFlowStop, kAcceptableTimeWindowMs);

        case definitions::CommandType::kLevelSwitchState:
            return modules::LevelSwitch::instance().queue_command(definitions::CommandType::kLevelSwitchState, kAcceptableTimeWindowMs);
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
