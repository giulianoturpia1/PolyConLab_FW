#include "WifiServer.hpp"

#include <Arduino.h>
#include <WiFi.h>

#include <memory>

#include "Config.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

static WiFiServer gWifiServer(config::kWifiServerPort);

// Persistent handle to the single associated client (kApMaxClients == 1), held across polls so its
// identity stays stable and its disconnect is observable on an idle tick. While it is connected it
// owns the sole session; reject_extra_clients() closes any other socket the host opens.
static WiFiClient gClient;

namespace modules {

static const TaggedLogger kLog("WifiServer");

////////////////////////////////////// Helpers //////////////////////////////////////

///
/// @brief Convert a 4-byte config array into an Arduino IPAddress.
///
static IPAddress to_ip(const std::array<uint8_t, 4>& octets) {
    return IPAddress(octets[0], octets[1], octets[2], octets[3]);
}

///
/// @brief Accept and close every surplus TCP socket beyond the primary session.
///
/// kApMaxClients == 1 caps 802.11 associations at one host, but that host can still open several TCP
/// sockets. Closing each extra keeps the guarantee single-client at the transport layer, not just the
/// radio, and stops them lingering in the LwIP backlog. Called only while gClient is connected, under
/// client_mutex_ (the caller holds it).
///
static void reject_extra_clients() {
    for (WiFiClient extra = gWifiServer.available(); extra.connected(); extra = gWifiServer.available()) {
        extra.stop();
        kLog.warning("Conexion TCP adicional rechazada: la sesion WiFi ya esta en uso por otro cliente.");
    }
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
/// The lambda captures the client by shared_ptr so it outlives the deferred response round-trip
/// (command → StateMachine → response), and takes client_mutex before writing so it is safe to call
/// from any task — including the StateMachine task running concurrently with the App task.
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

    // Drain up to kMaxLinesPerPollTick lines per tick. A client that sends a burst — a round of
    // sensor queries, say — is read in one tick instead of one line per tick, while the bound keeps
    // a busy client from holding the App task for an unbounded stretch.
    for (uint8_t line = 0; line < config::kMaxLinesPerPollTick; ++line) {
        if (xSemaphoreTake(client_mutex_, pdMS_TO_TICKS(config::kClientMutexPollTimeoutMs)) != pdTRUE) {
            kLog.warning("poll: no se pudo tomar el mutex de cliente, saltando tick.");
            return;
        }

        // Adopt the next arrival only when we have no live client; once connected, service it directly so
        // its identity stays stable (kApMaxClients == 1), and close any surplus socket so the session is
        // single-client at the transport layer, not merely single-association at the 802.11 layer.
        if (!gClient.connected()) {
            gClient = gWifiServer.available();
        } else {
            reject_extra_clients();
        }

        // Refresh the cached identity every tick — including idle ones — so connected_client() reports
        // an accurate presence and the comms layer can free the operator role when the client leaves.
        connected_client_ = gClient.connected() ? static_cast<ClientId>(gClient.remoteIP()) : 0;

        if (!gClient.connected() || !gClient.available()) {
            xSemaphoreGive(client_mutex_);
            return;  // nothing more buffered: the tick is done
        }

        String arduino_msg = gClient.readStringUntil('\n');
        const ClientId client_id = connected_client_;
        xSemaphoreGive(client_mutex_);  // Release before handler — no more client I/O needed here.

        std::string msg(arduino_msg.c_str());
        msg = utils::trim(msg);
        if (msg.length() == 0) {
            kLog.warning("poll: mensaje recibido pero vacio tras acondicionamiento, descartando.");
            continue;  // keep draining: the next line may be valid
        }

        kLog.debug("Nuevo Mensaje del Cliente: " + msg);

        auto client_ptr = std::make_shared<WiFiClient>(gClient);
        ClientResponseCallback respond = create_response_callback(client_ptr, client_mutex_);

        handler(msg, client_id, respond);
    }
}

WifiServer::ClientId WifiServer::connected_client() const {
    return connected_client_;
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

WifiServer::WifiServer() : is_initialized_(false), wifi_link_up_(false), client_mutex_(xSemaphoreCreateMutex()), connected_client_(0) {}

}  // namespace modules
