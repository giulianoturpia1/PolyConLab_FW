///
/// @file CheckupState.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Checkup state module.
/// @version 0.1
/// @date 15-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "IState.hpp"

namespace modules {

class CheckupState final : public IState {
   public:
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event) override;
    void on_enter(StateMachine* context) override;
    const char* name() const override;

   private:
    bool scadaOk_ = false;
    bool sensorOk_ = false;

    void apply_safe_outputs();
};

}  // namespace modules
