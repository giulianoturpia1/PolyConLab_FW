#include "LocalServer.hpp"

#include "Communications.hpp"
#include "HALGeneral.hpp"
#include "Helpers.hpp"

namespace modules {

LocalServer& LocalServer::instance() {
	static LocalServer instance;
	return instance;
}

LocalServer::LocalServer() : server_(nullptr), initialized_(false) {}

void LocalServer::initialize(EthernetServer& server) {
	server_ = &server;
	initialized_ = false;
}

bool LocalServer::begin(byte mac[], IPAddress ip) {
	Ethernet.begin(mac, ip);

	if (Ethernet.hardwareStatus() == EthernetNoHardware) {
		modules::hal_general().log("Ethernet shield was not found. Sorry, can't run without hardware.");
		initialized_ = false;
		return false;
	}

	if (Ethernet.linkStatus() == LinkOFF) {
		modules::hal_general().log("Ethernet cable is not connected.");
		initialized_ = false;
		return false;
	}

	server_->begin();
	std::string msg = "Server is at ";
	IPAddress local_ip = Ethernet.localIP();
	for (int i = 0; i < 4; i++) {
		msg += std::to_string(local_ip[i]);
		if (i < 3) msg += ".";
	}
	modules::hal_general().log(msg);
	initialized_ = true;
	return true;
}

void LocalServer::poll(Communications& handler) {
	if (!initialized_) {
		return;
	}

	EthernetClient client = server_->available();
	if (!client || !client.connected() || !client.available()) {
		return;
	}

	String arduino_msg = client.readStringUntil('\n');
	std::string msg(arduino_msg.c_str());
	msg = utils::trim(msg);
	if (msg.length() == 0) {
		return;
	}

	modules::hal_general().log("New client message: " + msg);

	// Set up a callback that sends responses back to this specific client
	handler.set_ethernet_response_callback([&client](const std::string& response) {
		if (client.connected()) {
			client.println(response.c_str());
			modules::hal_general().log("Response sent: " + response);
		}
	});

	handler.handle_ethernet_message(msg);
}

}  // namespace modules
