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

#include <stdint.h>

#include "Definitions.hpp"
#include "SyncDevice.hpp"

namespace modules {

///
/// @brief Singleton level switch controller. Handles basic reading.
///
/// A SyncDevice: the state read is a single GPIO read executed inline on the
/// caller's task (no dedicated task or queue).
///
class LevelSwitch : public SyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the level switch controller.
    ///
    /// @return LevelSwitch&
    ///
    static LevelSwitch& instance();

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

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

   protected:
    /// @copydoc modules::SyncDevice::execute_command
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    LevelSwitch();
    // Rule of Five
    ~LevelSwitch() = default;
    LevelSwitch(const LevelSwitch&) = delete;
    LevelSwitch& operator=(const LevelSwitch&) = delete;
    LevelSwitch(LevelSwitch&&) = delete;
    LevelSwitch& operator=(LevelSwitch&&) = delete;

    uint32_t pin_;  ///< Pin used for level switch control
    bool status_;   ///< Current level switch status
};

}  // namespace modules
