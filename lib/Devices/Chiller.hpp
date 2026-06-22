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
#include "ICommandResponseQueue.hpp"
#include "IDevice.hpp"

namespace modules {

///
/// @brief Singleton chiller controller. Handles basic on/off and status logic.
///
class Chiller : public interfaces::IDevice {
   public:
    ///
    /// @brief Get the singleton instance of the Chiller controller.
    ///
    /// @return Chiller&
    ///
    static Chiller& instance();

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
    /// @brief Initialize the chiller controller with the pin.
    ///
    /// @param pin Pin for chiller control.
    ///
    bool begin(uint32_t pin);

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

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

    /// @copydoc interfaces::IDevice::queue_command(const definitions::Command&, uint32_t)
    bool queue_command(const definitions::Command& cmd, uint32_t timeout_ms) override;

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
    /// @brief Publish a response through the configured sink.
    ///
    /// @param response Response payload.
    /// @return true if the sink accepted the response.
    ///
    bool publish_response(const definitions::Command& response);

    ///
    /// @brief FreeRTOS task entrypoint for the chiller module. Waits for commands on the queue and processes them.
    ///
    /// @param param Pointer to the Chiller instance.
    ///
    static void task_entry(void* param);

    uint32_t pin_;                                       ///< Pin used for chiller control
    bool status_;                                        ///< Current chiller status
    QueueHandle_t queue_;                                ///< FreeRTOS queue handle for chiller commands
    interfaces::ICommandResponseQueue* response_queue_;  ///< Queue used to publish command responses
};

}  // namespace modules
