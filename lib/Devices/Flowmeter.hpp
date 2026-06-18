///
///@file Flowmeter.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Flowmeter sensor module for the reactor system.
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
    /// If the flowmeter is already running, this is a no-op and returns true.
    ///
    /// @param pinFlow_ Device pin.
    /// @param pulsesPerLiter Calibration constant: sensor pulses per liter. Defaults to 450.0.
    ///
    bool begin(uint32_t pinFlow_, float pulsesPerLiter = definitions::kFlowmeterPulsesPerLiter);

    ///
    /// @brief Stop the flowmeter. Detach interrupts and end timer.
    ///
    void stop();

    ///
    /// @brief Read flow rate in liters per minute. Returns the latest calculated flow rate based on pulse counts in the last second.
    ///
    /// @return Float Flow rate in liters per minute, or NaN if unavailable.
    ///
    float readFlow();

    ///
    /// @brief Get the flowmeter hardware status.
    ///
    /// @return true if the interrupt and timer are active, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the flow sensor input pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    ///
    /// @brief Queue a Flowmeter command through the module queue.
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
    /// @brief Process a Flowmeter command from the task loop. Only kFlowRead is accepted;
    /// all other command types return false immediately.
    ///
    /// @param command Command to be processed.
    /// @return true if the command was handled and a response published, false otherwise.
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
    /// @brief FreeRTOS task entry point for processing flowmeter commands from the queue.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    uint32_t pin_;                                       ///< Input pin for the flow sensor pulse signal
    bool status_;                                        ///< True when the hardware interrupt and timer are active
    float pulsesPerLiter_;                               ///< Calibration constant: sensor pulses per liter
    QueueHandle_t queue_;                                ///< FreeRTOS queue handle for flowmeter commands
    interfaces::ICommandResponseQueue* response_queue_;  ///< Queue used to publish command responses
};

}  // namespace modules
