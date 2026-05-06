#include "StateMachine.hpp"

#include "CalibrationState.hpp"
#include "CheckupState.hpp"
#include "HALGeneral.hpp"
#include "HardErrorState.hpp"
#include "Helpers.hpp"
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
QueueHandle_t gStateResponseQueue = nullptr;
constexpr uint32_t kStateMachineTaskPeriodMs = 20;
constexpr UBaseType_t kStateMachineTaskPriority = 3;
constexpr UBaseType_t kStateMachineQueueLength = 16;
constexpr uint32_t kStateMachineTaskStackSize = 4096;
constexpr unsigned long kCommandFeedbackTimeoutMs = 1000;
constexpr uint8_t kMaxEventsPerTick = 4;
constexpr uint8_t kMaxCommandsPerTick = 4;
constexpr uint8_t kMaxResponsesPerTick = 4;
definitions::SystemEvent currentEvent = definitions::SystemEvent::kPowerOn;
definitions::CommandRequest currentCommand = {definitions::CommandType::kHeaterOff, 0};
definitions::CommandRequest currentResponse = {definitions::CommandType::kHeaterOff, 0};
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
    gStateResponseQueue = xQueueCreate(kStateMachineQueueLength, sizeof(definitions::CommandRequest));
    if (gStateEventQueue == nullptr || gStateCommandQueue == nullptr || gStateResponseQueue == nullptr) {
        modules::hal_general().log("No se pudo crear la queue de la maquina de estados.");
        if (gStateEventQueue != nullptr) {
            vQueueDelete(gStateEventQueue);
            gStateEventQueue = nullptr;
        }
        if (gStateCommandQueue != nullptr) {
            vQueueDelete(gStateCommandQueue);
            gStateCommandQueue = nullptr;
        }
        if (gStateResponseQueue != nullptr) {
            vQueueDelete(gStateResponseQueue);
            gStateResponseQueue = nullptr;
        }
        return;
    }

    if (xTaskCreate(task_entry, kStateMachineTaskName, kStateMachineTaskStackSize, nullptr, kStateMachineTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea de la maquina de estados.");
        vQueueDelete(gStateEventQueue);
        vQueueDelete(gStateCommandQueue);
        vQueueDelete(gStateResponseQueue);
        gStateEventQueue = nullptr;
        gStateCommandQueue = nullptr;
        gStateResponseQueue = nullptr;
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

bool modules::StateMachine::queue_response(const definitions::CommandRequest& response, uint32_t timeout_ms) {
    if (gStateResponseQueue == nullptr) {
        return false;
    }

    if (xQueueSend(gStateResponseQueue, &response, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

void modules::StateMachine::tick() {
    // Process a bounded number of queued events, commands, and responses.
    attend_queues();

    // Check for command feedback timeout if there is a pending command.
    handle_command_feedback_timeout();

    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        stateChanged_ = false;
        return;
    }

    // Activate the current state handler's periodic tick and check for internal state transitions.
    // State-driven Transition.
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
      lastResponse_({definitions::CommandType::kHeaterOff, 0}),
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

bool modules::StateMachine::response_matches_command(const definitions::CommandRequest& request,
                                                     const definitions::CommandRequest& response) const {
    if (request.command != response.command) {
        return false;
    }

    switch (request.command) {
        case definitions::CommandType::kValveOpen:
        case definitions::CommandType::kValveClose:
            return request.pin == response.pin;
        default:
            return true;
    }
}

void modules::StateMachine::process_event(definitions::SystemEvent event) {
    // Special handling for communication loss event: pause the system regardless of current state.
    if (event == definitions::SystemEvent::kCommunicationLost) {
        modules::hal_general().log("Conexion Ethernet perdida, pausando sistema.");
        transition_to(definitions::SystemState::kPause);
        return;
    }

    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        return;
    }

    // First let the current state handler process the event and determine if a state transition is needed.
    // Event-driven Transition.
    const definitions::SystemState nextState = stateHandler->handle_event(this, event);
    if (nextState != currentState_) {
        transition_to(nextState);
    }
}

void modules::StateMachine::dispatch_command_request(const definitions::CommandRequest& request) {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        lastCommandConfirmed_ = false;
        commandPending_ = false;
        return;
    }

    if (!stateHandler->dispatch_command(this, request)) {
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

void modules::StateMachine::process_response(const definitions::CommandRequest& response) {
    lastResponse_ = response;

    if (commandPending_ && response_matches_command(pendingCommand_, response)) {
        commandPending_ = false;
        lastCommandConfirmed_ = true;
        modules::hal_general().log("Comando confirmado como ejecutado por modulo.");
    }

    const char* responseText = utils::response_for_command(response.command);
    if (responseText == nullptr) {
        return;
    }

    std::string logMessage = "Respuesta: ";
    logMessage += responseText;
    if (response.command == definitions::CommandType::kValveOpen || response.command == definitions::CommandType::kValveClose) {
        logMessage += std::to_string(response.pin);
    }

    modules::hal_general().log(logMessage);
}

void modules::StateMachine::attend_queues() {
    if (gStateEventQueue != nullptr) {
        for (uint8_t i = 0; i < kMaxEventsPerTick; ++i) {
            if (xQueueReceive(gStateEventQueue, &currentEvent, 0) != pdPASS) {
                break;
            }
            process_event(currentEvent);
        }
    }

    if (gStateCommandQueue != nullptr) {
        for (uint8_t i = 0; i < kMaxCommandsPerTick; ++i) {
            if (xQueueReceive(gStateCommandQueue, &currentCommand, 0) != pdPASS) {
                break;
            }
            dispatch_command_request(currentCommand);
        }
    }

    if (gStateResponseQueue != nullptr) {
        for (uint8_t i = 0; i < kMaxResponsesPerTick; ++i) {
            if (xQueueReceive(gStateResponseQueue, &currentResponse, 0) != pdPASS) {
                break;
            }
            process_response(currentResponse);
        }
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
    (void) param;

    while (true) {
        // Tick executes one FSM cycle including queue intake and state logic.
        modules::StateMachine::instance().tick();
        vTaskDelay(pdMS_TO_TICKS(kStateMachineTaskPeriodMs));
    }
}
