#include "Communications.hpp"

#include <cmath>

#include "Chiller.hpp"
#include "Definitions.hpp"
#include "Flowmeter.hpp"
#include "Heater.hpp"
#include "Helpers.hpp"
#include "LevelSwitch.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
#include "StateMachine.hpp"
#include "Thermocouple.hpp"

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

////////////////////////////////////// Private Methods //////////////////////////////////////

Communications::Communications() : ethernet_response_callback_(nullptr), serial_response_callback_(nullptr) {}

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
        return false;
    }

    uint32_t pin = 0;
    if (cmd->command == definitions::CommandType::kValveOpen || cmd->command == definitions::CommandType::kValveClose ||
        cmd->command == definitions::CommandType::kValveState) {
        pin = parse_valve_pin(msg);
    }

    if (modules::StateMachine::instance().queue_command({cmd->command, pin}, kCommsQueueTimeoutMs)) {
        return true;
    }

    respond(std::string(utils::response(definitions::ResponseType::kErrorUnknownCommand)));
    return true;
}

bool Communications::handle_event(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    if (!utils::starts_with(msg, "EVT:") && !utils::starts_with(msg, "EVENT:")) {
        return false;
    }

    std::string event_name = utils::starts_with(msg, "EVT:") ? msg.substr(4) : msg.substr(6);

    event_name = utils::trim(event_name);
    if (event_name.length() == 0) {
        respond(std::string(utils::response(definitions::ResponseType::kErrorEmptyEvent)));
        return true;
    }

    const auto it = std::find_if(definitions::kEventMap.begin(), definitions::kEventMap.end(),
                                 [&event_name](const definitions::EventMap& entry) { return event_name == entry.name; });
    if (it == definitions::kEventMap.end()) {
        respond(std::string(utils::response(definitions::ResponseType::kErrorUnknownEvent)));
        return true;
    }

    if (!modules::StateMachine::instance().queue_event(it->event, kCommsQueueTimeoutMs)) {
        respond(std::string(utils::response(definitions::ResponseType::kErrorUnknownEvent)));
        return true;
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
