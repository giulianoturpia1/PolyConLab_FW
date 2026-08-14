///
/// @file WifiServer.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief WiFi (ESP32 native radio) local server interface for command reception.
/// @version 0.1
/// @date 06-08-2026
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
/// @brief WiFi access-point (SoftAP) TCP server that reads lines and forwards them to a
/// caller-provided handler. Implemented as a singleton.
///
/// The PLC hosts its own WiFi network with a built-in DHCP server, so operator/observer devices
/// connect directly to the PLC without an external router. Exposes a transport-agnostic
/// request/response contract so the @c Communications module is decoupled from the link layer.
/// @c client_mutex_ serialises @c WiFiClient writes because the response callback runs on the
/// StateMachine task while @c poll() runs on the App task.
///
/// @note Unlike a station, an access point has no uplink to lose: once started it stays up until
/// the PLC resets. @c is_available() therefore reports whether the AP was brought up, not client
/// presence — so the "communication lost" pause no longer triggers on a client disconnect.
///
/// @note Single-client on two layers: @c config::kApMaxClients == 1 caps the SoftAP at one 802.11
/// station (so a second host cannot join despite the shared password), and @c poll() closes any
/// surplus socket the lone host opens — so exactly one SCADA client is ever served at a time.
///
class WifiServer final : public ITcpServer {
   public:
    ///
    /// @brief Get the singleton instance of WifiServer.
    ///
    static WifiServer& instance();

    ///
    /// @brief Start the WiFi access point (static AP IP + built-in DHCP server) and start the TCP
    /// server. SSID, password, channel, client cap and AP IP are read from @c config::.
    ///
    /// @return true when the AP is up and the server is listening, false if the AP failed to start.
    ///
    bool begin() override;

    ///
    /// @brief Poll for new client data and dispatch messages through the provided handler.
    ///
    void poll(const ClientMessageHandler& handler) override;

    ///
    /// @brief Check that the access point is running. An AP has no uplink to lose, so this simply
    /// reports whether begin() successfully brought the AP up.
    ///
    /// @return true when the AP is up, false otherwise.
    ///
    bool ensure_connection() override;

    ///
    /// @brief Whether the access point is up — reflects whether begin() succeeded, since a SoftAP has
    /// no uplink to lose.
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
    WifiServer();
    // Rule of Five
    ~WifiServer() = default;
    WifiServer(const WifiServer&) = delete;
    WifiServer& operator=(const WifiServer&) = delete;
    WifiServer(WifiServer&&) = delete;
    WifiServer& operator=(WifiServer&&) = delete;

    bool is_initialized_;
    bool wifi_link_up_;
    SemaphoreHandle_t client_mutex_;  ///< Serialises WiFiClient writes across the App and StateMachine tasks.
    ClientId connected_client_;       ///< Cached identity of the connected client (0 = none); refreshed each poll().
};

}  // namespace modules
