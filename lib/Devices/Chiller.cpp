#include "Chiller.hpp"

#include "HALGeneral.hpp"

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Chiller& modules::Chiller::instance() {
    static Chiller instance;
    return instance;
}

void modules::Chiller::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_OUTPUT);
    off();
}

void modules::Chiller::on() {
    modules::hal_general().digital_write(pin_, modules::PIN_HIGH);
    status_ = true;
}

void modules::Chiller::off() {
    modules::hal_general().digital_write(pin_, modules::PIN_LOW);
    status_ = false;
}

bool modules::Chiller::status() const {
    return status_;
}

uint32_t modules::Chiller::pin() const {
    return pin_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::Chiller::Chiller() : pin_(0), status_(false) {}
