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

#include <map>
#include <memory>
#include <string>

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
/// The StateMachine task constantly polls for events, commands, and response reports from the respective queues. Which represent the
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
    void initialize();

    ///
    /// @brief Start the dedicated FSM FreeRTOS task and event queue.
    ///
    void start_task();

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
    bool queue_command(const definitions::CommandRequest& request, uint32_t timeout_ms = 0);

    ///
    /// @brief Queue an executed response report in the FSM task's local queue.
    ///
    /// @note This is the intended method to send response reports from other tasks or ISRs. For non-RTOS paths.
    /// @param response Executed response payload.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if response was queued, false otherwise.
    ///
    bool queue_response(const definitions::CommandRequest& response, uint32_t timeout_ms = 0);

    ///
    /// @brief Execute one state-machine cycle (to be called repeatedly in a loop).
    /// This consumes a bounded number of queued event/command/response messages and then executes the
    /// handler for the current state.
    ///
    ///
    void tick();

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
    /// @brief Get the State Name object as a string
    ///
    /// @param state The state to get the name for
    /// @return const char*
    ///
    static const std::string& get_state_name(definitions::SystemState state);

    ///
    /// @brief Query whether there is a command pending execution confirmation.
    ///
    /// @return true if waiting for a matching module response report.
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
    /// @param command Reported executed command.
    /// @return true when report confirms the pending command execution.
    ///
    bool response_matches_command(const definitions::CommandRequest& request, const definitions::CommandRequest& response) const;

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
    void dispatch_command_request(const definitions::CommandRequest& request);

    ///
    /// @brief Process an executed response report.
    /// Intended for internal scheduler/task use and non-RTOS fallback paths.
    ///
    /// @param response Executed response report.
    ///
    void process_response(const definitions::CommandRequest& response);

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
    /// Used by all states to check if a pending command has not been confirmed as executed within a reasonable time window, which could
    /// indicate a fault or communication issue with the modules.
    ///
    void handle_command_feedback_timeout();

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
    bool stateChanged_;                           ///< Flag indicating if the state has changed since the last tick
    definitions::CommandRequest lastResponse_;    ///< Last executed response
    definitions::CommandRequest pendingCommand_;  ///< Last command request pending execution confirmation
    unsigned long pendingCommandTimeMs_;          ///< Timestamp of when the pending command was sent
    bool commandPending_;                         ///< Flag indicating if there is a command pending execution confirmation
    bool lastCommandConfirmed_;                   ///< Flag indicating if the last command was confirmed as executed by a module
};

}  // namespace modules
