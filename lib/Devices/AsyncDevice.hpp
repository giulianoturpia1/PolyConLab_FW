///
/// @file AsyncDevice.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Active Object base: a device with its own FreeRTOS task and command queue.
/// @version 0.1
/// @date 14-08-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdint.h>

#include "Config.hpp"
#include "Definitions.hpp"
#include "ICommandResponseQueue.hpp"
#include "IDevice.hpp"
#include "Logger.hpp"

namespace modules {

///
/// @brief Base for devices whose commands may block or must be serialized on a
/// dedicated context (e.g. blocking I2C reads on a shared bus).
///
/// Implements the Active Object pattern once, so subclasses only supply their
/// command logic in execute_command() (Template Method). The base owns the queue and
/// the task: create_task() spins up a task pinned to the configured core that
/// drains full definitions::Command items and, for each, calls execute_command() then
/// publishes the response to the configured sink.
///
/// The queue stores whole Command values (sizeof(definitions::Command)), so
/// cmd.extra always round-trips — there is no per-device truncation to work around.
///
class AsyncDevice : public interfaces::IDevice {
   public:
    ///
    /// @brief Create the dedicated FreeRTOS task and command queue. Idempotent.
    ///
    /// @return true if the task/queue exist after the call.
    ///
    bool create_task() {
        if (queue_ != nullptr) {
            return true;
        }

        queue_ = xQueueCreate(config::kDeviceQueueLength, sizeof(QueuedCommand));
        if (queue_ == nullptr) {
            TaggedLogger(task_name_).error("No se pudo crear la cola del dispositivo.");
            return false;
        }

        if (xTaskCreatePinnedToCore(task_entry, task_name_, stack_size_, this, priority_, nullptr, core_id_) != pdPASS) {
            TaggedLogger(task_name_).error("No se pudo crear la tarea del dispositivo.");
            vQueueDelete(queue_);
            queue_ = nullptr;
            return false;
        }

        return true;
    }

    ///
    /// @brief Set the sink used to publish command responses. nullptr disables publishing.
    ///
    void set_response_queue(interfaces::ICommandResponseQueue* queue) {
        response_queue_ = queue;
    }

    /// @copydoc interfaces::IDevice::issue_command
    bool issue_command(const definitions::Command& cmd, uint32_t timeout_ms) override {
        if (queue_ == nullptr) {
            return false;
        }
        const QueuedCommand item = {cmd, true};
        return xQueueSend(queue_, &item, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
    }

    /// @copydoc interfaces::IDevice::enforce_command
    bool enforce_command(const definitions::Command& cmd, uint32_t timeout_ms) override {
        if (queue_ == nullptr) {
            return false;
        }
        const QueuedCommand item = {cmd, false};
        return xQueueSend(queue_, &item, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
    }

    ///
    /// @copydoc interfaces::IDevice::read
    /// @note Runs execute_command() inline on the caller's task (bypassing the queue) and returns the
    ///       response; relies on execute_command() being internally thread-safe against the device task.
    ///
    definitions::Command read(const definitions::Command& cmd) override {
        definitions::Command out = cmd;
        execute_command(cmd, out);
        return out;
    }

   protected:
    ///
    /// @brief Configure the task identity used by create_task().
    ///
    /// @param task_name FreeRTOS task name (also used as the log tag).
    /// @param stack_size Task stack size in FreeRTOS words.
    /// @param priority Task priority.
    /// @param core_id Core the task is pinned to.
    ///
    AsyncDevice(const char* task_name, uint32_t stack_size, UBaseType_t priority, BaseType_t core_id)
        : task_name_(task_name),
          stack_size_(stack_size),
          priority_(priority),
          core_id_(core_id),
          queue_(nullptr),
          response_queue_(nullptr) {}

    ///
    /// @brief Execute a command against the hardware and fill @p out_response.
    ///
    /// Runs on the device task. Return false to suppress the response (e.g. unsupported command).
    ///
    /// @param cmd Command drained from the queue.
    /// @param out_response Response payload to publish on success.
    /// @return true if a response should be published.
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
        return response_queue_->publish(response, config::kDeviceResponseTimeoutMs);
    }

   private:
    ///
    /// @brief Queue element: a command plus whether its response should be published.
    ///
    /// enforce_command() enqueues with publish == false so the task suppresses the report,
    /// keeping autonomous actuations off the StateMachine confirmation pipeline.
    ///
    struct QueuedCommand {
        definitions::Command cmd;  ///< Command to execute on the device task.
        bool publish;              ///< Publish the response (true) or suppress it (false).
    };

    ///
    /// @brief Task loop: drain the queue, run execute_command(), publish the response when requested.
    ///
    static void task_entry(void* param) {
        AsyncDevice* self = static_cast<AsyncDevice*>(param);
        QueuedCommand item;
        definitions::Command response;

        while (self->queue_ != nullptr) {
            if (xQueueReceive(self->queue_, &item, config::kDeviceWaitTicks) == pdPASS) {
                if (self->execute_command(item.cmd, response) && item.publish) {
                    self->publish_response(response);
                }
            }
        }
    }

    const char* task_name_;                              ///< Task name and log tag.
    uint32_t stack_size_;                                ///< Task stack size in words.
    UBaseType_t priority_;                               ///< Task priority.
    BaseType_t core_id_;                                 ///< Pinned core.
    QueueHandle_t queue_;                                ///< Command queue handle.
    interfaces::ICommandResponseQueue* response_queue_;  ///< Response sink (may be nullptr).
};

}  // namespace modules
