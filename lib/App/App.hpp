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
#include <SPI.h>
#include <Ethernet.h>
#include "Devices.hpp"
// Pins definitions
// ------- Actuators pins -------
#define RELAYHEATER A1_0
#define RELAYBOMBA A1_0 
// Valves pins
#define RELAY_AUS_1 R1_4
#define RELAY_AUS_2 R1_5
#define RELAY_AUS_3 R1_6

//------- Sensors pins -------
#define RES_ST_1 T1
#define RES_ST_2 T2
#define RES_ST_3 T3
#define RES_ST_4 T4

int initServidorTCP(byte _mac[], IPAddress ip_, EthernetServer& server_);
void procesarComando(String msg, EthernetClient client);