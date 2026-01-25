///
///@file App.hpp
///@author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
///@brief Application layer module that manages TCP/Ethernet communication and command processing.
///       Handles incoming network commands, parses them, and orchestrates device control
///       through the Devices module. Provides server initialization and command response formatting
///       for remote reactor control.
///@version 0.1
///@date 2026-01-16
///
///@copyright Copyright (c) 2026
///
///

#pragma once

#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>

#include "Devices.hpp"

int initTCPServer(byte _mac[], IPAddress ip_, EthernetServer& server_);
void processCommand(String msg, EthernetClient client);
