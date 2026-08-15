///
///@file Flowmeter.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Flowmeter sensor module for the reactor system.
///@version 0.1
///@date 08-04-2026
///
///@copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "Definitions.hpp"
#include "SyncDevice.hpp"

namespace modules {

///
/// @brief Singleton Flowmeter controller. It manages the flowmeter sensor and its readings.
/// The flowmeter measures flow by pulses managed by an interrupt service routine, and calculates flow rate in liters per minute.
///
/// A SyncDevice: the pulse counting runs autonomously in an ISR and a hardware
/// timer set up in begin(); a kFlowRead command just reads the latest snapshot
/// inline on the caller's task (no dedicated task or queue).
///
class Flowmeter : public SyncDevice {
   public:
    ///
    /// @brief Get the singleton instance of the Flowmeter controller.
    ///
    /// @return Flowmeter&
    ///
    static Flowmeter& instance();

    ///
    /// @brief Initialize flowmeter with the sensor pin and necessary hardware setup. Attach interrupts and timer.
    /// If the flowmeter is already running, this is a no-op and returns true.
    ///
    /// @param pinFlow_ Device pin.
    /// @param pulsesPerLiter Calibration constant: sensor pulses per liter. Defaults to 450.0.
    ///
    bool begin(uint32_t pinFlow_, float pulsesPerLiter = definitions::kFlowmeterPulsesPerLiter);

    ///
    /// @brief Stop the flowmeter. Detach interrupts and end timer.
    ///
    void stop();

    ///
    /// @brief Read flow rate in liters per minute. Returns the latest calculated flow rate based on pulse counts in the last second.
    ///
    /// @return Float Flow rate in liters per minute, or NaN if unavailable.
    ///
    float readFlow();

    ///
    /// @brief Get the flowmeter hardware status.
    ///
    /// @return true if the interrupt and timer are active, false otherwise.
    ///
    bool status() const;

    ///
    /// @brief Get the flow sensor input pin.
    ///
    /// @return uint32_t
    ///
    uint32_t pin() const;

    /// @copydoc interfaces::IDevice::accepts
    bool accepts(definitions::CommandType type) const override;

   protected:
    ///
    /// @copydoc modules::SyncDevice::execute_command
    /// @note Only kFlowRead is handled; the flow rate is packed as IEEE-754 bits in the response's extra.
    ///
    bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) override;

   private:
    Flowmeter();

    // Rule of Five
    ~Flowmeter() = default;
    Flowmeter(const Flowmeter&) = delete;
    Flowmeter& operator=(const Flowmeter&) = delete;
    Flowmeter(Flowmeter&&) = delete;
    Flowmeter& operator=(Flowmeter&&) = delete;

    uint32_t pin_;          ///< Input pin for the flow sensor pulse signal
    bool status_;           ///< True when the hardware interrupt and timer are active
    float pulsesPerLiter_;  ///< Calibration constant: sensor pulses per liter
};

}  // namespace modules
