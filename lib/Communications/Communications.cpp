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

    respond(utils::response(definitions::ResponseType::kErrorUnknownCommand));
}

bool Communications::handle_device_command(const std::string& msg,
                                           const std::function<void(const std::string&)>& respond) {
    const definitions::CommandMap* cmd = utils::find_command(msg);
    if (!cmd) {
        // Check for temperature commands that aren't in the map
        if (msg == "LEER_TEMP_INCAMISA") {
                float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::kTempSensor1));
                respond(std::string(utils::response(definitions::ResponseType::kTempInPrefix)) +
                    utils::format_float(temp, 2));
            return true;
        }
        if (msg == "LEER_TEMP_OUTCAMISA") {
                float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::kTempSensor2));
                respond(std::string(utils::response(definitions::ResponseType::kTempOutPrefix)) +
                    utils::format_float(temp, 2));
            return true;
        }
        if (msg == "LEER_TEMP_CAMISA") {
                float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::kTempSensor3));
                respond(std::string(utils::response(definitions::ResponseType::kTempBathPrefix)) +
                    utils::format_float(temp, 2));
            return true;
        }
        if (msg == "LEER_TEMP_MEZCLA") {
                float temp = readTempSensor(static_cast<uint32_t>(definitions::Pins::kTempSensor4));
                respond(std::string(utils::response(definitions::ResponseType::kTempMixPrefix)) +
                    utils::format_float(temp, 2));
            return true;
        }
        return false;
    }

    switch (cmd->command) {
        case definitions::CommandType::kHeaterOn:
            heaterON(static_cast<uint32_t>(definitions::Pins::kHeater));
            respond(utils::response(definitions::ResponseType::kHeaterOn));
            return true;

        case definitions::CommandType::kHeaterOff:
            heaterOFF(static_cast<uint32_t>(definitions::Pins::kHeater));
            respond(utils::response(definitions::ResponseType::kHeaterOff));
            return true;

        case definitions::CommandType::kHeaterState: {
            bool state = heaterStatus();
            respond(std::string(utils::response(definitions::ResponseType::kHeaterStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOn)
                           : utils::response(definitions::ResponseType::kStatusOff)));
            return true;
        }

        case definitions::CommandType::kPumpOn:
            modules::Pump::instance().on();
            respond(utils::response(definitions::ResponseType::kPumpOn));
            return true;

        case definitions::CommandType::kPumpOff:
            modules::Pump::instance().off();
            respond(utils::response(definitions::ResponseType::kPumpOff));
            return true;

        case definitions::CommandType::kPumpState: {
            bool state = modules::Pump::instance().status();
            respond(std::string(utils::response(definitions::ResponseType::kPumpStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOn)
                           : utils::response(definitions::ResponseType::kStatusOff)));
            return true;
        }

        case definitions::CommandType::kValveOpen:
            valveON(static_cast<uint32_t>(definitions::Pins::kValve1));
            respond(utils::response(definitions::ResponseType::kValveOpen));
            return true;

        case definitions::CommandType::kValveClose:
            valveOFF(static_cast<uint32_t>(definitions::Pins::kValve1));
            respond(utils::response(definitions::ResponseType::kValveClosed));
            return true;

        case definitions::CommandType::kValveState: {
            bool state = valveStatus(static_cast<uint32_t>(definitions::Pins::kValve1));
            respond(std::string(utils::response(definitions::ResponseType::kValveStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOpen)
                           : utils::response(definitions::ResponseType::kStatusClosed)));
            return true;
        }

        case definitions::CommandType::kFlowRead: {
            float flow_rate = readFlowMeter();
            respond(std::string(utils::response(definitions::ResponseType::kFlowPrefix)) +
                    utils::format_float(flow_rate, 2));
            return true;
        }

        case definitions::CommandType::kPing:
            respond(utils::response(definitions::ResponseType::kPingOk));
            return true;

        case definitions::CommandType::kStateQuery: {
            auto state = modules::StateMachine::instance().get_current_state();
            respond(std::string(utils::response(definitions::ResponseType::kStatePrefix)) +
                    modules::StateMachine::get_state_name(state));
            return true;
        }

        default:
            respond(utils::response(definitions::ResponseType::kErrorUnknownCommand));
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
        respond(utils::response(definitions::ResponseType::kErrorEmptyEvent));
        return true;
    }

    if (!dispatch_event(event_name)) {
        respond(utils::response(definitions::ResponseType::kErrorUnknownEvent));
        return true;
    }

    respond(std::string(utils::response(definitions::ResponseType::kEventOkPrefix)) + event_name);
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

Communications::Communications() : ethernet_response_callback_(nullptr), serial_response_callback_(nullptr) {}

}  // namespace modules
