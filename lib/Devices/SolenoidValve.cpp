#include "SolenoidValve.hpp"

#include <algorithm>

#include "HALGeneral.hpp"
#include "Logger.hpp"

namespace modules {

static const TaggedLogger kLog("SolenoidValve");

////////////////////////////////////// Public Methods //////////////////////////////////////

SolenoidValve& SolenoidValve::instance() {
    static SolenoidValve instance;
    return instance;
}

void SolenoidValve::begin(const uint32_t* pins, size_t count) {
    valve_count_ = 0;

    if (pins == nullptr || count == 0) {
        kLog.warning("No se añadieron pines de valvulas para la inicializacion.");
        return;
    }

    const uint8_t bounded_count = static_cast<uint8_t>(std::min(count, static_cast<size_t>(definitions::kMaxSolenoidValves)));
    for (uint8_t index = 0; index < bounded_count; ++index) {
        add(pins[index]);
    }
}

bool SolenoidValve::add(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (valve_count_ >= definitions::kMaxSolenoidValves) {
        kLog.error("No se pudo agregar el pin de valvula: " + std::to_string(pin) + ". Se alcanzo el limite de capacidad.");
        return false;  // Capacity limit reached or pin not already registered
    }

    if (index < definitions::kMaxSolenoidValves) {
        kLog.warning("El pin de valvula ya estaba registrado: " + std::to_string(pin));
        return true;  // Pin already registered, consider it a success
    }

    valves_[valve_count_].pin = pin;
    valves_[valve_count_].status = false;
    hal_general().pin_mode(pin, PIN_MODE_OUTPUT);  // Valves are output devices
    // This depends whether the hardware is active HIGH or LOW. Assuming active HIGH for opening the valve, we set it to
    // LOW by default.
    ++valve_count_;
    close(pin);  // Ensure valve is closed by default
    return true;
}

bool SolenoidValve::remove(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        kLog.warning("Se intento eliminar un pin de valvula no registrado: " + std::to_string(pin));
        return false;  // Pin not found
    }

    close(pin);  // Ensure valve is closed before removal

    // Shift remaining valves to fill the gap
    for (uint8_t i = index; i < valve_count_ - 1; ++i) {
        valves_[i] = valves_[i + 1];
    }
    --valve_count_;
    return true;
}

bool SolenoidValve::open(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        kLog.warning("Se intento abrir un pin de valvula no registrado: " + std::to_string(pin));
        return false;  // Pin not found
    }

    hal_general().digital_write(pin, PIN_HIGH);
    valves_[static_cast<size_t>(index)].status = true;
    return true;
}

bool SolenoidValve::close(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        kLog.warning("Se intento cerrar un pin de valvula no registrado: " + std::to_string(pin));
        return false;  // Pin not found
    }

    hal_general().digital_write(pin, PIN_LOW);
    valves_[static_cast<size_t>(index)].status = false;
    return true;
}

bool SolenoidValve::status(uint32_t pin) const {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        kLog.warning("Se intento leer el estado de un pin de valvula no registrado: " + std::to_string(pin));
        return false;
    }

    return valves_[static_cast<size_t>(index)].status;
}

uint8_t SolenoidValve::count() const {
    return valve_count_;
}

bool SolenoidValve::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kValveOpen || type == definitions::CommandType::kValveClose ||
           type == definitions::CommandType::kValveState;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

SolenoidValve::SolenoidValve() : valves_({}), valve_count_(0) {}

uint8_t SolenoidValve::find_index(uint32_t pin) const {
    for (uint8_t index = 0; index < valve_count_; ++index) {
        if (valves_[index].pin == pin) {
            return index;
        }
    }

    return definitions::kMaxSolenoidValves;  // Return an out-of-range index to indicate not found
}

bool SolenoidValve::on_command(const definitions::Command& cmd, definitions::Command& out_response) {
    const uint32_t pin = cmd.extra;
    switch (cmd.type) {
        case definitions::CommandType::kValveOpen:
            if (!open(pin)) {
                return false;
            }
            out_response = {definitions::CommandType::kValveOpen, pin};
            return true;

        case definitions::CommandType::kValveClose:
            if (!close(pin)) {
                return false;
            }
            out_response = {definitions::CommandType::kValveClose, pin};
            return true;

        case definitions::CommandType::kValveState:
            out_response = {definitions::CommandType::kValveState, pin};
            return true;

        default:
            return false;
    }
}

}  // namespace modules
