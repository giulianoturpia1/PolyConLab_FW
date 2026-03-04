#include "Communications.hpp"

#include "Definitions.hpp"
#include "Devices.hpp"
#include "Helpers.hpp"
#include "Pump.hpp"
#include "StateMachine.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#endif

#include <algorithm>
#include <array>

namespace modules {

Communications& Communications::instance() {
    static Communications instance;
    return instance;
}

Communications::Communications() : ethernet_response_callback_(nullptr), serial_response_callback_(nullptr) {}

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

void Communications::handle_message(const std::string& msg, const std::function<void(const std::string&)>& respond) {
    if (msg.length() == 0) {
        return;
    }

    if (handle_device_command(msg, respond)) {
        return;
    }

    if (handle_event_command(msg, respond)) {
        return;
    }

    respond(utils::response(definitions::ResponseType::ERROR_UNKNOWN_COMMAND));
}

bool Communications::handle_device_command(const std::string& msg,
                                           const std::function<void(const std::string&)>& respond) {
    const definitions::CommandMap* cmd = utils::find_command(msg);
    if (!cmd) {
        // Check for temperature commands that aren't in the map
        if (msg == "LEER_TEMP_INCAMISA") {
            float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt1));
            respond(std::string(utils::response(definitions::ResponseType::TEMP_IN_PREFIX)) +
                    utils::format_float(temp, 2));
            return true;
        }
        if (msg == "LEER_TEMP_OUTCAMISA") {
            float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt2));
            respond(std::string(utils::response(definitions::ResponseType::TEMP_OUT_PREFIX)) +
                    utils::format_float(temp, 2));
            return true;
        }
        if (msg == "LEER_TEMP_CAMISA") {
            float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt3));
            respond(std::string(utils::response(definitions::ResponseType::TEMP_BATH_PREFIX)) +
                    utils::format_float(temp, 2));
            return true;
        }
        if (msg == "LEER_TEMP_MEZCLA") {
            float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::ResSt4));
            respond(std::string(utils::response(definitions::ResponseType::TEMP_MIX_PREFIX)) +
                    utils::format_float(temp, 2));
            return true;
        }
        return false;
    }

    switch (cmd->command) {
        case definitions::CommandType::HEATER_ON:
            heaterON(static_cast<uint32_t>(definitions::Pins::RelayHeater));
            respond(utils::response(definitions::ResponseType::HEATER_ON));
            return true;

        case definitions::CommandType::HEATER_OFF:
            heaterOFF(static_cast<uint32_t>(definitions::Pins::RelayHeater));
            respond(utils::response(definitions::ResponseType::HEATER_OFF));
            return true;

        case definitions::CommandType::HEATER_STATE: {
            bool state = heaterStatus();
            respond(std::string(utils::response(definitions::ResponseType::HEATER_STATE_PREFIX)) +
                    (state ? utils::response(definitions::ResponseType::STATUS_ON)
                           : utils::response(definitions::ResponseType::STATUS_OFF)));
            return true;
        }

        case definitions::CommandType::PUMP_ON:
            modules::Pump::instance().on();
            respond(utils::response(definitions::ResponseType::PUMP_ON));
            return true;

        case definitions::CommandType::PUMP_OFF:
            modules::Pump::instance().off();
            respond(utils::response(definitions::ResponseType::PUMP_OFF));
            return true;

        case definitions::CommandType::PUMP_STATE: {
            bool state = modules::Pump::instance().status();
            respond(std::string(utils::response(definitions::ResponseType::PUMP_STATE_PREFIX)) +
                    (state ? utils::response(definitions::ResponseType::STATUS_ON)
                           : utils::response(definitions::ResponseType::STATUS_OFF)));
            return true;
        }

        case definitions::CommandType::VALVE_OPEN:
            valveON(static_cast<uint32_t>(definitions::Pins::RelayAus1));
            respond(utils::response(definitions::ResponseType::VALVE_OPEN));
            return true;

        case definitions::CommandType::VALVE_CLOSE:
            valveOFF(static_cast<uint32_t>(definitions::Pins::RelayAus1));
            respond(utils::response(definitions::ResponseType::VALVE_CLOSED));
            return true;

        case definitions::CommandType::VALVE_STATE: {
            bool state = valveStatus(static_cast<uint32_t>(definitions::Pins::RelayAus1));
            respond(std::string(utils::response(definitions::ResponseType::VALVE_STATE_PREFIX)) +
                    (state ? utils::response(definitions::ResponseType::STATUS_OPEN)
                           : utils::response(definitions::ResponseType::STATUS_CLOSED)));
            return true;
        }

        case definitions::CommandType::FLOW_READ: {
            float flow_rate = readFlowMeter();
            respond(std::string(utils::response(definitions::ResponseType::FLOW_PREFIX)) +
                    utils::format_float(flow_rate, 2));
            return true;
        }

        case definitions::CommandType::PING:
            respond(utils::response(definitions::ResponseType::PING_OK));
            return true;

        case definitions::CommandType::STATE_QUERY: {
            auto state = modules::StateMachine::instance().get_current_state();
            respond(std::string(utils::response(definitions::ResponseType::STATE_PREFIX)) +
                    modules::StateMachine::get_state_name(state));
            return true;
        }

        default:
            respond(utils::response(definitions::ResponseType::ERROR_UNKNOWN_COMMAND));
            return false;
    }
}

bool Communications::handle_event_command(const std::string& msg,
                                          const std::function<void(const std::string&)>& respond) {
    if (!utils::starts_with(msg, "EVT:") && !utils::starts_with(msg, "EVENT:")) {
        return false;
    }

    std::string event_name = utils::starts_with(msg, "EVT:") ? msg.substr(4) : msg.substr(6);

    event_name = utils::trim(event_name);
    if (event_name.length() == 0) {
        respond(utils::response(definitions::ResponseType::ERROR_EMPTY_EVENT));
        return true;
    }

    if (!dispatch_event(event_name)) {
        respond(utils::response(definitions::ResponseType::ERROR_UNKNOWN_EVENT));
        return true;
    }

    respond(std::string(utils::response(definitions::ResponseType::EVENT_OK_PREFIX)) + event_name);
    return true;
}

bool Communications::dispatch_event(const std::string& event_name) {
    const auto it =
        std::find_if(definitions::kEventMap.begin(), definitions::kEventMap.end(),
                     [&event_name](const definitions::EventMap& entry) { return event_name == entry.name; });

    if (it != definitions::kEventMap.end()) {
        modules::StateMachine::instance().process_event(it->event);
        return true;
    }

    return false;
}

}  // namespace modules
