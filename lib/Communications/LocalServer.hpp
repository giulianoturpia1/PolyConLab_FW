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
    using ClientResponseCallback = std::function<void(const std::string&)>;
    using ClientMessageHandler = std::function<void(const std::string&, const ClientResponseCallback&)>;

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
    void poll(const ClientMessageHandler& handler);

    ///
    /// @brief Check Ethernet link health and recover server after reconnection when possible.
    ///
    /// @return true when Ethernet hardware and link are available, false otherwise.
    ///
    bool ensure_connection();

    ///
    /// @brief Return latest known Ethernet availability status.
    ///
    bool is_link_up() const;

   private:
    LocalServer();
    // Rule of Five
    ~LocalServer() = default;
    LocalServer(const LocalServer&) = delete;
    LocalServer& operator=(const LocalServer&) = delete;
    LocalServer(LocalServer&&) = delete;
    LocalServer& operator=(LocalServer&&) = delete;

    EthernetServer* ethernet_server_;
    bool is_initialized_;
    bool ethernet_link_up_;
    byte ethernet_mac_address_[6];
    IPAddress ethernet_ip_address_;
};

}  // namespace modules
