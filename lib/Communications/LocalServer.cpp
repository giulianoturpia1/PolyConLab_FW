#include "LocalServer.hpp"

#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("LocalServer");

////////////////////////////////////// Public Methods //////////////////////////////////////

LocalServer& LocalServer::instance() {
    static LocalServer instance;
    return instance;
}

void LocalServer::initialize(EthernetServer& server) {
    ethernet_server_ = &server;
    is_initialized_ = false;
}

bool LocalServer::begin(const std::array<byte, 6>& mac, const IPAddress& ip) {
    ethernet_mac_address_ = mac;
    ethernet_ip_address_ = ip;
    Ethernet.begin(ethernet_mac_address_.data(), ethernet_ip_address_);

    if (!ensure_connection()) {
        return false;
    }

    ethernet_server_->begin();
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

void LocalServer::poll(const ClientMessageHandler& handler) {
    if (!ensure_connection()) {
        return;
    }

    EthernetClient client = ethernet_server_->available();
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

LocalServer::ClientResponseCallback LocalServer::create_response_callback(std::shared_ptr<EthernetClient> client) {
    return [client](const std::string& response) {
        if (client->connected()) {
            client->println(response.c_str());
            kLog.debug("Respuesta Enviada: " + response);
        }
    };
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
    ethernet_server_->begin();
    is_initialized_ = true;
}

LocalServer::LocalServer()
    : ethernet_server_(nullptr),
      is_initialized_(false),
      ethernet_link_up_(false),
      ethernet_mac_address_{},
      ethernet_ip_address_(0, 0, 0, 0) {}

}  // namespace modules
