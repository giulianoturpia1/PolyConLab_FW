///
/// @file Pump.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Pump control module for the reactor system.
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
/// @brief Singleton pump controller. Handles basic on/off and status logic.
///
/// A SyncDevice: commands are a single GPIO write executed inline on the caller's
/// task (no dedicated task or queue).
///
class Pump : public SyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the Pump controller.
    ///
    /// @return Pump&
    ///
    static Pump& instance();

    ///
    /// @brief Initialize the pump controller with the pin.
    ///
    /// @param pin Pin for pump control.
    ///
    bool begin(uint32_t pin);

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

   protected:
    /// @copydoc modules::SyncDevice::execute_command
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    Pump();
    // Rule of Five
    ~Pump() = default;
    Pump(const Pump&) = delete;
    Pump& operator=(const Pump&) = delete;
    Pump(Pump&&) = delete;
    Pump& operator=(Pump&&) = delete;

    ///
    /// @brief Turn the pump on. Internal actuation reached via execute_command().
    ///
    void on();

    ///
    /// @brief Turn the pump off. Internal actuation reached via execute_command()/begin().
    ///
    void off();

    ///
    /// @brief Get the pump status.
    ///
    /// @return true if pump is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the pump pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    uint32_t pin_;  ///< Pin used for pump control
    bool status_;   ///< Current pump status (true = ON, false = OFF)
};

}  // namespace modules
