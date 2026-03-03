#include "Communications.hpp"

#include <array>

#include "Definitions.hpp"
#include "Devices.hpp"
#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "StateMachine.hpp"

namespace modules {

Communications& Communications::instance() {
	static Communications instance;
	return instance;
}

Communications::Communications()
    : ethernet_response_callback_(nullptr), serial_response_callback_(nullptr) {}

void Communications::set_ethernet_response_callback(
    const std::function<void(const std::string&)>& callback) {
	ethernet_response_callback_ = callback;
}

void Communications::set_serial_response_callback(
    const std::function<void(const std::string&)>& callback) {
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

	respond("ERROR:UNKNOWN_COMMAND");
}

bool Communications::handle_device_command(const std::string& msg, const std::function<void(const std::string&)>& respond) {
	const definitions::CommandMap* cmd = utils::find_command(msg);
	if (!cmd) {
		// Check for temperature commands that aren't in the map
		if (msg == "LEER_TEMP_INCAMISA") {
			float temp = readTempSensor(RES_ST_1);
			respond(std::string("TEMP_IN:") + utils::format_float(temp, 2));
			return true;
		}
		if (msg == "LEER_TEMP_OUTCAMISA") {
			float temp = readTempSensor(RES_ST_2);
			respond(std::string("TEMP_OUT:") + utils::format_float(temp, 2));
			return true;
		}
		if (msg == "LEER_TEMP_CAMISA") {
			float temp = readTempSensor(RES_ST_3);
			respond(std::string("TEMP_BATH:") + utils::format_float(temp, 2));
			return true;
		}
		if (msg == "LEER_TEMP_MEZCLA") {
			float temp = readTempSensor(RES_ST_4);
			respond(std::string("TEMP_MIX:") + utils::format_float(temp, 2));
			return true;
		}
		return false;
	}

	switch (cmd->command) {
		case definitions::CommandType::HEATER_ON:
			heaterON(RELAYHEATER);
			respond("RES:ON");
			return true;

		case definitions::CommandType::HEATER_OFF:
			heaterOFF(RELAYHEATER);
			respond("RES:OFF");
			return true;

		case definitions::CommandType::HEATER_STATE: {
			bool state = heaterStatus();
			respond(std::string("RES:STATE,") + (state ? "ON" : "OFF"));
			return true;
		}

		case definitions::CommandType::PUMP_ON:
			pumpON(RELAYPUMP);
			respond("BBA:ON");
			return true;

		case definitions::CommandType::PUMP_OFF:
			pumpOFF(RELAYPUMP);
			respond("BBA:OFF");
			return true;

		case definitions::CommandType::PUMP_STATE: {
			bool state = statusPump();
			respond(std::string("BBA:STATE,") + (state ? "ON" : "OFF"));
			return true;
		}

		case definitions::CommandType::VALVE_OPEN:
			valveON(RELAY_AUS_1);
			respond("VALVE:OPEN");
			return true;

		case definitions::CommandType::VALVE_CLOSE:
			valveOFF(RELAY_AUS_1);
			respond("VALVE:CLOSED");
			return true;

		case definitions::CommandType::VALVE_STATE: {
			bool state = valveStatus(RELAY_AUS_1);
			respond(std::string("VALVE:STATE,") + (state ? "OPEN" : "CLOSED"));
			return true;
		}

		case definitions::CommandType::FLOW_READ: {
			float flow_rate = readFlowMeter();
			respond(std::string("CAUDAL:") + utils::format_float(flow_rate, 2));
			return true;
		}

		case definitions::CommandType::PING:
			respond("PONG");
			return true;

		case definitions::CommandType::STATE_QUERY: {
			auto state = modules::StateMachine::instance().get_current_state();
			respond(std::string("STATE:") + modules::StateMachine::get_state_name(state));
			return true;
		}

		default:
			return false;
	}

}

bool Communications::handle_event_command(const std::string& msg, const std::function<void(const std::string&)>& respond) {
	if (!utils::starts_with(msg, "EVT:") && !utils::starts_with(msg, "EVENT:")) {
		return false;
	}

	std::string event_name = msg;
	if (utils::starts_with(msg, "EVT:")) {
		event_name = msg.substr(4);
	} else {
		event_name = msg.substr(6);
	}

	event_name = utils::trim(event_name);
	if (event_name.length() == 0) {
		respond("ERROR:EMPTY_EVENT");
		return true;
	}

	if (!dispatch_event(event_name)) {
		respond("ERROR:UNKNOWN_EVENT");
		return true;
	}

	respond(std::string("EVENT:OK,") + event_name);
	return true;
}

bool Communications::dispatch_event(const std::string& event_name) {
	for (const auto& entry : definitions::kEventMap) {
		if (event_name == entry.name) {
			modules::StateMachine::instance().process_event(entry.event);
			return true;
		}
	}

	return false;
}

}// namespace modules
