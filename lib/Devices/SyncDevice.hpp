///
/// @file SyncDevice.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Base class for devices actuated synchronously on the caller's task (no task, no queue).
/// @version 0.1
/// @date 14-08-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <stdint.h>

#include "Config.hpp"
#include "Definitions.hpp"
#include "ICommandResponseQueue.hpp"
#include "IDevice.hpp"

namespace modules {

///
/// @brief Base for devices whose commands are bounded, non-blocking operations
/// (e.g. a single GPIO write) that run inline on the caller's task.
///
/// A SyncDevice owns no FreeRTOS task and no queue: the dispatching task (the
/// StateMachine on Core 0) is the sole writer of the device, so there is no race
/// to serialize. Subclasses implement execute_command(), reached through the three
/// IDevice entry points (Template Method):
///
/// + issue_command() runs the command inline and publishes the response to the
///   configured sink, so the confirmation/client-acknowledgement pipeline is identical
///   to an AsyncDevice. This is the path for external, confirmable commands.
/// + enforce_command() runs the command inline WITHOUT publishing, for autonomous
///   control-loop and safe-output actuations that must not inject a spurious
///   confirmation into the StateMachine response queue.
/// + read() runs the command inline and returns the response to the caller, for
///   synchronous internal reads.
///
/// All three are reached polymorphically through the DeviceDispatcher, so callers stay
/// unaware of the concrete device type.
///
/// create_task() and set_response_queue() mirror the AsyncDevice lifecycle API so
/// App wiring stays uniform; create_task() is a no-op because a SyncDevice has no task.
///
class SyncDevice : public interfaces::IDevice {
   public:
    ///
    /// @brief No-op: a synchronous device has no dedicated task. Kept for a uniform
    /// device lifecycle API.
    ///
    /// @return true always.
    ///
    bool create_task() {
        return true;
    }

    ///
    /// @brief Set the sink used to publish command responses. nullptr disables publishing.
    ///
    void set_response_queue(interfaces::ICommandResponseQueue* queue) {
        response_queue_ = queue;
    }

    ///
    /// @copydoc interfaces::IDevice::issue_command
    /// @note Executes synchronously on the caller's task (@p timeout_ms is ignored) and
    ///       publishes the response, preserving the downstream confirmation pipeline.
    ///
    bool issue_command(const definitions::Command& cmd, uint32_t /*timeout_ms*/) override {
        definitions::Command response;
        if (!execute_command(cmd, response)) {
            return false;
        }
        publish_response(response);
        return true;
    }

    ///
    /// @copydoc interfaces::IDevice::enforce_command
    /// @note Executes synchronously on the caller's task (@p timeout_ms is ignored) and does
    ///       NOT publish a response, so autonomous actuations stay off the confirmation pipeline.
    ///
    bool enforce_command(const definitions::Command& cmd, uint32_t /*timeout_ms*/) override {
        definitions::Command response;
        return execute_command(cmd, response);
    }

    ///
    /// @copydoc interfaces::IDevice::read
    /// @note Runs inline on the caller's task and returns the response without publishing it.
    ///
    definitions::Command read(const definitions::Command& cmd) override {
        definitions::Command out = cmd;
        execute_command(cmd, out);
        return out;
    }

   protected:
    SyncDevice() : response_queue_(nullptr) {}

    ///
    /// @brief Execute a command against the hardware and fill @p out_response.
    ///
    /// @param cmd Command to run.
    /// @param out_response Response payload (command type + result bits).
    /// @return true if the command was handled; false if unsupported.
    ///
    virtual bool execute_command(const definitions::Command& cmd, definitions::Command& out_response) = 0;

    ///
    /// @brief Publish a response through the configured sink.
    ///
    /// @param response Response payload.
    /// @return true if the sink accepted the response.
    ///
    bool publish_response(const definitions::Command& response) {
        if (response_queue_ == nullptr) {
            return false;
        }
        // Non-blocking: this runs on the StateMachine task, which is the sole drainer
        // of the response queue — blocking here would stall its own consumer.
        return response_queue_->publish(response, config::kSyncResponsePublishTimeoutMs);
    }

   private:
    interfaces::ICommandResponseQueue* response_queue_;  ///< Response sink (may be nullptr).
};

}  // namespace modules
