///
/// @file LocalServer.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Ethernet local server interface for command reception.
/// @version 0.1
/// @date 26-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>

namespace modules {

class Communications;

///
/// @brief Ethernet server wrapper that reads lines and forwards them to the communications module.
/// Implemented as a singleton.
///
class LocalServer {
   public:
	///
	/// @brief Get the singleton instance of LocalServer
	///
	static LocalServer& instance();

	///
	/// @brief Initialize the server with an EthernetServer instance
	///
	void initialize(EthernetServer& server);

	///
	/// @brief Start the Ethernet server and validate link/hardware.
	///
	bool begin(byte mac[], IPAddress ip);

	///
	/// @brief Poll for new client data.
	///
	void poll(Communications& handler);

   private:
	LocalServer();
	// Rule of Five
	~LocalServer() = default;
	LocalServer(const LocalServer&) = delete;
	LocalServer& operator=(const LocalServer&) = delete;
	LocalServer(LocalServer&&) = delete;
	LocalServer& operator=(LocalServer&&) = delete;

	EthernetServer* server_;
	bool initialized_;
};

}  // namespace modules
