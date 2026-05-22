#include "Communications.hpp"

#include "Definitions.hpp"
#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "StateMachine.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

namespace modules {

constexpr uint32_t kCommsQueueTimeoutMs = 10;

////////////////////////////////////// Public Methods //////////////////////////////////////

Communications& Communications::instance() {
    static Communications instance;
    return instance;
}

void Communications::set_ethernet_response_callback(const std::function<void(const std::string&)>& callback) {
    ethernet_response_callback_ = callback;
}

void Communications::set_serial_response_callback(const std::function<void(const std::string&)>& callback) {
    serial_response_callback_ = callback;
}

void Communications::initialize_server(EthernetServer& server, const byte mac[6], const IPAddress& ip) {
    local_server_.initialize(server);
    local_server_.begin(mac, ip);
}

void Communications::handle_ethernet_message(const std::string& msg) {
    if (!ethernet_response_callback_) {
        return;
    }

    handle_message(msg, ethernet_response_callback_);
}

void Communications::handle_serial_message(const std::string& msg) {
    if (!serial_response_callback_) {
        return;
    }

    handle_message(msg, serial_response_callback_);
}

void Communications::poll_serial() {
    if (!Serial.available()) {
        return;
    }

    String arduino_msg = Serial.readStringUntil('\n');
    arduino_msg.trim();
    if (arduino_msg.length() == 0) {
        return;
    }

    std::string msg(arduino_msg.c_str());
    if (!serial_response_callback_) {
        return;
    }
    handle_message(msg, serial_response_callback_);
}

void Communications::poll_ethernet() {
    // Check Ethernet link status and queue disconnection event if link is lost
    if (!local_server_.is_link_up()) {
        StateMachine::instance().queue_event(definitions::SystemEvent::kCommunicationLost);
        hal_general().log("Ethernet desconectado. Pausando sistema.");
    } else {
        // Poll for ethernet messages. The handler receives both the message and a response callback
        // that is specific to the current client connection.
        local_server_.poll([this](const std::string& msg, const LocalServer::ClientResponseCallback& client_respond) {
            set_ethernet_response_callback(client_respond);
            handle_ethernet_message(msg);
        });
    }
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Communications::Communications()
    : ethernet_response_callback_(nullptr), serial_response_callback_(nullptr), local_server_(LocalServer::instance()) {}

void Communications::handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    if (msg.empty()) {
        return;
    }

    if (handle_command(msg, respond)) {
        return;
    }

    if (handle_event(msg, respond)) {
        return;
    }

    respond(utils::response(definitions::ResponseType::kErrorUnknownCommand));
}

bool Communications::handle_command(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    // Extract base command (before comma, if present, as some commands have format "DEVICE_CMD,N")
    std::string base_command = msg;
    size_t comma_pos = msg.find(',');
    if (comma_pos != std::string::npos) {
        base_command = msg.substr(0, comma_pos);
    }

    const definitions::CommandMap* cmd = utils::find_command(base_command);
    if (!cmd) {
        respond(utils::response(definitions::ResponseType::kErrorUnknownCommand));
        return false;
    }

    uint32_t extra = 0;
    if (cmd->command == definitions::CommandType::kValveOpen || cmd->command == definitions::CommandType::kValveClose ||
        cmd->command == definitions::CommandType::kValveState) {
        extra = parse_valve_pin(msg);
    }

    if (!StateMachine::instance().queue_command({cmd->command, extra}, kCommsQueueTimeoutMs)) {
        respond(utils::response(definitions::ResponseType::kErrorUnknownCommand));
        return false;
    }

    return true;
}

bool Communications::handle_event(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    // Trim the message to remove leading/trailing whitespace and check if it's empty
    std::string event_name = utils::trim(msg);
    if (event_name.length() == 0) {
        respond(utils::response(definitions::ResponseType::kErrorEmptyEvent));
        return true;
    }

    const definitions::EventMap* evt = utils::find_event(event_name);
    if (!evt) {
        respond(utils::response(definitions::ResponseType::kErrorUnknownEvent));
        return true;
    }

    if (!StateMachine::instance().queue_event(evt->event, kCommsQueueTimeoutMs)) {
        respond(utils::response(definitions::ResponseType::kErrorUnknownEvent));
        return false;
    }

    return true;
}

uint32_t Communications::parse_valve_pin(const std::string& msg) const {
    size_t comma_pos = msg.find(',');
    if (comma_pos == std::string::npos || comma_pos + 1 >= msg.length()) {
        return 0;  // Invalid format
    }

    char valve_num = msg[comma_pos + 1];
    switch (valve_num) {
        case '1': {
            return static_cast<uint32_t>(definitions::Pins::kValve1);
        }
        case '2': {
            return static_cast<uint32_t>(definitions::Pins::kValve2);
        }
        default: {
            return 0;  // Invalid valve number
        }
    }
}
}  // namespace modules
