#include "SolenoidValve.hpp"

#include <algorithm>

#include "HALGeneral.hpp"

////////////////////////////////////// Public Methods //////////////////////////////////////

namespace modules {

SolenoidValve& SolenoidValve::instance() {
    static SolenoidValve instance;
    return instance;
}

void SolenoidValve::begin(const uint32_t* pins, size_t count) {
    valve_count_ = 0;

    if (pins == nullptr || count == 0) {
        hal_general().log("No valve pins provided for initialization.");
        return;
    }

    const uint8_t bounded_count =
        static_cast<uint8_t>(std::min(count, static_cast<size_t>(definitions::kMaxSolenoidValves)));
    for (uint8_t index = 0; index < bounded_count; ++index) {
        add(pins[index]);
    }
}

bool SolenoidValve::add(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (valve_count_ >= definitions::kMaxSolenoidValves) {
        hal_general().log("Failed to add valve pin: " + std::to_string(pin) + ". Capacity limit reached.");
        return false;  // Capacity limit reached or pin not already registered
    }

    if (index < definitions::kMaxSolenoidValves) {
        hal_general().log("Valve pin already registered: " + std::to_string(pin));
        return true;  // Pin already registered, consider it a success
    }

    valves_[valve_count_].pin = pin;
    valves_[valve_count_].status = false;
    hal_general().pin_mode(pin, PIN_MODE_OUTPUT);  // Valves are output devices
    // This depends whether the hardware is active HIGH or LOW. Assuming active HIGH for opening the valve, we set it to
    // LOW by default.

    close(pin);  // Ensure valve is closed by default
    ++valve_count_;
    return true;
}

bool SolenoidValve::remove(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Attempted to remove unregistered valve pin: " + std::to_string(pin));
        return false;  // Pin not found
    }

    close(pin);  // Ensure valve is closed before removal

    // Shift remaining valves to fill the gap
    for (uint8_t i = index; i < valve_count_ - 1; ++i) {
        valves_[i] = valves_[i + 1];
    }
    --valve_count_;
    return true;
}

bool SolenoidValve::open(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Attempted to open unregistered valve pin: " + std::to_string(pin));
        return false;  // Pin not found
    }

    hal_general().digital_write(pin, PIN_HIGH);
    valves_[static_cast<size_t>(index)].status = true;
    return true;
}

bool SolenoidValve::close(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Attempted to close unregistered valve pin: " + std::to_string(pin));
        return false;  // Pin not found
    }

    hal_general().digital_write(pin, PIN_LOW);
    valves_[static_cast<size_t>(index)].status = false;
    return true;
}

bool SolenoidValve::status(uint32_t pin) const {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Attempted to get status of unregistered valve pin: " + std::to_string(pin));
        return false;
    }

    return valves_[static_cast<size_t>(index)].status;
}

uint8_t SolenoidValve::count() const {
    return valve_count_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

SolenoidValve::SolenoidValve() : valves_({}), valve_count_(0) {}

uint8_t SolenoidValve::find_index(uint32_t pin) const {
    for (uint8_t index = 0; index < valve_count_; ++index) {
        if (valves_[index].pin == pin) {
            return index;
        }
    }

    return definitions::kMaxSolenoidValves;  // Return an out-of-range index to indicate not found
}

}  // namespace modules
