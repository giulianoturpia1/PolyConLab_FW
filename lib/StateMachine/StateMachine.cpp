#include "StateMachine.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"

// FreeRTOS includes, neccesary
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <algorithm>

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

QueueHandle_t gStateEventQueue = nullptr;
QueueHandle_t gStateCommandQueue = nullptr;
QueueHandle_t gStateActionQueue = nullptr;
constexpr uint32_t kStateMachineTaskPeriodMs = 20;
constexpr UBaseType_t kStateMachineTaskPriority = 3;
constexpr UBaseType_t kStateMachineQueueLength = 16;
constexpr uint32_t kStateMachineTaskStackSize = 4096;
constexpr char kStateMachineTaskName[] = "state_machine_task";

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::StateMachine& modules::StateMachine::instance() {
    static StateMachine instance;
    return instance;
}

void modules::StateMachine::initialize() {
    currentState_ = definitions::SystemState::kStartup;
    previousState_ = definitions::SystemState::kStartup;
    stateEntryTime_ = modules::hal_general().millis();
    stateChanged_ = true;
    on_state_enter();
}

void modules::StateMachine::tick() {
    // Execute the current state handler
    switch (currentState_) {
        case definitions::SystemState::kStartup:
            handle_startup();
            break;
        case definitions::SystemState::kCheckup:
            handle_checkup();
            break;
        case definitions::SystemState::kCalibration:
            handle_calibration();
            break;
        case definitions::SystemState::kManual:
            handle_manual();
            break;
        case definitions::SystemState::kSoftError:
            handle_soft_error();
            break;
        case definitions::SystemState::kCriticalError:
            handle_critical_error();
            break;

        default:
            // Unknown state, transition to CRITICAL_ERROR
            transition_to(definitions::SystemState::kCriticalError);
            break;
    }

    stateChanged_ = false;
}

