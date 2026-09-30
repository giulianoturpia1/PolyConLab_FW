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
#include <freertos/semphr.h>
#include <stddef.h>
#include <stdint.h>

#include <array>
#include <limits>

#include "Adafruit_MCP9600.h"
#include "AsyncDevice.hpp"
#include "Definitions.hpp"

namespace modules {

///
/// @brief Singleton Thermocouple Amplifier controller. It supports multiple Thermocouple Amplifiers and manages their states.
/// Thermocouple Amplifiers are identified by their I2C addresses.
///
/// An AsyncDevice: temperature reads are blocking I2C transactions served on a
/// dedicated task (Core 1). All bus access — from the task and from synchronous
/// callers (e.g. control states on Core 0) — is serialized by a shared bus mutex.
///
class Thermocouple : public AsyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the Thermocouple controller.
    ///
    /// @return Thermocouple&
    ///
    static Thermocouple& instance();

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

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

    ///
    /// @brief Whether the Thermocouple Amplifier at @p address is currently registered.
    ///
    /// True after a successful begin()/add() for this address that has not since been dropped by
    /// remove() or a failed reinit().
    ///
    /// @param address I2C address of the Thermocouple Amplifier.
    /// @return bool True if the address is currently registered.
    ///
    bool is_initialized(uint32_t address) const;

   protected:
    ///
    /// @copydoc modules::AsyncDevice::execute_command
    /// @note Resolves cmd.type to the corresponding I2C address (kTempInRead → kTempIn, etc.),
    ///       reads the sensor and packs the temperature as IEEE-754 bits in the response's extra.
    ///
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    Thermocouple();

    // Rule of Five
    ~Thermocouple() = default;
    Thermocouple(const Thermocouple&) = delete;
    Thermocouple& operator=(const Thermocouple&) = delete;
    Thermocouple(Thermocouple&&) = delete;
    Thermocouple& operator=(Thermocouple&&) = delete;

    ///
    /// @brief Register a Thermocouple with a certain address. Used internally by begin().
    /// Should count be higher than the maximum supported, only the first kMaxThermocouples will be registered.
    /// @note If an address that already exists in the controller, it will not be registered again, but it will not cause any
    /// issue as the controller will ignore duplicates.
    ///
    /// @param address Address of Thermocouple Amplifier.
    /// @return true if the thermocouple was registered or already existed, false on capacity limit.
    ///
    bool add(uint32_t address);

    ///
    /// @brief Remove a Thermocouple Amplifier from the controller.
    ///
    /// @param address Address of Thermocouple Amplifier.
    /// @return true if the thermocouple was found and removed, false if the address was not registered.
    ///
    bool remove(uint32_t address);

    ///
    /// @brief Re-initialise the Thermocouple Amplifier at a single I2C address (re-run its begin()).
    ///
    /// Acquires the shared I2C bus mutex, drops any stale registration for @p address, then
    /// re-registers and re-configures the amplifier. Used by the SYST:REINIT:* command path to
    /// recover a thermocouple that failed hardware init without a full PLC reset.
    ///
    /// @param address I2C address of the Thermocouple Amplifier to re-initialise.
    /// @return true if the amplifier initialised successfully, false otherwise.
    ///
    bool reinit(uint32_t address);

    ///
    /// @brief Get number of registered Thermocouple Amplifiers.
    ///
    /// @return uint8_t
    ///
    uint8_t count() const;

    ///
    /// @brief Read temperature in Celsius of the Thermocouple Amplifier with the address given.
    ///
    /// Acquires the shared I2C bus mutex for the duration of the transaction, so it is safe to
    /// call from any task. Returns NaN if the bus cannot be acquired within config::kI2cBusTimeoutMs.
    /// Reached via execute_command() (command path and synchronous read() alike).
    ///
    /// @param address I2C address of the Thermocouple Amplifier.
    /// @return temperature in °C or NaN if unavailable or address not registered.
    ///
    float readCelcius(uint32_t address);

    ///
    /// @brief Find the index of a Thermocouple Amplifier by its address.
    ///
    /// @param address Address of the Thermocouple Amplifier.
    /// @return uint8_t Index of the Thermocouple Amplifier in the internal array, or an out-of-range value if not found.
    ///
    uint8_t find_index(uint32_t address) const;

    ///
    /// @brief Internal structure to hold thermocouple information.
    ///
    struct ThermocoupleSlot {
        uint32_t address;                                                       ///< I2C address of the Thermocouple Amplifier
        bool status;                                                            ///< True when the amplifier was successfully initialized
        float last_valid_temperature{std::numeric_limits<float>::quiet_NaN()};  ///< Last accepted temperature reading for spike filtering
        Adafruit_MCP9600 device;                                                ///< MCP9600 driver instance for this thermocouple
    };

    std::array<ThermocoupleSlot, definitions::kMaxThermocouples> thermocouples_;  ///< Array of registered thermocouples and their devices
    uint8_t thermoc_count_;                                                       ///< Current count of registered thermocouples
    SemaphoreHandle_t bus_mutex_;  ///< Serializes access to the shared I2C bus (priority-inheriting)
};

}  // namespace modules
