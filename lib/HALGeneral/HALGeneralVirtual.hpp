///
/// @file HALGeneralVirtual.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Virtual Hardware Abstraction Layer for testing
/// @version 0.1
/// @date 28-01-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>

#include "HALGeneral.hpp"

namespace modules {

///
/// @brief Virtual HALGeneral implementation for native testing
///
/// This class provides a virtual implementation of HALGeneral for testing
/// on native platforms without actual hardware.
///
class HALGeneralVirtual : public HALGeneral {
   public:
    // ==================== Time Functions ====================

    unsigned long millis() const override {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
        return static_cast<unsigned long>(duration.count());
    }

    unsigned long micros() const override {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
        return static_cast<unsigned long>(duration.count());
    }

    void delay(unsigned long ms) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void delay_microseconds(unsigned int us) override {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

    // ==================== Serial Communication ====================

    void serial_begin(unsigned long baud) override {
        std::cout << "[HALGeneralVirtual] Serial initialized at " << baud << " baud" << std::endl;
    }

    void serial_print(const std::string& str) override {
        std::cout << str;
    }

    void serial_println(const std::string& str) override {
        std::cout << str << std::endl;
    }

    void serial_print(int value) override {
        std::cout << value;
    }

    void serial_println(int value) override {
        std::cout << value << std::endl;
    }

    int serial_available() override {
        // Virtual implementation: no serial data available
        return 0;
    }

    int serial_read() override {
        // Virtual implementation: return -1 (no data)
        return -1;
    }

    // ==================== Digital I/O ====================

    void pin_mode(uint32_t pin, uint8_t mode) override {
        pin_modes_[pin] = mode;
        std::cout << "[HALGeneralVirtual] Pin " << pin << " set to mode " << static_cast<int>(mode)
                  << std::endl;
    }

    void digital_write(uint32_t pin, uint8_t value) override {
        digital_pins_[pin] = value;
        std::cout << "[HALGeneralVirtual] Digital write: Pin " << pin << " = " << static_cast<int>(value)
                  << std::endl;
    }

    int digital_read(uint32_t pin) override {
        auto it = digital_pins_.find(pin);
        if (it != digital_pins_.end()) {
            return it->second;
        }
        return PIN_LOW;  // Default to LOW
    }

    // ==================== Analog I/O ====================

    int analog_read(uint32_t pin) override {
        auto it = analog_pins_.find(pin);
        if (it != analog_pins_.end()) {
            return it->second;
        }
        return 0;  // Default to 0
    }

    void analog_write(uint32_t pin, int value) override {
        analog_pins_[pin] = value;
        std::cout << "[HALGeneralVirtual] Analog write: Pin " << pin << " = " << value << std::endl;
    }

    // ==================== System Functions ====================

    void system_restart() override {
        std::cout << "[HALGeneralVirtual] System restart requested (virtual - not actually restarting)" << std::endl;
    }

    uint32_t get_free_heap() override {
        // Return a virtual value (e.g., 100KB)
        return 102400;
    }

    std::string get_chip_id() override {
        return "VIRTUAL_CHIP_ID_12345678";
    }

    // ==================== Test Helper Methods ====================

    ///
    /// @brief Set a virtual analog value for testing
    ///
    /// @param pin Pin number
    /// @param value Analog value
    ///
    void set_analog_value(uint32_t pin, int value) {
        analog_pins_[pin] = value;
    }

    ///
    /// @brief Set a virtual digital value for testing
    ///
    /// @param pin Pin number
    /// @param value Digital value
    ///
    void set_digital_value(uint32_t pin, uint8_t value) {
        digital_pins_[pin] = value;
    }

    ///
    /// @brief Get the current pin mode
    ///
    /// @param pin Pin number
    /// @return uint8_t Pin mode
    ///
    uint8_t get_pin_mode(uint32_t pin) const {
        auto it = pin_modes_.find(pin);
        if (it != pin_modes_.end()) {
            return it->second;
        }
        return PIN_MODE_INPUT;
    }

   private:
    friend class HALGeneralManager;

    HALGeneralVirtual() : start_time_(std::chrono::steady_clock::now()) {}
    // Rule of Five
    ~HALGeneralVirtual() override = default;
    HALGeneralVirtual(const HALGeneralVirtual&) = delete;
    HALGeneralVirtual& operator=(const HALGeneralVirtual&) = delete;
    HALGeneralVirtual(HALGeneralVirtual&&) = delete;
    HALGeneralVirtual& operator=(HALGeneralVirtual&&) = delete;

    std::chrono::steady_clock::time_point start_time_;  ///< Start time point for timing functions
    std::map<uint32_t, uint8_t> pin_modes_;              ///< Pin modes map
    std::map<uint32_t, uint8_t> digital_pins_;           ///< Digital pin states map
    std::map<uint32_t, int> analog_pins_;                ///< Analog pin values map
};

}  // namespace modules
