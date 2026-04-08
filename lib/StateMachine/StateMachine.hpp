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
/// The current implementation utilizes a FreeRTOS task to run the state machine loop, which processes events and commands from queues.
/// The main loop is implemented in the tick() method, which executes the handler for the current state.
/// The StateMachine task constantly polls for events, commands, and action reports from the respective queues. Which represent the
/// following:
///
/// + Events are used to trigger state transitions.
///
/// + Commands are used to request actions to be executed by the modules, which may or may not trigger state transitions depending on the
/// current state and command logic.
///
/// + Action reports are used by modules to report back the execution of commands, which can be used for feedback confirmation and to
/// trigger further actions or transitions if needed.
///
/// The StateMachine acts as a "Mediator" as in the Mediator design pattern, coordinating interactions between different modules.
///
/// @note The task calling tick() should have a high priority to ensure timely state handling, and the tick() method should be called at a
/// regular interval (e.g. every 20ms) to maintain system responsiveness.
///
/// process_event() is the one to execute the state transitions,
/// while tick() only executes the current state's handler.
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
    /// @brief Start the dedicated FSM FreeRTOS task and event queue.
    ///
    void start_task();

    ///
    /// @brief Enqueue an event to be handled by the FSM task.
    ///
    /// @note This is the intended method to trigger state transitions from other tasks or ISRs.
    ///
    /// @param event Event to enqueue.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if event was queued, false otherwise.
    ///
    bool enqueue_event(definitions::SystemEvent event, uint32_t timeout_ms = 0);

    ///
    /// @brief Enqueue a command request to be decided and executed by the FSM task.
    ///
    /// @note This is the intended method to send commands from other tasks or ISRs.
    ///
    /// @param request Command request.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if request was queued, false otherwise.
    ///
    bool enqueue_command(const definitions::CommandRequest& request, uint32_t timeout_ms = 0);

    ///
    /// @brief Enqueue an executed action report to be handled by the FSM task.
    ///
    /// @note This is the intended method to send action reports from other tasks or ISRs. For non-RTOS paths.
    /// @param action Executed action payload.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if action was queued, false otherwise.
    ///
    bool enqueue_action(const definitions::CommandRequest& action, uint32_t timeout_ms = 0);

    // Getters

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

    // FreeRTOS task

    ///
    /// @brief FreeRTOS task entrypoint for the StateMachine runtime loop.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

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
