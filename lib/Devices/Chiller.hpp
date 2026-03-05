///
/// @file Chiller.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Chiller control module for the reactor system.
/// @version 0.1
/// @date 04-03-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <stdint.h>

namespace modules {

///
/// @brief Singleton chiller controller. Handles basic on/off and status logic.
///
class Chiller {
   public:
    ///
    /// @brief Get the singleton instance of the Chiller controller.
    ///
    /// @return Chiller&
    ///
    static Chiller& instance();

    ///
    /// @brief Initialize the chiller controller with the pin.
    ///
    /// @param pin Pin for chiller control.
    ///
    void begin(uint32_t pin);

    ///
    /// @brief Turn the chiller on.
    ///
    void on();

    ///
    /// @brief Turn the chiller off.
    ///
    void off();

    ///
    /// @brief Get the chiller status.
    ///
    /// @return true if chiller is ON, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the chiller pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

   private:
    Chiller();
    // Rule of Five
    ~Chiller() = default;
    Chiller(const Chiller&) = delete;
    Chiller& operator=(const Chiller&) = delete;
    Chiller(Chiller&&) = delete;
    Chiller& operator=(Chiller&&) = delete;

    uint32_t pin_;
    bool status_;
};

}  // namespace modules
