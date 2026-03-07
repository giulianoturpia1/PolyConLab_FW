///
/// @file Heater.cpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Heater control implementation for the reactor system.
/// @version 0.1
/// @date 24-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#include "Heater.hpp"

#include "HALGeneral.hpp"

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Heater& modules::Heater::instance() {
    static Heater instance;
    return instance;
}

modules::Heater::Heater() : pin_(0), status_(false) {}

void modules::Heater::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_OUTPUT);
    off();
}

void modules::Heater::on() {
    modules::hal_general().digital_write(pin_, modules::PIN_HIGH);
    status_ = true;
}

void modules::Heater::off() {
    modules::hal_general().digital_write(pin_, modules::PIN_LOW);
    status_ = false;
}

bool modules::Heater::status() const {
    return status_;
}

uint32_t modules::Heater::pin() const {
    return pin_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////
