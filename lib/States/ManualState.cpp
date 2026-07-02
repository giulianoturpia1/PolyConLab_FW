#include "ManualState.hpp"

#include "DeviceDispatcher.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"

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
        default:
            return definitions::SystemState::kManual;
    }
}

bool ManualState::dispatch_command(StateMachine* context, const definitions::Command& request) {
    (void) context;
    return DeviceDispatcher::instance().forward_to_device(request, kAcceptableTimeWindowMs);
}

void ManualState::on_enter(StateMachine* context) {
    (void) context;
    kLog.info("Iniciando Estado: MANUAL - Esperando comandos del usuario");
}

const char* ManualState::name() const {
    return "MANUAL";
}

}  // namespace modules
