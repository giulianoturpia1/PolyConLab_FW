#include "Communications.hpp"

#include "Chiller.hpp"
#include "Definitions.hpp"
#include "Devices.hpp"
#include "Helpers.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"
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
    // Extract base command (before comma, if present, as some commands have format "DEVICE_CMD,N")
    std::string base_command = msg;
    size_t comma_pos = msg.find(',');
    if (comma_pos != std::string::npos) {
        base_command = msg.substr(0, comma_pos);
    }

    const definitions::CommandMap* cmd = utils::find_command(base_command);
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
        case definitions::CommandType::kHeaterOn: {
            heaterON(static_cast<uint32_t>(definitions::Pins::kHeater));
            respond(std::string(utils::response(definitions::ResponseType::kHeaterOn)));
            return true;
        }

        case definitions::CommandType::kHeaterOff: {
            heaterOFF(static_cast<uint32_t>(definitions::Pins::kHeater));
            respond(std::string(utils::response(definitions::ResponseType::kHeaterOff)));
            return true;
        }

        case definitions::CommandType::kHeaterState: {
            bool state = heaterStatus();
            respond(std::string(utils::response(definitions::ResponseType::kHeaterStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOn)
                           : utils::response(definitions::ResponseType::kStatusOff)));
            return true;
        }

        case definitions::CommandType::kPumpOn: {
            modules::Pump::instance().on();
            respond(std::string(utils::response(definitions::ResponseType::kPumpOn)));
            return true;
        }

        case definitions::CommandType::kPumpOff: {
            modules::Pump::instance().off();
            respond(std::string(utils::response(definitions::ResponseType::kPumpOff)));
            return true;
        }

        case definitions::CommandType::kPumpState: {
            bool state = modules::Pump::instance().status();
            respond(std::string(utils::response(definitions::ResponseType::kPumpStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOn)
                           : utils::response(definitions::ResponseType::kStatusOff)));
            return true;
        }

        case definitions::CommandType::kChillerOn: {
            modules::Chiller::instance().on();
            respond(std::string(utils::response(definitions::ResponseType::kChillerOn)));
            return true;
        }

        case definitions::CommandType::kChillerOff: {
            modules::Chiller::instance().off();
            respond(std::string(utils::response(definitions::ResponseType::kChillerOff)));
            return true;
        }

        case definitions::CommandType::kChillerState: {
            bool state = modules::Chiller::instance().status();
            respond(std::string(utils::response(definitions::ResponseType::kChillerStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOn)
                           : utils::response(definitions::ResponseType::kStatusOff)));
            return true;
        }

        case definitions::CommandType::kValveOpen: {
            uint32_t valve_pin = parse_valve_pin(msg);
            size_t comma_pos = msg.find(',');
            char valve_num = (comma_pos != std::string::npos) ? msg[comma_pos + 1]
                                                              : '0';  // Assume a single digit valve number for response
            modules::SolenoidValve::instance().open(valve_pin);
            respond(std::string(utils::response(definitions::ResponseType::kValveOpen)) + std::string(1, valve_num));
            return true;
        }

        case definitions::CommandType::kValveClose: {
            uint32_t valve_pin = parse_valve_pin(msg);
            size_t comma_pos = msg.find(',');
            char valve_num = (comma_pos != std::string::npos) ? msg[comma_pos + 1]
                                                              : '0';  // Assume a single digit valve number for response
            modules::SolenoidValve::instance().close(valve_pin);
            respond(std::string(utils::response(definitions::ResponseType::kValveClosed)) + std::string(1, valve_num));
            return true;
        }

        case definitions::CommandType::kValveState: {
            uint32_t valve_pin = parse_valve_pin(msg);
            size_t comma_pos = msg.find(',');
            char valve_num = (comma_pos != std::string::npos) ? msg[comma_pos + 1]
                                                              : '0';  // Assume a single digit valve number for response
            bool state = modules::SolenoidValve::instance().status(valve_pin);
            respond(std::string(utils::response(definitions::ResponseType::kValveStatePrefix)) +
                    (state ? utils::response(definitions::ResponseType::kStatusOpen)
                           : utils::response(definitions::ResponseType::kStatusClosed)) +
                    std::string(1, valve_num));
            return true;
        }

        case definitions::CommandType::kFlowRead: {
            float flow_rate = readFlowMeter();
            respond(std::string(utils::response(definitions::ResponseType::kFlowPrefix)) +
                    utils::format_float(flow_rate, 2));
            return true;
        }

        case definitions::CommandType::kPing: {
            respond(std::string(utils::response(definitions::ResponseType::kPingOk)));
            return true;
        }

        case definitions::CommandType::kStateQuery: {
            auto state = modules::StateMachine::instance().get_current_state();
            respond(std::string(utils::response(definitions::ResponseType::kStatePrefix)) +
                    modules::StateMachine::get_state_name(state));
            return true;
        }

        default: {
            respond(std::string(utils::response(definitions::ResponseType::kErrorUnknownCommand)));
            return false;
        }
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
        respond(std::string(utils::response(definitions::ResponseType::kErrorEmptyEvent)));
        return true;
    }

    if (!dispatch_event(event_name)) {
        respond(std::string(utils::response(definitions::ResponseType::kErrorUnknownEvent)));
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
