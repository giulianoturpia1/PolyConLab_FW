///
///@file App.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Application layer module that manages the program startup, the main loop and the shutdown of the system.
///@version 0.2
///@date 2026-03-07
///
///@copyright Copyright (c) 2026
///
///

#pragma once

#include <string>

// Forward declarations
namespace interfaces {
class HALGeneral;
}

namespace modules {
class Communications;
class Heater;
class Pump;
class Chiller;
class SolenoidValve;
class Thermocouple;
class Flowmeter;
class LevelSwitch;
class StateMachine;
}  // namespace modules

namespace modules {

///
/// @brief Application orchestrator for startup and main runtime cycle.
/// Implemented as a singleton class, similar to StateMachine.
/// The App class is responsible for initializing all modules, starting their tasks, and running the main loop that
/// polls for communications and dispatches them to the appropriate handlers.
///
/// The main loop is implemented in the tick() method, which is called periodically from a dedicated FreeRTOS task. This is implemented
/// in the task_entry() static method, and the task is started by calling start_task().
///
class App {
   public:
    ///
    /// @brief Get the singleton instance of the App.
    ///
    /// @return App&
    ///
    static App& instance();

    ///
    /// @brief Initialize firmware application services and module tasks. This serves as the main entry point for the firmware execution.
    ///
    void begin();

   private:
    App();

    // Rule of Five
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    ///
    /// @brief Configure the serial response callback to log responses through HAL.
    ///
    ///
    void configure_serial_response_callback();

    ///
    /// @brief Initialize the Ethernet server and related settings.
    ///
    ///
    void initialize_network_server();

    ///
    /// @brief Initialize all modules and the state machine, and start their tasks.
    ///
    ///
    void initialize_system();

    ///
    /// @brief Initialize hardware-facing device modules.
    ///
    void initialize_devices();

    ///
    /// @brief Start the tasks for the initialized modules.
    ///
    void initialize_tasks();

    ///
    /// @brief Poll serial messages and dispatch them to the communications handler.
    ///
    ///
    void poll_serial_messages();

    ///
    /// @brief Handle one ethernet message using the current client response callback.
    ///
    void handle_ethernet_message(const std::string& msg, const void* client_respond);

    ///
    /// @brief Start the dedicated App FreeRTOS task that executes run() periodically.
    ///
    void start_task();

    ///
    /// @brief Execute one application cycle (communications polling and dispatch).
    ///
    void tick();

    ///
    /// @brief FreeRTOS task entrypoint for the App runtime loop.
    ///
    static void task_entry(void* param);

    // Member variables - References to Modules

    StateMachine& state_machine_;
    interfaces::HALGeneral& hal_;
    void* local_server_;
    Communications& communications_;
    Heater& heater_;
    Pump& pump_;
    Chiller& chiller_;
    SolenoidValve& valves_;
    Thermocouple& thermocouples_;
    Flowmeter& flowmeter_;
    LevelSwitch& level_switch_;
};

}  // namespace modules