void modules::StateMachine::start_task() {
    if (gStateEventQueue != nullptr) {
        modules::hal_general().log("La tarea de la maquina de estados ya esta en ejecucion.");
        return;
    }

    gStateEventQueue = xQueueCreate(kStateMachineQueueLength, sizeof(definitions::SystemEvent));
    gStateCommandQueue = xQueueCreate(kStateMachineQueueLength, sizeof(definitions::CommandRequest));
    gStateActionQueue = xQueueCreate(kStateMachineQueueLength, sizeof(definitions::CommandRequest));
    if (gStateEventQueue == nullptr || gStateCommandQueue == nullptr || gStateActionQueue == nullptr) {
        modules::hal_general().log("No se pudo crear la queue de la maquina de estados.");
        if (gStateEventQueue != nullptr) {
            vQueueDelete(gStateEventQueue);
            gStateEventQueue = nullptr;
        }
        if (gStateCommandQueue != nullptr) {
            vQueueDelete(gStateCommandQueue);
            gStateCommandQueue = nullptr;
        }
        if (gStateActionQueue != nullptr) {
            vQueueDelete(gStateActionQueue);
            gStateActionQueue = nullptr;
        }
        return;
    }

    if (xTaskCreate(task_entry, kStateMachineTaskName, kStateMachineTaskStackSize, nullptr, kStateMachineTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea de la maquina de estados.");
        vQueueDelete(gStateEventQueue);
        vQueueDelete(gStateCommandQueue);
        vQueueDelete(gStateActionQueue);
        gStateEventQueue = nullptr;
        gStateCommandQueue = nullptr;
        gStateActionQueue = nullptr;
    }
}

bool modules::StateMachine::enqueue_event(definitions::SystemEvent event, uint32_t timeout_ms) {
    if (gStateEventQueue == nullptr || xQueueSend(gStateEventQueue, &event, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

bool modules::StateMachine::enqueue_command(const definitions::CommandRequest& request, uint32_t timeout_ms) {
    if (gStateCommandQueue == nullptr || xQueueSend(gStateCommandQueue, &request, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

bool modules::StateMachine::enqueue_action(const definitions::CommandRequest& action, uint32_t timeout_ms) {
    if (gStateActionQueue == nullptr) {
        return false;
    }

    if (xQueueSend(gStateActionQueue, &action, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

definitions::SystemState modules::StateMachine::get_current_state() const {
    return currentState_;
}

definitions::SystemState modules::StateMachine::get_previous_state() const {
    return previousState_;
}

bool modules::StateMachine::is_command_pending() const {
    return commandPending_;
}

bool modules::StateMachine::was_last_command_confirmed() const {
    return lastCommandConfirmed_;
}

bool modules::StateMachine::is_in_state(definitions::SystemState state) const {
    return currentState_ == state;
}

const std::string& modules::StateMachine::get_state_name(definitions::SystemState state) {
    static const std::string unknown = "UNKNOWN";
    auto it = stateNames_.find(state);
    return (it != stateNames_.end()) ? it->second : unknown;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::StateMachine::StateMachine()
    : currentState_(definitions::SystemState::kStartup),
      previousState_(definitions::SystemState::kStartup),
      stateEntryTime_(0),
      stateChanged_(false),
      lastAction_({definitions::CommandType::kHeaterOff, 0}),
      pendingCommand_({definitions::CommandType::kHeaterOff, 0}),
      pendingCommandTimeMs_(0),
      commandPending_(false),
      lastCommandConfirmed_(false) {}

void modules::StateMachine::process_event(definitions::SystemEvent event) {
    definitions::SystemState nextState = currentState_;
    switch (currentState_) {
        case definitions::SystemState::kStartup:
            if (event == definitions::SystemEvent::kInitOk) {
                nextState = definitions::SystemState::kCheckup;
            } else if (event == definitions::SystemEvent::kInitFatal || event == definitions::SystemEvent::kWatchdogFatal) {
                nextState = definitions::SystemState::kCriticalError;
            }
            break;

        case definitions::SystemState::kCheckup:
            if (event == definitions::SystemEvent::kScadaLinkOk && event == definitions::SystemEvent::kSensorsOk) {
                nextState = definitions::SystemState::kManual;
            } else if (event == definitions::SystemEvent::kStartCalibration) {
                nextState = definitions::SystemState::kCalibration;
            } else if (event == definitions::SystemEvent::kFaultRecoverable) {
                nextState = definitions::SystemState::kSoftError;
            } else if (event == definitions::SystemEvent::kFaultCritical) {
                nextState = definitions::SystemState::kCriticalError;
            } else if (event == definitions::SystemEvent::kCmdResetFault) {
                nextState = definitions::SystemState::kStartup;
            }
            break;

        case definitions::SystemState::kCalibration:
            if (event == definitions::SystemEvent::kFinishCalibration) {
                nextState = definitions::SystemState::kCheckup;
            } else if (event == definitions::SystemEvent::kFaultRecoverable) {
                nextState = definitions::SystemState::kSoftError;
            } else if (event == definitions::SystemEvent::kFaultCritical) {
                nextState = definitions::SystemState::kCriticalError;
            }
            break;

        case definitions::SystemState::kManual:
            if (event == definitions::SystemEvent::kCmdStartAuto || event == definitions::SystemEvent::kCmdStartDemo) {
                // Stay in MANUAL for now (Auto/Demo states not implemented yet)
                nextState = definitions::SystemState::kManual;
            } else if (event == definitions::SystemEvent::kStartCalibration) {
                nextState = definitions::SystemState::kCalibration;
            } else if (event == definitions::SystemEvent::kFaultRecoverable) {
                nextState = definitions::SystemState::kSoftError;
            } else if (event == definitions::SystemEvent::kFaultCritical || event == definitions::SystemEvent::kStop) {
                nextState = definitions::SystemState::kCriticalError;
            } else if (event == definitions::SystemEvent::kCmdAbort) {
                nextState = definitions::SystemState::kManual;
            }
            break;

        case definitions::SystemState::kSoftError:
            if (event == definitions::SystemEvent::kCmdRetryCheck && event == definitions::SystemEvent::kConditionCleared) {
                nextState = previousState_;  // Return to previous state
            } else if (event == definitions::SystemEvent::kFaultCritical || event == definitions::SystemEvent::kTooManyRetries) {
                nextState = definitions::SystemState::kCriticalError;
            }
            break;

        case definitions::SystemState::kCriticalError:
            if (event == definitions::SystemEvent::kCmdResetFault || event == definitions::SystemEvent::kShutdownButton ||
                event == definitions::SystemEvent::kCmdShutdown) {
                nextState = definitions::SystemState::kStartup;
            }
            break;
    }

    if (nextState != currentState_) {
        transition_to(nextState);
    }
}

void modules::StateMachine::process_command(const definitions::CommandRequest& request) {
    if (!(currentState_ == definitions::SystemState::kManual || currentState_ == definitions::SystemState::kCalibration)) {
        modules::hal_general().log("Command received but actuation is not allowed in current state: " + get_state_name(currentState_));
        lastCommandConfirmed_ = false;
        commandPending_ = false;
        return;
    }

    if (!dispatch_to_module(request)) {
        modules::hal_general().log("No se pudo despachar el comando solicitado.");
        lastCommandConfirmed_ = false;
        commandPending_ = false;
        return;
    }

    pendingCommand_ = request;
    pendingCommandTimeMs_ = modules::hal_general().millis();
    commandPending_ = true;
    lastCommandConfirmed_ = false;
}

void modules::StateMachine::transition_to(definitions::SystemState newState) {
    on_state_exit();
    previousState_ = currentState_;
    currentState_ = newState;
    stateEntryTime_ = modules::hal_general().millis();
    stateChanged_ = true;
    on_state_enter();
}

void modules::StateMachine::on_state_exit() {
    // Cleanup for the current state before leaving
    // Can be extended with specific exit actions per state
}

void modules::StateMachine::on_state_enter() {
    // Safety-oriented entry actions are centralized here to run exactly once per transition.
    switch (currentState_) {
        case definitions::SystemState::kStartup:
        case definitions::SystemState::kCheckup:
        case definitions::SystemState::kCalibration:
        case definitions::SystemState::kSoftError: {
            modules::Heater::instance().send_command(definitions::CommandType::kHeaterOff, 10);
            modules::Pump::instance().send_command(definitions::CommandType::kPumpOff, 10);
            modules::Chiller::instance().send_command(definitions::CommandType::kChillerOff, 10);
        } break;

        case definitions::SystemState::kCriticalError: {
            modules::Heater::instance().send_command(definitions::CommandType::kHeaterOff, 10);
            modules::Pump::instance().send_command(definitions::CommandType::kPumpOff, 10);
            modules::Chiller::instance().send_command(definitions::CommandType::kChillerOff, 10);
#if defined(PLC_38AR) || defined(PLC_19R)
            modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveClose,
                                                            static_cast<uint32_t>(definitions::Pins::kValve1), 10);
            modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveClose,
                                                            static_cast<uint32_t>(definitions::Pins::kValve2), 10);
#endif
        } break;

        case definitions::SystemState::kManual:
        default:
            break;
    }
}

bool modules::StateMachine::dispatch_to_module(const definitions::CommandRequest& request) {
    static uint32_t acceptableTimeWindowMs = 100;  // Time window to accept command confirmations
    switch (request.command) {
        case definitions::CommandType::kHeaterOn:
            return modules::Heater::instance().send_command(definitions::CommandType::kHeaterOn, acceptableTimeWindowMs);
        case definitions::CommandType::kHeaterOff:
            return modules::Heater::instance().send_command(definitions::CommandType::kHeaterOff, acceptableTimeWindowMs);
        case definitions::CommandType::kPumpOn:
            return modules::Pump::instance().send_command(definitions::CommandType::kPumpOn, acceptableTimeWindowMs);
        case definitions::CommandType::kPumpOff:
            return modules::Pump::instance().send_command(definitions::CommandType::kPumpOff, acceptableTimeWindowMs);
        case definitions::CommandType::kChillerOn:
            return modules::Chiller::instance().send_command(definitions::CommandType::kChillerOn, acceptableTimeWindowMs);
        case definitions::CommandType::kChillerOff:
            return modules::Chiller::instance().send_command(definitions::CommandType::kChillerOff, acceptableTimeWindowMs);
        case definitions::CommandType::kValveOpen:
            return modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveOpen, request.pin,
                                                                   acceptableTimeWindowMs);
        case definitions::CommandType::kValveClose:
            return modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveClose, request.pin,
                                                                   acceptableTimeWindowMs);
        default:
            return false;
    }
}

bool modules::StateMachine::action_matches_command(const definitions::CommandRequest& request,
                                                   const definitions::CommandRequest& action) const {
    if (request.command != action.command) {
        return false;
    }

    switch (request.command) {
        case definitions::CommandType::kValveOpen:
        case definitions::CommandType::kValveClose:
            return request.pin == action.pin;
        default:
            return true;
    }
}

void modules::StateMachine::handle_command_feedback_timeout() {
    if (!commandPending_) {
        return;
    }

    constexpr unsigned long kCommandFeedbackTimeoutMs = 1000;
    const unsigned long now_ms = modules::hal_general().millis();
    if ((now_ms - pendingCommandTimeMs_) >= kCommandFeedbackTimeoutMs) {
        modules::hal_general().log("Timeout esperando confirmacion de ejecucion para comando pendiente.");
        commandPending_ = false;
        lastCommandConfirmed_ = false;
    }
}

void modules::StateMachine::handle_startup() {
    if (stateChanged_) {
        modules::hal_general().log("Iniciando Estado: STARTUP");
    }

    handle_command_feedback_timeout();

    // TODO: Initialize HW/RTOS
    // TODO: Initialize TCP/BLE
    // TODO: Initialize lights
    // TODO: Call processEvent(definitions::SystemEvent::INIT_OK) when initialization is complete
}

void modules::StateMachine::handle_checkup() {
    // Diagnostic phase - check system health
    if (stateChanged_) {
        modules::hal_general().log("Iniciando Estado: CHECKUP");
    }

    handle_command_feedback_timeout();

    // TODO: Health-check sensors
    // TODO: Handshake SCADA/BLE
    // TODO: Perform analog checks
    // TODO: Call processEvent(definitions::SystemEvent::SCADA_LINK_OK) and
    // processEvent(definitions::SystemEvent::SENSORS_OK) when checks pass
}

void modules::StateMachine::handle_calibration() {
    // Calibration phase with ordered stop and protocol execution
    if (stateChanged_) {
        modules::hal_general().log("Iniciando Estado: CALIBRATION");
    }

    handle_command_feedback_timeout();

    // TODO: Execute ordered stop
    // TODO: Execute calibration protocol
    // TODO: Return to automatic checkups
    // TODO: Call processEvent(definitions::SystemEvent::FINISH_CALIBRATION) when complete
}

void modules::StateMachine::handle_manual() {
    // Manual/Interactive mode - process user commands and monitor device feedback
    if (stateChanged_) {
        modules::hal_general().log("Iniciando Estado: MANUAL - Esperando comandos del usuario");
    }

    handle_command_feedback_timeout();
}

void modules::StateMachine::handle_soft_error() {
    // Partial error state - log error and allow recovery
    if (stateChanged_) {
        modules::hal_general().log("Iniciando Estado: SOFT_ERROR - Error recuperable");
    }

    handle_command_feedback_timeout();

    // TODO: Implement partial stop
    // TODO: Display HMI warning + LED/Buzzer
    // TODO: Allow retry/acknowledgment
}

void modules::StateMachine::handle_critical_error() {
    // Critical error state - full stop and lockdown
    if (stateChanged_) {
        modules::hal_general().log("Iniciando Estado: CRITICAL_ERROR - Sistema bloqueado");
    }

    handle_command_feedback_timeout();

    // TODO: Implement full stop
    // TODO: Lock control
    // TODO: Require reset
}

void modules::StateMachine::process_action(const definitions::CommandRequest& action) {
    // Store the last action for state handlers to reference
    lastAction_ = action;

    if (commandPending_ && action_matches_command(pendingCommand_, action)) {
        commandPending_ = false;
        lastCommandConfirmed_ = true;
        modules::hal_general().log("Comando confirmado como ejecutado por modulo.");
    }

    // Log the action for debugging
    switch (action.command) {
        case definitions::CommandType::kHeaterOn:
            modules::hal_general().log("Accion: Calentador encendido");
            break;
        case definitions::CommandType::kHeaterOff:
            modules::hal_general().log("Accion: Calentador apagado");
            break;
        case definitions::CommandType::kPumpOn:
            modules::hal_general().log("Accion: Bomba encendida");
            break;
        case definitions::CommandType::kPumpOff:
            modules::hal_general().log("Accion: Bomba apagada");
            break;
        case definitions::CommandType::kChillerOn:
            modules::hal_general().log("Accion: Enfriador encendido");
            break;
        case definitions::CommandType::kChillerOff:
            modules::hal_general().log("Accion: Enfriador apagado");
            break;
        case definitions::CommandType::kValveOpen:
            modules::hal_general().log("Accion: Valvula abierta");
            break;
        case definitions::CommandType::kValveClose:
            modules::hal_general().log("Accion: Valvula cerrada");
            break;
        default:
            break;
    }

    // State-specific action processing can be added in state handlers
    // For now, actions are just logged for feedback confirmation
}

void modules::StateMachine::task_entry(void* param) {
    definitions::SystemEvent event = definitions::SystemEvent::kPowerOn;
    definitions::CommandRequest command = {definitions::CommandType::kHeaterOff, 0};
    definitions::CommandRequest action = {definitions::CommandType::kHeaterOff, 0};

    while (true) {
        while (xQueueReceive(gStateEventQueue, &event, 0) == pdPASS) {
            modules::StateMachine::instance().process_event(event);
        }

        while (xQueueReceive(gStateCommandQueue, &command, 0) == pdPASS) {
            modules::StateMachine::instance().process_command(command);
        }

        while (xQueueReceive(gStateActionQueue, &action, 0) == pdPASS) {
            modules::StateMachine::instance().process_action(action);
        }

        // Tick the state machine to execute the current state handler
        modules::StateMachine::instance().tick();
        vTaskDelay(pdMS_TO_TICKS(kStateMachineTaskPeriodMs));
    }
}

////////////////////////////////////// State Names Mapping //////////////////////////////////////
const std::map<definitions::SystemState, std::string> modules::StateMachine::stateNames_ = {
    {definitions::SystemState::kStartup, "STARTUP"},         {definitions::SystemState::kCheckup, "CHECKUP"},
    {definitions::SystemState::kCalibration, "CALIBRATION"}, {definitions::SystemState::kManual, "MANUAL"},
    {definitions::SystemState::kSoftError, "SOFT_ERROR"},    {definitions::SystemState::kCriticalError, "CRITICAL_ERROR"}};
