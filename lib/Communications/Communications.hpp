///
/// @file Communications.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Communications module that isolates command handling from the application layer.
/// @version 0.2
/// @date 26-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "Authorizer.hpp"
#include "Definitions.hpp"
#include "TcpServer.hpp"

namespace modules {

///
/// @brief Communications manager that routes SCPI messages from WiFi and Serial transports.
/// Implemented as a singleton.
///
/// Each incoming message carries a write-back to its sender (the WiFi client's callback, or
/// serial_send_ for Serial); the one for the message being dispatched is held in active_responder_,
/// and respond_now() replies through it. Commands needing a StateMachine round-trip capture it into
/// pending_responder_ before queuing, so handle_response() later replies to the original client even
/// if another message was served meanwhile. Inline commands (SYST:LOG*, invalid params, unknown)
/// answer immediately via respond_now() without pending_responder_.
///
/// Authorization is delegated to authorizer_ (see Authorizer): Serial is always privileged, WiFi
/// actuation is gated to the first client that claims the operator role, and poll_network() frees the
/// role on disconnect via Authorizer::sync_operator_presence().
///
/// @note Closing the Serial connection (COM port close or USB unplug) pulses the USB-serial
/// bridge's DTR/RTS auto-reset (EN), rebooting the whole board and dropping WiFi too. See PCL-69.
///
class Communications {
   public:
    ///
    /// @brief Get the singleton instance of Communications.
    ///
    static Communications& instance();

    ///
    /// @brief Initialize the communications layer: join WiFi, start the TCP server and wire the
    /// Serial response path. WiFi credentials and static IP are read from @c config::.
    ///
    /// @param serial_send Callable that writes a SCPI response string to the Serial port.
    ///
    void begin(std::function<void(const std::string&)> serial_send);

    ///
    /// @brief Run one communication cycle: poll every source once and dispatch any pending messages.
    /// Drives the network (TCP) transport and the serial console; callers need not know which
    /// transports exist. Intended to be called once per App tick.
    ///
    void tick();

    ///
    /// @brief Format a command outcome and deliver the SCPI string to the originating client.
    /// Invoked by the StateMachine for every command outcome, success or failure.
    ///
    /// @param cmd Outcome Command delivered by the StateMachine.
    ///
    void handle_response(const definitions::Command& cmd);

   private:
    Communications();
    ~Communications() = default;
    Communications(const Communications&) = delete;
    Communications& operator=(const Communications&) = delete;
    Communications(Communications&&) = delete;
    Communications& operator=(Communications&&) = delete;

    ///
    /// @brief Poll the network (TCP) server for messages and dispatch them.
    /// Handles availability checking and queues connectivity events on change. The concrete
    /// transport (WiFi or Ethernet) is selected at compile time via the TcpServer alias.
    ///
    void poll_network();

    ///
    /// @brief Poll the serial port for messages and dispatch them.
    ///
    void poll_serial();

    ///
    /// @brief Trim and route an incoming message to handle_event() or handle_command().
    /// Empty or whitespace-only messages respond immediately with kErrorUnknownCommand.
    ///
    /// @param msg Incoming raw message string.
    ///
    void handle_message(const std::string& msg);

    ///
    /// @brief Parse, authorize and dispatch a SCPI device command.
    ///
    /// @param msg Trimmed SCPI command string.
    ///
    void handle_command(const std::string& msg);

    ///
    /// @brief Parse and dispatch an FSM event string to the StateMachine.
    ///
    /// @param msg Trimmed event string including the "EVT:" prefix.
    ///
    void handle_event(const std::string& msg);

    ///
    /// @brief Send a response immediately to the client currently being served (active_responder_).
    ///
    /// @param response SCPI-formatted response string.
    ///
    void respond_now(const std::string& response);

    ///
    /// @brief Extract the valve index from a SCPI valve command and map it to the hardware pin.
    ///
    /// @param msg Full SCPI valve command string.
    /// @return Corresponding Pins enum value cast to uint32_t, or 0 if absent or unrecognised.
    ///
    uint32_t parse_valve_pin(const std::string& msg) const;

    ///
    /// @brief Parse the setpoint argument of a TEMP:SET command and pack it into Command::extra bits.
    ///
    /// Rejects a missing argument, trailing garbage, non-finite values, and values outside
    /// [config::kTempSetpointMinC, kTempSetpointMaxC]. On success writes the IEEE-754 bit pattern.
    ///
    /// @param msg      Full SCPI setpoint command string (e.g. "TEMP:SET:IN,25.0").
    /// @param out_bits Receives the float bit pattern when parsing succeeds.
    /// @return true if a valid, in-range setpoint was parsed; false otherwise.
    ///
    bool parse_setpoint_bits(const std::string& msg, uint32_t& out_bits) const;

    /// @brief Write-back that delivers one SCPI response line to a specific client.
    using Responder = std::function<void(const std::string&)>;

    Responder serial_send_;        ///< Serial write-back. Supplied once via begin() and reused for every response.
    Responder active_responder_;   ///< Write-back for the message being dispatched (WiFi client callback, or serial_send_).
    Responder pending_responder_;  ///< Responder captured for the in-flight queued command; invoked and cleared in handle_response().
    definitions::Transport active_transport_;  ///< Transport of the current message. Used for authorization/identity, not for routing.
    definitions::ClientId active_client_;      ///< WiFi sender of the current message; 0 for serial.
    Authorizer authorizer_;                    ///< Owns the operator-role state and the actuation gate.
    TcpServer& server_;                        ///< Active TCP transport singleton (WiFi or Ethernet, chosen at compile time).
    bool last_available_;                      ///< Last known transport availability; detects changes to queue connectivity events.
};

}  // namespace modules
