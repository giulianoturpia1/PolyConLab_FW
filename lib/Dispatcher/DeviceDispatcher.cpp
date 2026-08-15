#include "DeviceDispatcher.hpp"

#include <cmath>

#include "Helpers.hpp"
#include "IDevice.hpp"

namespace modules {

DeviceDispatcher& DeviceDispatcher::instance() {
    static DeviceDispatcher instance;
    return instance;
}

DeviceDispatcher::DeviceDispatcher() : devices_(nullptr), device_count_(0) {}

void DeviceDispatcher::set_devices(interfaces::IDevice* const* devices, size_t count) {
    devices_ = devices;
    device_count_ = count;
}

bool DeviceDispatcher::issue_to_device(const definitions::Command& cmd, uint32_t timeout_ms) {
    for (size_t i = 0; i < device_count_; ++i) {
        if (devices_[i] != nullptr && devices_[i]->accepts(cmd.type)) {
            return devices_[i]->issue_command(cmd, timeout_ms);
        }
    }
    return false;
}

bool DeviceDispatcher::enforce_to_device(const definitions::Command& cmd, uint32_t timeout_ms) {
    for (size_t i = 0; i < device_count_; ++i) {
        if (devices_[i] != nullptr && devices_[i]->accepts(cmd.type)) {
            return devices_[i]->enforce_command(cmd, timeout_ms);
        }
    }
    return false;
}

definitions::Command DeviceDispatcher::read_from_device(const definitions::Command& cmd) {
    for (size_t i = 0; i < device_count_; ++i) {
        if (devices_[i] != nullptr && devices_[i]->accepts(cmd.type)) {
            return devices_[i]->read(cmd);
        }
    }
    // No device handles this read: return a NaN sensor sentinel so float-sensor callers fail safe.
    return {cmd.type, utils::float_to_bits(NAN)};
}

}  // namespace modules
