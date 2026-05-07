///
/// @file ICommandResponseQueue.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Interface for command response queues.
/// This allows modules to publish command execution responses to a sink, which can be used for feedback confirmation and to trigger further
/// actions or transitions if needed.
/// @version 0.1
/// @date 17-04-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include "Definitions.hpp"

namespace interfaces {

class ICommandResponseQueue {
   public:
    virtual ~ICommandResponseQueue() = default;

    ///
    /// @brief Publish a command response to a module's command response queue.
    ///
    /// @param response The command response to publish, containing the command type and any relevant data (e.g., pin number).
    /// @param timeout The maximum time to wait for a command to be executed and the response to be published, in milliseconds.
    /// @return true
    /// @return false
    ///
    virtual bool publish(const definitions::CommandRequest& response, uint32_t timeout) = 0;
};

}  // namespace interfaces
