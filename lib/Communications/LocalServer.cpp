#include "LocalServer.hpp"

#include "HALGeneral.hpp"
#include "Helpers.hpp"

namespace modules {

LocalServer& LocalServer::instance() {
	static LocalServer instance;
	return instance;
}

LocalServer::LocalServer()
    : server_(nullptr), initialized_(false), mac_{0, 0, 0, 0, 0, 0}, ip_(0, 0, 0, 0) {}

void LocalServer::initialize(EthernetServer& server) {
	server_ = &server;
	initialized_ = false;
}

bool LocalServer::begin(const byte mac[6], const IPAddress& ip) {
	for (int i = 0; i < 6; ++i) {
		mac_[i] = mac[i];
	}
	ip_ = ip;

	Ethernet.begin(mac_, ip_);

	if (Ethernet.hardwareStatus() == EthernetNoHardware) {
		modules::hal_general().log("Ethernet shield was not found. Can't run without hardware.");
		initialized_ = false;
		return false;
	}

	if (Ethernet.linkStatus() == LinkOFF) {
		modules::hal_general().log("Ethernet cable is not connected.");
		initialized_ = false;
		return false;
	}

	server_->begin();
	std::string msg = "Server is at: ";
	IPAddress local_ip = Ethernet.localIP();
	for (int i = 0; i < 4; i++) {
		msg += std::to_string(local_ip[i]);
		if (i < 3) msg += ".";
	}
	modules::hal_general().log(msg);
	initialized_ = true;
	return true;
}

void LocalServer::poll(const MessageHandler& handler) {
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

	ResponseCallback respond = [&client](const std::string& response) {
		if (client.connected()) {
			client.println(response.c_str());
			modules::hal_general().log("Response sent: " + response);
		}
	};

	handler(msg, respond);
}

}  // namespace modules
