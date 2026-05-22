#include "CheckupState.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Logger.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

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
