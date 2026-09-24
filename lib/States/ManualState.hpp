///
/// @file ManualState.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Manual state module.
/// @version 0.1
/// @date 15-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "Config.hpp"
#include "IState.hpp"

namespace modules {

class ManualState final : public IState {
   public:
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event) override;
    definitions::CommandOutcome handle_command(StateMachine* context, const definitions::Command& request,
                                               definitions::Command& out_response) override;
    definitions::SystemState tick(StateMachine* context) override;
    void on_enter(StateMachine* context) override;
    void on_exit(StateMachine* context) override;
    const char* name() const override;

   private:
    bool enabled_{false};                              // toggle: temperature control on/off
    float setpoint_{config::kManualDefaultSetpointC};  // inlet setpoint (°C), set by TEMP:SET:IN
    float deadband_{config::kManualDeadbandC};         // hysteresis half-width (°C), starts at the config default
    uint8_t bath_misses_{0};                           // consecutive control periods without a valid bath reading
    uint8_t mix_misses_{0};                            // consecutive control periods without a valid mix reading

    ///
    /// @brief Count one more control period without a valid reading and report whether it is now a sensor fault.
    ///
    /// @param misses Per-sensor consecutive-miss counter (saturates, reset by the caller on a valid reading).
    /// @return true once the reading has been missing for config::kManualSensorFaultPeriods periods.
    ///
    static bool reading_lost(uint8_t& misses);
};

}  // namespace modules
