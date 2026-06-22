#include "DeviceDispatcher.hpp"

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

bool DeviceDispatcher::forward_to_device(const definitions::Command& cmd, uint32_t timeout_ms) {
    for (size_t i = 0; i < device_count_; ++i) {
        if (devices_[i] != nullptr && devices_[i]->accepts(cmd.type)) {
            return devices_[i]->queue_command(cmd, timeout_ms);
        }
    }
    return false;
}

}  // namespace modules
