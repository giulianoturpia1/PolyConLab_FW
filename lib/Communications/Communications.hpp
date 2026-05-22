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

#include "LocalServer.hpp"

namespace modules {

///
/// @brief Communications manager that routes messages from multiple transports.
/// Implemented as a singleton.
/// The class provides operations to set callbacks for sending responses back through the appropriate transport (Ethernet
/// or Serial), in order to keep the transport layer isolated from the application logic.
///
/// The main operation is handle_message(), which routes incoming messages based on their prefix:
///   - Messages starting with "EVT:" are dispatched to handle_event() and trigger state machine transitions.
///   - All other messages are treated as SCPI commands and dispatched to handle_command().
/// Each handler is fully self-contained and owns its own error responses.
///
///
class Communications {
   public:
    ///
    /// @brief Get the singleton instance of Communications
    ///
    static Communications& instance();

    ///
    /// @brief Set the callback for sending Ethernet responses
    ///
    void set_ethernet_response_callback(const std::function<void(const std::string&)>& callback);

    ///
    /// @brief Set the callback for sending serial responses
    ///
    void set_serial_response_callback(const std::function<void(const std::string&)>& callback);

    ///
    /// @brief Initialize the Ethernet server with the given parameters.
    ///
    void initialize_server(EthernetServer& server, const std::array<byte, 6>& mac, const IPAddress& ip);

    ///
    /// @brief Handle a message coming from Ethernet.
    ///
    void handle_ethernet_message(const std::string& msg);

    ///
    /// @brief Handle a message coming from Serial.
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
    /// Empty or whitespace-only messages respond with kErrorUnknownCommand.
    /// Messages starting with "EVT:" are forwarded to handle_event(); all others to handle_command().
    /// System commands (e.g. "SYST:LOG ON") are registered in kCommandMap as CommandEntry records and handled inline
    /// inside handle_command() without being queued to the StateMachine.
    /// Handlers receive an already-trimmed string.
    ///
    /// @param msg    Incoming raw message string.
    /// @param respond Callback used to send a response back to the caller.
    ///
    void handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Parse and dispatch a SCPI command to the state machine.
    /// Looks up the command in the command map and queues it for execution.
    /// Responds with kErrorUnknownCommand if the command is not recognized or cannot be queued.
    ///
    /// @param msg    Full SCPI command string (e.g. "OUTP:HEAT ON", "OUTP:VALV OPEN,1").
    /// @param respond Callback used to send a response back to the caller.
    ///
    void handle_command(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Parse and dispatch an event to the state machine.
    /// Looks up the event in the event map and queues it for processing.
    /// Responds with kErrorEmptyEvent if no event type follows "EVT:", or
    /// kErrorUnknownEvent if the event is not recognized or cannot be queued.
    ///
    /// @param msg    Full event string, including the "EVT:" prefix (e.g. "EVT:PAUSE").
    /// @param respond Callback used to send a response back to the caller.
    ///
    void handle_event(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Parse the valve number from a SCPI valve command and return the corresponding pin.
    /// Expected format: "OUTP:VALV <ACTION>,N" where N is 1 or 2.
    ///
    /// @param msg Full SCPI valve command string.
    /// @return uint32_t Corresponding pin number, or 0 if the format is invalid.
    ///
    uint32_t parse_valve_pin(const std::string& msg) const;

    std::function<void(const std::string&)> ethernet_response_callback_;  ///< Callback for sending responses back through Ethernet
    std::function<void(const std::string&)> serial_response_callback_;    ///< Callback for sending responses back through Serial
    LocalServer& local_server_;  ///< Reference to the LocalServer singleton for handling Ethernet communication
    bool last_link_up_;          ///< Last known Ethernet link status, used to detect changes and queue events accordingly
};

}  // namespace modules
