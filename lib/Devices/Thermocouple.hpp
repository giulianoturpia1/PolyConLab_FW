///
///@file Thermocouple.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Thermocouple Amplifier control module for the reactor system.
///@version 0.1
///@date 07-04-2026
///
///@copyright Copyright (c) 2026
///
///

#pragma once

#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stddef.h>
#include <stdint.h>

#include <array>

#include "Adafruit_MCP9600.h"
#include "Definitions.hpp"
#include "ICommandResponseQueue.hpp"

namespace modules {

///
/// @brief Singleton Thermocouple Amplifier controller. It supports multiple Thermocouple Amplifiers and manages their states.
/// Thermocouple Amplifiers are identified by their I2C addresses.
///
class Thermocouple {
   public:
    ///
    /// @brief Get the singleton instance of the Thermocouple controller.
    ///
    /// @return Thermocouple&
    ///
    static Thermocouple& instance();

    ///
    /// @brief Set the sink used to publish command execution responses.
    ///
    /// @param queue Response queue implementation. If nullptr, response publishing is disabled.
    ///
    void set_response_queue(interfaces::ICommandResponseQueue* queue);

    ///
    /// @brief Initialize controller with an array of I2C addresses.
    /// Should count be higher than the maximum supported, only the first kMaxThermocouples will be registered.
    /// @note If an address that already exists in the controller, it will not be registered again, but it will not cause any
    /// issue as the controller will ignore duplicates.
    ///
    /// @param adds Pointer to an array of addresses.
    /// @param count Number of addresses in the array.
    ///
    bool begin(const uint32_t* adds, size_t count);

    ///
    /// @brief Register a Thermocouple with a certain address.
    /// Should count be higher than the maximum supported, only the first kMaxThermocouples will be registered.
    /// @note If an address that already exists in the controller, it will not be registered again, but it will not cause any
    /// issue as the controller will ignore duplicates.
    ///
    /// @param address Address of Thermocouple Amplifier.
    /// @return true if the valve was registered or already existed, false on capacity limit.
    ///
    bool add(uint32_t address);

    ///
    /// @brief Remove a Thermocouple Amplifier from the controller.
    ///
    /// @param address Address of Thermocouple Amplifier.
    /// @return true if the valve was found and removed, false if the pin was not registered.
    /// @return false if the address was not registered.
    ///
    bool remove(uint32_t address);

    ///
    /// @brief Read temperature in Celsius of the Thermocouple Amplifier with the address given.
    ///
    /// @param address I2C address of the Thermocouple Amplifier.
    /// @return temperature in °C or NaN if unavailable or address not registered.
    ///
    float readCelcius(uint32_t address);

    ///
    /// @brief Get number of registered Thermocouple Amplifiers.
    ///
    /// @return uint8_t
    ///
    uint8_t count() const;

    ///
    /// @brief Start the dedicated FreeRTOS task/queue for this module.
    ///
    bool start_task();

    ///
    /// @brief Send a Thermocouple Amplifier command through the module queue.
    ///
    /// @param command Command to be sent.
    /// @param address Address of the Thermocouple Amplifier.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if command was queued, false otherwise.
    ///
    bool send_command(definitions::CommandType command, uint32_t address, uint32_t timeout_ms = 0);

   private:
    Thermocouple();

    // Rule of Five
    ~Thermocouple() = default;
    Thermocouple(const Thermocouple&) = delete;
    Thermocouple& operator=(const Thermocouple&) = delete;
    Thermocouple(Thermocouple&&) = delete;
    Thermocouple& operator=(Thermocouple&&) = delete;

    ///
    /// @brief Find the index of a Thermocouple Amplifier by its address.
    ///
    /// @param address Address of the Thermocouple Amplifier.
    /// @return uint8_t Index of the Thermocouple Amplifier in the internal array, or an out-of-range value if not found.
    ///
    uint8_t find_index(uint32_t address) const;

    ///
    /// @brief Process a Thermocouple Amplifier command directly, without queueing. Used as a fallback when the queue is unavailable.
    ///
    /// @param command Command to be processed.
    /// @param address Address of the Thermocouple Amplifier.
    /// @return true
    /// @return false
    ///
    bool process_command(definitions::CommandType command, uint32_t address);

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
    /// @brief Internal structure to hold thermocouple information.
    ///
    struct ThermocoupleSlot {
        uint32_t address;         // I2C address of the Thermocouple Amplifier
        bool status;              //< Current Thermocouple Amplifier status
        Adafruit_MCP9600 device;  //< MCP9600 driver instance for this thermocouple
    };

    std::array<ThermocoupleSlot, definitions::kMaxThermocouples>
        thermocouples_;                                  ///< Array of registered thermocouples and their MCP devices
    uint8_t thermoc_count_;                              ///< Current count of registered thermocouples
    QueueHandle_t queue_;                                ///< FreeRTOS queue handle for thermocouple commands
    interfaces::ICommandResponseQueue* response_queue_;  ///< Queue used to publish command responses
};

}  // namespace modules
