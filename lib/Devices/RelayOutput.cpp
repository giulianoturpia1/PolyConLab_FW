#include "RelayOutput.hpp"

#include <string>

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("RelayOutput");

////////////////////////////////////// Public Methods //////////////////////////////////////

RelayOutput& RelayOutput::heater() {
    static RelayOutput instance("HEATER", definitions::CommandType::kHeaterOn, definitions::CommandType::kHeaterOff,
                                definitions::CommandType::kHeaterState, config::kHeaterRelay);
    return instance;
}

RelayOutput& RelayOutput::pump() {
    static RelayOutput instance("PUMP", definitions::CommandType::kPumpOn, definitions::CommandType::kPumpOff,
                                definitions::CommandType::kPumpState, config::kPumpRelay);
    return instance;
}

RelayOutput& RelayOutput::mixer() {
    static RelayOutput instance("MIXER", definitions::CommandType::kMixerOn, definitions::CommandType::kMixerOff,
                                definitions::CommandType::kMixerState, config::kMixerRelay);
    return instance;
}

bool RelayOutput::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error(std::string("Pin invalido para ") + name_ + ".");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    write_output(relay_.safe_on);
    return true;
}

bool RelayOutput::accepts(definitions::CommandType type) const {
    return type == on_cmd_ || type == off_cmd_ || type == state_cmd_;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

RelayOutput::RelayOutput(const char* name, definitions::CommandType on_cmd, definitions::CommandType off_cmd,
                         definitions::CommandType state_cmd, const config::RelayConfig& relay)
    : name_(name), on_cmd_(on_cmd), off_cmd_(off_cmd), state_cmd_(state_cmd), relay_(relay), pin_(0), status_(false) {}

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
    write_output(true);
}

void RelayOutput::off() {
    write_output(false);
}

void RelayOutput::write_output(bool on) {
    // NO contact: energising the coil (HIGH) powers the load. NC contact: the load is powered
    // while the coil is de-energised (LOW), so the level is inverted.
    const bool pin_high = (on != relay_.normally_closed);
    hal_general().digital_write(pin_, pin_high ? PIN_HIGH : PIN_LOW);
    status_ = on;
}

bool RelayOutput::status() const {
    return status_;
}

uint32_t RelayOutput::pin() const {
    return pin_;
}

}  // namespace modules
