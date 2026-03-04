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

#include <functional>

namespace modules {

///
/// @brief Ethernet server wrapper that reads lines and forwards them to a caller-provided handler.
/// Implemented as a singleton.
///
class LocalServer {
   public:
    using ResponseCallback = std::function<void(const std::string&)>;
    using MessageHandler = std::function<void(const std::string&, const ResponseCallback&)>;

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
    bool begin(const byte mac[6], const IPAddress& ip);

    ///
    /// @brief Poll for new client data and dispatch messages through the provided handler.
    ///
    void poll(const MessageHandler& handler);

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
    byte mac_[6];
    IPAddress ip_;
};

}  // namespace modules
