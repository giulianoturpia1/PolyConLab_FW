#include "CalibrationState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

definitions::SystemState modules::CalibrationState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kFinishCalibration:
            return definitions::SystemState::kCheckup;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
        case definitions::SystemEvent::kStop:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kCmdShutdown:
            return definitions::SystemState::kShutdown;
        default:
            return definitions::SystemState::kCalibration;
    }
}

bool modules::CalibrationState::dispatch_command(StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;

    constexpr uint32_t kAcceptableTimeWindowMs = 100;
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

void modules::CalibrationState::on_enter(StateMachine* context) {
    (void) context;
    modules::hal_general().log("Iniciando Estado: CALIBRATION");
    apply_safe_outputs();
}

const char* modules::CalibrationState::name() const {
    return "CALIBRATION";
}

definitions::SystemState modules::CalibrationState::tick(StateMachine* context) {
    if (context == nullptr) {
        return definitions::SystemState::kHardError;
    }

    return context->get_current_state();
}

void modules::CalibrationState::apply_safe_outputs() {
    modules::Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    modules::Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    modules::Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}
