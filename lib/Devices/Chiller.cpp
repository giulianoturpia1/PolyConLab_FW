#include "Chiller.hpp"

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("Chiller");

////////////////////////////////////// Public Methods //////////////////////////////////////

Chiller& Chiller::instance() {
    static Chiller instance;
    return instance;
}

bool Chiller::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error("Pin invalido para el chiller.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

bool Chiller::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kChillerOn || type == definitions::CommandType::kChillerOff ||
           type == definitions::CommandType::kChillerState;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Chiller::Chiller() : pin_(0), status_(false) {}

bool Chiller::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    switch (cmd.type) {
        case definitions::CommandType::kChillerOn:
            on();
            break;

        case definitions::CommandType::kChillerOff:
            off();
            break;

        case definitions::CommandType::kChillerState:
            break;

        default:
            return false;
    }

    out_response = {cmd.type, 0u};
    return true;
}

void Chiller::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void Chiller::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool Chiller::status() const {
    return status_;
}

uint32_t Chiller::pin() const {
    return pin_;
}

}  // namespace modules
