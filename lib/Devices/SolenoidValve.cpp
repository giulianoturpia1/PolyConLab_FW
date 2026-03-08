#include "SolenoidValve.hpp"

#include <freertos/task.h>

#include <algorithm>

#include "HALGeneral.hpp"

constexpr UBaseType_t kSolenoidValveTaskPriority = 2;

struct ValveCommand {
    definitions::CommandType command;
    uint32_t pin;
};

void modules::SolenoidValve::task_entry(void* param) {
    SolenoidValve* self = static_cast<SolenoidValve*>(param);
    ValveCommand command = {definitions::CommandType::kValveClose, 0};

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            if (command.command == definitions::CommandType::kValveOpen) {
                self->open(command.pin);
            } else {
                self->close(command.pin);
            }
        }
    }
}

////////////////////////////////////// Public Methods //////////////////////////////////////

namespace modules {

SolenoidValve& SolenoidValve::instance() {
    static SolenoidValve instance;
    return instance;
}

void SolenoidValve::begin(const uint32_t* pins, size_t count) {
    valve_count_ = 0;

    if (pins == nullptr || count == 0) {
        hal_general().log("No se añadieron pines de valvulas para la inicializacion.");
        return;
    }

    const uint8_t bounded_count =
        static_cast<uint8_t>(std::min(count, static_cast<size_t>(definitions::kMaxSolenoidValves)));
    for (uint8_t index = 0; index < bounded_count; ++index) {
        add(pins[index]);
    }
}

bool SolenoidValve::add(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (valve_count_ >= definitions::kMaxSolenoidValves) {
        hal_general().log("No se pudo agregar el pin de valvula: " + std::to_string(pin) +
                          ". Se alcanzo el limite de capacidad.");
        return false;  // Capacity limit reached or pin not already registered
    }

    if (index < definitions::kMaxSolenoidValves) {
        hal_general().log("El pin de valvula ya estaba registrado: " + std::to_string(pin));
        return true;  // Pin already registered, consider it a success
    }

    valves_[valve_count_].pin = pin;
    valves_[valve_count_].status = false;
    hal_general().pin_mode(pin, PIN_MODE_OUTPUT);  // Valves are output devices
    // This depends whether the hardware is active HIGH or LOW. Assuming active HIGH for opening the valve, we set it to
    // LOW by default.

    close(pin);  // Ensure valve is closed by default
    ++valve_count_;
    return true;
}

bool SolenoidValve::remove(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Se intento eliminar un pin de valvula no registrado: " + std::to_string(pin));
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
        hal_general().log("Se intento abrir un pin de valvula no registrado: " + std::to_string(pin));
        return false;  // Pin not found
    }

    hal_general().digital_write(pin, PIN_HIGH);
    valves_[static_cast<size_t>(index)].status = true;
    return true;
}

bool SolenoidValve::close(uint32_t pin) {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Se intento cerrar un pin de valvula no registrado: " + std::to_string(pin));
        return false;  // Pin not found
    }

    hal_general().digital_write(pin, PIN_LOW);
    valves_[static_cast<size_t>(index)].status = false;
    return true;
}

bool SolenoidValve::status(uint32_t pin) const {
    uint8_t index = find_index(pin);
    if (index >= definitions::kMaxSolenoidValves) {
        hal_general().log("Se intento leer el estado de un pin de valvula no registrado: " + std::to_string(pin));
        return false;
    }

    return valves_[static_cast<size_t>(index)].status;
}

uint8_t SolenoidValve::count() const {
    return valve_count_;
}

void SolenoidValve::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(8, sizeof(ValveCommand));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola de valvulas.");
        return;
    }

    if (xTaskCreate(task_entry, "valve_task", 3072, this, kSolenoidValveTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea de valvulas.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool SolenoidValve::send_command(definitions::CommandType command, uint32_t pin, uint32_t timeout_ms) {
    if (command != definitions::CommandType::kValveOpen && command != definitions::CommandType::kValveClose) {
        return false;
    }

    if (queue_ == nullptr) {
        if (command == definitions::CommandType::kValveOpen) {
            open(pin);
        } else {
            close(pin);
        }
        return false;
    }

    const ValveCommand queue_command = {command, pin};
    if (xQueueSend(queue_, &queue_command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        if (command == definitions::CommandType::kValveOpen) {
            open(pin);
        } else {
            close(pin);
        }
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

SolenoidValve::SolenoidValve() : valves_({}), valve_count_(0), queue_(nullptr) {}

uint8_t SolenoidValve::find_index(uint32_t pin) const {
    for (uint8_t index = 0; index < valve_count_; ++index) {
        if (valves_[index].pin == pin) {
            return index;
        }
    }

    return definitions::kMaxSolenoidValves;  // Return an out-of-range index to indicate not found
}

}  // namespace modules
