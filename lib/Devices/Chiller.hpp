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

#include <stdint.h>

#include "Definitions.hpp"
#include "SyncDevice.hpp"

namespace modules {

///
/// @brief Singleton chiller controller. Handles basic on/off and status logic.
///
/// A SyncDevice: commands are a single GPIO write executed inline on the caller's
/// task (no dedicated task or queue).
///
class Chiller : public SyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the Chiller controller.
    ///
    /// @return Chiller&
    ///
    static Chiller& instance();

    ///
    /// @brief Initialize the chiller controller with the pin.
    ///
    /// @param pin Pin for chiller control.
    ///
    bool begin(uint32_t pin);

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

   protected:
    /// @copydoc modules::SyncDevice::execute_command
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    Chiller();
    // Rule of Five
    ~Chiller() = default;
    Chiller(const Chiller&) = delete;
    Chiller& operator=(const Chiller&) = delete;
    Chiller(Chiller&&) = delete;
    Chiller& operator=(Chiller&&) = delete;

    ///
    /// @brief Turn the chiller on. Internal actuation reached via execute_command().
    ///
    void on();

    ///
    /// @brief Turn the chiller off. Internal actuation reached via execute_command()/begin().
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

    uint32_t pin_;  ///< Pin used for chiller control
    bool status_;   ///< Current chiller status
};

}  // namespace modules
