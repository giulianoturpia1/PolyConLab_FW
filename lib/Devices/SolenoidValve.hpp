///
/// @file SolenoidValve.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Solenoid valve control module for the reactor system.
/// @version 0.1
/// @date 04-03-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stddef.h>
#include <stdint.h>

#include <array>

#include "Definitions.hpp"
#include "ICommandResponseQueue.hpp"

namespace modules {

///
/// @brief Singleton solenoid valve controller. It supports multiple valves and manages their states.
/// Valves are identified by their pin numbers.
///
class SolenoidValve {
   public:
    ///
    /// @brief Get the singleton instance of the SolenoidValve controller.
    ///
    /// @return SolenoidValve&
    ///
    static SolenoidValve& instance();

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
    /// @brief Initialize controller with an array of valve pins.
    /// Should count be higher than the maximum supported, only the first kMaxSolenoidValves will be registered.
    /// @note If a pin that already exists in the controller, it will not be registered again, but it will not cause any
    /// issue as the controller will ignore duplicates.
    ///
    /// @param pins Pointer to an array of pins.
    /// @param count Number of pins in the array.
    ///
    void begin(const uint32_t* pins, size_t count);

    ///
    /// @brief Register a valve attached to a certain pin.
    /// Should count be higher than the maximum supported, only the first kMaxSolenoidValves will be registered.
    /// @note If a pin that already exists in the controller, it will not be registered again, but it will not cause any
    /// issue as the controller will ignore duplicates.
    ///
    /// @param pin Pin for valve control.
    /// @return true if the valve was registered or already existed, false on capacity limit.
    ///
    bool add(uint32_t pin);

    ///
    /// @brief Remove a valve from the controller.
    ///
    /// @param pin Pin for valve control.
    /// @return true if the valve was found and removed, false if the pin was not registered.
    /// @return false if the pin was not registered.
    ///
    bool remove(uint32_t pin);

    ///
    /// @brief Open valve associated to the given pin.
    ///
    /// @param pin Pin for valve control.
    /// @return true if pin is registered and command was applied, false otherwise.
    ///
    bool open(uint32_t pin);

    ///
    /// @brief Close valve associated to the given pin.
    ///
    /// @param pin Pin for valve control.
    /// @return true if pin is registered and command was applied, false otherwise.
    ///
    bool close(uint32_t pin);

    ///
    /// @brief Get current valve status.
    ///
    /// @param pin Pin for valve control.
    /// @return true if valve is OPEN, false if CLOSED or unknown pin.
    ///
    bool status(uint32_t pin) const;

    ///
    /// @brief Get number of registered valves.
    ///
    /// @return uint8_t
    ///
    uint8_t count() const;

    ///
    /// @brief Queue a valve command in the module's local queue.
    ///
    /// @param command Command to be sent.
    /// @param pin Pin for valve control.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false otherwise.
    ///
    bool queue_command(definitions::CommandType command, uint32_t pin, uint32_t timeout_ms = 0);

   private:
    SolenoidValve();

    // Rule of Five
    ~SolenoidValve() = default;
    SolenoidValve(const SolenoidValve&) = delete;
    SolenoidValve& operator=(const SolenoidValve&) = delete;
    SolenoidValve(SolenoidValve&&) = delete;
    SolenoidValve& operator=(SolenoidValve&&) = delete;

    ///
    /// @brief Find the index of a valve by its pin.
    ///
    /// @param pin Pin for valve control.
    /// @return uint8_t Index of the valve in the internal array, or an out-of-range value if not found.
    ///
    uint8_t find_index(uint32_t pin) const;

    ///
    /// @brief Process a valve command directly, without queueing. Used as a fallback when the queue is unavailable.
    ///
    /// @param command Command to be processed.
    /// @param pin Pin for valve control.
    /// @return true
    /// @return false
    ///
    bool process_command(definitions::CommandType command, uint32_t pin);

    ///
    /// @brief Publish a response through the configured sink.
    ///
    /// @param response Response payload.
    /// @return true if the sink accepted the response.
    ///
    bool publish_response(const definitions::CommandRequest& response);

    ///
    /// @brief FreeRTOS task entry point for processing valve commands from the queue.
    ///
    /// @param param
    ///
    static void task_entry(void* param);

    ///
    /// @brief Internal structure to hold valve information (pin and status).
    ///
    ///
    struct ValveSlot {
        uint32_t pin;  //< Pin for valve control
        bool status;   //< Current valve status
    };

    std::array<ValveSlot, definitions::kMaxSolenoidValves> valves_;  ///< Array to hold registered valves
    uint8_t valve_count_;                                            ///< Current count of registered valves
    QueueHandle_t queue_;                                            ///< FreeRTOS queue handle for valve commands
    interfaces::ICommandResponseQueue* response_queue_;              ///< Queue used to publish command responses
};

}  // namespace modules
