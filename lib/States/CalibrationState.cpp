#include "CalibrationState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

namespace modules {

definitions::SystemState CalibrationState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kRetryCheckup:
            return definitions::SystemState::kCheckup;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        default:
            return definitions::SystemState::kCalibration;
    }
}

bool CalibrationState::dispatch_command(StateMachine* context, const definitions::CommandRequest& request) {
    (void) context;

    constexpr uint32_t kAcceptableTimeWindowMs = 100;
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

void CalibrationState::on_enter(StateMachine* context) {
    (void) context;
    hal_general().log("Iniciando Estado: CALIBRATION");
    apply_safe_outputs();
}

const char* CalibrationState::name() const {
    return "CALIBRATION";
}

void CalibrationState::apply_safe_outputs() {
    Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
}

}  // namespace modules
