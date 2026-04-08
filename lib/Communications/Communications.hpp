///
/// @file Communications.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Communications module that isolates command handling from the application layer.
/// @version 0.1
/// @date 26-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <functional>
#include <string>

namespace modules {

///
/// @brief Communications manager that routes commands from multiple transports.
/// Implemented as a singleton.
/// The class provides operations to set callbacks for sending responses back through the appropriate transport (Ethernet
/// or Serial), in order to keep the transport layer isolated from the application logic.
///
/// The main operation is handle_message(), which takes an incoming message and a response callback, and routes the message to the
/// appropriate handler based on its content (device command or event command). The device command handler processes commands related to
/// devices (e.g., setting flow rate), while the event command handler processes commands that trigger events in the state machine (e.g.,
/// "EVT:START_CALIBRATION").
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
    /// @brief Handle a message coming from Ethernet.
    ///
    void handle_ethernet_message(const std::string& msg);

    ///
    /// @brief Handle a message coming from Serial.
    ///
    void handle_serial_message(const std::string& msg);

   private:
    Communications();
    // Rule of Five
    ~Communications() = default;
    Communications(const Communications&) = delete;
    Communications& operator=(const Communications&) = delete;
    Communications(Communications&&) = delete;
    Communications& operator=(Communications&&) = delete;

    ///
    /// @brief Handle a command message, routing it to the appropriate handler. At the moment, it only supports device
    /// commands and event commands.
    ///
    /// @param msg
    /// @param respond
    ///
    void handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Handle a command message.The command name is extracted from the message and dispatched to the
    /// corresponding handler. Dispatches the command if it is valid, otherwise responds with an error. Dispatches the
    /// event if it is valid, otherwise responds with an error.
    ///
    /// @param msg
    /// @param respond
    /// @return true
    /// @return false
    ///
    bool handle_command(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Handle an event message. The event name is extracted from the message and dispatched to the state
    /// machine. Dispatches the event if it is valid, otherwise responds with an error.
    ///
    /// @param msg
    /// @param respond
    /// @return true
    /// @return false
    ///
    bool handle_event(const std::string& msg, const std::function<void(const std::string&)>& respond);

    ///
    /// @brief Parse valve number from command message and return corresponding pin.
    /// Expected format: "VALVE_CMD,N" where N is 1 or 2.
    ///
    /// @param msg Command message
    /// @return uint32_t Pin number, or 0 if invalid
    ///
    uint32_t parse_valve_pin(const std::string& msg) const;

    std::function<void(const std::string&)> ethernet_response_callback_;
    std::function<void(const std::string&)> serial_response_callback_;
};

}  // namespace modules
