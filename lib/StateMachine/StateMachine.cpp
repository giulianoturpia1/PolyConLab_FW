#include "StateMachine.hpp"

#include <Arduino.h>

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::StateMachine& modules::StateMachine::instance() {
    static StateMachine instance;
    return instance;
}

void modules::StateMachine::begin() {
    currentState_ = definitions::SystemState::STARTUP;
    previousState_ = definitions::SystemState::STARTUP;
    stateEntryTime_ = millis();
    stateChanged_ = true;
    on_state_enter();
}

void modules::StateMachine::run() {
    stateChanged_ = false;

    // Execute the current state handler
    switch (currentState_) {
        case definitions::SystemState::STARTUP:
            handle_startup();
            break;
        case definitions::SystemState::CHECKUP:
            handle_checkup();
            break;
        case definitions::SystemState::CALIBRATION:
            handle_calibration();
            break;
        case definitions::SystemState::MANUAL:
            handle_manual();
            break;
        case definitions::SystemState::SOFT_ERROR:
            handle_soft_error();
            break;
        case definitions::SystemState::CRITICAL_ERROR:
            handle_critical_error();
            break;

        default:
            // Unknown state, transition to CRITICAL_ERROR
            transition_to(definitions::SystemState::CRITICAL_ERROR);
            break;
    }
}

void modules::StateMachine::process_event(definitions::SystemEvent event) {
    definitions::SystemState nextState = currentState_;
    switch (currentState_) {
        case definitions::SystemState::STARTUP:
            if (event == definitions::SystemEvent::INIT_OK) {
                nextState = definitions::SystemState::CHECKUP;
            } else if (event == definitions::SystemEvent::INIT_FATAL ||
                       event == definitions::SystemEvent::WATCHDOG_FATAL) {
                nextState = definitions::SystemState::CRITICAL_ERROR;
            }
            break;

        case definitions::SystemState::CHECKUP:
            if (event == definitions::SystemEvent::SCADA_LINK_OK && event == definitions::SystemEvent::SENSORS_OK) {
                nextState = definitions::SystemState::MANUAL;
            } else if (event == definitions::SystemEvent::START_CALIBRATION) {
                nextState = definitions::SystemState::CALIBRATION;
            } else if (event == definitions::SystemEvent::FAULT_RECOVERABLE) {
                nextState = definitions::SystemState::SOFT_ERROR;
            } else if (event == definitions::SystemEvent::FAULT_CRITICAL) {
                nextState = definitions::SystemState::CRITICAL_ERROR;
            } else if (event == definitions::SystemEvent::CMD_RESET_FAULT) {
                nextState = definitions::SystemState::STARTUP;
            }
            break;

        case definitions::SystemState::CALIBRATION:
            if (event == definitions::SystemEvent::FINISH_CALIBRATION) {
                nextState = definitions::SystemState::CHECKUP;
            } else if (event == definitions::SystemEvent::FAULT_RECOVERABLE) {
                nextState = definitions::SystemState::SOFT_ERROR;
            } else if (event == definitions::SystemEvent::FAULT_CRITICAL) {
                nextState = definitions::SystemState::CRITICAL_ERROR;
            }
            break;

        case definitions::SystemState::MANUAL:
            if (event == definitions::SystemEvent::CMD_START_AUTO ||
                event == definitions::SystemEvent::CMD_START_DEMO) {
                // Stay in MANUAL for now (Auto/Demo states not implemented yet)
                nextState = definitions::SystemState::MANUAL;
            } else if (event == definitions::SystemEvent::START_CALIBRATION) {
                nextState = definitions::SystemState::CALIBRATION;
            } else if (event == definitions::SystemEvent::FAULT_RECOVERABLE) {
                nextState = definitions::SystemState::SOFT_ERROR;
            } else if (event == definitions::SystemEvent::FAULT_CRITICAL || event == definitions::SystemEvent::STOP) {
                nextState = definitions::SystemState::CRITICAL_ERROR;
            } else if (event == definitions::SystemEvent::CMD_ABORT) {
                nextState = definitions::SystemState::MANUAL;
            }
            break;

        case definitions::SystemState::SOFT_ERROR:
            if (event == definitions::SystemEvent::CMD_RETRY_CHECK &&
                event == definitions::SystemEvent::CONDITION_CLEARED) {
                nextState = previousState_;  // Return to previous state
            } else if (event == definitions::SystemEvent::FAULT_CRITICAL ||
                       event == definitions::SystemEvent::TOO_MANY_RETRIES) {
                nextState = definitions::SystemState::CRITICAL_ERROR;
            }
            break;

        case definitions::SystemState::CRITICAL_ERROR:
            if (event == definitions::SystemEvent::CMD_RESET_FAULT ||
                event == definitions::SystemEvent::SHUTDOWN_BUTTON || event == definitions::SystemEvent::CMD_SHUTDOWN) {
                nextState = definitions::SystemState::STARTUP;
            }
            break;
    }

    if (nextState != currentState_) {
        transition_to(nextState);
    }
}

