#include "StateMachine.hpp"

#include "HALGeneral.hpp"

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::StateMachine& modules::StateMachine::instance() {
    static StateMachine instance;
    return instance;
}

void modules::StateMachine::begin() {
    currentState_ = definitions::SystemState::kStartup;
    previousState_ = definitions::SystemState::kStartup;
    stateEntryTime_ = modules::hal_general().millis();
    stateChanged_ = true;
    on_state_enter();
}

void modules::StateMachine::run() {
    stateChanged_ = false;

    // Execute the current state handler
    switch (currentState_) {
        case definitions::SystemState::kStartup:
            handle_startup();
            break;
        case definitions::SystemState::kCheckup:
            handle_checkup();
            break;
        case definitions::SystemState::kCalibration:
            handle_calibration();
            break;
        case definitions::SystemState::kManual:
            handle_manual();
            break;
        case definitions::SystemState::kSoftError:
            handle_soft_error();
            break;
        case definitions::SystemState::kCriticalError:
            handle_critical_error();
            break;

        default:
            // Unknown state, transition to CRITICAL_ERROR
            transition_to(definitions::SystemState::kCriticalError);
            break;
    }
}

void modules::StateMachine::process_event(definitions::SystemEvent event) {
    definitions::SystemState nextState = currentState_;
    switch (currentState_) {
        case definitions::SystemState::kStartup:
            if (event == definitions::SystemEvent::kInitOk) {
                nextState = definitions::SystemState::kCheckup;
            } else if (event == definitions::SystemEvent::kInitFatal ||
                       event == definitions::SystemEvent::kWatchdogFatal) {
                nextState = definitions::SystemState::kCriticalError;
            }
            break;

        case definitions::SystemState::kCheckup:
            if (event == definitions::SystemEvent::kScadaLinkOk && event == definitions::SystemEvent::kSensorsOk) {
                nextState = definitions::SystemState::kManual;
            } else if (event == definitions::SystemEvent::kStartCalibration) {
                nextState = definitions::SystemState::kCalibration;
            } else if (event == definitions::SystemEvent::kFaultRecoverable) {
                nextState = definitions::SystemState::kSoftError;
            } else if (event == definitions::SystemEvent::kFaultCritical) {
                nextState = definitions::SystemState::kCriticalError;
            } else if (event == definitions::SystemEvent::kCmdResetFault) {
                nextState = definitions::SystemState::kStartup;
            }
            break;

        case definitions::SystemState::kCalibration:
            if (event == definitions::SystemEvent::kFinishCalibration) {
                nextState = definitions::SystemState::kCheckup;
            } else if (event == definitions::SystemEvent::kFaultRecoverable) {
                nextState = definitions::SystemState::kSoftError;
            } else if (event == definitions::SystemEvent::kFaultCritical) {
                nextState = definitions::SystemState::kCriticalError;
            }
            break;

        case definitions::SystemState::kManual:
            if (event == definitions::SystemEvent::kCmdStartAuto || event == definitions::SystemEvent::kCmdStartDemo) {
                // Stay in MANUAL for now (Auto/Demo states not implemented yet)
                nextState = definitions::SystemState::kManual;
            } else if (event == definitions::SystemEvent::kStartCalibration) {
                nextState = definitions::SystemState::kCalibration;
            } else if (event == definitions::SystemEvent::kFaultRecoverable) {
                nextState = definitions::SystemState::kSoftError;
            } else if (event == definitions::SystemEvent::kFaultCritical || event == definitions::SystemEvent::kStop) {
                nextState = definitions::SystemState::kCriticalError;
            } else if (event == definitions::SystemEvent::kCmdAbort) {
                nextState = definitions::SystemState::kManual;
            }
            break;

        case definitions::SystemState::kSoftError:
            if (event == definitions::SystemEvent::kCmdRetryCheck &&
                event == definitions::SystemEvent::kConditionCleared) {
                nextState = previousState_;  // Return to previous state
            } else if (event == definitions::SystemEvent::kFaultCritical ||
                       event == definitions::SystemEvent::kTooManyRetries) {
                nextState = definitions::SystemState::kCriticalError;
            }
            break;

        case definitions::SystemState::kCriticalError:
            if (event == definitions::SystemEvent::kCmdResetFault ||
                event == definitions::SystemEvent::kShutdownButton || event == definitions::SystemEvent::kCmdShutdown) {
                nextState = definitions::SystemState::kStartup;
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
    : currentState_(definitions::SystemState::kStartup),
      previousState_(definitions::SystemState::kStartup),
      stateEntryTime_(0),
      stateChanged_(false) {}

void modules::StateMachine::transition_to(definitions::SystemState newState) {
    on_state_exit();
    previousState_ = currentState_;
    currentState_ = newState;
    stateEntryTime_ = modules::hal_general().millis();
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
    {definitions::SystemState::kStartup, "STARTUP"},
    {definitions::SystemState::kCheckup, "CHECKUP"},
    {definitions::SystemState::kCalibration, "CALIBRATION"},
    {definitions::SystemState::kManual, "MANUAL"},
    {definitions::SystemState::kSoftError, "SOFT_ERROR"},
    {definitions::SystemState::kCriticalError, "CRITICAL_ERROR"}};
