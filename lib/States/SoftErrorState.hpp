///
/// @file SoftErrorState.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Soft error state module.
/// @version 0.1
/// @date 15-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "IState.hpp"

namespace modules {

class SoftErrorState final : public IState {
   public:
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event);
    void on_enter(StateMachine* context);
    const char* name() const;
    definitions::SystemState tick(StateMachine* context);

   private:
    bool retryRequested_ = false;
    bool ackRequested_ = false;
    bool conditionCleared_ = false;

    void apply_safe_outputs();
};

}  // namespace modules
