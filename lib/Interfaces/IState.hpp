///
/// @file IState.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Interface for state handlers in the StateMachine. Each state in the StateMachine should have a corresponding class that
/// implements this interface.
/// @version 0.1
/// @date 15-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "Definitions.hpp"

namespace modules {

// Forward declaration.
class StateMachine;
}  // namespace modules

namespace interfaces {

class IState {
   public:
    virtual ~IState() = default;

    ///
    /// @brief Handle a state machine event and return the next state.
    ///
    /// @param context FSM runtime context.
    /// @param event Incoming event.
    /// @return definitions::SystemState Next state to apply.
    ///
    virtual definitions::SystemState handle_event(modules::StateMachine* context, definitions::SystemEvent event) = 0;

    ///
    /// @brief Validate and dispatch a command request while this state is active.
    ///
    /// @param context FSM runtime context.
    /// @param request Command request to evaluate/dispatch.
    /// @return true if command was accepted and dispatched; false otherwise.
    ///
    virtual bool dispatch_command(modules::StateMachine* context, const definitions::Command& request);

    ///
    /// @brief Optional callback executed when entering the state. Can be used for actions like applying safe outputs, logging, etc.
    ///
    /// @param context FSM runtime context.
    ///
    virtual void on_enter(modules::StateMachine* context);

    ///
    /// @brief Optional callback executed when exiting the state. Can be used for cleanup actions if needed.
    ///
    /// @param context FSM runtime context.
    ///
    virtual void on_exit(modules::StateMachine* context);

    ///
    /// @brief Human-readable constant state label.
    ///
    virtual const char* name() const = 0;

    ///
    /// @brief Periodic state handler executed from FSM tick().
    ///
    /// @param context FSM runtime context.
    /// @return definitions::SystemState Next state to apply.
    ///
    virtual definitions::SystemState tick(modules::StateMachine* context);

   protected:
    IState() = default;  // Prevent direct instantiation of the interface
    // Rule of Five
    IState(const IState&) = delete;
    IState& operator=(const IState&) = delete;
    IState(IState&&) = delete;
    IState& operator=(IState&&) = delete;

    ///
    /// @brief Turn off all actuators (Heater, Pump, Chiller). Call from on_enter() when entering any safe or error state.
    ///
    static void apply_safe_outputs();

    ///
    /// @brief Close both solenoid valves. No-op on targets that do not define valve pins.
    ///
    static void close_all_valves();
};

}  // namespace interfaces

namespace modules {

using interfaces::IState;

}  // namespace modules
