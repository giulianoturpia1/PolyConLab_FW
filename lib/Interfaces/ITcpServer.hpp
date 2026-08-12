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
/// Context: the PLC exposes its SCPI command interface over a TCP socket. The physical link
/// (OSI layers 1-2) may be WiFi (SoftAP, 802.11) or Ethernet (802.3); above layer 2 both
/// present an identical TCP byte stream, so the command layer must not care which is wired.
///
/// Theactive transport is chosen at COMPILE time through the TcpServer alias (-DTRANSPORT_ETHERNET),
/// never at run time.. The interface forces both concretes to expose the same operations, so switching the alias
/// target is safe; Communications binds to the concrete alias (not ITcpServer&), keeping calls
/// devirtualizable.
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
};

}  // namespace modules
