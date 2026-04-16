#include "StateMachine.hpp"

#include "CalibrationState.hpp"
#include "CheckupState.hpp"
#include "HALGeneral.hpp"
#include "HardErrorState.hpp"
#include "IState.hpp"
#include "ManualState.hpp"
#include "PauseState.hpp"
#include "ShutdownState.hpp"
#include "SoftErrorState.hpp"
#include "StartupState.hpp"

// FreeRTOS includes, neccesary
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

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
constexpr unsigned long kCommandFeedbackTimeoutMs = 1000;
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
    commandPending_ = false;
    lastCommandConfirmed_ = false;
    pendingCommandTimeMs_ = 0;
    on_state_enter();
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

bool modules::StateMachine::queue_event(definitions::SystemEvent event, uint32_t timeout_ms) {
    if (gStateEventQueue == nullptr || xQueueSend(gStateEventQueue, &event, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

bool modules::StateMachine::queue_command(const definitions::CommandRequest& request, uint32_t timeout_ms) {
    if (gStateCommandQueue == nullptr || xQueueSend(gStateCommandQueue, &request, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

bool modules::StateMachine::queue_action(const definitions::CommandRequest& action, uint32_t timeout_ms) {
    if (gStateActionQueue == nullptr) {
        return false;
    }

    if (xQueueSend(gStateActionQueue, &action, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

void modules::StateMachine::tick() {
    handle_command_feedback_timeout();

    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        stateChanged_ = false;
        return;
    }

    const definitions::SystemState nextState = stateHandler->tick(this);
    if (nextState != currentState_) {
        transition_to(nextState);
    }

    stateChanged_ = false;
}

definitions::SystemState modules::StateMachine::get_current_state() const {
    return currentState_;
}

definitions::SystemState modules::StateMachine::get_previous_state() const {
    return previousState_;
}

const std::string& modules::StateMachine::get_state_name(definitions::SystemState state) {
    static const std::string unknown = "UNKNOWN";
    static std::string cachedName;

    for (const auto& entry : definitions::kStateNameMap) {
        if (entry.state == state) {
            cachedName = entry.name;
            return cachedName;
        }
    }
    return unknown;
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
      lastCommandConfirmed_(false) {
    stateHandlers_.emplace(definitions::SystemState::kStartup, std::unique_ptr<IState>(new StartupState()));
    stateHandlers_.emplace(definitions::SystemState::kCheckup, std::unique_ptr<IState>(new CheckupState()));
    stateHandlers_.emplace(definitions::SystemState::kCalibration, std::unique_ptr<IState>(new CalibrationState()));
    stateHandlers_.emplace(definitions::SystemState::kManual, std::unique_ptr<IState>(new ManualState()));
    stateHandlers_.emplace(definitions::SystemState::kPause, std::unique_ptr<IState>(new PauseState()));
    stateHandlers_.emplace(definitions::SystemState::kSoftError, std::unique_ptr<IState>(new SoftErrorState()));
    stateHandlers_.emplace(definitions::SystemState::kHardError, std::unique_ptr<IState>(new HardErrorState()));
    stateHandlers_.emplace(definitions::SystemState::kShutdown, std::unique_ptr<IState>(new ShutdownState()));
}

modules::IState* modules::StateMachine::get_state_handler(definitions::SystemState state) {
    auto it = stateHandlers_.find(state);
    if (it == stateHandlers_.end()) {
        return nullptr;
    }

    return it->second.get();
}

const modules::IState* modules::StateMachine::get_state_handler(definitions::SystemState state) const {
    auto it = stateHandlers_.find(state);
    if (it == stateHandlers_.end()) {
        return nullptr;
    }

    return it->second.get();
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

void modules::StateMachine::process_event(definitions::SystemEvent event) {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        return;
    }

    // First let the current state handler process the event and determine if a state transition is needed.
    const definitions::SystemState nextState = stateHandler->handle_event(this, event);
    if (nextState != currentState_) {
        transition_to(nextState);
    }
}

void modules::StateMachine::process_command(const definitions::CommandRequest& request) {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        lastCommandConfirmed_ = false;
        commandPending_ = false;
        return;
    }

    if (!stateHandler->handle_command(this, request)) {
        modules::hal_general().log("Comando recibido, pero no se pudo ejecutar en el estado actual: " + get_state_name(currentState_));
        lastCommandConfirmed_ = false;
        commandPending_ = false;
        return;
    }

    pendingCommand_ = request;
    pendingCommandTimeMs_ = modules::hal_general().millis();
    commandPending_ = true;
    lastCommandConfirmed_ = false;
}

void modules::StateMachine::process_action(const definitions::CommandRequest& action) {
    lastAction_ = action;

    if (commandPending_ && action_matches_command(pendingCommand_, action)) {
        commandPending_ = false;
        lastCommandConfirmed_ = true;
        modules::hal_general().log("Comando confirmado como ejecutado por modulo.");
    }

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
}

void modules::StateMachine::transition_to(definitions::SystemState newState) {
    if (newState == currentState_) {
        return;
    }

    on_state_exit();
    previousState_ = currentState_;
    currentState_ = newState;
    stateEntryTime_ = modules::hal_general().millis();
    stateChanged_ = true;
    on_state_enter();
}

void modules::StateMachine::on_state_exit() {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler != nullptr) {
        stateHandler->on_exit(this);
    }
}

void modules::StateMachine::on_state_enter() {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler != nullptr) {
        stateHandler->on_enter(this);
    }
}

void modules::StateMachine::handle_command_feedback_timeout() {
    if (!commandPending_) {
        return;
    }
    const unsigned long now_ms = modules::hal_general().millis();
    if ((now_ms - pendingCommandTimeMs_) >= kCommandFeedbackTimeoutMs) {
        modules::hal_general().log("Timeout esperando confirmacion de ejecucion para comando pendiente.");
        commandPending_ = false;
        lastCommandConfirmed_ = false;
    }
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