definitions::SystemState modules::StateMachine::get_current_state() const {
    return currentState_;
}

definitions::SystemState modules::StateMachine::get_previous_state() const {
    return previousState_;
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
    : currentState_(definitions::SystemState::STARTUP),
      previousState_(definitions::SystemState::STARTUP),
      stateEntryTime_(0),
      stateChanged_(false) {}

void modules::StateMachine::transition_to(definitions::SystemState newState) {
    on_state_exit();
    previousState_ = currentState_;
    currentState_ = newState;
    stateEntryTime_ = millis();
    stateChanged_ = true;
    on_state_enter();
}

void modules::StateMachine::on_state_exit() {
    // Cleanup for the current state before leaving
    // Can be extended with specific exit actions per state
}

void modules::StateMachine::on_state_enter() {
    // Initialization for the new state after entering
    // Can be extended with specific entry actions per state
}

void modules::StateMachine::handle_startup() {
    // TODO: Initialize HW/RTOS
    // TODO: Initialize TCP/BLE
    // TODO: Initialize lights
    // TODO: Call processEvent(definitions::SystemEvent::INIT_OK) when initialization is complete
}

void modules::StateMachine::handle_checkup() {
    // TODO: Health-check sensors
    // TODO: Handshake SCADA/BLE
    // TODO: Perform analog checks
    // TODO: Call processEvent(definitions::SystemEvent::SCADA_LINK_OK) and
    // processEvent(definitions::SystemEvent::SENSORS_OK) when checks pass
}

void modules::StateMachine::handle_calibration() {
    // TODO: Execute ordered stop
    // TODO: Execute calibration protocol
    // TODO: Return to automatic checkups
    // TODO: Call processEvent(definitions::SystemEvent::FINISH_CALIBRATION) when complete
}

void modules::StateMachine::handle_manual() {
    // TODO: Process commands
    // TODO: Keep interfaces active
    // TODO: Wait for user input
}

void modules::StateMachine::handle_soft_error() {
    // TODO: Implement partial stop
    // TODO: Display HMI warning + LED/Buzzer
    // TODO: Allow retry/acknowledgment
}

void modules::StateMachine::handle_critical_error() {
    // TODO: Implement full stop
    // TODO: Lock control
    // TODO: Require reset
}

////////////////////////////////////// State Names Mapping //////////////////////////////////////
const std::map<definitions::SystemState, std::string> modules::StateMachine::stateNames_ = {
    {definitions::SystemState::STARTUP, "STARTUP"},
    {definitions::SystemState::CHECKUP, "CHECKUP"},
    {definitions::SystemState::CALIBRATION, "CALIBRATION"},
    {definitions::SystemState::MANUAL, "MANUAL"},
    {definitions::SystemState::SOFT_ERROR, "SOFT_ERROR"},
    {definitions::SystemState::CRITICAL_ERROR, "CRITICAL_ERROR"}};
