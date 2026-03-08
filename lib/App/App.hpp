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

// Forward declarations
namespace modules {
class HALGeneral;
class LocalServer;
class Communications;
class Heater;
class Pump;
class Chiller;
class SolenoidValve;
class StateMachine;
}  // namespace modules

namespace modules {

///
/// @brief Application orchestrator for startup and main runtime cycle.
/// Implemented as a singleton class, similar to StateMachine.
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
    /// @brief Initialize firmware application services and module tasks.
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
    void initialize_modules();

    ///
    /// @brief Initialize hardware-facing device modules.
    ///
    void initialize_devices();

    ///
    /// @brief Poll serial messages and dispatch them to the communications handler.
    ///
    ///
    void poll_serial_messages();

    ///
    /// @brief Start the dedicated App FreeRTOS task that executes run() periodically.
    ///
    void start_task();

    ///
    /// @brief FreeRTOS task entrypoint for the App runtime loop.
    ///
    static void task_entry(void* param);

    ///
    /// @brief Execute one application cycle (communications polling and dispatch).
    ///
    void tick();

    HALGeneral& hal_;
    LocalServer& local_server_;
    Communications& communications_;
    Heater& heater_;
    Pump& pump_;
    Chiller& chiller_;
    SolenoidValve& valves_;
    StateMachine& state_machine_;
};

}  // namespace modules
