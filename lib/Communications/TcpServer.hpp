///
/// @file TcpServer.hpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Compile-time selection of the active ITcpServer transport (WiFi or Ethernet).
/// @version 0.1
/// @date 12-08-2026
///
/// @copyright Copyright (c) 2026
///
/// @details
///
/// The transport is fixed for a given build via -DTRANSPORT_ETHERNET
/// and cannot change at run time. Only the selected server's translation unit is
/// compiled; Communications depends on the @c TcpServer alias, never on a concrete WiFi/Ethernet
/// name, so switching transports is a one-line change here.
///

#pragma once

#include "WifiServer.hpp"
#ifdef TRANSPORT_ETHERNET
#include "EthernetServer.hpp"
#else
#endif

namespace modules {

#ifdef TRANSPORT_ETHERNET
using TcpServer = EthernetServer;
#else
using TcpServer = WifiServer;
#endif

}  // namespace modules
