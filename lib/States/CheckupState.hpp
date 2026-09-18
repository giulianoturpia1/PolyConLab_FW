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
    /// @brief Allow read-only sensor queries and SYST:REINIT:* re-init commands during CHECKUP
    ///        so the SCADA can probe each device (deciding EVT:SENSORS OK vs EVT:FAULT
    ///        RECOVERABLE) and recover a failed sensor.  Actuator commands are rejected.
    definitions::CommandOutcome handle_command(StateMachine* context, const definitions::Command& request,
                                               definitions::Command& out_response) override;
    void on_enter(StateMachine* context) override;
    const char* name() const override;

   private:
    bool scadaOk_ = false;
    bool sensorOk_ = false;
};

}  // namespace modules
