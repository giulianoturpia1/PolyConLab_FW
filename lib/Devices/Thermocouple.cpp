#include "Thermocouple.hpp"

#include <freertos/task.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "HALGeneral.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"

constexpr uint32_t kThermocoupleTimeoutMs = 20;
constexpr TickType_t kThermocoupleTaskWatchdogTicks = portMAX_DELAY;
constexpr UBaseType_t kThermocoupleTaskPriority = 2;
constexpr UBaseType_t kThermocoupleQueueLength = 8;
constexpr uint32_t kThermocoupleTaskStackSize = 3072;
constexpr char kThermocoupleTaskName[] = "thermocouple_task";

struct ThermocoupleCommand {
    definitions::CommandType command;
    uint32_t address;
};

namespace modules {

static const TaggedLogger kLog("Thermocouple");

////////////////////////////////////// Public Methods //////////////////////////////////////

Thermocouple& Thermocouple::instance() {
    static Thermocouple instance;
    return instance;
}

bool Thermocouple::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(kThermocoupleQueueLength, sizeof(ThermocoupleCommand));
    if (queue_ == nullptr) {
        kLog.error("No se pudo crear la cola de termocuplas.");
        return false;
    }

    if (xTaskCreate(task_entry, kThermocoupleTaskName, kThermocoupleTaskStackSize, this, kThermocoupleTaskPriority, nullptr) != pdPASS) {
        kLog.error("No se pudo crear la tarea de termocuplas.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

void Thermocouple::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool Thermocouple::begin(const uint32_t* adds, size_t count) {
    thermoc_count_ = 0;

    if (adds == nullptr || count == 0) {
        kLog.warning("No se añadieron direcciones de termocuplas para la inicializacion.");
        return false;
    }

    const uint8_t bounded_count = static_cast<uint8_t>(std::min(count, static_cast<size_t>(definitions::kMaxThermocouples)));
    for (uint8_t index = 0; index < bounded_count; ++index) {
        if (!add(adds[index])) {
            return false;
        }
    }

    return true;
}

bool Thermocouple::add(uint32_t address) {
    const std::string hex_addr = utils::format_hex(address);

    uint8_t index = find_index(address);
    if (thermoc_count_ >= definitions::kMaxThermocouples) {
        kLog.error("No se pudo agregar la direccion de la termocupla: " + hex_addr + ". Se alcanzo el limite de capacidad.");
        return false;  // Capacity limit reached
    }

    if (index < thermoc_count_) {
        kLog.warning("La termocupla ya estaba registrada: " + hex_addr);
        return true;  // Address already registered, consider it a success
    }

    // Initialize MCP9600 at the current slot
    uint8_t slot_index = thermoc_count_;
    kLog.info("Iniciando termocupla en la direccion: " + hex_addr);
    if (!thermocouples_[slot_index].device.begin(address)) {
        kLog.error("No se pudo iniciar la termocupla en la direccion: " + hex_addr);
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
        kLog.warning("Se intento eliminar una direccion no registrada: " + utils::format_hex(address));
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
        kLog.warning("Se intento leer una direccion no registrada: " + utils::format_hex(address));
        return NAN;  // Return NaN if address not found
    }

    // Return the thermocouple temperature reading in Celsius
    return thermocouples_[index].device.readThermocouple();
}

uint8_t Thermocouple::count() const {
    return thermoc_count_;
}

bool Thermocouple::queue_command(definitions::CommandType command, uint32_t address, uint32_t timeout_ms) {
    const ThermocoupleCommand queue_command = {command, address};
    if (queue_ == nullptr || xQueueSend(queue_, &queue_command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Thermocouple::Thermocouple() : thermocouples_({}), thermoc_count_(0), queue_(nullptr), response_queue_(nullptr) {}

uint8_t Thermocouple::find_index(uint32_t address) const {
    for (uint8_t index = 0; index < thermoc_count_; ++index) {
        if (thermocouples_[index].address == address) {
            return index;
        }
    }

    return definitions::kMaxThermocouples;  // Return an out-of-range index to indicate not found
}

bool Thermocouple::process_command(definitions::CommandType command, uint32_t address) {
    uint32_t sensor_address = address;
    if (sensor_address == 0) {
        kLog.error("No se pudo procesar el comando. Direccion I2C invalida.");
        return false;  // Invalid address
    }

    const float temperature = readCelcius(sensor_address);
    if (command != definitions::CommandType::kTempInRead && command != definitions::CommandType::kTempOutRead &&
        command != definitions::CommandType::kTempBathRead && command != definitions::CommandType::kTempMixRead) {
        return false;
    }

    uint32_t temp_bits;
    std::memcpy(&temp_bits, &temperature, sizeof(float));
    publish_response({command, temp_bits});
    return true;
}

bool Thermocouple::publish_response(const definitions::Command& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kThermocoupleTimeoutMs);
}

void Thermocouple::task_entry(void* param) {
    Thermocouple* self = static_cast<Thermocouple*>(param);
    ThermocoupleCommand command;

    while (true) {
        if (xQueueReceive(self->queue_, &command, kThermocoupleTaskWatchdogTicks) == pdPASS) {
            self->process_command(command.command, command.address);
        }
        if (self->queue_ == nullptr) {
            break;
        }
    }
}

}  // namespace modules
