#include "Heater.hpp"

#include <freertos/task.h>

#include "Config.hpp"
#include "HALGeneral.hpp"
#include "Logger.hpp"

constexpr char kHeaterTaskName[] = "heater_task";

namespace modules {

static const TaggedLogger kLog("Heater");

////////////////////////////////////// Public Methods //////////////////////////////////////

Heater& Heater::instance() {
    static Heater instance;
    return instance;
}

bool Heater::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(config::kDeviceQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        kLog.error("No se pudo crear la cola del calentador.");
        return false;
    }

    if (xTaskCreatePinnedToCore(task_entry, kHeaterTaskName, config::kHeaterTaskStackSize, this, config::kDeviceTaskPriority, nullptr,
                                config::kI2cCoreId) != pdPASS) {
        kLog.error("No se pudo crear la tarea del calentador.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

void Heater::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool Heater::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error("Pin invalido para el calentador.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

void Heater::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void Heater::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool Heater::status() const {
    return status_;
}

uint32_t Heater::pin() const {
    return pin_;
}

bool Heater::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kHeaterOn || type == definitions::CommandType::kHeaterOff ||
           type == definitions::CommandType::kHeaterState;
}

bool Heater::queue_command(const definitions::Command& cmd, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }
    return xQueueSend(queue_, &cmd.type, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Heater::Heater() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool Heater::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kHeaterOn:
            on();
            publish_response({definitions::CommandType::kHeaterOn, 0});
            return true;

        case definitions::CommandType::kHeaterOff:
            off();
            publish_response({definitions::CommandType::kHeaterOff, 0});
            return true;

        case definitions::CommandType::kHeaterState:
            publish_response({definitions::CommandType::kHeaterState, 0});
            return true;

        default:
            return false;
    }
}

bool Heater::publish_response(const definitions::Command& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, config::kDeviceResponseTimeoutMs);
}

void Heater::task_entry(void* param) {
    Heater* self = static_cast<Heater*>(param);
    definitions::CommandType command = definitions::CommandType::kHeaterOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, config::kDeviceWaitTicks) == pdPASS) {
            self->process_command(command);
        }
        if (self->queue_ == nullptr) {
            break;
        }
    }
}

}  // namespace modules
