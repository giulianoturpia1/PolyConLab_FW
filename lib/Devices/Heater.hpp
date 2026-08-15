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

#include <stdint.h>

#include "Definitions.hpp"
#include "SyncDevice.hpp"

namespace modules {

///
/// @brief Singleton heater controller. Handles basic on/off and status logic.
///
/// A SyncDevice: commands are a single GPIO write executed inline on the caller's
/// task (no dedicated task or queue).
///
class Heater : public SyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the Heater controller.
    ///
    /// @return Heater&
    ///
    static Heater& instance();

    ///
    /// @brief Initialize the heater controller with the relay pin.
    ///
    /// @param pin Relay pin for heater control.
    ///
    bool begin(uint32_t pin);

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

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

   protected:
    /// @copydoc modules::SyncDevice::execute_command
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    Heater();
    // Rule of Five
    ~Heater() = default;
    Heater(const Heater&) = delete;
    Heater& operator=(const Heater&) = delete;
    Heater(Heater&&) = delete;
    Heater& operator=(Heater&&) = delete;

    uint32_t pin_;  ///< Pin used for heater control
    bool status_;   ///< Current heater status
};

}  // namespace modules
