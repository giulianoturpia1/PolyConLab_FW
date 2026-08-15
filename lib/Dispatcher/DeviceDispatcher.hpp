///
/// @file DeviceDispatcher.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Routes Commands to the first registered IDevice that accepts them.
/// @version 0.1
/// @date 22-06-2026
///
/// @copyright Copyright (c) 2026
///

#pragma once

#include <stddef.h>

#include "Definitions.hpp"

namespace interfaces {
class IDevice;
}

namespace modules {

///
/// @brief Singleton command router. Holds the registered device list and dispatches
/// Commands to whichever IDevice::accepts() first, relieving both the StateMachine
/// and individual states from knowing concrete device types.
///
class DeviceDispatcher {
   public:
    ///
    /// @brief Get the singleton instance.
    ///
    static DeviceDispatcher& instance();

    ///
    /// @brief Register the device list. Call once during App init before any dispatch.
    ///
    /// @param devices Non-owning pointer to an array of IDevice pointers.
    /// @param count   Number of devices in the array.
    ///
    void set_devices(interfaces::IDevice* const* devices, size_t count);

    ///
    /// @brief Forward a Command to the first device whose accepts() returns true.
    ///
    /// The device publishes a response report, so this is the path for client-originated,
    /// confirmable commands routed through the StateMachine.
    ///
    /// @param cmd        Command to dispatch.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if a device accepted and queued the command.
    ///
    bool issue_to_device(const definitions::Command& cmd, uint32_t timeout_ms);

    ///
    /// @brief Enforce a Command on the first device whose accepts() returns true, WITHOUT
    /// publishing a response report.
    ///
    /// Same routing as issue_to_device(), but the device suppresses its response. This is
    /// the path for autonomous control-loop and safe-output actuations, which are not client
    /// commands and must not produce confirmations. Callers stay unaware of concrete device types.
    ///
    /// @param cmd        Command to enforce.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if a device accepted and queued/executed the command.
    ///
    bool enforce_to_device(const definitions::Command& cmd, uint32_t timeout_ms);

    ///
    /// @brief Read synchronously from the first device whose accepts() returns true, returning its
    /// response Command.
    ///
    /// Routes to IDevice::read(): the operation runs inline on the caller's task and the reading is
    /// returned in the response's extra (device-defined encoding, e.g. IEEE-754 temperature bits).
    /// If no device accepts the command, returns the command with a NaN sensor sentinel in extra so
    /// float-sensor callers fail safe via std::isnan.
    ///
    /// @param cmd Read command to dispatch.
    /// @return definitions::Command Response with the reading packed in extra.
    ///
    definitions::Command read_from_device(const definitions::Command& cmd);

   private:
    DeviceDispatcher();
    DeviceDispatcher(const DeviceDispatcher&) = delete;
    DeviceDispatcher& operator=(const DeviceDispatcher&) = delete;
    DeviceDispatcher(DeviceDispatcher&&) = delete;
    DeviceDispatcher& operator=(DeviceDispatcher&&) = delete;

    interfaces::IDevice* const* devices_;
    size_t device_count_;
};

}  // namespace modules
