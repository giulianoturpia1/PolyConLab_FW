///
/// @file IDevice.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Generic device interface for command dispatch in the StateMachine routing layer.
/// @version 0.1
/// @date 18-06-2026
///
/// @copyright Copyright (c) 2026
///

#pragma once

#include <stddef.h>

#include "Definitions.hpp"

namespace interfaces {

class IDevice {
   public:
    virtual ~IDevice() = default;

    ///
    /// @brief Return true if this device handles the given command type.
    ///
    /// @param type The CommandType to check.
    /// @return true if this device accepts and can process the command.
    ///
    virtual bool accepts(definitions::CommandType type) const = 0;

    ///
    /// @brief Dispatch a full Command to the device — the single command entry point.
    ///
    /// This is the one and only way an external, confirmable command reaches a device.
    /// Concrete devices decide the execution model behind the polymorphism: a SyncDevice
    /// runs @p cmd inline on the caller's task, while an AsyncDevice enqueues it for its
    /// dedicated task. In both cases the device publishes a response to its
    /// configured sink, so the StateMachine confirmation pipeline is identical regardless
    /// of device type. Callers never need to know which kind of device they are talking to.
    ///
    /// The device is responsible for extracting any parameter it needs from cmd.extra
    /// (e.g. valve pin for kValveOpen) or from its own internal command-to-address mapping
    /// (e.g. kTempInRead -> I2C address in Thermocouple).
    ///
    /// @note Autonomous control-loop and safe-output actuations must NOT go through this
    ///       method: they are not client-originated commands and would inject spurious
    ///       confirmations into the StateMachine response queue. Such internal logic uses
    ///       enforce_command() instead.
    ///
    /// @param cmd Command to dispatch.
    /// @param timeout_ms Queue send timeout in milliseconds (ignored by synchronous devices).
    /// @return true if the command was accepted and queued/executed.
    ///
    virtual bool issue_command(const definitions::Command& cmd, uint32_t timeout_ms) = 0;

    ///
    /// @brief Dispatch a command WITHOUT publishing a response — the enforced entry point.
    ///
    /// Identical dispatch semantics to issue_command() (same polymorphism: inline for a
    /// SyncDevice, queued for an AsyncDevice), except the device does not publish a
    /// response to its sink. It is the path for autonomous, fire-and-forget actuations
    /// that are not client-originated commands — control-loop and safe-output actions —
    /// so they never inject a spurious confirmation into the StateMachine response queue.
    ///
    /// Callers reach this through DeviceDispatcher::enforce_to_device(), so they remain
    /// unaware of the concrete device type, exactly as with the confirmable command path.
    ///
    /// @param cmd Command to enforce.
    /// @param timeout_ms Queue send timeout in milliseconds (ignored by synchronous devices).
    /// @return true if the command was accepted and queued/executed.
    ///
    virtual bool enforce_command(const definitions::Command& cmd, uint32_t timeout_ms) = 0;

    ///
    /// @brief Execute a read synchronously and return its response — the inline query entry point.
    ///
    /// Runs the operation on the CALLER's task and returns the device's response immediately,
    /// without publishing a report. This is the path for internal consumers that need a value
    /// now (e.g. the control loop reading temperature for a hysteresis decision), as opposed to
    /// the client read path (kTempInRead via issue_command, answered later on the response queue).
    ///
    /// The meaning of the returned Command::extra is defined by the device and the command type
    /// (e.g. IEEE-754 temperature bits for a thermocouple read); the caller decodes it accordingly.
    /// A failed read is reported in-band through that same payload (e.g. NaN bits for a sensor
    /// fault), so there is no separate status flag — the device gives extra its meaning.
    ///
    /// @warning Runs execute_command() inline, off the device's own task, so a device may be read this
    ///          way only if its execute_command() is internally thread-safe against that task (the
    ///          Thermocouple serializes bus access with a mutex). Devices that cannot guarantee
    ///          this must not be read synchronously.
    ///
    /// @param cmd Read command to execute.
    /// @return definitions::Command The device response, with the reading packed in extra.
    ///
    virtual definitions::Command read(const definitions::Command& cmd) = 0;

   protected:
    IDevice() = default;
    IDevice(const IDevice&) = delete;
    IDevice& operator=(const IDevice&) = delete;
    IDevice(IDevice&&) = delete;
    IDevice& operator=(IDevice&&) = delete;
};

}  // namespace interfaces

namespace modules {
using interfaces::IDevice;
}  // namespace modules
