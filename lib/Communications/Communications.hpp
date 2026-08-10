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

#include "Definitions.hpp"
#include "WifiServer.hpp"

namespace modules {

///
/// @brief Communications manager that routes SCPI messages from WiFi and Serial transports.
/// Implemented as a singleton.
///
/// Incoming messages are tagged with their origin transport (WiFi or Serial) and dispatched
/// by handle_message(). Responses are delivered through a transport-specific path:
///
///   - WiFi: network_send_, set per-message to the connected TCP client's write function.
///
///   - Serial: serial_send_, supplied once at initialisation via initialize_server().
///
/// For commands that require a StateMachine round-trip, the active transport is stored in
/// pending_transport_ before the command is queued. When the StateMachine calls handle_response(),
/// the response is routed to whichever transport originated the command.
///
/// Inline commands (SYST:LOG*, invalid params, unknown) respond immediately via respond_now()
/// without involving pending_transport_.
///
/// @note Closing the Serial connection (e.g. closing the COM port or unplugging the USB
/// cable) resets the whole board — the USB-serial bridge's DTR/RTS auto-reset circuit pulses
/// EN — not just the Serial transport. This also drops the WiFi connection, since the entire
/// firmware, including the WiFi server, reboots. See PCL-69. Note this is a hardware limitation
/// of the platforms used, not a software limitation of the Communications module.
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
    /// @brief Poll the WiFi server for messages and dispatch them.
    /// Handles link status checking and queues connectivity events if the link changes.
    ///
    void poll_wifi();

    ///
    /// @brief Poll the serial port for messages and dispatch them.
    ///
    void poll_serial();

    ///
    /// @brief Format a command outcome and deliver the SCPI string to the originating client.
    /// Invoked by the StateMachine via its response callback for every command outcome,
    /// both success and failure.
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
    /// @brief Trim and route an incoming message to handle_event() or handle_command().
    /// Empty or whitespace-only messages respond immediately with kErrorUnknownCommand.
    ///
    /// @param msg Incoming raw message string.
    ///
    void handle_message(const std::string& msg);

    ///
    /// @brief Parse and dispatch a SCPI device command.
    /// Logger commands respond immediately via respond_now(). All other commands store
    /// active_transport_ into pending_transport_ and enqueue to the StateMachine.
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
    /// @brief Send a response string immediately to the transport that is currently active.
    /// Routes to network_send_ when active_transport_ is kWifi,
    /// or to serial_send_ when active_transport_ is kSerial.
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

    std::function<void(const std::string&)> network_send_;  ///< TCP client write function for the message currently being processed.
                                                            ///< Overwritten by poll_wifi() on every incoming message.
    std::function<void(const std::string&)>
        serial_send_;  ///< Serial write function. Supplied once via initialize_server() and reused for every response.
    definitions::Transport active_transport_;   ///< Transport of the message currently being dispatching. Set before handle_message().
    definitions::Transport pending_transport_;  ///< Transport of the in-flight device command awaiting a StateMachine response. Set in
                                                ///< handle_command() before queuing; cleared in handle_response() after delivery.
    WifiServer& wifi_server_;                   ///< Reference to the WifiServer singleton for WiFi communication.
    bool last_link_up_;                         ///< Last known WiFi link status; used to detect changes and queue connectivity events.
};

}  // namespace modules
