///
/// @file Pump.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Pump control module for the reactor system.
/// @version 0.1
/// @date 24-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdint.h>

#include "Definitions.hpp"

namespace modules {

///
/// @brief Singleton pump controller. Handles basic on/off and status logic.
///
class Pump {
   public:
    ///
    /// @brief Get the singleton instance of the Pump controller.
    ///
    /// @return Pump&
    ///
    static Pump& instance();

    ///
    /// @brief Initialize the pump controller with the pin.
    ///
    /// @param pin Pin for pump control.
    ///
    void begin(uint32_t pin);

    ///
    /// @brief Turn the pump on.
    ///
    void on();

    ///
    /// @brief Turn the pump off.
    ///
    void off();

    ///
    /// @brief Get the pump status.
    ///
    /// @return true if pump is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the pump pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    ///
    /// @brief Start the dedicated FreeRTOS task/queue for this module.
    ///
    void start_task();

    ///
    /// @brief Send a pump command through the module queue.
    /// Falls back to direct call when queueing is unavailable.
    ///
    /// @param command Command to be sent.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false if direct fallback was used.
    ///
    bool send_command(definitions::CommandType command, uint32_t timeout_ms = 0);

   private:
    Pump();
    // Rule of Five
    ~Pump() = default;
    Pump(const Pump&) = delete;
    Pump& operator=(const Pump&) = delete;
    Pump(Pump&&) = delete;
    Pump& operator=(Pump&&) = delete;

    uint32_t pin_;
    bool status_;
    QueueHandle_t queue_;

    static void task_entry(void* param);
};

}  // namespace modules
