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
/// to serialize. Subclasses implement execute_command(); both entry points funnel
/// through it (Template Method):
///
/// + queue_command() keeps the existing dispatch contract — it runs the command
///   inline and publishes the response to the configured sink, so the downstream
///   confirmation and client-acknowledgement pipeline is unchanged from the old
///   task-based devices.
/// + execute() runs the command and returns its outcome and response to the
///   caller without publishing, for callers that want an immediate, self-checked
///   action (e.g. safety shutoffs).
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
    /// @copydoc interfaces::IDevice::execute
    /// @note Runs inline and returns the response without publishing it.
    ///
    bool execute(const definitions::Command& cmd, definitions::Command& out_response) override {
        return execute_command(cmd, out_response);
    }

    ///
    /// @copydoc interfaces::IDevice::queue_command
    /// @note Executes synchronously on the caller's task (@p timeout_ms is ignored) and
    ///       publishes the response, preserving the downstream confirmation pipeline.
    ///
    bool queue_command(const definitions::Command& cmd, uint32_t /*timeout_ms*/) override {
        definitions::Command response;
        if (!execute_command(cmd, response)) {
            return false;
        }
        publish_response(response);
        return true;
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
