#include "LevelSwitch.hpp"

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("LevelSwitch");

////////////////////////////////////// Public Methods //////////////////////////////////////

LevelSwitch& LevelSwitch::instance() {
    static LevelSwitch instance;
    return instance;
}

bool LevelSwitch::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error("Pin invalido para el interruptor de nivel.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_INPUT);
    status_ = hal_general().digital_read(pin_);
    return true;
}

bool LevelSwitch::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kLevelSwitchState;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

LevelSwitch::LevelSwitch() : pin_(0), status_(false) {}

bool LevelSwitch::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    if (cmd.type != definitions::CommandType::kLevelSwitchState) {
        return false;
    }

    // Invertimos la lectura física porque el sensor es activo-bajo.
    const uint32_t level_state = hal_general().digital_read(pin_) == PIN_LOW ? PIN_HIGH : PIN_LOW;
    out_response = {definitions::CommandType::kLevelSwitchState, level_state};
    return true;
}

bool LevelSwitch::status() const {
    // El sensor de nivel está activo-bajo: 0V significa cerrado/alto lógico.
    return hal_general().digital_read(pin_) == PIN_LOW;
}

uint32_t LevelSwitch::pin() const {
    return pin_;
}

}  // namespace modules
