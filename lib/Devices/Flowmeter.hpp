///
///@file Flowmeter.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Flowmeter control module for the reactor system.
///@version 0.1
///@date 08-04-2026
///
///@copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stddef.h>
#include <stdint.h>

#include "Definitions.hpp"
#include "ICommandResponseQueue.hpp"

namespace modules {

///
/// @brief Singleton Flowmeter controller. It manages the flowmeter sensor and its readings.
/// The flowmeter measures flow by pulses managed by an interrupt service routine, and calculates flow rate in liters per minute.
///
class Flowmeter {
   public:
    ///
    /// @brief Get the singleton instance of the Flowmeter controller.
    ///
    /// @return Flowmeter&
    ///
    static Flowmeter& instance();

    ///
    /// @brief Start the dedicated FreeRTOS task/queue for this module.
    ///
    bool create_task();

    ///
    /// @brief Set the sink used to publish command execution responses.
    ///
    /// @param queue Response queue implementation. If nullptr, response publishing is disabled.
    ///
    void set_response_queue(interfaces::ICommandResponseQueue* queue);

    ///
    /// @brief Initialize flowmeter with the sensor pin and necessary hardware setup. Attach interrupts and timer.
    ///
    /// @param pinFlow_ Device Pin.
    ///
    bool begin(uint32_t pinFlow_);

    ///
    /// @brief Stop the flowmeter. Detach interrupts and end timer.
    ///
    /// @param pinFlow_ Device pin.
    ///
    void stop(uint32_t pinFlow_);

    ///
    /// @brief Read flow rate in liters per minute. Returns the latest calculated flow rate based on pulse counts in the last second.
    ///
    /// @return Float Flow rate in liters per minute, or NaN if unavailable.
    ///
    float readFlow();

    ///
    /// @brief Get the flowmeter status.
    ///
    /// @return true if flowmeter is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the flowmeter relay pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    ///
    /// @brief Send a Flowmeter command through the module queue.
    /// @param command Command to be sent.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false otherwise.
    ///
    bool queue_command(definitions::CommandType command, uint32_t timeout_ms = 0);

   private:
    Flowmeter();

    // Rule of Five
    ~Flowmeter() = default;
    Flowmeter(const Flowmeter&) = delete;
    Flowmeter& operator=(const Flowmeter&) = delete;
    Flowmeter(Flowmeter&&) = delete;
    Flowmeter& operator=(Flowmeter&&) = delete;

    ///
    /// @brief Process a Flowmeter command directly, without queueing. Used as a fallback when the queue is unavailable.
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
    /// @brief FreeRTOS task entry point for processing flowmeter commands from the queue.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    uint32_t pin_;                                       ///< Pin used for flowmeter control
    bool status_;                                        ///< Current flowmeter status
    QueueHandle_t queue_;                                ///< FreeRTOS queue handle for flowmeter commands
    interfaces::ICommandResponseQueue* response_queue_;  ///< Queue used to publish command responses
};

}  // namespace modules
