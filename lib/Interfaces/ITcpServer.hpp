///
/// @file ITcpServer.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Transport-agnostic TCP line-server contract for the communications layer.
/// @version 0.1
/// @date 12-08-2026
///
/// @copyright Copyright (c) 2026
///
/// @details
/// The PLC exposes its SCPI command interface over TCP. The link (OSI layers 1-2) may be WiFi
/// (SoftAP, 802.11) or Ethernet (802.3), but both present an identical byte stream above layer 2, so
/// the command layer stays link-agnostic. The transport is chosen at compile time via the TcpServer
/// alias (-DTRANSPORT_ETHERNET), never at run time; Communications binds to that concrete alias (not
/// ITcpServer&), so calls stay devirtualizable.
///

#pragma once

#include <functional>
#include <string>

#include "Definitions.hpp"

namespace modules {

///
/// @brief Contract for a single-client TCP server that reads command lines and forwards them,
/// with a per-client write-back and a stable client identity, independent of the physical link.
///
class ITcpServer {
   public:
    using ClientId = definitions::ClientId;
    using ClientResponseCallback = std::function<void(const std::string&)>;
    using ClientMessageHandler = std::function<void(const std::string&, ClientId, const ClientResponseCallback&)>;

    virtual ~ITcpServer() = default;

    ///
    /// @brief Bring the server up. Reads its own parameters from @c config::.
    /// @return true when the server is listening, false if it failed to start.
    ///
    virtual bool begin() = 0;

    ///
    /// @brief Poll for one client line and dispatch it through @p handler.
    ///
    virtual void poll(const ClientMessageHandler& handler) = 0;

    ///
    /// @brief Ensure the link/hardware is available, recovering the listener if possible.
    /// @return true when the server is usable.
    ///
    virtual bool ensure_connection() = 0;

    ///
    /// @brief Whether the server is currently up and able to serve clients.
    /// Neutral across transports: for a SoftAP this means the AP is up (it has no uplink to lose);
    /// for Ethernet it means the PHY link is up.
    ///
    virtual bool is_available() const = 0;

    ///
    /// @brief Identity of the currently connected client, or 0 when none is connected.
    ///
    /// Single-client contract: at most one client at a time, so this fully describes presence. It lets
    /// the comms layer free the operator role when its holder disconnects — needed because a SoftAP
    /// keeps is_available() true with no client, so the link-down release never fires over WiFi.
    ///
    /// @return ClientId of the connected client, or 0 if there is none.
    ///
    virtual ClientId connected_client() const = 0;
};

}  // namespace modules
