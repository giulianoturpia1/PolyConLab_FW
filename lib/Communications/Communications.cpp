#include "Communications.hpp"

#include <cmath>

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

void Communications::begin(std::function<void(const std::string&)> serial_send) {
    serial_send_ = std::move(serial_send);
    server_.begin();
}

void Communications::tick() {
    poll_network();
    poll_serial();
}

void Communications::poll_network() {
    const bool available = server_.is_available();

    if (!available) {
        if (last_available_) {
            kLog.info("WiFi desconectado. Pausando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kCommunicationLost);
            authorizer_.release();
        }
    } else {
        if (!last_available_) {
            kLog.info("WiFi reconectado. Reanudando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kUnpause);
        }
        // Free the operator role once its holding client has left. A SoftAP keeps is_available()
        // true with zero clients (no uplink to lose), so the link-down release() above never fires
        // over WiFi — presence tracking is what lets the next client claim the role after a
        // disconnect. Uses the identity cached by the previous poll() tick.
        if (authorizer_.sync_operator_presence(server_.connected_client())) {
            kLog.info("Rol de operador liberado: el cliente operador se desconecto.");
        }
        server_.poll(
            [this](const std::string& msg, definitions::ClientId client_id, const TcpServer::ClientResponseCallback& client_respond) {
                active_responder_ = client_respond;
                active_transport_ = definitions::Transport::kWifi;
                active_client_ = client_id;
                if (authorizer_.claim(client_id)) {
                    kLog.info("Rol de operador asignado al cliente " + std::to_string(client_id));
                }
                handle_message(msg);
            });
    }

    last_available_ = available;
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
    active_client_ = 0;  // serial console has no client id
    active_responder_ = serial_send_;
    handle_message(msg);
}

void Communications::handle_response(const definitions::Command& cmd) {
    if (!pending_responder_) {
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
        case definitions::CommandType::kValveState:
            response = (cmd.extra & definitions::kValveStatusOpenMask) ? utils::to_scpi(definitions::ResponseType::kStatusOpen)
                                                                       : utils::to_scpi(definitions::ResponseType::kStatusClosed);
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
        // Sensor re-initialisation: extra holds the ResponseType (kReinitOk / kErrorReinitFailed)
        case definitions::CommandType::kTempInReinit:
        case definitions::CommandType::kTempOutReinit:
        case definitions::CommandType::kTempBathReinit:
        case definitions::CommandType::kTempMixReinit:
        case definitions::CommandType::kFlowReinit:
        case definitions::CommandType::kLevelSwitchReinit:
            response = utils::to_scpi(static_cast<definitions::ResponseType>(cmd.extra));
            break;
        // All other commands: static lookup through kCommandResponseMap
        default:
            response = utils::to_scpi(cmd.type);
            break;
    }

    // Deliver through the responder captured when the command was queued, so the reply reaches the
    // client that issued it — not merely whatever transport happens to be active now.
    pending_responder_(response);
    pending_responder_ = nullptr;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Communications::Communications()
    : serial_send_(nullptr),
      active_responder_(nullptr),
      pending_responder_(nullptr),
      active_transport_(definitions::Transport::kNone),
      active_client_(0),
      server_(TcpServer::instance()),
      last_available_(true) {}  // assume up at boot so the first poll_network() tick won't fire a spurious kUnpause

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

    // Only the operator (or serial) may drive physical outputs; queries stay open.
    if (!authorizer_.is_authorized(*cmd, active_transport_, active_client_)) {
        kLog.warning("Comando de actuacion rechazado: el cliente no es el operador.");
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorNotOperator));
        return;
    }

    // Commands answered locally return here; valve and setpoint commands compute their Command::extra
    // payload (pin / packed float) and fall through to the queue; every other command falls through
    // unchanged with extra == 0.
    uint32_t extra = 0;
    switch (cmd->command) {
        case definitions::CommandType::kLogOn:
            logger().set_enabled(true);
            respond_now(utils::to_scpi(definitions::ResponseType::kStatusOn));
            return;
        case definitions::CommandType::kLogOff:
            logger().set_enabled(false);
            respond_now(utils::to_scpi(definitions::ResponseType::kStatusOff));
            return;
        case definitions::CommandType::kLogQuery: {
            const definitions::ResponseType state =
                logger().is_enabled() ? definitions::ResponseType::kStatusOn : definitions::ResponseType::kStatusOff;
            respond_now(utils::to_scpi(state));
            return;
        }
        case definitions::CommandType::kFaultQuery:
            // Comma-separated list of devices that failed init, or "NONE".
            respond_now(modules::FaultRegistry::instance().get());
            return;
        case definitions::CommandType::kValveOpen:
        case definitions::CommandType::kValveClose:
        case definitions::CommandType::kValveState:
            extra = parse_valve_pin(msg);
            if (extra == 0) {
                respond_now(utils::to_scpi(definitions::ResponseType::kErrorInvalidParam));
                return;
            }
            break;
        case definitions::CommandType::kTempSetIn:
            if (!parse_setpoint_bits(msg, extra)) {
                respond_now(utils::to_scpi(definitions::ResponseType::kErrorInvalidParam));
                return;
            }
            break;
        default:
            break;
    }

    // Queue the commands not handled above to the StateMachine.
    pending_responder_ = active_responder_;
    if (!StateMachine::instance().queue_command({cmd->command, extra}, config::kCommsQueueTimeoutMs)) {
        pending_responder_ = nullptr;
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
    if (active_responder_) {
        active_responder_(response);
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

bool Communications::parse_setpoint_bits(const std::string& msg, uint32_t& out_bits) const {
    const size_t comma_pos = msg.find(',');
    if (comma_pos == std::string::npos || comma_pos + 1 >= msg.length()) {
        return false;  // no argument
    }

    const std::string arg = utils::trim(msg.substr(comma_pos + 1));
    size_t consumed = 0;
    float setpoint = 0.0f;
    try {
        setpoint = std::stof(arg, &consumed);
    } catch (...) {
        return false;  // not a number / out of float range
    }

    if (consumed != arg.length() || !std::isfinite(setpoint)) {
        return false;  // trailing garbage, or NaN/inf
    }
    if (setpoint < config::kTempSetpointMinC || setpoint > config::kTempSetpointMaxC) {
        return false;  // outside the configured process envelope
    }

    out_bits = utils::float_to_bits(setpoint);
    return true;
}
}  // namespace modules
