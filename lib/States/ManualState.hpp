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
    void on_enter(StateMachine* context) override;
    const char* name() const override;
};

}  // namespace modules
