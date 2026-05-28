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

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

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
    /// @brief Start the Ethernet server and validate link/hardware.
    /// Accepts MAC address and IP address as plain byte arrays (no Arduino types required).
    ///
    bool begin(const std::array<uint8_t, 6>& mac, const std::array<uint8_t, 4>& ip);

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

    ///
    /// @brief Restart the TCP listener after a link recovery. Does not re-configure MAC/IP.
    ///
    void restart_server();

    bool is_initialized_;
    bool ethernet_link_up_;
    SemaphoreHandle_t spi_mutex_;  ///< Protects all W5x00 SPI calls from concurrent task access.
};

}  // namespace modules
