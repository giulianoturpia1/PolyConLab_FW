#include "StateMachine.hpp"

#include "Chiller.hpp"
#include "HALGeneral.hpp"
#include "Heater.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"

// FreeRTOS includes, neccesary
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <algorithm>

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

QueueHandle_t state_event_queue = nullptr;
QueueHandle_t state_command_queue = nullptr;
constexpr uint32_t kStateMachineTaskPeriodMs = 20;
constexpr UBaseType_t kStateMachineTaskPriority = 3;

////////////////////////////////////// Task Function //////////////////////////////////////

void state_machine_task(void* /*param*/) {
    definitions::SystemEvent event = definitions::SystemEvent::kPowerOn;
    definitions::CommandRequest command = {definitions::CommandType::kHeaterOff, 0};

    while (true) {
        while (xQueueReceive(state_event_queue, &event, 0) == pdPASS) {
            modules::StateMachine::instance().process_event(event);
        }

        while (xQueueReceive(state_command_queue, &command, 0) == pdPASS) {
            modules::StateMachine::instance().process_command(command);
        }

        modules::StateMachine::instance().tick();
        vTaskDelay(pdMS_TO_TICKS(kStateMachineTaskPeriodMs));
    }
}

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
    on_state_enter();
}

void modules::StateMachine::tick() {
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

void modules::StateMachine::process_command(const definitions::CommandRequest& request) {
    execute_command(request);
}

bool modules::StateMachine::enqueue_event(definitions::SystemEvent event, uint32_t timeout_ms) {
    if (state_event_queue == nullptr) {
        process_event(event);
        return false;
    }

    if (xQueueSend(state_event_queue, &event, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        process_event(event);
        return false;
    }

    return true;
}

bool modules::StateMachine::enqueue_command(const definitions::CommandRequest& request, uint32_t timeout_ms) {
    if (state_command_queue == nullptr) {
        process_command(request);
        return false;
    }

    if (xQueueSend(state_command_queue, &request, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        process_command(request);
        return false;
    }

    return true;
}

bool modules::StateMachine::route_command_name(const std::string& command_name, uint32_t pin, uint32_t timeout_ms) {
    const auto it =
        std::find_if(definitions::kCommandMap.begin(), definitions::kCommandMap.end(),
                     [&command_name](const definitions::CommandMap& entry) { return command_name == entry.name; });

    if (it == definitions::kCommandMap.end()) {
        return false;
    }

    switch (it->command) {
        case definitions::CommandType::kHeaterOn:
        case definitions::CommandType::kHeaterOff:
        case definitions::CommandType::kPumpOn:
        case definitions::CommandType::kPumpOff:
        case definitions::CommandType::kChillerOn:
        case definitions::CommandType::kChillerOff:
        case definitions::CommandType::kValveOpen:
        case definitions::CommandType::kValveClose:
            enqueue_command({it->command, pin}, timeout_ms);
            return true;
        default:
            return false;
    }
}

bool modules::StateMachine::route_event_name(const std::string& event_name, uint32_t timeout_ms) {
    const auto it =
        std::find_if(definitions::kEventMap.begin(), definitions::kEventMap.end(),
                     [&event_name](const definitions::EventMap& entry) { return event_name == entry.name; });

    if (it == definitions::kEventMap.end()) {
        return false;
    }

    enqueue_event(it->event, timeout_ms);
    return true;
}

void modules::StateMachine::start_task() {
    if (state_event_queue != nullptr) {
        modules::hal_general().log("La tarea de la maquina de estados ya esta en ejecucion.");
        return;
    }

    state_event_queue = xQueueCreate(16, sizeof(definitions::SystemEvent));
    state_command_queue = xQueueCreate(16, sizeof(definitions::CommandRequest));
    if (state_event_queue == nullptr || state_command_queue == nullptr) {
        modules::hal_general().log("No se pudo crear la queue de la maquina de estados.");
        if (state_event_queue != nullptr) {
            vQueueDelete(state_event_queue);
            state_event_queue = nullptr;
        }
        if (state_command_queue != nullptr) {
            vQueueDelete(state_command_queue);
            state_command_queue = nullptr;
        }
        return;
    }

    if (xTaskCreate(state_machine_task, "state_machine_task", 4096, nullptr, kStateMachineTaskPriority, nullptr) !=
        pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea de la maquina de estados.");
        vQueueDelete(state_event_queue);
        vQueueDelete(state_command_queue);
        state_event_queue = nullptr;
        state_command_queue = nullptr;
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
    switch (currentState_) {
        case definitions::SystemState::kStartup:
        case definitions::SystemState::kCheckup:
        case definitions::SystemState::kCalibration:
        case definitions::SystemState::kSoftError: {
            modules::Heater::instance().send_command(definitions::CommandType::kHeaterOff, 10);
            modules::Pump::instance().send_command(definitions::CommandType::kPumpOff, 10);
            modules::Chiller::instance().send_command(definitions::CommandType::kChillerOff, 10);
            break;
        }

        case definitions::SystemState::kCriticalError: {
            modules::Heater::instance().send_command(definitions::CommandType::kHeaterOff, 10);
            modules::Pump::instance().send_command(definitions::CommandType::kPumpOff, 10);
            modules::Chiller::instance().send_command(definitions::CommandType::kChillerOff, 10);

#if defined(PLC_38AR) || defined(PLC_19R)
            const uint32_t valve1_pin = static_cast<uint32_t>(definitions::Pins::kValve1);
            const uint32_t valve2_pin = static_cast<uint32_t>(definitions::Pins::kValve2);
            modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveClose, valve1_pin, 10);
            modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveClose, valve2_pin, 10);
#endif
            break;
        }

        case definitions::SystemState::kManual:
        default:
            break;
    }
}

void modules::StateMachine::execute_command(const definitions::CommandRequest& request) {
    const bool allow_actuation =
        currentState_ == definitions::SystemState::kManual || currentState_ == definitions::SystemState::kCalibration;

    if (!allow_actuation) {
        modules::hal_general().log("Command received but actuation is not allowed in current state: " +
                                   get_state_name(currentState_));
        return;
    }

    switch (request.command) {
        case definitions::CommandType::kHeaterOn:
            modules::Heater::instance().send_command(definitions::CommandType::kHeaterOn, 10);
            break;
        case definitions::CommandType::kHeaterOff:
            modules::Heater::instance().send_command(definitions::CommandType::kHeaterOff, 10);
            break;
        case definitions::CommandType::kPumpOn:
            modules::Pump::instance().send_command(definitions::CommandType::kPumpOn, 10);
            break;
        case definitions::CommandType::kPumpOff:
            modules::Pump::instance().send_command(definitions::CommandType::kPumpOff, 10);
            break;
        case definitions::CommandType::kChillerOn:
            modules::Chiller::instance().send_command(definitions::CommandType::kChillerOn, 10);
            break;
        case definitions::CommandType::kChillerOff:
            modules::Chiller::instance().send_command(definitions::CommandType::kChillerOff, 10);
            break;
        case definitions::CommandType::kValveOpen:
            modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveOpen, request.pin, 10);
            break;
        case definitions::CommandType::kValveClose:
            modules::SolenoidValve::instance().send_command(definitions::CommandType::kValveClose, request.pin, 10);
            break;
        default:
            break;
    }
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
