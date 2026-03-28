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

#include <functional>
#include <map>
#include <string>

#include "Definitions.hpp"

namespace modules {

///
/// @brief Finite State Machine for system state management. It handles state transitions and is
/// responsible for executing state-specific logic. Implemented as a singleton class
///
/// The StateMachine class should be run in a task, and should be executed in the
/// following way:
///
/// 		+ initialize(), then tick() in a loop
///
/// 		+ process_event(event) can be called from other tasks to trigger state transitions
///
/// @note The task calling tick() should have a higher priority than tasks
///       which call process_event(), to ensure state transitions.
/// @note Furthermore, process_event() is the one to execute the state transitions,
///       while tick() only executes the current state's handler.
///
class StateMachine {
   public:
    ///
    /// @brief Type definition for state handler functions
    ///
    ///
    using StateHandler = std::function<void()>;

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
    /// @brief Execute one state-machine cycle (to be called repeatedly in a loop).
    /// This executes the handler for the current state and does not handle event logic transition, that
    /// should be done via process_event().
    ///
    ///
    void tick();

    ///
    /// @brief Process an event of SystemEvent type, in a running state machine.
    /// This checks the current state and certain events to determine if a state transition is needed or
    /// not.
    ///
    /// @param event The event to process
    ///
    void process_event(definitions::SystemEvent event);

    ///
    /// @brief Process a command request immediately.
    /// Intended for internal scheduler/task use and non-RTOS fallback paths.
    ///
    /// @param request Command request.
    ///
    void process_command(const definitions::CommandRequest& request);

    ///
    /// @brief Process an executed action report.
    /// Intended for internal scheduler/task use and non-RTOS fallback paths.
    ///
    /// @param action Executed action report.
    ///
    void process_action(const definitions::CommandRequest& action);

    ///
    /// @brief Enqueue an event to be handled by the FSM task.
    ///
    ///
    /// @param event Event to enqueue.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if event was queued, false otherwise.
    ///
    bool enqueue_event(definitions::SystemEvent event, uint32_t timeout_ms = 0);

    ///
    /// @brief Enqueue a command request to be decided and executed by the FSM task.
    ///
    ///
    /// @param request Command request.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if request was queued, false otherwise.
    ///
    bool enqueue_command(const definitions::CommandRequest& request, uint32_t timeout_ms = 0);

    ///
    /// @brief Enqueue an executed action report to be handled by the FSM task.
    ///
    /// @param action Executed action payload.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if action was queued, false otherwise.
    ///
    bool enqueue_action(const definitions::CommandRequest& action, uint32_t timeout_ms = 0);

    ///
    /// @brief Route a command name through the state-machine command path.
    /// Command names not handled by the FSM command path return false.
    ///
    /// @param command_name Command string (e.g. "HEATER_ON").
    /// @param pin Optional pin payload for parameterized commands.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command name is known and was routed, false otherwise.
    ///
    bool route_command_name(const std::string& command_name, uint32_t pin = 0, uint32_t timeout_ms = 0);

    ///
    /// @brief Route an event name through the state-machine event path.
    /// Event names not handled by the FSM event path return false.
    ///
    /// @param event_name Event name string (e.g. "INIT_OK").
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if event name is known and was routed, false otherwise.
    ///
    bool route_event_name(const std::string& event_name, uint32_t timeout_ms = 0);

    ///
    /// @brief Start the dedicated FSM FreeRTOS task and event queue.
    ///
    void start_task();

    ///
    /// @brief Query whether there is a command pending execution confirmation.
    ///
    /// @return true if waiting for a matching module action report.
    ///
    bool is_command_pending() const;

    ///
    /// @brief Query whether the most recently dispatched command was confirmed as executed.
    ///
    /// @return true when a matching module action report has been received.
    ///
    bool was_last_command_confirmed() const;

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
    /// @brief Check if the state machine is in a specific state
    ///
    /// @param state The state to check
    /// @return true
    /// @return false
    ///
    bool is_in_state(definitions::SystemState state) const;

    ///
    /// @brief Get the State Name object as a string
    ///
    /// @param state The state to get the name for
    /// @return const char*
    ///
    static const std::string& get_state_name(definitions::SystemState state);

   private:
    StateMachine();
    // Rule of Five
    ~StateMachine() = default;
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

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

    ///
    /// @brief Execute an already accepted command request.
    /// This is the internal command executor used by process_command(). It enqueues the command to the corresponding
    /// module or executes it directly, depending on the command type and current state.
    ///
    /// @param request Command request.
    ///
    void execute_command(const definitions::CommandRequest& request);

    ///
    /// @brief Dispatch a command request to its corresponding module queue/task.
    ///
    /// @param request Command request.
    /// @return true when dispatch path was accepted.
    ///
    bool dispatch_to_module(const definitions::CommandRequest& request);

    ///
    /// @brief Compare command intent against an executed command report.
    ///
    /// @param request Pending command request.
    /// @param command Reported executed command.
    /// @return true when report confirms the pending command execution.
    ///
    bool action_matches_command(const definitions::CommandRequest& request, const definitions::CommandRequest& command) const;

    ///
    /// @brief Monitor pending command confirmation timeout.
    ///
    void handle_command_feedback_timeout();

    // State handlers for SystemState enum class
    void handle_startup();
    void handle_checkup();
    void handle_calibration();
    void handle_manual();
    void handle_soft_error();
    void handle_critical_error();

    static const std::map<definitions::SystemState, std::string> stateNames_;  ///< Mapping of SystemState values to human-readable names
    definitions::SystemState currentState_;                                    ///< Current state of the system
    definitions::SystemState previousState_;                                   ///< Previous state of the system
    unsigned long stateEntryTime_;                                             ///< Timestamp of when the current state was entered
    bool stateChanged_;                           ///< Flag indicating if the state has changed since the last tick
    definitions::CommandRequest lastAction_;      ///< Last executed action
    definitions::CommandRequest pendingCommand_;  ///< Last command request pending execution confirmation
    unsigned long pendingCommandTimeMs_;          ///< Timestamp of when the pending command was sent
    bool commandPending_;                         ///< Flag indicating if there is a command pending execution confirmation
    bool lastCommandConfirmed_;                   ///< Flag indicating if the last command was confirmed as executed by a module
};

}  // namespace modules
