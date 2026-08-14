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

#include "IState.hpp"

namespace modules {

class ManualState final : public IState {
   public:
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event) override;
    bool dispatch_command(StateMachine* context, const definitions::Command& request) override;
    definitions::SystemState tick(StateMachine* context) override;
    void on_enter(StateMachine* context) override;
    void on_exit(StateMachine* context) override;
    const char* name() const override;

   private:
    bool enabled_{false};    // toggle: temperature control on/off
    float setpoint_{20.0f};  // default 20.0°C
    float deadband_{0.5f};   // ±0.5°C hysteresis
};

}  // namespace modules
