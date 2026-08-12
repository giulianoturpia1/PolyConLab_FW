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
/// Each incoming message carries a write-back to its sender: for WiFi the connected client's
/// callback, for Serial the serial_send_ function supplied once at initialisation. The write-back
/// for the message currently being dispatched is held in active_responder_; respond_now() delivers
/// immediate replies through it.
///
/// For commands that require a StateMachine round-trip, active_responder_ is captured into
/// pending_responder_ before the command is queued. When the StateMachine calls handle_response(),
/// the reply is delivered through that captured responder, so it always reaches the client that
/// issued the command even if another message is served in between.
///
/// Inline commands (SYST:LOG*, invalid params, unknown) respond immediately via respond_now()
/// without involving pending_responder_.
///
/// Authorization is delegated to authorizer_ (see Authorizer): the local Serial console is always
/// privileged; over WiFi the first client to interact claims the operator role, actuation commands
/// are gated to the operator and read-only queries stay open. The role is released on link drop.
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
    void initialize_server(std::function<void(const std::string&)> serial_send);

    ///
    /// @brief Poll every communication source once and dispatch any pending messages.
    /// Drives the network (TCP) transport and the serial console; callers need not know which
    /// transports exist. Intended to be called once per App tick.
    ///
    void poll();

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

    /// @brief Write-back that delivers one SCPI response line to a specific client.
    using Responder = std::function<void(const std::string&)>;

    Responder serial_send_;        ///< Serial write-back. Supplied once via initialize_server() and reused for every response.
    Responder active_responder_;   ///< Write-back for the message being dispatched (WiFi client callback, or serial_send_).
    Responder pending_responder_;  ///< Responder captured for the in-flight queued command; invoked and cleared in handle_response().
    definitions::Transport active_transport_;  ///< Transport of the current message. Used for authorization/identity, not for routing.
    definitions::ClientId active_client_;      ///< WiFi sender of the current message; 0 for serial.
    Authorizer authorizer_;                    ///< Owns the operator-role state and the actuation gate.
    TcpServer& server_;                        ///< Active TCP transport singleton (WiFi or Ethernet, chosen at compile time).
    bool last_available_;                      ///< Last known transport availability; detects changes to queue connectivity events.
};

}  // namespace modules
