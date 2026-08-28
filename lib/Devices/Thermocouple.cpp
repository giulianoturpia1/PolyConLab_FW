#include "Thermocouple.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Config.hpp"
#include "Helpers.hpp"
#include "Logger.hpp"
#include "ScopedLock.hpp"
#include "TargetDefinitions.hpp"

constexpr char kThermocoupleTaskName[] = "thermocouple_task";

namespace modules {

static const TaggedLogger kLog("Thermocouple");

////////////////////////////////////// Public Methods //////////////////////////////////////

Thermocouple& Thermocouple::instance() {
    static Thermocouple instance;
    return instance;
}

bool Thermocouple::begin(const uint32_t* adds, size_t count) {
    thermoc_count_ = 0;

    if (bus_mutex_ == nullptr) {
        bus_mutex_ = xSemaphoreCreateMutex();
        if (bus_mutex_ == nullptr) {
            kLog.error("No se pudo crear el mutex del bus I2C de termocuplas; las lecturas devolveran NaN.");
        }
    }

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

bool Thermocouple::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kTempInRead || type == definitions::CommandType::kTempOutRead ||
           type == definitions::CommandType::kTempBathRead || type == definitions::CommandType::kTempMixRead ||
           type == definitions::CommandType::kTempInReinit || type == definitions::CommandType::kTempOutReinit ||
           type == definitions::CommandType::kTempBathReinit || type == definitions::CommandType::kTempMixReinit;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Thermocouple::Thermocouple()
    : AsyncDevice(kThermocoupleTaskName, config::kThermocoupleTaskStackSize, config::kThermocoupleTaskPriority, config::kI2cCoreId),
      thermocouples_({}),
      thermoc_count_(0),
      bus_mutex_(nullptr) {}

uint8_t Thermocouple::find_index(uint32_t address) const {
    for (uint8_t index = 0; index < thermoc_count_; ++index) {
        if (thermocouples_[index].address == address) {
            return index;
        }
    }

    return definitions::kMaxThermocouples;  // Return an out-of-range index to indicate not found
}

bool Thermocouple::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    uint32_t address;
    bool reinit_request = false;
    switch (cmd.type) {
        case definitions::CommandType::kTempInRead:
            address = static_cast<uint32_t>(definitions::Addresses::kTempIn);
            break;
        case definitions::CommandType::kTempOutRead:
            address = static_cast<uint32_t>(definitions::Addresses::kTempOut);
            break;
        case definitions::CommandType::kTempBathRead:
            address = static_cast<uint32_t>(definitions::Addresses::kTempBath);
            break;
        case definitions::CommandType::kTempMixRead:
            address = static_cast<uint32_t>(definitions::Addresses::kTempMix);
            break;
        case definitions::CommandType::kTempInReinit:
            address = static_cast<uint32_t>(definitions::Addresses::kTempIn);
            reinit_request = true;
            break;
        case definitions::CommandType::kTempOutReinit:
            address = static_cast<uint32_t>(definitions::Addresses::kTempOut);
            reinit_request = true;
            break;
        case definitions::CommandType::kTempBathReinit:
            address = static_cast<uint32_t>(definitions::Addresses::kTempBath);
            reinit_request = true;
            break;
        case definitions::CommandType::kTempMixReinit:
            address = static_cast<uint32_t>(definitions::Addresses::kTempMix);
            reinit_request = true;
            break;
        default:
            return false;
    }

    if (reinit_request) {
        const bool ok = reinit(address);
        out_response = {cmd.type,
                        static_cast<uint32_t>(ok ? definitions::ResponseType::kReinitOk : definitions::ResponseType::kErrorReinitFailed)};
        return true;
    }

    const float temperature = readCelcius(address);
    uint32_t temp_bits;
    std::memcpy(&temp_bits, &temperature, sizeof(float));
    out_response = {cmd.type, temp_bits};
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

bool Thermocouple::reinit(uint32_t address) {
    // Serialize against any other bus user (the read task or a synchronous Core-0 reader) while
    // we re-touch the array and re-run the amplifier's begin() over I2C.
    ScopedLock lock(bus_mutex_, pdMS_TO_TICKS(config::kI2cBusTimeoutMs));
    if (!lock.held()) {
        kLog.warning("No se pudo adquirir el bus I2C de termocuplas para reinicializar.");
        return false;
    }

    // remove() is a no-op (returns false) when the address was never registered — e.g. a sensor
    // that failed init at boot — so a plain add() re-registers it either way.
    remove(address);
    return add(address);
}

float Thermocouple::readCelcius(uint32_t address) {
    uint8_t index = find_index(address);
    if (index >= definitions::kMaxThermocouples) {
        kLog.warning("Se intento leer una direccion no registrada: " + utils::format_hex(address));
        return NAN;  // Return NaN if address not found
    }

    // Serialize the transaction against any other bus user (task or synchronous caller).
    ScopedLock lock(bus_mutex_, pdMS_TO_TICKS(config::kI2cBusTimeoutMs));
    if (!lock.held()) {
        kLog.warning("No se pudo adquirir el bus I2C de termocuplas.");
        return NAN;
    }

    // Return the thermocouple temperature reading in Celsius
    return thermocouples_[index].device.readThermocouple();
}

uint8_t Thermocouple::count() const {
    return thermoc_count_;
}

}  // namespace modules
