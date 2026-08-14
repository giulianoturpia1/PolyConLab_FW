///
/// @file EthernetServer.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Ethernet (over SPI) TCP server — one ITcpServer transport, selected at compile time.
/// @version 0.1
/// @date 12-08-2026
///
/// @copyright Copyright (c) 2026
///
///

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstdint>
#include <functional>
#include <string>

#include "ITcpServer.hpp"

namespace modules {

///
/// @brief Ethernet TCP server that reads lines and forwards them to a caller-provided handler.
/// Implemented as a singleton. Selected via the TcpServer alias when -DTRANSPORT_ETHERNET is set.
///
/// @note Unlike the WiFi SoftAP, a wired link can genuinely drop, so is_available() reflects the PHY
/// link state (and ensure_connection() restarts the listener after a recovery). The module lives on
/// the SPI bus, so a spi_mutex_ serialises all chip access across the App and StateMachine tasks.
///
class EthernetServer final : public ITcpServer {
   public:
    ///
    /// @brief Get the singleton instance of EthernetServer.
    ///
    static EthernetServer& instance();

    ///
    /// @brief Start the Ethernet interface and TCP server. MAC, static IP and port are read from
    /// @c config::.
    ///
    /// @return true when the link is up and the server is listening, false otherwise.
    ///
    bool begin() override;

    ///
    /// @brief Poll for new client data and dispatch messages through the provided handler.
    ///
    void poll(const ClientMessageHandler& handler) override;

    ///
    /// @brief Check Ethernet link/hardware health and recover the listener after reconnection.
    ///
    /// @return true when the hardware and link are available, false otherwise.
    ///
    bool ensure_connection() override;

    ///
    /// @brief Whether the Ethernet PHY link and hardware are up.
    ///
    bool is_available() const override;

    ///
    /// @brief Identity of the currently connected client, or 0 when none is connected.
    ///
    /// Refreshed every poll() (including idle ticks) from the persistent client handle, so the
    /// communications layer can detect that the operator's client has left and release the role.
    ///
    ClientId connected_client() const override;

   private:
    EthernetServer();
    ~EthernetServer() = default;
    EthernetServer(const EthernetServer&) = delete;
    EthernetServer& operator=(const EthernetServer&) = delete;
    EthernetServer(EthernetServer&&) = delete;
    EthernetServer& operator=(EthernetServer&&) = delete;

    ///
    /// @brief Restart the TCP listener after a link recovery. Does not re-configure MAC/IP.
    ///
    void restart_server();

    bool is_initialized_;
    bool ethernet_link_up_;
    SemaphoreHandle_t spi_mutex_;  ///< Serialises W5x00 SPI access across the App and StateMachine tasks.
    ClientId connected_client_;    ///< Cached identity of the connected client (0 = none); refreshed each poll().
};

}  // namespace modules
