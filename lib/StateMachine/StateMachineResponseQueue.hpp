///
/// @file StateMachineResponseQueue.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Adapter class to allow the StateMachine to be used as a command response sink for modules.
///
/// This allows modules to publish command execution responses directly to the StateMachine's response queue,
/// which decouples the StateMachine from other parts which do not need to know its implementation details.
/// @version 0.1
/// @date 17-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "ICommandResponseQueue.hpp"

namespace modules {

class StateMachineResponseQueue final : public interfaces::ICommandResponseQueue {
   public:
    static StateMachineResponseQueue& instance();

    ///
    /// @brief Publish a command response to the StateMachine's response queue.
    ///
    /// @param response The command response to publish, containing the command type and any relevant data (e.g., pin number).
    /// @param timeout The maximum time to wait for a command to be executed and the response to be published, in milliseconds.
    /// @return true
    /// @return false
    ///
    bool publish(const definitions::Command& response, uint32_t timeout) override;

   private:
    StateMachineResponseQueue() = default;
    // Rule of Five
    ~StateMachineResponseQueue() override = default;
    StateMachineResponseQueue(const StateMachineResponseQueue&) = delete;
    StateMachineResponseQueue& operator=(const StateMachineResponseQueue&) = delete;
    StateMachineResponseQueue(StateMachineResponseQueue&&) = delete;
    StateMachineResponseQueue& operator=(StateMachineResponseQueue&&) = delete;
};

}  // namespace modules
