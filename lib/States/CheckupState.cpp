#include "CheckupState.hpp"

#include "Chiller.hpp"
#include "Flowmeter.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "LevelSwitch.hpp"
#include "Logger.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"
#include "Thermocouple.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

constexpr uint32_t kCheckupTimeWindowMs = 200;

namespace modules {

static const TaggedLogger kLog("CheckupState");

definitions::SystemState CheckupState::handle_event(StateMachine* context, definitions::SystemEvent event) {
    (void) context;

    switch (event) {
        case definitions::SystemEvent::kScadaOk:
            scadaOk_ = true;
            break;
        case definitions::SystemEvent::kSensorOk:
            sensorOk_ = true;
            break;
        case definitions::SystemEvent::kFaultRecoverable:
            return definitions::SystemState::kSoftError;
        case definitions::SystemEvent::kFaultCritical:
        case definitions::SystemEvent::kShutdownButton:
            return definitions::SystemState::kHardError;
        case definitions::SystemEvent::kShutdown:
            return definitions::SystemState::kShutdown;
        default:
            break;
    }

    if (scadaOk_ && sensorOk_) {
        return definitions::SystemState::kManual;
    }

    return definitions::SystemState::kCheckup;
}

void CheckupState::on_enter(StateMachine* context) {
    (void) context;
    scadaOk_ = false;
    sensorOk_ = false;
    kLog.info("Iniciando Estado: CHECKUP");
    apply_safe_outputs();
}

const char* CheckupState::name() const {
    return "CHECKUP";
}

bool CheckupState::dispatch_command(StateMachine* context, const definitions::Command& request) {
    (void) context;
    // Only read-only sensor queries are accepted during CHECKUP so the SCADA
    // can probe each device.  Actuator commands are always rejected here.
    switch (request.type) {
        case definitions::CommandType::kTempInRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempInRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempIn), kCheckupTimeWindowMs);

        case definitions::CommandType::kTempOutRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempOutRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempOut), kCheckupTimeWindowMs);

        case definitions::CommandType::kTempBathRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempBathRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempBath), kCheckupTimeWindowMs);

        case definitions::CommandType::kTempMixRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempMixRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempMix), kCheckupTimeWindowMs);

        case definitions::CommandType::kFlowRead:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowRead, kCheckupTimeWindowMs);

        case definitions::CommandType::kLevelSwitchState:
            return LevelSwitch::instance().queue_command(definitions::CommandType::kLevelSwitchState, kCheckupTimeWindowMs);

        default:
            return false;
    }
}

void CheckupState::apply_safe_outputs() {
    Heater::instance().queue_command(definitions::CommandType::kHeaterOff, 10);
    Pump::instance().queue_command(definitions::CommandType::kPumpOff, 10);
    Chiller::instance().queue_command(definitions::CommandType::kChillerOff, 10);
#if defined(PLC_38AR)
    SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, static_cast<uint32_t>(Q0_1), 10);
    SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, static_cast<uint32_t>(Q0_3), 10);
#elif defined(PLC_19R)
    SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, static_cast<uint32_t>(R1_1), 10);
    SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, static_cast<uint32_t>(R1_2), 10);
#endif
}

}  // namespace modules
