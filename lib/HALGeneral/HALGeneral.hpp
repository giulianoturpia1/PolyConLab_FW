///
/// @file HALGeneral.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief General Hardware Abstraction Layer interface for PolyConLab system
/// @version 0.1
/// @date 28-01-2026
///
/// @copyright Copyright (c) 2026
///
/// @details This General HAL provides a platform-independent interface to abstract
///          Arduino specific calls for basic I/O operations, enabling code
///          portability and easier testing on different platforms.
///

#pragma once

#include <cstdint>
#include <string>

namespace interfaces {

///
/// @brief General Hardware Abstraction Layer interface
///
/// This class defines the interface for basic hardware operations like
/// GPIO, time, serial, and analog I/O. Concrete implementations should
/// be provided for different platforms (Arduino, native, etc.) Purely virtual.
///
class HALGeneral {
   public:
    virtual ~HALGeneral() = default;

    // ==================== Time Functions ====================

    ///
    /// @brief Get current time in milliseconds since program start
    ///
    /// @return unsigned long Current time in milliseconds
    ///
    virtual unsigned long millis() const = 0;

    ///
    /// @brief Get current time in microseconds since program start
    ///
    /// @return unsigned long Current time in microseconds
    ///
    virtual unsigned long micros() const = 0;

    ///
    /// @brief Delay execution for specified milliseconds
    ///
    /// @param ms Milliseconds to delay
    ///
    virtual void delay(unsigned long ms) = 0;

    ///
    /// @brief Delay execution for specified microseconds
    ///
    /// @param us Microseconds to delay
    ///
    virtual void delay_microseconds(unsigned int us) = 0;

    // ==================== Serial Communication ====================

    ///
    /// @brief Initialize serial communication
    ///
    /// @param baud Baud rate for serial communication
    ///
    virtual void serial_begin(unsigned long baud) = 0;

    ///
    /// @brief Print string to serial
    ///
    /// @param str String to print
    ///
    virtual void serial_print(const std::string& str) = 0;

    ///
    /// @brief Print string with newline to serial
    ///
    /// @param str String to print
    ///
    virtual void serial_println(const std::string& str) = 0;

    ///
    /// @brief Print integer to serial
    ///
    /// @param value Integer value to print
    ///
    virtual void serial_print(int value) = 0;

    ///
    /// @brief Print integer with newline to serial
    ///
    /// @param value Integer value to print
    ///
    virtual void serial_println(int value) = 0;

    ///
    /// @brief Check if serial data is available
    ///
    /// @return int Number of bytes available
    ///
    virtual int serial_available() = 0;

    ///
    /// @brief Read a byte from serial
    ///
    /// @return int Byte read, or -1 if no data available
    ///
    virtual int serial_read() = 0;

    ///
    /// @brief Read one line from serial, stopping at '\n' or when no bytes remain
    ///
    /// @return std::string Accumulated characters, excluding the terminating '\n'
    ///
    virtual std::string serial_read_line() {
        std::string line;
        while (serial_available() > 0) {
            int ch = serial_read();
            if (ch < 0)
                break;
            if (static_cast<char>(ch) == '\n')
                break;
            line += static_cast<char>(ch);
        }
        return line;
    }

    // ==================== Logging ====================

    ///
    /// @brief Log a message (platform-independent logging)
    ///
    /// @param message Message to log
    ///
    virtual void log(const std::string& message) = 0;

    // ==================== Digital I/O ====================

    ///
    /// @brief Set pin mode
    ///
    /// @param pin Pin number
    /// @param mode Mode (INPUT, OUTPUT, INPUT_PULLUP)
    ///
    virtual void pin_mode(uint32_t pin, uint8_t mode) = 0;

    ///
    /// @brief Write digital value to pin
    ///
    /// @param pin Pin number
    /// @param value HIGH or LOW
    ///
    virtual void digital_write(uint32_t pin, uint8_t value) = 0;

    ///
    /// @brief Read digital value from pin
    ///
    /// @param pin Pin number
    /// @return int Pin value (HIGH or LOW)
    ///
    virtual int digital_read(uint32_t pin) = 0;

    // ==================== Analog I/O ====================

    ///
    /// @brief Read analog value from pin
    ///
    /// @param pin Pin number
    /// @return int Analog value (0-1023 or 0-4095 depending on platform)
    ///
    virtual int analog_read(uint32_t pin) = 0;

    ///
    /// @brief Write analog value to pin (PWM)
    ///
    /// @param pin Pin number
    /// @param value Analog value (0-255)
    ///
    virtual void analog_write(uint32_t pin, int value) = 0;

    // ==================== I2C Bus ====================

    ///
    /// @brief Set the clock of the shared I2C bus, starting the bus first if it is not running.
    ///
    /// @param frequency_hz Requested bus clock in Hz
    /// @return uint32_t Clock the bus actually runs at, in Hz (0 if it could not be set)
    ///
    virtual uint32_t i2c_set_clock(uint32_t frequency_hz) = 0;

    // ==================== System Functions ====================

    ///
    /// @brief Restart/reset the system
    ///
    virtual void system_restart() = 0;

    ///
    /// @brief Get free heap memory
    ///
    /// @return uint32_t Free heap size in bytes
    ///
    virtual uint32_t get_free_heap() = 0;

    ///
    /// @brief Get chip/device ID
    ///
    /// @return std::string Device ID as string
    ///
    virtual std::string get_chip_id() = 0;
};

}  // namespace interfaces

namespace modules {

using interfaces::HALGeneral;

// Pin mode constants
constexpr uint8_t PIN_MODE_INPUT = 0x00;
constexpr uint8_t PIN_MODE_OUTPUT = 0x01;
constexpr uint8_t PIN_MODE_INPUT_PULLUP = 0x02;

// Digital values
constexpr uint8_t PIN_LOW = 0x00;
constexpr uint8_t PIN_HIGH = 0x01;

///
/// @brief General Hardware Abstraction Layer Manager
///
/// Manages the global HALGeneral instance using a singleton pattern.
/// Automatically detects and initializes the correct HAL implementation.
/// Implemented as a singleton class.
///
class HALGeneralManager {
   public:
    ///
    /// @brief Get the singleton instance of the General HAL Manager
    ///
    /// @return HALGeneralManager&
    ///
    static HALGeneralManager& instance();

    ///
    /// @brief Get the HALGeneral instance
    ///
    /// @return HALGeneral& Reference to the active HAL implementation
    ///
    interfaces::HALGeneral& get();

    ///
    /// @brief Set a custom HALGeneral implementation (for dependency injection)
    ///
    /// @param hal Pointer to custom HAL implementation
    ///
    void set(interfaces::HALGeneral* hal);

   private:
    HALGeneralManager();
    // Rule of Five
    ~HALGeneralManager() = default;
    HALGeneralManager(const HALGeneralManager&) = delete;
    HALGeneralManager& operator=(const HALGeneralManager&) = delete;
    HALGeneralManager(HALGeneralManager&&) = delete;
    HALGeneralManager& operator=(HALGeneralManager&&) = delete;

    interfaces::HALGeneral* hal_;  ///< Pointer to the active HAL implementation (initialized in constructor)
};

///
/// @brief Convenience function to get the HALGeneral instance
///
/// @return HALGeneral& Reference to the active HAL implementation
///
inline interfaces::HALGeneral& hal_general() {
    return HALGeneralManager::instance().get();
}

}  // namespace modules
