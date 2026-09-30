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
    definitions::SystemState handle_event(StateMachine* context, definitions::SystemEvent event) override;
    /// @brief Accept only SYST:REINIT:* sensor re-init commands during SOFT ERROR so the operator
    ///        can recover a failed sensor in place; all other commands are rejected.
    definitions::CommandOutcome handle_command(StateMachine* context, const definitions::Command& request,
                                               definitions::Command& out_response) override;
    void on_enter(StateMachine* context) override;
    const char* name() const override;

   private:
    bool retryRequested_ = false;
    bool ackRequested_ = false;
    bool conditionCleared_ = false;
};

}  // namespace modules
