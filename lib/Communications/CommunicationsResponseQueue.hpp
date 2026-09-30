///
/// @file CommunicationsResponseQueue.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Adapter class to allow Communications to be used as a command response sink.
///
/// Devices publish their responses into the StateMachine through that adapter, and the StateMachine publishes command outcomes out to
/// Communications through this one. Both sides of the mediator therefore speak the same interface,
/// and neither has to know what the other is.
///
/// @version 0.1
/// @date 16-09-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "ICommandResponseQueue.hpp"

namespace modules {

class CommunicationsResponseQueue final : public interfaces::ICommandResponseQueue {
   public:
    static CommunicationsResponseQueue& instance();

    ///
    /// @brief Publish a command outcome to the Communications delivery queue.
    ///
    /// @param response The command outcome to deliver, carrying the token of the command it answers.
    /// @param timeout The maximum time to wait for room in the queue, in milliseconds.
    /// @return true if the queue accepted the response.
    ///
    bool publish(const definitions::Command& response, uint32_t timeout) override;

   private:
    CommunicationsResponseQueue() = default;
    // Rule of Five
    ~CommunicationsResponseQueue() override = default;
    CommunicationsResponseQueue(const CommunicationsResponseQueue&) = delete;
    CommunicationsResponseQueue& operator=(const CommunicationsResponseQueue&) = delete;
    CommunicationsResponseQueue(CommunicationsResponseQueue&&) = delete;
    CommunicationsResponseQueue& operator=(CommunicationsResponseQueue&&) = delete;
};

}  // namespace modules
