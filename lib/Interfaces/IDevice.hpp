///
/// @file IDevice.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Generic device interface for command dispatch in the StateMachine routing layer.
/// @version 0.1
/// @date 18-06-2026
///
/// @copyright Copyright (c) 2026
///

#pragma once

#include <stddef.h>

#include "Definitions.hpp"

namespace interfaces {

class IDevice {
   public:
    virtual ~IDevice() = default;

    ///
    /// @brief Return true if this device handles the given command type.
    ///
    /// @param type The CommandType to check.
    /// @return true if this device accepts and can process the command.
    ///
    virtual bool accepts(definitions::CommandType type) const = 0;

    ///
    /// @brief Enqueue a full Command to the device's internal queue.
    ///
    /// The device is responsible for extracting any parameter it needs from cmd.extra
    /// (e.g. valve pin for kValveOpen) or from its own internal command-to-address mapping
    /// (e.g. kTempInRead -> I2C address in Thermocouple).
    ///
    /// @param cmd Command to dispatch.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if the command was accepted and queued.
    ///
    virtual bool queue_command(const definitions::Command& cmd, uint32_t timeout_ms) = 0;

   protected:
    IDevice() = default;
    IDevice(const IDevice&) = delete;
    IDevice& operator=(const IDevice&) = delete;
    IDevice(IDevice&&) = delete;
    IDevice& operator=(IDevice&&) = delete;
};

}  // namespace interfaces

namespace modules {
using interfaces::IDevice;
}  // namespace modules
