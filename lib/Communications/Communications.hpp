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

#include <Ethernet.h>

#include <array>
#include <functional>
#include <string>

#include "Definitions.hpp"
#include "LocalServer.hpp"

namespace modules {

///
/// @brief Communications manager that routes SCPI messages from Ethernet and Serial transports.
/// Implemented as a singleton.
///
/// Incoming messages are trimmed and routed by handle_message():
///   - "EVT:*" strings are dispatched to handle_event() → StateMachine event queue.
///   - All other strings are treated as SCPI commands and dispatched to handle_command().
///
/// Response delivery uses a deferred callback pattern:
///   - For commands that require a StateMachine round-trip, handle_command() stores the
///     transport-specific respond callable in pending_respond_ before queuing the command.
///   - When the StateMachine finishes (success or failure), it invokes the registered
///     response_callback_, which calls handle_response(). handle_response() formats the
///     SCPI string and fires pending_respond_ to deliver it to the original client.
///   - Inline commands (SYST:LOG*, PING, STATE?, invalid params) respond immediately
///     without touching pending_respond_.
///
///
class Communications {
   public:
    ///
    /// @brief Get the singleton instance of Communications
    ///
    static Communications& instance();

    ///
    /// @brief Update the Ethernet response callback for the message currently being processed.
    /// Called automatically by poll_ethernet() on every incoming message, wiring the specific
    /// TCP client's write function as the active respond target. Not a persistent transport sink.
    ///
    /// @param callback Callable that writes a SCPI response string to the connected TCP client.
    ///
    void set_ethernet_response_callback(const std::function<void(const std::string&)>& callback);

    ///
    /// @brief Set the persistent callback for sending responses back through Serial.
    /// Called once during initialization. Unlike the Ethernet callback, this is not
    /// updated per-message because Serial has a single fixed output target.
    ///
    /// @param callback Callable that writes a SCPI response string to the Serial port.
    ///
    void set_serial_response_callback(const std::function<void(const std::string&)>& callback);

    ///
    /// @brief Initialize and start the Ethernet TCP server.
    ///
    /// @param server EthernetServer instance to bind (provided by the Arduino Ethernet library).
    /// @param mac    6-byte MAC address for the network interface.
    /// @param ip     Static IP address to assign to the interface.
    ///
    void initialize_server(EthernetServer& server, const std::array<byte, 6>& mac, const IPAddress& ip);

    ///
    /// @brief Process a single message received from an Ethernet client.
    /// Silently discards the message if no Ethernet response callback is currently set.
    ///
    /// @param msg Raw message string received from the TCP client.
    ///
    void handle_ethernet_message(const std::string& msg);

    ///
    /// @brief Process a single message received from Serial.
    /// Silently discards the message if no Serial response callback is set.
    ///
    /// @param msg Raw message string received from the Serial port.
    ///
    void handle_serial_message(const std::string& msg);

    ///
    /// @brief Poll the Ethernet server for messages and dispatch them appropriately.
    /// Handles link status checking and queues disconnection events if the link is lost.
    ///
    void poll_ethernet();

    ///
    /// @brief Poll the serial port for messages and dispatch them appropriately.
    ///
    void poll_serial();

    ///
    /// @brief Format a command outcome and deliver the SCPI string to the pending client.
    /// Invoked by the StateMachine via its response callback for every command outcome,
    /// both success and failure. Fires and clears pending_respond_ after delivery.
    /// @param cmd Outcome Command delivered by the StateMachine.
    ///
    void handle_response(const definitions::Command& cmd);

   private:
    Communications();
    // Rule of Five
    ~Communications() = default;
    Communications(const Communications&) = delete;
    Communications& operator=(const Communications&) = delete;
    Communications(Communications&&) = delete;
    Communications& operator=(Communications&&) = delete;

    ///
    /// @brief Trim and route an incoming message to the appropriate handler based on its prefix.
    /// Empty or whitespace-only messages respond immediately with kErrorUnknownCommand.
    /// "EVT:*" strings are forwarded to handle_event(); all others to handle_command().
    /// Downstream handlers receive an already-trimmed string.
    ///
    /// @param msg     Incoming raw message string (trimming applied internally).
    /// @param respond Transport-specific callable to send the SCPI response string to the client.
    ///
    void handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Parse and dispatch a SCPI device command.
    /// Logger commands (SYST:LOG ON/OFF/?) are handled inline and respond immediately.
    /// Valve commands parse the pin index from the message before queuing.
    /// All other commands store respond as pending_respond_ and enqueue to the StateMachine;
    /// the deferred response is delivered later via handle_response().
    ///
    /// Immediate error responses:
    ///   - kErrorUnknownCommand: command string not found in kCommandMap, or queue send failed.
    ///   - kErrorInvalidParam:   valve index missing or not in {1, 2}.
    ///
    /// @param msg     Trimmed SCPI command string (e.g. "OUTP:HEAT ON", "OUTP:VALV OPEN,1").
    /// @param respond Transport-specific callable to send the SCPI response string to the client.
    ///
    void handle_command(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Parse and dispatch an FSM event string to the StateMachine.
    /// Successfully queued events produce no acknowledgement response (known gap).
    ///
    /// Immediate error responses:
    ///   - kErrorEmptyEvent:   nothing follows the "EVT:" prefix.
    ///   - kErrorUnknownEvent: event string not found in kEventMap, or queue send failed.
    ///
    /// @param msg     Trimmed event string including the "EVT:" prefix (e.g. "EVT:PAUSE").
    /// @param respond Transport-specific callable to send the SCPI response string to the client.
    ///
    void handle_event(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Extract the valve index from a SCPI valve command and map it to the hardware pin.
    /// Expected format: "OUTP:VALV <ACTION>,N" where N is '1' or '2'.
    ///
    /// @param msg Full SCPI valve command string.
    /// @return Corresponding Pins enum value cast to uint32_t, or 0 if N is absent or unrecognised.
    ///
    uint32_t parse_valve_pin(const std::string& msg) const;

    std::function<void(const std::string&)> ethernet_response_callback_;
    ///< Per-message TCP client write function. Overwritten by poll_ethernet() on every incoming
    ///< message; calling it sends the response string to that specific connected client.
    std::function<void(const std::string&)> serial_response_callback_;
    ///< Persistent Serial write function. Set once during initialization.
    std::function<void(const std::string&)> pending_respond_;
    ///< Captures the transport respond callable for the single in-flight device command.
    ///< Set in handle_command() before queuing; fired and cleared in handle_response().
    ///< nullptr when no command is awaiting a deferred response.
    LocalServer& local_server_;  ///< Reference to the LocalServer singleton for Ethernet communication.
    bool last_link_up_;          ///< Last known Ethernet link status; used to detect changes and queue connectivity events.
};

}  // namespace modules
