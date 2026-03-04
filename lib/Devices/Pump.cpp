#include "Pump.hpp"

#include "HALGeneral.hpp"

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Pump& modules::Pump::instance() {
    static Pump instance;
    return instance;
}

modules::Pump::Pump() : pin_(0), status_(false) {}

void modules::Pump::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_OUTPUT);
    off();
}

void modules::Pump::on() {
    modules::hal_general().digital_write(pin_, modules::PIN_HIGH);
    status_ = true;
}

void modules::Pump::off() {
    modules::hal_general().digital_write(pin_, modules::PIN_LOW);
    status_ = false;
}

bool modules::Pump::status() const {
    return status_;
}

uint32_t modules::Pump::pin() const {
    return pin_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////
