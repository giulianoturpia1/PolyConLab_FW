#include "StateMachine.hpp"

#include "CheckupState.hpp"
#include "Config.hpp"
#include "HALGeneral.hpp"
#include "HardErrorState.hpp"
#include "Helpers.hpp"
#include "IState.hpp"
#include "Logger.hpp"
#include "ManualState.hpp"
#include "PauseState.hpp"
#include "ShutdownState.hpp"
#include "SoftErrorState.hpp"
#include "StartupState.hpp"

// FreeRTOS includes, neccesary
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "TargetDefinitions.hpp"

static QueueHandle_t gStateEventQueue = nullptr;
static QueueHandle_t gStateCommandQueue = nullptr;
static QueueHandle_t gStateResponseQueue = nullptr;
constexpr char kStateMachineTaskName[] = "state_machine_task";

namespace modules {

static const TaggedLogger kLog("StateMachine");

namespace {

/// Build the error reply for @p request, carrying its token so it reaches the right client.
definitions::Command make_error(const definitions::Command& request, definitions::ResponseType error) {
    return {definitions::CommandType::kCommandError, static_cast<uint32_t>(error), request.token};
}

}  // namespace

////////////////////////////////////// Public Methods //////////////////////////////////////

StateMachine& StateMachine::instance() {
    static StateMachine instance;
    return instance;
}

void StateMachine::begin() {
    currentState_ = definitions::SystemState::kStartup;
    previousState_ = definitions::SystemState::kStartup;
    stateEntryTime_ = hal_general().millis();
    stateChanged_ = true;
    lastCommandConfirmed_ = false;
    pending_.fill(PendingCommand());
    pendingCount_ = 0;
}

bool StateMachine::create_task() {
    static portMUX_TYPE startMux = portMUX_INITIALIZER_UNLOCKED;
    static bool task_started = false;

    taskENTER_CRITICAL(&startMux);
    if (task_started) {
        taskEXIT_CRITICAL(&startMux);
        kLog.warning("La tarea de la maquina de estados ya esta en ejecucion.");
        return true;
    }

    task_started = true;
    taskEXIT_CRITICAL(&startMux);

    gStateEventQueue = xQueueCreate(config::kStateMachineQueueLength, sizeof(definitions::SystemEvent));
    gStateCommandQueue = xQueueCreate(config::kStateMachineQueueLength, sizeof(definitions::Command));
    gStateResponseQueue = xQueueCreate(config::kStateMachineQueueLength, sizeof(definitions::Command));
    if (gStateEventQueue == nullptr || gStateCommandQueue == nullptr || gStateResponseQueue == nullptr) {
        kLog.error("No se pudo crear la cola de la maquina de estados.");
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
        taskENTER_CRITICAL(&startMux);
        task_started = false;
        taskEXIT_CRITICAL(&startMux);
        return false;
    }

    if (xTaskCreatePinnedToCore(task_entry, kStateMachineTaskName, config::kStateMachineTaskStackSize, nullptr,
                                config::kStateMachineTaskPriority, nullptr, config::kSpiCoreId) != pdPASS) {
        kLog.error("No se pudo crear la tarea de la maquina de estados.");
        vQueueDelete(gStateEventQueue);
        vQueueDelete(gStateCommandQueue);
        vQueueDelete(gStateResponseQueue);
        gStateEventQueue = nullptr;
        gStateCommandQueue = nullptr;
        gStateResponseQueue = nullptr;
        taskENTER_CRITICAL(&startMux);
        task_started = false;
        taskEXIT_CRITICAL(&startMux);
        return false;
    }

    return true;
}

bool StateMachine::queue_event(definitions::SystemEvent event, uint32_t timeout_ms) {
    if (gStateEventQueue == nullptr || xQueueSend(gStateEventQueue, &event, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

bool StateMachine::queue_command(const definitions::Command& request, uint32_t timeout_ms) {
    if (gStateCommandQueue == nullptr || xQueueSend(gStateCommandQueue, &request, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

bool StateMachine::queue_response(const definitions::Command& response, uint32_t timeout_ms) {
    if (gStateResponseQueue == nullptr) {
        return false;
    }

    if (xQueueSend(gStateResponseQueue, &response, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

void StateMachine::tick() {
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

definitions::SystemState StateMachine::get_current_state() const {
    return currentState_;
}

definitions::SystemState StateMachine::get_previous_state() const {
    return previousState_;
}

bool StateMachine::is_command_pending() const {
    return pendingCount_ > 0;
}

bool StateMachine::was_last_command_confirmed() const {
    return lastCommandConfirmed_;
}

bool StateMachine::is_in_state(definitions::SystemState state) const {
    return currentState_ == state;
}

void StateMachine::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_sink_ = queue;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

StateMachine::StateMachine()
    : currentState_(definitions::SystemState::kStartup),
      previousState_(definitions::SystemState::kStartup),
      stateEntryTime_(0),
      stateChanged_(false),
      lastResponse_({definitions::CommandType::kHeaterOff, 0}),
      pending_(),
      pendingCount_(0),
      lastCommandConfirmed_(false),
      response_sink_(nullptr) {
    stateHandlers_.emplace(definitions::SystemState::kStartup, std::unique_ptr<IState>(new StartupState()));
    stateHandlers_.emplace(definitions::SystemState::kCheckup, std::unique_ptr<IState>(new CheckupState()));
    stateHandlers_.emplace(definitions::SystemState::kManual, std::unique_ptr<IState>(new ManualState()));
    stateHandlers_.emplace(definitions::SystemState::kPause, std::unique_ptr<IState>(new PauseState()));
    stateHandlers_.emplace(definitions::SystemState::kSoftError, std::unique_ptr<IState>(new SoftErrorState()));
    stateHandlers_.emplace(definitions::SystemState::kHardError, std::unique_ptr<IState>(new HardErrorState()));
    stateHandlers_.emplace(definitions::SystemState::kShutdown, std::unique_ptr<IState>(new ShutdownState()));
}

IState* StateMachine::get_state_handler(definitions::SystemState state) {
    auto it = stateHandlers_.find(state);
    if (it == stateHandlers_.end()) {
        return nullptr;
    }

    return it->second.get();
}

const IState* StateMachine::get_state_handler(definitions::SystemState state) const {
    auto it = stateHandlers_.find(state);
    if (it == stateHandlers_.end()) {
        return nullptr;
    }

    return it->second.get();
}

bool StateMachine::response_matches_command(const definitions::Command& request, const definitions::Command& response) const {
    if (request.type != response.type) {
        return false;
    }

    switch (request.type) {
        case definitions::CommandType::kValveOpen:
        case definitions::CommandType::kValveClose:
            return request.extra == response.extra;
        case definitions::CommandType::kValveState:
            // The response payload carries the status flag in the top bit; compare pins only.
            return (request.extra & definitions::kValvePinMask) == (response.extra & definitions::kValvePinMask);
        default:
            return true;
    }
}

void StateMachine::process_event(definitions::SystemEvent event) {
    // Global transitions handled before delegating to the active state.
    if (event == definitions::SystemEvent::kCommunicationLost) {
        kLog.info("Conexion perdida, pausando sistema.");
        transition_to(definitions::SystemState::kPause);
        return;
    }
    if (event == definitions::SystemEvent::kFaultCritical || event == definitions::SystemEvent::kShutdownButton) {
        kLog.error("Fallo critico detectado, transicionando a HARD_ERROR.");
        transition_to(definitions::SystemState::kHardError);
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

void StateMachine::process_command(const definitions::Command& request) {
    // Every command produces exactly one outcome, published as soon as it is known. Only a command
    // handed to a device occupies a slot, and only until that device answers or its deadline passes;
    // the order the client sees is Communications' business, not this table's.

    // Capacity is checked first: a command dispatched without a slot to track it could never be
    // matched to its reply. Communications refuses at the same depth, so this is a guard.
    if (pendingCount_ >= pending_.size()) {
        kLog.warning("Comando rechazado: no hay lugar para otro comando en vuelo.");
        publish_response(make_error(request, definitions::ResponseType::kErrorBusy));
        return;
    }

    // SM-level queries need no device: answer them here.
    if (request.type == definitions::CommandType::kPing) {
        publish_response({definitions::CommandType::kPing, 0u, request.token});
        return;
    }
    if (request.type == definitions::CommandType::kStateQuery) {
        publish_response({definitions::CommandType::kStateQuery, static_cast<uint32_t>(currentState_), request.token});
        return;
    }

    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler == nullptr) {
        transition_to(definitions::SystemState::kHardError);
        publish_response(make_error(request, definitions::ResponseType::kErrorCommandRejected));
        return;
    }

    definitions::Command completed = definitions::Command();
    switch (stateHandler->handle_command(this, request, completed)) {
        case definitions::CommandOutcome::kPending:
            push_pending(request);
            return;
        case definitions::CommandOutcome::kCompleted:
            publish_response(completed);
            return;
        case definitions::CommandOutcome::kRejected:
        default:
            kLog.warning("Comando recibido, pero no se pudo ejecutar en el estado actual: " +
                         std::string(utils::state_name(currentState_)));
            publish_response(make_error(request, definitions::ResponseType::kErrorCommandRejected));
            return;
    }
}

void StateMachine::publish_response(const definitions::Command& response) {
    lastCommandConfirmed_ = response.type != definitions::CommandType::kCommandError;
    if (response_sink_ != nullptr) {
        // Non-blocking: the FSM must never stall waiting for the delivery layer to make room.
        response_sink_->publish(response, 0);
    }
}

bool StateMachine::push_pending(const definitions::Command& request) {
    for (PendingCommand& entry : pending_) {
        if (entry.occupied) {
            continue;
        }
        entry.request = request;
        entry.sentTimeMs = hal_general().millis();
        entry.occupied = true;
        ++pendingCount_;
        lastCommandConfirmed_ = false;
        return true;
    }
    return false;
}

void StateMachine::process_response(const definitions::Command& response) {
    lastResponse_ = response;

    // Match on the correlation id, so a reply from a command that already timed out cannot be
    // credited to a later command of the same type; response_matches_command stays as a second gate.
    for (PendingCommand& entry : pending_) {
        if (!entry.occupied || entry.request.token != response.token) {
            continue;
        }
        if (!response_matches_command(entry.request, response)) {
            continue;
        }
        entry.occupied = false;
        --pendingCount_;
        kLog.debug("Comando confirmado como ejecutado por modulo.");
        publish_response(response);
        break;
    }

    std::string logMessage = "Respuesta del modulo: ";
    logMessage = utils::command_name(response.type);
    kLog.debug(logMessage);
}

void StateMachine::attend_queues() {
    if (gStateEventQueue != nullptr) {
        definitions::SystemEvent event{};
        for (uint8_t i = 0; i < config::kMaxEventsPerTick; ++i) {
            if (xQueueReceive(gStateEventQueue, &event, 0) != pdPASS) {
                break;
            }
            process_event(event);
        }
    }

    if (gStateCommandQueue != nullptr) {
        definitions::Command command{};
        for (uint8_t i = 0; i < config::kMaxCommandsPerTick; ++i) {
            if (xQueueReceive(gStateCommandQueue, &command, 0) != pdPASS) {
                break;
            }
            process_command(command);
        }
    }

    if (gStateResponseQueue != nullptr) {
        definitions::Command response{};
        for (uint8_t i = 0; i < config::kMaxResponsesPerTick; ++i) {
            if (xQueueReceive(gStateResponseQueue, &response, 0) != pdPASS) {
                break;
            }
            process_response(response);
        }
    }
}

void StateMachine::transition_to(definitions::SystemState newState) {
    if (newState == currentState_) {
        return;
    }

    on_state_exit();
    previousState_ = currentState_;
    currentState_ = newState;
    stateEntryTime_ = hal_general().millis();
    stateChanged_ = true;
    on_state_enter();
}

void StateMachine::on_state_exit() {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler != nullptr) {
        stateHandler->on_exit(this);
    }
}

void StateMachine::on_state_enter() {
    IState* stateHandler = get_state_handler(currentState_);
    if (stateHandler != nullptr) {
        stateHandler->on_enter(this);
    }
}

void StateMachine::handle_command_feedback_timeout() {
    if (pendingCount_ == 0) {
        return;
    }

    const unsigned long now_ms = hal_general().millis();
    for (PendingCommand& entry : pending_) {
        if (!entry.occupied || (now_ms - entry.sentTimeMs) < config::kCommandFeedbackTimeoutMs) {
            continue;
        }

        std::string logMessage = "Timeout esperando confirmacion de ejecucion para comando pendiente: ";
        logMessage += utils::command_name(entry.request.type);
        // Special case for valve commands to include the valve number in the log.
        if (entry.request.type == definitions::CommandType::kValveOpen || entry.request.type == definitions::CommandType::kValveClose) {
            logMessage += "," + std::to_string(entry.request.extra);
        }
        kLog.warning(logMessage);

        const definitions::Command timed_out = make_error(entry.request, definitions::ResponseType::kErrorCommandTimeout);
        entry.occupied = false;
        --pendingCount_;
        publish_response(timed_out);
    }
}

void StateMachine::task_entry(void* param) {
    (void) param;

    while (true) {
        // Tick executes one FSM cycle including queue intake and state logic.
        StateMachine::instance().tick();
        vTaskDelay(pdMS_TO_TICKS(config::kStateMachineTaskPeriodMs));
    }
}

}  // namespace modules
