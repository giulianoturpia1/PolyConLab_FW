#include "RelayOutput.hpp"

#include <string>

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("RelayOutput");

////////////////////////////////////// Public Methods //////////////////////////////////////

RelayOutput& RelayOutput::mixer() {
    static RelayOutput instance("MIXER", definitions::CommandType::kMixerOn, definitions::CommandType::kMixerOff,
                                definitions::CommandType::kMixerState);
    return instance;
}

bool RelayOutput::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error(std::string("Pin invalido para ") + name_ + ".");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

bool RelayOutput::accepts(definitions::CommandType type) const {
    return type == on_cmd_ || type == off_cmd_ || type == state_cmd_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

RelayOutput::RelayOutput(const char* name, definitions::CommandType on_cmd, definitions::CommandType off_cmd,
                         definitions::CommandType state_cmd)
    : name_(name), on_cmd_(on_cmd), off_cmd_(off_cmd), state_cmd_(state_cmd), pin_(0), status_(false) {}

bool RelayOutput::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    if (cmd.type == on_cmd_) {
        on();
    } else if (cmd.type == off_cmd_) {
        off();
    } else if (cmd.type == state_cmd_) {
        // No actuation; report the current status below.
    } else {
        return false;
    }

    out_response = {cmd.type, status() ? 1u : 0u};
    return true;
}

void RelayOutput::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void RelayOutput::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool RelayOutput::status() const {
    return status_;
}

uint32_t RelayOutput::pin() const {
    return pin_;
}

}  // namespace modules
