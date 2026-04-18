#include "Thermocouple.hpp"

#include <freertos/task.h>

#include <algorithm>
#include <cmath>

#include "HALGeneral.hpp"

constexpr uint32_t kThermocoupleTimeoutMs = 20;
constexpr UBaseType_t kThermocoupleTaskPriority = 2;
constexpr UBaseType_t kThermocoupleQueueLength = 8;
constexpr uint32_t kThermocoupleTaskStackSize = 3072;
constexpr char kThermocoupleTaskName[] = "thermocouple_task";

struct ThermocoupleCommand {
    definitions::CommandType command;
    uint32_t address;
};

////////////////////////////////////// Public Methods //////////////////////////////////////

namespace modules {

Thermocouple& Thermocouple::instance() {
    static Thermocouple instance;
    return instance;
}

void Thermocouple::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

void Thermocouple::begin(const uint32_t* adds, size_t count) {
    thermoc_count_ = 0;

    if (adds == nullptr || count == 0) {
        hal_general().log("No se añadieron direcciones de thermocuplas para la inicializacion.");
        return;
    }

    const uint8_t bounded_count = static_cast<uint8_t>(std::min(count, static_cast<size_t>(definitions::kMaxThermocouples)));
    for (uint8_t index = 0; index < bounded_count; ++index) {
        add(adds[index]);
    }
}

bool Thermocouple::add(uint32_t address) {
    uint8_t index = find_index(address);
    if (thermoc_count_ >= definitions::kMaxThermocouples) {
        hal_general().log("No se pudo agregar la direccion de la termocupla: " + std::to_string(address) +
                          ". Se alcanzo el limite de capacidad.");
        return false;  // Capacity limit reached
    }

    if (index < thermoc_count_) {
        hal_general().log("La termocupla ya estaba registrada: " + std::to_string(address));
        return true;  // Address already registered, consider it a success
    }

    // Initialize MCP9600 at the current slot
    uint8_t slot_index = thermoc_count_;
    if (!thermocouples_[slot_index].device.begin(address)) {
        hal_general().log("No se pudo iniciar la termocupla en la direccion: " + std::to_string(address));
        return false;
    }

    // Store the address and mark as active
    thermocouples_[slot_index].address = address;
    thermocouples_[slot_index].status = true;
    ++thermoc_count_;

    // Configure the thermocouple with default settings
    thermocouples_[slot_index].device.setThermocoupleType(MCP9600_TYPE_K);
    thermocouples_[slot_index].device.setAmbientResolution(RES_ZERO_POINT_0625);
    thermocouples_[slot_index].device.setADCresolution(MCP9600_ADCRESOLUTION_18);  // 12, 14, 16, or 18
    thermocouples_[slot_index].device.setFilterCoefficient(3);
    thermocouples_[slot_index].device.enable(true);

    return true;
}

bool Thermocouple::remove(uint32_t address) {
    uint8_t index = find_index(address);
    if (index >= definitions::kMaxThermocouples) {
        hal_general().log("Se intento eliminar una direccion no registrada: " + std::to_string(address));
        return false;  // Address not found
    }

    // Shift remaining thermocouples to fill the gap
    for (uint8_t i = index; i < thermoc_count_ - 1; ++i) {
        thermocouples_[i] = std::move(thermocouples_[i + 1]);
    }
    --thermoc_count_;
    return true;
}

float Thermocouple::readCelcius(uint32_t address) {
    uint8_t index = find_index(address);
    if (index >= definitions::kMaxThermocouples) {
        hal_general().log("Se intento leer una direccion no registrada: " + std::to_string(address));
        return NAN;  // Return NaN if address not found
    }

    // Return the thermocouple temperature reading in Celsius
    return thermocouples_[index].device.readThermocouple();
}

uint8_t Thermocouple::count() const {
    return thermoc_count_;
}

void Thermocouple::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(kThermocoupleQueueLength, sizeof(ThermocoupleCommand));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola de termocuplas.");
        return;
    }

    if (xTaskCreate(task_entry, kThermocoupleTaskName, kThermocoupleTaskStackSize, this, kThermocoupleTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea de termocuplas.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool Thermocouple::send_command(definitions::CommandType command, uint32_t address, uint32_t timeout_ms) {
    const ThermocoupleCommand queue_command = {command, address};
    if (queue_ == nullptr || xQueueSend(queue_, &queue_command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

void modules::Thermocouple::task_entry(void* param) {
    Thermocouple* self = static_cast<Thermocouple*>(param);
    ThermocoupleCommand command;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command.command, command.address);
        }
    }
}

bool Thermocouple::process_command(definitions::CommandType command, uint32_t address) {
    uint32_t sensor_address = address;
    if (sensor_address == 0) {
        modules::hal_general().log("No se pudo procesar el comando. Direccion I2C invalida.");
        return false;  // Invalid address
    }

    const float temperature = readCelcius(sensor_address);
    if (command != definitions::CommandType::kTempInRead && command != definitions::CommandType::kTempOutRead &&
        command != definitions::CommandType::kTempBathRead && command != definitions::CommandType::kTempMixRead) {
        return false;
    }

    if (!std::isnan(temperature)) {
        publish_response({command, sensor_address});
    }
    return !std::isnan(temperature);
}

bool Thermocouple::publish_response(const definitions::CommandRequest& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kThermocoupleTimeoutMs);
}

Thermocouple::Thermocouple() : thermocouples_({}), thermoc_count_(0), queue_(nullptr), response_queue_(nullptr) {}

uint8_t Thermocouple::find_index(uint32_t address) const {
    for (uint8_t index = 0; index < thermoc_count_; ++index) {
        if (thermocouples_[index].address == address) {
            return index;
        }
    }

    return definitions::kMaxThermocouples;  // Return an out-of-range index to indicate not found
}

}  // namespace modules
