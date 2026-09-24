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
    bool bath_missing_{false};                         // bath reading currently unavailable (logged once per outage)
    bool mix_missing_{false};                          // mix reading currently unavailable (logged once per outage)

    ///
    /// @brief Track whether a reading is available and log only when that changes (lost / back).
    ///
    /// @param available Whether this period's reading is valid.
    /// @param missing Per-sensor outage flag, updated in place.
    /// @param sensor Sensor name for the log message (Spanish, e.g. "camisa").
    ///
    static void track_reading(bool available, bool& missing, const char* sensor);
};

}  // namespace modules
