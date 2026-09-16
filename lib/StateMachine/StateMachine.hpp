///
/// @file StateMachine.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Finite State Machine implementation for PolyConLab system
/// @version 0.1
/// @date 16-01-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "Config.hpp"
#include "Definitions.hpp"

namespace interfaces {
class IState;
}

namespace modules {
using interfaces::IState;

///
/// @brief Finite State Machine for system state management. It handles state transitions and is
/// responsible for executing state-specific logic. Implemented as a singleton class
///
/// The current implementation utilizes a FreeRTOS task to run the state machine loop, which processes events and commands from queues.
/// The main loop is implemented in the tick() method, which executes the handler for the current state.
/// The StateMachine task constantly polls for events, commands, and responses from the respective queues. Which represent the
/// following:
///
/// + Events are used to trigger state transitions.
///
/// + Commands are used to request actions to be executed by the modules, which may or may not trigger state transitions depending on the
/// current state and command logic.
///
/// + Response reports are used by modules to report back the execution of commands, which can be used for feedback confirmation and to
/// trigger further actions or transitions if needed.
///
/// The StateMachine acts as a "Mediator" as in the Mediator design pattern, coordinating interactions between different modules.
///
/// @note The task calling tick() should have a high priority to ensure timely state handling, and the tick() method should be called at a
/// regular interval (e.g. every 20ms) to maintain system responsiveness.
///
/// Events are enqueued via queue_event() and consumed by tick() each FSM cycle.
/// tick() performs bounded queue intake and executes the current state's handler.
///
class StateMachine {
   public:
    ///
    /// @brief Get the singleton instance of the StateMachine
    ///
    /// @return StateMachine&
    ///
    static StateMachine& instance();

    ///
    /// @brief Initialize the state machine. Starts in the STARTUP state.
    ///
    ///
    void begin();

    ///
    /// @brief Start the dedicated FSM FreeRTOS task and event queue.
    ///
    bool create_task();

    ///
    /// @brief Queue an event in the FSM task's local queue.
    ///
    /// @note This is the intended method to trigger state transitions from other tasks or ISRs.
    ///
    /// @param event Event to enqueue.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if event was queued, false otherwise.
    ///
    bool queue_event(definitions::SystemEvent event, uint32_t timeout_ms = 0);

    ///
    /// @brief Queue a command request in the FSM task's local queue.
    ///
    /// @note The active state decides whether and how to execute the queued command.
    ///
    /// @param request Command request.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if request was queued, false otherwise.
    ///
    bool queue_command(const definitions::Command& request, uint32_t timeout_ms = 0);

    ///
    /// @brief Queue an executed response in the FSM task's local queue.
    ///
    /// @note This is the intended method to send responses from other tasks or ISRs. For non-RTOS paths.
    /// @param response Executed response payload.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if response was queued, false otherwise.
    ///
    bool queue_response(const definitions::Command& response, uint32_t timeout_ms = 0);

    // Getters

    ///
    /// @brief Get the Current State object
    ///
    /// @return definitions::SystemState
    ///
    definitions::SystemState get_current_state() const;

    ///
    /// @brief Get the Previous State object
    ///
    /// @return definitions::SystemState
    ///
    definitions::SystemState get_previous_state() const;

    ///
    /// @brief Query whether there is a command pending execution confirmation.
    ///
    /// @return true if waiting for a matching module response.
    ///
    bool is_command_pending() const;

    ///
    /// @brief Query whether the most recently dispatched command was confirmed as executed.
    ///
    /// @return true when a matching module action report has been received.
    ///
    bool was_last_command_confirmed() const;

    ///
    /// @brief Check if the state machine is in a specific state
    ///
    /// @param state The state to check
    /// @return true
    /// @return false
    ///
    bool is_in_state(definitions::SystemState state) const;

    ///
    /// @brief Register a callback invoked for every command outcome: success or failure.
    ///
    /// On success the Command carries the confirmed payload.
    /// On failure cmd.type == CommandType::kCommandError and cmd.extra holds the ResponseType.
    ///
    /// @param callback Callable invoked with the outcome Command.
    ///
    void set_response_callback(std::function<void(const definitions::Command&)> callback);

   private:
    StateMachine();
    // Rule of Five
    ~StateMachine() = default;
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    ///
    /// @brief Get the state handler object for a given SystemState.
    ///
    /// This is used internally to delegate event handling and state logic execution to the appropriate state handler.
    ///
    /// @param state The SystemState to get the handler for.
    /// @return IState*
    ///
    IState* get_state_handler(definitions::SystemState state);
    ///
    /// @brief Get the state handler object for a given SystemState.
    ///
    /// This is used internally to delegate event handling and state logic execution to the appropriate state handler.
    ///
    /// @param state The SystemState to get the handler for.
    /// @return const IState*
    ///
    const IState* get_state_handler(definitions::SystemState state) const;

    ///
    /// @brief Compare command intent against an executed command report.
    ///
    /// @param request Pending command request.
    /// @param response Reported executed command.
    /// @return true when report confirms the pending command execution.
    ///
    bool response_matches_command(const definitions::Command& request, const definitions::Command& response) const;

