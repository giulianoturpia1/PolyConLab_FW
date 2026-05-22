#include "ManualState.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

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

constexpr uint32_t kAcceptableTimeWindowMs = 100;

namespace modules {

static const TaggedLogger kLog("ManualState");

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

bool ManualState::dispatch_command(StateMachine* context, const definitions::Command& request) {
    (void) context;

    switch (request.type) {
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
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveOpen, request.extra, kAcceptableTimeWindowMs);

        case definitions::CommandType::kValveClose:
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveClose, request.extra, kAcceptableTimeWindowMs);

        case definitions::CommandType::kValveState:
            return SolenoidValve::instance().queue_command(definitions::CommandType::kValveState, request.extra, kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempInRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempInRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempIn), kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempOutRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempOutRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempOut), kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempBathRead:
            return Thermocouple::instance().queue_command(
                definitions::CommandType::kTempBathRead, static_cast<uint32_t>(definitions::Addresses::kTempBath), kAcceptableTimeWindowMs);

        case definitions::CommandType::kTempMixRead:
            return Thermocouple::instance().queue_command(definitions::CommandType::kTempMixRead,
                                                          static_cast<uint32_t>(definitions::Addresses::kTempMix), kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowRead:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowRead, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowStatus:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowStatus, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowOn:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowOn, kAcceptableTimeWindowMs);

        case definitions::CommandType::kFlowOff:
            return Flowmeter::instance().queue_command(definitions::CommandType::kFlowOff, kAcceptableTimeWindowMs);

        case definitions::CommandType::kLevelSwitchState:
            return LevelSwitch::instance().queue_command(definitions::CommandType::kLevelSwitchState, kAcceptableTimeWindowMs);
        default:
            return false;
    }
}

void ManualState::on_enter(StateMachine* context) {
    (void) context;
    kLog.info("Iniciando Estado: MANUAL - Esperando comandos del usuario");
}

const char* ManualState::name() const {
    return "MANUAL";
}

}  // namespace modules
