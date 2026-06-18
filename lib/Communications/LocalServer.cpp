#include "LocalServer.hpp"

#include <Arduino.h>
#include <Ethernet.h>

#include <memory>

#include "Config.hpp"
#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

constexpr uint16_t kLocalServerPort = 80;
static EthernetServer gEthernetServer(kLocalServerPort);

namespace modules {

static const TaggedLogger kLog("LocalServer");

////////////////////////////////////// Public Methods //////////////////////////////////////

LocalServer& LocalServer::instance() {
    static LocalServer instance;
    return instance;
}

bool LocalServer::begin(const std::array<uint8_t, 6>& mac, const std::array<uint8_t, 4>& ip) {
    Ethernet.begin(const_cast<uint8_t*>(mac.data()), const_cast<uint8_t*>(ip.data()));

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
/// The returned lambda captures the client by shared_ptr so it stays alive for the
/// duration of the deferred response round-trip (command → StateMachine → response).
/// It acquires spi_mutex before touching the W5x00 so it is safe to call from any
/// task, including the StateMachine task running concurrently with the app task.
///
/// @param client    Shared pointer to the EthernetClient that sent the command.
/// @param spi_mutex FreeRTOS mutex that serialises all W5x00 SPI access.
/// @return LocalServer::ClientResponseCallback Callable that writes one response line to that client.
///
static LocalServer::ClientResponseCallback create_response_callback(std::shared_ptr<EthernetClient> client, SemaphoreHandle_t spi_mutex) {
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

void LocalServer::poll(const ClientMessageHandler& handler) {
    // Check link / hardware status first — these are read-only PHY queries and do
    // not need the mutex because no other task sends SPI frames at this point.
    if (!ensure_connection()) {
        kLog.warning("poll: ensure_connection fallo, Ethernet no disponible.");
        return;
    }

    // Acquire the mutex only for the actual W5x00 data I/O so the response
    // callback (called from the SM task) cannot interleave SPI frames with us.
    if (xSemaphoreTake(spi_mutex_, pdMS_TO_TICKS(config::kSpiMutexPollTimeoutMs)) != pdTRUE) {
        kLog.warning("poll: no se pudo tomar el mutex SPI, saltando tick.");
        return;
    }

    EthernetClient client = gEthernetServer.available();
    if (!client) {
        // kLog.debug("poll: ningun cliente conectado.");
        xSemaphoreGive(spi_mutex_);
        return;
    }
    if (!client.connected()) {
        // kLog.warning("poll: cliente presente pero no conectado (socket cerrado).");
        xSemaphoreGive(spi_mutex_);
        return;
    }
    if (!client.available()) {
        // kLog.debug("poll: cliente conectado pero sin datos disponibles.");
        xSemaphoreGive(spi_mutex_);
        return;
    }

    String arduino_msg = client.readStringUntil('\n');
    xSemaphoreGive(spi_mutex_);  // Release before handler — no more SPI needed here.

    std::string msg(arduino_msg.c_str());
    msg = utils::trim(msg);
    if (msg.length() == 0) {
        kLog.warning("poll: mensaje recibido pero vacio tras acondicionamiento, descartando.");
        return;
    }

    kLog.debug("Nuevo Mensaje del Cliente: " + msg);

    auto client_ptr = std::make_shared<EthernetClient>(client);
    ClientResponseCallback respond = create_response_callback(client_ptr, spi_mutex_);

    handler(msg, respond);
}

bool LocalServer::ensure_connection() {
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

bool LocalServer::is_link_up() const {
    return ethernet_link_up_ && is_initialized_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

void LocalServer::restart_server() {
    gEthernetServer.begin();
    is_initialized_ = true;
}

LocalServer::LocalServer() : is_initialized_(false), ethernet_link_up_(false), spi_mutex_(xSemaphoreCreateMutex()) {}

}  // namespace modules
