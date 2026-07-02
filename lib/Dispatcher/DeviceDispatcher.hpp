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
    /// @param cmd        Command to dispatch.
    /// @param timeout_ms Queue send timeout in milliseconds.
    /// @return true if a device accepted and queued the command.
    ///
    bool forward_to_device(const definitions::Command& cmd, uint32_t timeout_ms);

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
