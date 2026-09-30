///
/// @file ShutdownState.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Shutdown state module.
/// @version 0.1
/// @date 15-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "IState.hpp"

namespace modules {

class ShutdownState final : public IState {
   public:
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event) override;
    definitions::SystemState tick(StateMachine* context) override;
    void on_enter(StateMachine* context) override;
    const char* name() const override;

   private:
    unsigned long enteredAtMs_ = 0;
    bool completed_ = false;
};

}  // namespace modules
