///
/// @file Heater.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Heater control module for the reactor system.
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
#include "ICommandResponseQueue.hpp"

namespace modules {

///
/// @brief Singleton heater controller. Handles basic on/off and status logic.
///
class Heater {
   public:
    ///
    /// @brief Get the singleton instance of the Heater controller.
    ///
    /// @return Heater&
    ///
    static Heater& instance();

    ///
    /// @brief Set the sink used to publish command execution responses.
    ///
    /// @param sink Response sink implementation. If nullptr, response publishing is disabled.
    ///
    void set_response_queue(interfaces::ICommandResponseQueue* queue);

    ///
    /// @brief Initialize the heater controller with the relay pin.
    ///
    /// @param pin Relay pin for heater control.
    ///
    void begin(uint32_t pin);

    ///
    /// @brief Turn the heater on.
    ///
    void on();

    ///
    /// @brief Turn the heater off.
    ///
    void off();

    ///
    /// @brief Get the heater status.
    ///
    /// @return true if heater is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the heater relay pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    ///
    /// @brief Start the dedicated FreeRTOS task/queue for this module.
    ///
    void start_task();

    ///
    /// @brief Queue a heater command in the module's local queue.
    ///
    /// @param command Command to be sent.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false otherwise.
    ///
    bool queue_command(definitions::CommandType command, uint32_t timeout_ms = 0);

   private:
    Heater();
    // Rule of Five
    ~Heater() = default;
    Heater(const Heater&) = delete;
    Heater& operator=(const Heater&) = delete;
    Heater(Heater&&) = delete;
    Heater& operator=(Heater&&) = delete;

    ///
    /// @brief Process a heater command directly, without queueing. Used as a fallback when the queue is unavailable.
    ///
    /// @param command Command to be processed.
    /// @return true
    /// @return false
    ///
    bool process_command(definitions::CommandType command);

    ///
    /// @brief Publish a response through the configured sink.
    ///
    /// @param response Response payload.
    /// @return true if the sink accepted the response.
    ///
    bool publish_response(const definitions::CommandRequest& response);

    ///
    /// @brief FreeRTOS task entrypoint for the heater module. Waits for commands on the queue and processes them.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    uint32_t pin_;                                       ///< Pin used for heater control
    bool status_;                                        ///< Current heater status
    QueueHandle_t queue_;                                ///< FreeRTOS queue handle for heater commands
    interfaces::ICommandResponseQueue* response_queue_;  ///< Queue used to publish command responses
};

}  // namespace modules
