#include "LocalServer.hpp"

#include <Arduino.h>
#include <Ethernet.h>

#include <memory>

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
/// Calling it sends the SCPI response string over the TCP connection, provided the
/// client is still connected at delivery time. Silently discards if the connection
/// has closed.
///
/// @param client Shared pointer to the EthernetClient that sent the command.
/// @return LocalServer::ClientResponseCallback Callable that writes one response line to that client.
///
static LocalServer::ClientResponseCallback create_response_callback(std::shared_ptr<EthernetClient> client) {
    return [client](const std::string& response) {
        if (client->connected()) {
            client->println(response.c_str());
            kLog.debug("Respuesta Enviada: " + response);
        }
    };
}

void LocalServer::poll(const ClientMessageHandler& handler) {
    if (!ensure_connection()) {
        return;
    }

    EthernetClient client = gEthernetServer.available();
    if (!client || !client.connected() || !client.available()) {
        return;
    }

    String arduino_msg = client.readStringUntil('\n');
    std::string msg(arduino_msg.c_str());
    msg = utils::trim(msg);
    if (msg.length() == 0) {
        return;
    }

    kLog.debug("Nuevo Mensaje del Cliente: " + msg);

    auto client_ptr = std::make_shared<EthernetClient>(client);
    ClientResponseCallback respond = create_response_callback(client_ptr);

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

LocalServer::LocalServer() : is_initialized_(false), ethernet_link_up_(false) {}

}  // namespace modules
