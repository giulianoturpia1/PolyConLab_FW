///
/// @file test_config.hpp
/// @brief Shared test configuration and platform abstraction for doctest
/// @details Provides Arduino stubs for native builds and uses real Arduino APIs on target hardware.
///
#pragma once

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <chrono>
#include <cstdint>
#include <thread>

#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef INPUT
#define INPUT 0
#endif

// Track a start time for millis()/micros()
inline std::chrono::steady_clock::time_point& _test_start_time() {
    static auto start = std::chrono::steady_clock::now();
    return start;
}

inline unsigned long millis() {
    const auto now = std::chrono::steady_clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - _test_start_time()).count());
}

inline unsigned long micros() {
    const auto now = std::chrono::steady_clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - _test_start_time()).count());
}

inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void delayMicroseconds(unsigned int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

// Minimal stubs for pin operations when compiling native tests
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) {
    return LOW;
}
inline void analogWrite(int, int) {}
#endif
