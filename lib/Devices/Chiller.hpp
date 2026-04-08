///
/// @file Chiller.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Chiller control module for the reactor system.
/// @version 0.1
/// @date 04-03-2026
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
/// @brief Singleton chiller controller. Handles basic on/off and status logic.
///
class Chiller {
   public:
    ///
    /// @brief Get the singleton instance of the Chiller controller.
    ///
    /// @return Chiller&
    ///
    static Chiller& instance();

    ///
    /// @brief Initialize the chiller controller with the pin.
    ///
    /// @param pin Pin for chiller control.
    ///
    void begin(uint32_t pin);

    ///
    /// @brief Turn the chiller on.
    ///
    void on();

    ///
    /// @brief Turn the chiller off.
    ///
    void off();

    ///
    /// @brief Get the chiller status.
    ///
    /// @return true if chiller is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the chiller pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    ///
    /// @brief Start the dedicated FreeRTOS task/queue for this module.
    ///
    void start_task();

    ///
    /// @brief Queue a chiller command in the module's local queue.
    ///
    /// @param command Command to be sent.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false otherwise.
    ///
    bool queue_command(definitions::CommandType command, uint32_t timeout_ms = 0);

   private:
    Chiller();
    // Rule of Five
    ~Chiller() = default;
    Chiller(const Chiller&) = delete;
    Chiller& operator=(const Chiller&) = delete;
    Chiller(Chiller&&) = delete;
    Chiller& operator=(Chiller&&) = delete;

    ///
    /// @brief Process a chiller command directly, without queueing. Used as a fallback when the queue is unavailable.
    ///
    /// @param command
    /// @return true
    /// @return false
    ///
    bool process_command(definitions::CommandType command);

    ///
    /// @brief
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    uint32_t pin_;         ///< Pin used for chiller control
    bool status_;          ///< Current chiller status
    QueueHandle_t queue_;  ///< FreeRTOS queue handle for chiller commands
};

}  // namespace modules
