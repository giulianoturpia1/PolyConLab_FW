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
/// 		+ begin(), then run() in a loop
///
/// 		+ process_event(event) can be called from other tasks to trigger state transitions
///
/// @note The task calling run() should have a higher priority than tasks
///       which call process_event(), to ensure state transitions.
/// @note Furthermore, process_event() is the one to execute the state transitions,
///       while run() only executes the current state's handler.
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
    void begin();

    ///
    /// @brief Run the state machine (to be called in the main loop).
    /// This executes the handler for the current state and does not handle event logic transition, that
    /// should be done via process_event().
    ///
    ///
    void run();

    ///
    /// @brief Process an event of SystemEvent type, in a running state machine.
    /// This checks the current state and certain events to determine if a state transition is needed or
    /// not.
    ///
    /// @param event The event to process
    ///
    void process_event(definitions::SystemEvent event);

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

    // State handlers for SystemState enum class
    void handle_startup();
    void handle_checkup();
    void handle_calibration();
    void handle_manual();
    void handle_soft_error();
    void handle_critical_error();

    definitions::SystemState currentState_;
    definitions::SystemState previousState_;
    unsigned long stateEntryTime_;  ///< Timestamp of when the current state was entered
    bool stateChanged_;             ///< Flag indicating if the state has changed since the last run
    static const std::map<definitions::SystemState, std::string> stateNames_;
};

}  // namespace modules