    ///
    /// @brief Process an event of SystemEvent type.
    ///
    /// Intended for internal scheduler/task use and non-RTOS fallback paths.
    ///
    /// @param event The event to process.
    ///
    void process_event(definitions::SystemEvent event);

    ///
    /// @brief Dispatch a command request immediately.
    /// Intended for internal scheduler/task use and non-RTOS fallback paths.
    /// The active state's handler decides command acceptance/dispatch.
    ///
    /// @param request Command request.
    ///
    void process_command(const definitions::Command& request);

    ///
    /// @brief Process an executed response.
    /// Intended for internal scheduler/task use and non-RTOS fallback paths.
    ///
    /// @param response Executed response.
    ///
    void process_response(const definitions::Command& response);

    ///
    /// @brief Consume a bounded number of different queue messages in one FSM cycle.
    ///
    void attend_queues();

    // Auxiliary operations

    ///
    /// @brief Transition to a new state. This handles the exit and entry actions for states
    /// and any necessary additional processing during the transition
    ///
    /// @param newState The state to transition to
    ///
    void transition_to(definitions::SystemState newState);

    ///
    /// @brief Exit handler for the current state exit. Primarily used for cleanup before leaving a state.
    /// In case specific exit actions are needed per state, this method can be extended
    ///
    ///
    void on_state_exit();

    ///
    /// @brief Entry handler for entering a new state. Primarily used for initialization after entering a state.
    /// In case specific entry actions are needed per state, this method can be extended
    ///
    ///
    void on_state_enter();

    // State handlers for SystemState enum class

    ///
    /// @brief Monitor pending command confirmation timeout.
    ///
    /// Only the oldest in-flight command is aged: entries behind it were dispatched later, and because
    /// entries retire in order, timing the head out is also what unblocks anything confirmed behind it.
    ///
    void handle_command_feedback_timeout();

    ///
    /// @brief Close a command that needs no device round-trip, in order.
    ///
    /// Takes a slot and retires it immediately, so an answer that is ready at once (PING, STATE?, a
    /// rejection, a locally handled setpoint) still leaves behind any command dispatched before it.
    /// Falls back to answering out of order only if the table is full, which delivery by token makes
    /// harmless.
    ///
    /// @param request Command being answered.
    /// @param response Response to deliver.
    ///
    void complete_now(const definitions::Command& request, const definitions::Command& response);

    ///
    /// @brief Claim the next free in-flight slot for @p request.
    ///
    /// Used both for a command dispatched to a device and, through complete_now(), for one already
    /// answered, so that every command occupies the queue until its turn to reply comes.
    ///
    /// @param request Command as received, carrying its token.
    /// @return false when every slot is occupied.
    ///
    bool push_pending(const definitions::Command& request);

    ///
    /// @brief Retire confirmed entries from the head, firing response_callback_ for each.
    ///
    /// Stops at the first unconfirmed entry, so replies leave the firmware in the order the client
    /// issued the commands even when a device confirms out of order.
    ///
    void retire_pending();

    ///
    /// @brief Execute one state-machine cycle (to be called repeatedly in a loop).
    /// This consumes a bounded number of queued event/command/response messages and then executes the
    /// handler for the current state.
    ///
    ///
    void tick();

    // FreeRTOS task

    ///
    /// @brief FreeRTOS task entrypoint for the StateMachine runtime loop.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    std::map<definitions::SystemState, std::unique_ptr<IState>> stateHandlers_;  ///< Map of state handlers for each SystemState
    definitions::SystemState currentState_;                                      ///< Current state of the system
    definitions::SystemState previousState_;                                     ///< Previous state of the system
    unsigned long stateEntryTime_;                                               ///< Timestamp of when the current state was entered
    bool stateChanged_;                  ///< Flag indicating if the state has changed since the last tick
    definitions::Command lastResponse_;  ///< Last executed response

    ///
    /// @brief One command awaiting confirmation from the device that executes it.
    ///
    /// Every command that reaches process_command() takes a slot, including the ones answered without
    /// touching a device, and all of them leave through retire_pending(). A response that arrives
    /// before an older one is held in @c response until every entry ahead of it has retired, so the
    /// client sees exactly one reply per command, in the order it sent them — which is the ordering
    /// a SCPI client correlates by.
    ///
    struct PendingCommand {
        definitions::Command request;   ///< The command as dispatched, carrying its token
        definitions::Command response;  ///< The device's reply; meaningful once confirmed is true
        unsigned long sentTimeMs;       ///< Dispatch timestamp, aged by handle_command_feedback_timeout()
        bool occupied;                  ///< Slot holds a command that has not retired yet
        bool confirmed;                 ///< Reply received (or timed out); waiting to reach the head
    };

    std::array<PendingCommand, config::kMaxInFlightCommands> pending_;  ///< In-flight ring, oldest at pendingHead_
    uint8_t pendingHead_;                                               ///< Index of the oldest occupied slot
    uint8_t pendingCount_;                                              ///< Number of occupied slots
    bool lastCommandConfirmed_;  ///< True when the most recently retired command was confirmed, not timed out
    std::function<void(const definitions::Command&)>
        response_callback_;  ///< Fired on every command outcome; kCommandError type encodes failures
};

}  // namespace modules
