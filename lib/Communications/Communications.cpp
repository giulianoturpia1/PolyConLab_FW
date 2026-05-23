#include "Communications.hpp"

#include "Definitions.hpp"
#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

#include <cmath>
#include <cstdio>
#include <cstring>

namespace modules {

static const TaggedLogger kLog("Communications");

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

void Communications::initialize_server(EthernetServer& server, const std::array<byte, 6>& mac, const IPAddress& ip) {
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
    const bool link_up = local_server_.is_link_up();

    if (!link_up) {
        if (last_link_up_) {
            kLog.info("Ethernet desconectado. Pausando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kCommunicationLost);
        }
    } else {
        if (!last_link_up_) {
            kLog.info("Ethernet reconectado. Reanudando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kUnpause);
        }
        local_server_.poll([this](const std::string& msg, const LocalServer::ClientResponseCallback& client_respond) {
            set_ethernet_response_callback(client_respond);
            handle_ethernet_message(msg);
        });
    }

    last_link_up_ = link_up;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Communications::Communications()
    : ethernet_response_callback_(nullptr),
      serial_response_callback_(nullptr),
      local_server_(LocalServer::instance()),
      last_link_up_(true),
      pending_respond_(nullptr) {}

void Communications::handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    const std::string trimmed = utils::trim(msg);
    if (trimmed.empty()) {
        respond(utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand));
        return;
    }

    if (utils::starts_with(trimmed, "EVT:")) {
        handle_event(trimmed, respond);
        return;
    }

    handle_command(trimmed, respond);
}

void Communications::handle_command(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    const std::string base_command = msg.substr(0, msg.find(','));
    const definitions::CommandEntry* cmd = utils::find_command(base_command);
    if (!cmd) {
        respond(utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand));
        return;
    }

    if (cmd->command == definitions::CommandType::kLogOn) {
        logger().set_enabled(true);
        respond(utils::to_scpi(definitions::ResponseType::kStatusOn));
        return;
    }
    if (cmd->command == definitions::CommandType::kLogOff) {
        logger().set_enabled(false);
        respond(utils::to_scpi(definitions::ResponseType::kStatusOff));
        return;
    }
    if (cmd->command == definitions::CommandType::kLogQuery) {
        respond(utils::to_scpi(logger().is_enabled() ? definitions::ResponseType::kStatusOn : definitions::ResponseType::kStatusOff));
        return;
    }

    uint32_t extra = 0;
    if (cmd->command == definitions::CommandType::kValveOpen || cmd->command == definitions::CommandType::kValveClose ||
        cmd->command == definitions::CommandType::kValveState) {
        extra = parse_valve_pin(msg);
        if (extra == 0) {
            respond(utils::to_scpi(definitions::ResponseType::kErrorInvalidParam));
            return;
        }
    }

    pending_respond_ = respond;
    if (!StateMachine::instance().queue_command({cmd->command, extra}, kCommsQueueTimeoutMs)) {
        pending_respond_ = nullptr;
        respond(utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand));
    }
}

void Communications::handle_event(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    // "EVT:" with nothing after it
    if (msg.length() <= 4) {
        respond(utils::to_scpi(definitions::ResponseType::kErrorEmptyEvent));
        return;
    }

    const definitions::EventEntry* evt = utils::find_event(msg);
    if (!evt) {
        respond(utils::to_scpi(definitions::ResponseType::kErrorUnknownEvent));
        return;
    }

    if (!StateMachine::instance().queue_event(evt->event, kCommsQueueTimeoutMs)) {
        respond(utils::to_scpi(definitions::ResponseType::kErrorUnknownEvent));
    }
}

void Communications::handle_response(const definitions::Command& cmd) {
    if (!pending_respond_) {
        return;
    }

    std::string response;

    switch (cmd.type) {
        // Boolean state queries: extra = 1 → ON/HIGH, 0 → OFF/LOW
        case definitions::CommandType::kHeaterState:
        case definitions::CommandType::kPumpState:
        case definitions::CommandType::kChillerState:
            response =
                cmd.extra ? utils::to_scpi(definitions::ResponseType::kStatusOn) : utils::to_scpi(definitions::ResponseType::kStatusOff);
            break;
        case definitions::CommandType::kLevelSwitchState:
            response =
                cmd.extra ? utils::to_scpi(definitions::ResponseType::kStatusHigh) : utils::to_scpi(definitions::ResponseType::kStatusLow);
            break;
        // Numeric reads: extra holds IEEE-754 float bits packed via memcpy
        case definitions::CommandType::kTempInRead:
        case definitions::CommandType::kTempOutRead:
        case definitions::CommandType::kTempBathRead:
        case definitions::CommandType::kTempMixRead:
        case definitions::CommandType::kFlowRead: {
            float val;
            std::memcpy(&val, &cmd.extra, sizeof(float));
            if (std::isnan(val)) {
                response = utils::to_scpi(definitions::ResponseType::kErrorSensorFault);
                break;
            }
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.2f", val);
            response = buf;
            break;
        }
        // State query: extra holds the SystemState enum value cast to uint32_t
        case definitions::CommandType::kStateQuery:
            response = utils::state_name(static_cast<definitions::SystemState>(cmd.extra));
            break;
        // Error sentinel: extra holds the ResponseType cast to uint32_t
        case definitions::CommandType::kCommandError:
            response = utils::to_scpi(static_cast<definitions::ResponseType>(cmd.extra));
            break;
        // All other commands: static lookup through kCommandResponseMap
        default:
            response = utils::to_scpi(cmd.type);
            break;
    }

    pending_respond_(response);
    pending_respond_ = nullptr;
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
