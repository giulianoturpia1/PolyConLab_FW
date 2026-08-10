#include "Communications.hpp"

#include "Config.hpp"
#include "Definitions.hpp"
#include "FaultRegistry.hpp"
#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"
#include "StateMachine.hpp"
#include "TargetDefinitions.hpp"

namespace modules {

static const TaggedLogger kLog("Communications");

////////////////////////////////////// Public Methods //////////////////////////////////////

Communications& Communications::instance() {
    static Communications instance;
    return instance;
}

void Communications::initialize_server(std::function<void(const std::string&)> serial_send) {
    serial_send_ = std::move(serial_send);
    wifi_server_.begin();
}

void Communications::poll_wifi() {
    const bool link_up = wifi_server_.is_link_up();

    if (!link_up) {
        if (last_link_up_) {
            kLog.info("WiFi desconectado. Pausando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kCommunicationLost);
        }
    } else {
        if (!last_link_up_) {
            kLog.info("WiFi reconectado. Reanudando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kUnpause);
        }
        wifi_server_.poll([this](const std::string& msg, const WifiServer::ClientResponseCallback& client_respond) {
            network_send_ = client_respond;
            active_transport_ = definitions::Transport::kWifi;
            handle_message(msg);
        });
    }

    last_link_up_ = link_up;
}

void Communications::poll_serial() {
    if (hal_general().serial_available() <= 0) {
        return;
    }

    const std::string msg = utils::trim(hal_general().serial_read_line());
    if (msg.empty()) {
        return;
    }

    active_transport_ = definitions::Transport::kSerial;
    handle_message(msg);
}

void Communications::handle_response(const definitions::Command& cmd) {
    if (pending_transport_ == definitions::Transport::kNone) {
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
        case definitions::CommandType::kFlowRead:
            response = utils::format_sensor_value(cmd.extra);
            break;
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

    respond_now(response);

    pending_transport_ = definitions::Transport::kNone;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Communications::Communications()
    : network_send_(nullptr),
      serial_send_(nullptr),
      active_transport_(definitions::Transport::kNone),
      pending_transport_(definitions::Transport::kNone),
      wifi_server_(WifiServer::instance()),
      last_link_up_(true) {}  // true = assume link is up at construction so the first
                              // poll_wifi() tick does not fire a spurious kUnpause.

void Communications::handle_message(const std::string& msg) {
    const std::string trimmed = utils::trim(msg);
    if (trimmed.empty()) {
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand));
        return;
    }

    if (utils::starts_with(trimmed, "EVT:")) {
        handle_event(trimmed);
        return;
    }

    handle_command(trimmed);
}

void Communications::handle_command(const std::string& msg) {
    const std::string base_command = msg.substr(0, msg.find(','));
    const definitions::CommandEntry* cmd = utils::find_command(base_command);
    if (!cmd) {
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand));
        return;
    }

    if (cmd->command == definitions::CommandType::kLogOn) {
        logger().set_enabled(true);
        respond_now(utils::to_scpi(definitions::ResponseType::kStatusOn));
        return;
    }
    if (cmd->command == definitions::CommandType::kLogOff) {
        logger().set_enabled(false);
        respond_now(utils::to_scpi(definitions::ResponseType::kStatusOff));
        return;
    }
    if (cmd->command == definitions::CommandType::kLogQuery) {
        respond_now(utils::to_scpi(logger().is_enabled() ? definitions::ResponseType::kStatusOn : definitions::ResponseType::kStatusOff));
        return;
    }
    if (cmd->command == definitions::CommandType::kFaultQuery) {
        // Return a comma-separated list of devices that failed hardware
        // initialisation, or "NONE" if all devices started successfully.
        respond_now(modules::FaultRegistry::instance().get());
        return;
    }

    uint32_t extra = 0;
    if (cmd->command == definitions::CommandType::kValveOpen || cmd->command == definitions::CommandType::kValveClose ||
        cmd->command == definitions::CommandType::kValveState) {
        extra = parse_valve_pin(msg);
        if (extra == 0) {
            respond_now(utils::to_scpi(definitions::ResponseType::kErrorInvalidParam));
            return;
        }
    }

    pending_transport_ = active_transport_;
    if (!StateMachine::instance().queue_command({cmd->command, extra}, config::kCommsQueueTimeoutMs)) {
        pending_transport_ = definitions::Transport::kNone;
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand));
    }
}

void Communications::handle_event(const std::string& msg) {
    if (msg.length() <= 4) {
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorEmptyEvent));
        return;
    }

    const definitions::EventEntry* evt = utils::find_event(msg);
    if (!evt) {
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorUnknownEvent));
        return;
    }

    if (!StateMachine::instance().queue_event(evt->event, config::kCommsQueueTimeoutMs)) {
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorUnknownEvent));
    }
}

void Communications::respond_now(const std::string& response) {
    if (active_transport_ == definitions::Transport::kWifi && network_send_) {
        network_send_(response);
        return;
    }
    if (active_transport_ == definitions::Transport::kSerial && serial_send_) {
        serial_send_(response);
        return;
    }
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
