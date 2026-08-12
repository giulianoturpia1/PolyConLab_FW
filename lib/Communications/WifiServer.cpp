#include "WifiServer.hpp"

#include <Arduino.h>
#include <WiFi.h>

#include <memory>

#include "Config.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

static WiFiServer gWifiServer(config::kWifiServerPort);

namespace modules {

static const TaggedLogger kLog("WifiServer");

////////////////////////////////////// Helpers //////////////////////////////////////

///
/// @brief Convert a 4-byte config array into an Arduino IPAddress.
///
static IPAddress to_ip(const std::array<uint8_t, 4>& octets) {
    return IPAddress(octets[0], octets[1], octets[2], octets[3]);
}

////////////////////////////////////// Public Methods //////////////////////////////////////

WifiServer& WifiServer::instance() {
    static WifiServer instance;
    return instance;
}

bool WifiServer::begin() {
    WiFi.mode(WIFI_AP);

    if (!WiFi.softAPConfig(to_ip(config::kApIp), to_ip(config::kApGateway), to_ip(config::kApSubnet))) {
        kLog.warning("softAPConfig fallo: no se pudo aplicar la IP del punto de acceso.");
    }

    if (!WiFi.softAP(config::kApSsid, config::kApPassword, config::kApChannel, config::kApHidden, config::kApMaxClients)) {
        kLog.error("No se pudo iniciar el punto de acceso WiFi.");
        wifi_link_up_ = false;
        is_initialized_ = false;
        return false;
    }

    gWifiServer.begin();

    std::string msg = "Access Point '";
    msg += config::kApSsid;
    msg += "' activo en: ";
    IPAddress ap_ip = WiFi.softAPIP();
    for (int i = 0; i < 4; i++) {
        msg += std::to_string(ap_ip[i]);
        if (i < 3)
            msg += ".";
    }
    kLog.info(msg);

    wifi_link_up_ = true;
    is_initialized_ = true;
    return true;
}

///
/// @brief Build a write-back callable bound to a specific connected TCP client.
///
/// The returned lambda captures the client by shared_ptr so it stays alive for the
/// duration of the deferred response round-trip (command → StateMachine → response).
/// It acquires client_mutex before writing so it is safe to call from any task,
/// including the StateMachine task running concurrently with the App task.
///
/// @param client       Shared pointer to the WiFiClient that sent the command.
/// @param client_mutex FreeRTOS mutex that serialises WiFiClient writes.
/// @return WifiServer::ClientResponseCallback Callable that writes one response line to that client.
///
static WifiServer::ClientResponseCallback create_response_callback(std::shared_ptr<WiFiClient> client, SemaphoreHandle_t client_mutex) {
    return [client, client_mutex](const std::string& response) {
        if (xSemaphoreTake(client_mutex, pdMS_TO_TICKS(config::kClientMutexCallbackTimeoutMs)) != pdTRUE) {
            kLog.warning("callback: no se pudo tomar el mutex de cliente, respuesta descartada: " + response);
            return;
        }
        if (!client->connected()) {
            kLog.warning("callback: Cliente desconectado, respuesta descartada: " + response);
            xSemaphoreGive(client_mutex);
            return;
        }
        client->println(response.c_str());
        kLog.debug("Respuesta Enviada: " + response);
        xSemaphoreGive(client_mutex);
    };
}

void WifiServer::poll(const ClientMessageHandler& handler) {
    if (!ensure_connection()) {
        kLog.warning("poll: ensure_connection fallo, WiFi no disponible.");
        return;
    }

    if (xSemaphoreTake(client_mutex_, pdMS_TO_TICKS(config::kClientMutexPollTimeoutMs)) != pdTRUE) {
        kLog.warning("poll: no se pudo tomar el mutex de cliente, saltando tick.");
        return;
    }

    WiFiClient client = gWifiServer.available();
    if (!client) {
        xSemaphoreGive(client_mutex_);
        return;
    }
    if (!client.connected()) {
        xSemaphoreGive(client_mutex_);
        return;
    }
    if (!client.available()) {
        xSemaphoreGive(client_mutex_);
        return;
    }

    String arduino_msg = client.readStringUntil('\n');
    xSemaphoreGive(client_mutex_);  // Release before handler — no more client I/O needed here.

    std::string msg(arduino_msg.c_str());
    msg = utils::trim(msg);
    if (msg.length() == 0) {
        kLog.warning("poll: mensaje recibido pero vacio tras acondicionamiento, descartando.");
        return;
    }

    kLog.debug("Nuevo Mensaje del Cliente: " + msg);

    auto client_ptr = std::make_shared<WiFiClient>(client);
    ClientResponseCallback respond = create_response_callback(client_ptr, client_mutex_);

    const ClientId client_id = static_cast<ClientId>(client.remoteIP());

    handler(msg, client_id, respond);
}

bool WifiServer::ensure_connection() {
    // A SoftAP has no uplink to associate with: once started it stays up until the
    // PLC resets. Availability therefore tracks whether begin() brought the AP up.
    return is_initialized_;
}

bool WifiServer::is_available() const {
    return wifi_link_up_ && is_initialized_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

WifiServer::WifiServer() : is_initialized_(false), wifi_link_up_(false), client_mutex_(xSemaphoreCreateMutex()) {}

}  // namespace modules
