///
/// @file HALGeneralArduino.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Arduino Hardware Abstraction Layer implementation
/// @version 0.1
/// @date 28-01-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "HALGeneral.hpp"

namespace modules {

///
/// @brief Arduino specific HALGeneral implementation
///
/// This class provides the concrete implementation of HALGeneral for
/// Arduino platforms, wrapping Arduino library calls.
///
class HALGeneralArduino : public interfaces::HALGeneral {
   public:
    // ==================== Time Functions ====================

    unsigned long millis() const override {
        return ::millis();
    }

    unsigned long micros() const override {
        return ::micros();
    }

    void delay(unsigned long ms) override {
        ::delay(ms);
    }

    void delay_microseconds(unsigned int us) override {
        ::delayMicroseconds(us);
    }

    // ==================== Serial Communication ====================

    void serial_begin(unsigned long baud) override {
        Serial.begin(baud);
    }

    void serial_print(const std::string& str) override {
        Serial.print(str.c_str());
    }

    void serial_println(const std::string& str) override {
        Serial.println(str.c_str());
    }

    void serial_print(int value) override {
        Serial.print(value);
    }

    void serial_println(int value) override {
        Serial.println(value);
    }

    int serial_available() override {
        return Serial.available();
    }

    int serial_read() override {
        return Serial.read();
    }

    // ==================== Logging ====================

    void log(const std::string& message) override {
        Serial.print("[LOG] ");
        Serial.println(message.c_str());
    }

    // ==================== Digital I/O ====================

    void pin_mode(uint32_t pin, uint8_t mode) override {
        uint8_t arduino_mode;
        switch (mode) {
            case PIN_MODE_INPUT:
                arduino_mode = INPUT;
                break;
            case PIN_MODE_OUTPUT:
                arduino_mode = OUTPUT;
                break;
            case PIN_MODE_INPUT_PULLUP:
                arduino_mode = INPUT_PULLUP;
                break;
            default:
                arduino_mode = INPUT;
                break;
        }
        pinMode((pin), arduino_mode);
    }

    void digital_write(uint32_t pin, uint8_t value) override {
        digitalWrite(pin, value);
    }

    int digital_read(uint32_t pin) override {
        return digitalRead(pin);
    }

    // ==================== Analog I/O ====================

    int analog_read(uint32_t pin) override {
        return analogRead(pin);
    }

    void analog_write(uint32_t pin, int value) override {
        analogWrite(pin, value);
    }

    // ==================== I2C Bus ====================

    uint32_t i2c_set_clock(uint32_t frequency_hz) override {
        // The PLC core already runs this bus for its I/O expanders; begin() then only attaches Wire to
        // it, which setClock() needs, and leaves the running configuration alone.
        if (!Wire.begin() || !Wire.setClock(frequency_hz)) {
            return 0;
        }
        return Wire.getClock();
    }

    // ==================== System Functions ====================

    void system_restart() override {
        ESP.restart();
    }

    uint32_t get_free_heap() override {
        return ESP.getFreeHeap();
    }

    std::string get_chip_id() override {
        uint64_t chipid = ESP.getEfuseMac();
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%04X%08X", (uint16_t) (chipid >> 32), (uint32_t) chipid);
        return std::string(buffer);
    }

   private:
    friend class HALGeneralManager;

    HALGeneralArduino() = default;
    // Rule of Five
    ~HALGeneralArduino() override = default;
    HALGeneralArduino(const HALGeneralArduino&) = delete;
    HALGeneralArduino& operator=(const HALGeneralArduino&) = delete;
    HALGeneralArduino(HALGeneralArduino&&) = delete;
    HALGeneralArduino& operator=(HALGeneralArduino&&) = delete;
};

}  // namespace modules
