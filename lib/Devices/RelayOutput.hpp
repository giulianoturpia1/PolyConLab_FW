///
/// @file RelayOutput.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Generic relay-driven output control module for the reactor system.
/// @version 0.1
/// @date 17-09-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <stdint.h>

#include "Definitions.hpp"
#include "SyncDevice.hpp"

namespace modules {

///
/// @brief Generic controller for a single digital output that enables an external relay/PSU.
/// Handles basic on/off and status logic against a command triple supplied at construction.
///
/// A SyncDevice: commands are a single GPIO write executed inline on the caller's
/// task (no dedicated task or queue). Instances are the heater(), pump() and mixer() outputs.
/// An output that needs behaviour beyond plain on/off/status gets its own class instead.
///
class RelayOutput : public SyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the heater RelayOutput.
    ///
    /// @return RelayOutput&
    ///
    static RelayOutput& heater();

    ///
    /// @brief Get the singleton instance of the circulation pump RelayOutput.
    ///
    /// @return RelayOutput&
    ///
    static RelayOutput& pump();

    ///
    /// @brief Get the singleton instance of the mixer/emulsifier RelayOutput.
    ///
    /// @return RelayOutput&
    ///
    static RelayOutput& mixer();

    ///
    /// @brief Initialize the relay output with the pin.
    ///
    /// @param pin Pin for relay control.
    ///
    bool begin(uint32_t pin);

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

   protected:
    /// @copydoc modules::SyncDevice::execute_command
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    RelayOutput(const char* name, definitions::CommandType on_cmd, definitions::CommandType off_cmd, definitions::CommandType state_cmd);
    // Rule of Five
    ~RelayOutput() = default;
    RelayOutput(const RelayOutput&) = delete;
    RelayOutput& operator=(const RelayOutput&) = delete;
    RelayOutput(RelayOutput&&) = delete;
    RelayOutput& operator=(RelayOutput&&) = delete;

    ///
    /// @brief Turn the output on. Internal actuation reached via execute_command().
    ///
    void on();

    ///
    /// @brief Turn the output off. Internal actuation reached via execute_command()/begin().
    ///
    void off();

    ///
    /// @brief Get the output status.
    ///
    /// @return true if the output is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the output pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    const char* name_;                          ///< Instance name, used in log messages (e.g. "HEATER", "MIXER")
    const definitions::CommandType on_cmd_;     ///< Command type that turns the output on
    const definitions::CommandType off_cmd_;    ///< Command type that turns the output off
    const definitions::CommandType state_cmd_;  ///< Command type that queries the output status
    uint32_t pin_;                              ///< Pin used for output control
    bool status_;                               ///< Current output status
};

}  // namespace modules
