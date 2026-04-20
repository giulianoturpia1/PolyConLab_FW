///
/// @file CalibrationState.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Calibration state module.
/// @version 0.1
/// @date 15-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "IState.hpp"

namespace modules {

class CalibrationState final : public IState {
   public:
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event);
    bool dispatch_command(StateMachine* context, const definitions::CommandRequest& request);
    void on_enter(StateMachine* context);
    const char* name() const;
    definitions::SystemState tick(StateMachine* context);

   private:
    void apply_safe_outputs();
};

}  // namespace modules
