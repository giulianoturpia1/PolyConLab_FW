///
/// @file LevelSwitch.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Level switch control module for the reactor system.
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
/// @brief Singleton level switch controller. Handles basic reading.
///
class LevelSwitch {
   public:
    ///
    /// @brief Get the singleton instance of the level switch controller.
    ///
    /// @return LevelSwitch&
    ///
    static LevelSwitch& instance();

    ///
    /// @brief Start the dedicated FreeRTOS task/queue for this module.
    ///
    bool create_task();

    ///
    /// @brief Set the sink used to publish command execution responses.
    ///
    /// @param sink Response sink implementation. If nullptr, response publishing is disabled.
    ///
    void set_response_queue(interfaces::ICommandResponseQueue* queue);

    ///
    /// @brief Initialize the level switch controller with the relay pin.
    ///
    /// @param pin Relay pin for level switch.
    ///
    bool begin(uint32_t pin);

    ///
    /// @brief Get the level switch status.
    ///
    /// @return true if switch is ON (high level), false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the level switch relay pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    ///
    /// @brief Queue a level switch command in the module's local queue.
    ///
    /// @param command Command to be sent.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false otherwise.
    ///
    bool queue_command(definitions::CommandType command, uint32_t timeout_ms = 0);

   private:
    LevelSwitch();
    // Rule of Five
    ~LevelSwitch() = default;
    LevelSwitch(const LevelSwitch&) = delete;
    LevelSwitch& operator=(const LevelSwitch&) = delete;
    LevelSwitch(LevelSwitch&&) = delete;
    LevelSwitch& operator=(LevelSwitch&&) = delete;

    ///
    /// @brief Process a level switch command directly, without queueing. Used as a fallback when the queue is unavailable.
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
    bool publish_response(const definitions::Command& response);

    ///
    /// @brief FreeRTOS task entrypoint for the level switch module. Waits for commands on the queue and processes them.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    uint32_t pin_;                                       ///< Pin used for level switch control
    bool status_;                                        ///< Current level switch status
    QueueHandle_t queue_;                                ///< FreeRTOS queue handle for level switch commands
    interfaces::ICommandResponseQueue* response_queue_;  ///< Queue used to publish command responses
};

}  // namespace modules
