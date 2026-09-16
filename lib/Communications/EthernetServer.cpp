#include "EthernetServer.hpp"

#include <Arduino.h>
#include <Ethernet.h>

#include <array>
#include <memory>

#include "Config.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

// Arduino's own class (Ethernet.h) is also named EthernetServer. Bind the alias to the global
// ::EthernetServer explicitly so it can never be confused with modules::EthernetServer (this class).
using ArduinoEthernetServer = ::EthernetServer;
static ArduinoEthernetServer gEthernetServer(config::kEthernetPort);

// Persistent handle to the single connected client, held across polls so its identity stays stable
// and its disconnect is observable on an idle tick. While it is connected it owns the sole session;
// reject_extra_clients() closes any other socket that connects.
static EthernetClient gClient;

namespace modules {

static const TaggedLogger kLog("EthernetServer");

///
/// @brief Accept and close every surplus TCP socket beyond the primary session.
///
/// A wired LAN has no association cap, so several hosts can reach the listener at once. accept()
/// hands back each newly established socket exactly once; the primary is skipped via operator==
/// (it compares socket index) and every other socket is stopped. Called only while gClient is
/// connected, under spi_mutex_ (the caller holds it).
///
static void reject_extra_clients() {
    for (EthernetClient extra = gEthernetServer.accept(); extra; extra = gEthernetServer.accept()) {
        if (extra == gClient) {
            continue;  // accept() may return the primary socket once — never close it.
        }
        extra.stop();
        kLog.warning("Conexion TCP adicional rechazada: la sesion Ethernet ya esta en uso por otro cliente.");
    }
}

////////////////////////////////////// Public Methods //////////////////////////////////////

EthernetServer& EthernetServer::instance() {
    static EthernetServer instance;
    return instance;
}

bool EthernetServer::begin() {
    std::array<uint8_t, 6> mac = config::kEthernetMac;
    std::array<uint8_t, 4> ip = config::kEthernetIp;
    Ethernet.begin(mac.data(), ip.data());

    if (!ensure_connection()) {
        return false;
    }

    gEthernetServer.begin();

    std::string msg = "Server is at: ";
    IPAddress local_ip = Ethernet.localIP();
    for (int i = 0; i < 4; i++) {
        msg += std::to_string(local_ip[i]);
        if (i < 3)
            msg += ".";
    }
    kLog.info(msg);

    ethernet_link_up_ = true;
    is_initialized_ = true;
    return true;
}

///
/// @brief Build a write-back callable bound to a specific connected TCP client.
///
/// The lambda captures the client by shared_ptr so it outlives the deferred response round-trip
/// (command → StateMachine → response), and takes spi_mutex before touching the W5x00 so it is safe
/// to call from any task — including the StateMachine task running concurrently with the App task.
///
/// @param client    Shared pointer to the EthernetClient that sent the command.
/// @param spi_mutex FreeRTOS mutex that serialises all W5x00 SPI access.
/// @return EthernetServer::ClientResponseCallback Callable that writes one response line to that client.
///
static EthernetServer::ClientResponseCallback create_response_callback(std::shared_ptr<EthernetClient> client,
                                                                       SemaphoreHandle_t spi_mutex) {
    return [client, spi_mutex](const std::string& response) {
        if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(config::kSpiMutexCallbackTimeoutMs)) != pdTRUE) {
            kLog.warning("callback: no se pudo tomar el mutex SPI, respuesta descartada: " + response);
            return;
        }
        if (!client->connected()) {
            kLog.warning("callback: Cliente desconectado, respuesta descartada: " + response);
            xSemaphoreGive(spi_mutex);
            return;
        }
        client->println(response.c_str());
        kLog.debug("Respuesta Enviada: " + response);
        xSemaphoreGive(spi_mutex);
    };
}

void EthernetServer::poll(const ClientMessageHandler& handler) {
    if (!ensure_connection()) {
        kLog.warning("poll: ensure_connection fallo, Ethernet no disponible.");
        return;
    }

    // Drain up to kMaxLinesPerPollTick lines per tick, mirroring WifiServer::poll: a burst of queries
    // is read in one tick instead of one line per tick, bounded so one client cannot hold the App task.
    for (uint8_t line = 0; line < config::kMaxLinesPerPollTick; ++line) {
        if (xSemaphoreTake(spi_mutex_, pdMS_TO_TICKS(config::kSpiMutexPollTimeoutMs)) != pdTRUE) {
            kLog.warning("poll: no se pudo tomar el mutex SPI, saltando tick.");
            return;
        }

        // Adopt the next arrival only when we have no live client; once connected, service it directly so
        // its identity stays stable, and close any surplus socket so the session is single-client at the
        // transport layer (a wired LAN, unlike the WiFi SoftAP, has no association cap of its own).
        if (!gClient.connected()) {
            gClient = gEthernetServer.available();
        } else {
            reject_extra_clients();
        }

        // Refresh the cached identity every tick — including idle ones — so connected_client() reports
        // an accurate presence and the comms layer can free the operator role when the client leaves.
        connected_client_ = gClient.connected() ? static_cast<ClientId>(gClient.remoteIP()) : 0;

        if (!gClient.connected() || !gClient.available()) {
            xSemaphoreGive(spi_mutex_);
            return;  // nothing more buffered: the tick is done
        }

        String arduino_msg = gClient.readStringUntil('\n');
        const ClientId client_id = connected_client_;
        xSemaphoreGive(spi_mutex_);  // Release before handler — no more SPI needed here.

        std::string msg(arduino_msg.c_str());
        msg = utils::trim(msg);
        if (msg.length() == 0) {
            kLog.warning("poll: mensaje recibido pero vacio tras acondicionamiento, descartando.");
            continue;  // keep draining: the next line may be valid
        }

        kLog.debug("Nuevo Mensaje del Cliente: " + msg);

        auto client_ptr = std::make_shared<EthernetClient>(gClient);
        ClientResponseCallback respond = create_response_callback(client_ptr, spi_mutex_);

        handler(msg, client_id, respond);
    }
}

EthernetServer::ClientId EthernetServer::connected_client() const {
    return connected_client_;
}

bool EthernetServer::ensure_connection() {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        if (is_initialized_) {
            kLog.error("Se perdio el shield Ethernet durante ejecucion.");
        }
        is_initialized_ = false;
        ethernet_link_up_ = false;
        return false;
    }

    const bool is_link_up_now = Ethernet.linkStatus() != LinkOFF;
    if (!is_link_up_now) {
        if (ethernet_link_up_) {
            kLog.info("Conexion Ethernet perdida.");
        }
        is_initialized_ = false;
        ethernet_link_up_ = false;
        return false;
    }

    if (!ethernet_link_up_) {
        kLog.info("Conexion Ethernet restablecida.");
        restart_server();
    }
    ethernet_link_up_ = true;

    return true;
}

bool EthernetServer::is_available() const {
    return ethernet_link_up_ && is_initialized_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

void EthernetServer::restart_server() {
    gEthernetServer.begin();
    is_initialized_ = true;
}

EthernetServer::EthernetServer()
    : is_initialized_(false), ethernet_link_up_(false), spi_mutex_(xSemaphoreCreateMutex()), connected_client_(0) {}

}  // namespace modules
