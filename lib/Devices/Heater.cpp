#include "Heater.hpp"

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("Heater");

////////////////////////////////////// Public Methods //////////////////////////////////////

Heater& Heater::instance() {
    static Heater instance;
    return instance;
}

bool Heater::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error("Pin invalido para el calentador.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

bool Heater::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kHeaterOn || type == definitions::CommandType::kHeaterOff ||
           type == definitions::CommandType::kHeaterState;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Heater::Heater() : pin_(0), status_(false) {}

bool Heater::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    switch (cmd.type) {
        case definitions::CommandType::kHeaterOn:
            on();
            break;

        case definitions::CommandType::kHeaterOff:
            off();
            break;

        case definitions::CommandType::kHeaterState:
            break;

        default:
            return false;
    }

    out_response = {cmd.type, 0u};
    return true;
}

void Heater::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void Heater::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool Heater::status() const {
    return status_;
}

uint32_t Heater::pin() const {
    return pin_;
}

}  // namespace modules
