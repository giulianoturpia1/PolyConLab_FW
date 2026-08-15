#include "Pump.hpp"

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("Pump");

////////////////////////////////////// Public Methods //////////////////////////////////////

Pump& Pump::instance() {
    static Pump instance;
    return instance;
}

bool Pump::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error("Pin invalido para la bomba.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

void Pump::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void Pump::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool Pump::status() const {
    return status_;
}

uint32_t Pump::pin() const {
    return pin_;
}

bool Pump::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kPumpOn || type == definitions::CommandType::kPumpOff ||
           type == definitions::CommandType::kPumpState;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Pump::Pump() : pin_(0), status_(false) {}

bool Pump::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    switch (cmd.type) {
        case definitions::CommandType::kPumpOn:
            on();
            break;

        case definitions::CommandType::kPumpOff:
            off();
            break;

        case definitions::CommandType::kPumpState:
            break;

        default:
            return false;
    }

    out_response = {cmd.type, 0u};
    return true;
}

}  // namespace modules
