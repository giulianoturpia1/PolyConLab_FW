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

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "Authorizer.hpp"
#include "Config.hpp"
#include "Definitions.hpp"
#include "TcpServer.hpp"

namespace modules {

///
/// @brief Communications manager that routes SCPI messages from WiFi and Serial transports.
/// Implemented as a singleton.
///
/// Each incoming message carries a write-back to its sender (the WiFi client's callback, or
/// serial_send_ for Serial); the one for the message being dispatched is held in active_responder_
/// and copied into that message's outbound slot, so a reply reaches the client that asked for it
/// even after other messages have been served.
///
/// Ordering and delivery both live here, because both belong to the conversation this layer owns.
/// Every accepted message that owes a reply takes the next slot of outbound_ as it arrives; a reply
/// this layer can give at once fills its slot immediately, and one that needs the StateMachine fills
/// it later, found by the token this layer assigned. flush_outbound() writes slots out from the head
/// while they are answered, so the client is answered once per command, in the order it asked,
/// whether or not the StateMachine finished them in that order.
///
/// The StateMachine publishes outcomes from its own task, so they arrive through response_queue_ as
/// plain Command values and are formatted and delivered on the App task in tick(). Nothing in
/// outbound_ is touched by two tasks, and only the flush writes to a client.
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
    /// @brief Accept a command outcome from the StateMachine for delivery on the next tick().
    ///
    /// Called from the StateMachine's task through CommunicationsResponseQueue: it only hands the
    /// value over, so no client state is touched from that task.
    ///
    /// @param response Outcome Command, carrying the token of the command it answers.
    /// @param timeout Maximum time to wait for room in the queue, in milliseconds.
    /// @return true if the queue accepted it; false if it was full or not yet created.
    ///
    bool queue_response(const definitions::Command& response, uint32_t timeout);

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
    /// @brief Parse and dispatch an FSM event string to the StateMachine. Replies OK once queued, an error otherwise.
    ///
    /// @param msg Trimmed event string including the "EVT:" prefix.
    ///
    void handle_event(const std::string& msg);

    ///
    /// @brief Answer the message being served without a StateMachine round-trip.
    ///
    /// Takes the next outbound slot and fills it, so an answer that is ready at once still leaves
    /// after any reply owed to an earlier message. Despite the name nothing is written here; the
    /// write happens in flush_outbound().
    ///
    /// @param response SCPI-formatted response string.
    ///
    void respond_now(const std::string& response);

    ///
    /// @brief Take the next correlation id: never 0 (which marks untagged traffic) and never one
    /// already held by an outstanding command.
    ///
    uint16_t next_token();

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

    ///
    /// @brief One reply owed to a client, held until every reply owed before it has gone out.
    ///
    struct Outbound {
        uint16_t token;        ///< Id the StateMachine's reply will carry; 0 when answered here
        Responder responder;   ///< Write-back for the client that asked
        bool occupied;         ///< Slot is in use
        bool answered;         ///< response is filled and the slot is ready to go out
        std::string response;  ///< SCPI line to write once this slot reaches the head
    };

    ///
    /// @brief Claim the next outbound slot for the message being served.
    ///
    /// @param token Correlation id the reply will carry, or 0 when the caller fills the slot itself.
    /// @return The claimed slot, or nullptr when the ring is full.
    ///
    Outbound* claim_outbound(uint16_t token);

    ///
    /// @brief Move every outcome the StateMachine published into its outbound slot.
    ///
    void drain_responses();

    ///
    /// @brief Write out answered slots from the head, stopping at the first one still waiting.
    ///
    void flush_outbound();

    ///
    /// @brief Render a command outcome as its SCPI wire string.
    ///
    std::string format_response(const definitions::Command& cmd) const;

    ///
    /// @brief Count the slots still waiting on the StateMachine, which is what it is tracking too.
    ///
    uint8_t commands_in_flight() const;

    Responder serial_send_;       ///< Serial write-back. Supplied once via begin() and reused for every response.
    Responder active_responder_;  ///< Write-back for the message being dispatched (WiFi client callback, or serial_send_).
    std::array<Outbound, config::kOutboundRingLength> outbound_;  ///< Replies owed, oldest at outbound_head_
    uint8_t outbound_head_;                                       ///< Index of the oldest slot still owed
    uint8_t outbound_count_;                                      ///< Slots in use
    uint16_t next_token_;                                         ///< Correlation id generator
    QueueHandle_t response_queue_;                                ///< Outcomes published by the StateMachine's task, drained in tick()
    definitions::Transport active_transport_;  ///< Transport of the current message. Used for authorization/identity, not for routing.
    definitions::ClientId active_client_;      ///< WiFi sender of the current message; 0 for serial.
    Authorizer authorizer_;                    ///< Owns the operator-role state and the actuation gate.
    TcpServer& server_;                        ///< Active TCP transport singleton (WiFi or Ethernet, chosen at compile time).
    bool last_available_;                      ///< Last known transport availability; detects changes to queue connectivity events.
};

}  // namespace modules
