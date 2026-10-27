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
    if (response_queue_ == nullptr) {
        response_queue_ = xQueueCreate(config::kOutboundRingLength, sizeof(definitions::Command));
        if (response_queue_ == nullptr) {
            kLog.error("No se pudo crear la cola de respuestas hacia el cliente.");
        }
    }
    server_.begin();
}

void Communications::tick() {
    // Outcomes the StateMachine published since the last tick are matched to their slots first, then
    // new traffic is read (which may fill slots of its own), then everything ready goes out in order.
    drain_responses();
    poll_network();
    poll_serial();
    flush_outbound();
}

void Communications::poll_network() {
    // poll() is skipped while unavailable, so the link is re-checked here.
    server_.ensure_connection();
    const bool available = server_.is_available();

    if (!available) {
        if (last_available_) {
            kLog.info("Red desconectada. Pausando sistema.");
            StateMachine::instance().queue_event(definitions::SystemEvent::kCommunicationLost);
            authorizer_.release();
        }
    } else {
        if (!last_available_) {
            kLog.info("Red reconectada. Reanudando sistema.");
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
                active_transport_ = definitions::Transport::kTcp;
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

bool Communications::queue_response(const definitions::Command& response, uint32_t timeout) {
    if (response_queue_ == nullptr) {
        return false;
    }
    // Runs on the StateMachine's task: hand the value over and touch nothing else.
    return xQueueSend(response_queue_, &response, pdMS_TO_TICKS(timeout)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Communications::Communications()
    : serial_send_(nullptr),
      active_responder_(nullptr),
      outbound_(),
      outbound_head_(0),
      outbound_count_(0),
      next_token_(0),
      response_queue_(nullptr),
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

    // Queue the commands not handled above to the StateMachine, holding this client against the
    // token the reply will carry. Refuse up front when no slot is free: the StateMachine tracks the
    // same number of commands, so there would be nowhere to track this one either.
    if (commands_in_flight() >= config::kMaxInFlightCommands) {
        kLog.warning("Comando rechazado: hay demasiados comandos esperando respuesta.");
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorBusy));
        return;
    }

    const uint16_t token = next_token();
    Outbound* slot = claim_outbound(token);
    if (slot == nullptr) {
        kLog.warning("Comando rechazado: no hay lugar en la cola de salida.");
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorBusy));
        return;
    }

    if (!StateMachine::instance().queue_command({cmd->command, extra, token}, config::kCommsQueueTimeoutMs)) {
        // The command never reached the StateMachine, so nothing will fill this slot: answer it here.
        slot->token = 0;
        slot->response = utils::to_scpi(definitions::ResponseType::kErrorUnknownCommand);
        slot->answered = true;
    }
}

uint16_t Communications::next_token() {
    // Skip 0 (untagged) and anything an outstanding command still holds.
    bool in_use = true;
    while (in_use) {
        ++next_token_;
        if (next_token_ == 0) {
            next_token_ = 1;
        }
        in_use = false;
        for (const Outbound& candidate : outbound_) {
            if (candidate.occupied && candidate.token == next_token_) {
                in_use = true;
                break;
            }
        }
    }
    return next_token_;
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

    // Every event gets exactly one reply, so a client matching replies by position stays in step.
    if (!StateMachine::instance().queue_event(evt->event, config::kCommsQueueTimeoutMs)) {
        respond_now(utils::to_scpi(definitions::ResponseType::kErrorBusy));  // queue full: the event may be retried
        return;
    }
    respond_now(utils::to_scpi(definitions::ResponseType::kEventOk));
}

void Communications::respond_now(const std::string& response) {
    Outbound* slot = claim_outbound(0);
    if (slot == nullptr) {
        // No room to keep this in order. Answering out of turn would desynchronise a client that
        // correlates by position, so the message goes unanswered and its own timeout covers it.
        kLog.warning("Sin lugar en la cola de salida: respuesta descartada.");
        return;
    }
    slot->response = response;
    slot->answered = true;
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

std::string Communications::format_response(const definitions::Command& cmd) const {
    std::string response;

    switch (cmd.type) {
        // Boolean state queries: extra = 1 → ON/HIGH, 0 → OFF/LOW
        case definitions::CommandType::kHeaterState:
        case definitions::CommandType::kPumpState:
        case definitions::CommandType::kMixerState:
        case definitions::CommandType::kTempSetIn:  // ManualState answers with the resulting control state
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

    return response;
}

Communications::Outbound* Communications::claim_outbound(uint16_t token) {
    if (outbound_count_ >= outbound_.size()) {
        return nullptr;
    }

    Outbound& slot = outbound_[(outbound_head_ + outbound_count_) % outbound_.size()];
    slot.token = token;
    slot.responder = active_responder_;
    slot.occupied = true;
    slot.answered = false;
    slot.response.clear();
    ++outbound_count_;
    return &slot;
}

void Communications::drain_responses() {
    if (response_queue_ == nullptr) {
        return;
    }

    definitions::Command response{};
    while (xQueueReceive(response_queue_, &response, 0) == pdPASS) {
        Outbound* slot = nullptr;
        for (uint8_t i = 0; i < outbound_count_; ++i) {
            Outbound& candidate = outbound_[(outbound_head_ + i) % outbound_.size()];
            if (candidate.occupied && !candidate.answered && candidate.token == response.token) {
                slot = &candidate;
                break;
            }
        }

        if (slot == nullptr) {
            kLog.warning("Respuesta sin cliente asociado, descartada.");
            continue;
        }

        slot->response = format_response(response);
        slot->answered = true;
    }
}

void Communications::flush_outbound() {
    while (outbound_count_ > 0) {
        Outbound& head = outbound_[outbound_head_];
        if (!head.answered) {
            return;  // an earlier message is still owed a reply; everything behind it waits
        }

        if (head.responder) {
            head.responder(head.response);
        }

        head = Outbound();
        outbound_head_ = static_cast<uint8_t>((outbound_head_ + 1) % outbound_.size());
        --outbound_count_;
    }
}

uint8_t Communications::commands_in_flight() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < outbound_count_; ++i) {
        const Outbound& slot = outbound_[(outbound_head_ + i) % outbound_.size()];
        if (slot.occupied && !slot.answered) {
            ++count;
        }
    }
    return count;
}

}  // namespace modules
