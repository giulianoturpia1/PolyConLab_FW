#include "Heater.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"

constexpr uint32_t kHeaterTimeoutMs = 20;
constexpr UBaseType_t kHeaterTaskPriority = 2;
constexpr UBaseType_t kHeaterQueueLength = 8;
constexpr uint32_t kHeaterTaskStackSize = 2048;
constexpr char kHeaterTaskName[] = "heater_task";
constexpr uint32_t kHeaterTaskPollTimeoutMs = 20;

namespace modules {

////////////////////////////////////// Public Methods //////////////////////////////////////

Heater& Heater::instance() {
    static Heater instance;
    return instance;
}

void Heater::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool Heater::begin(uint32_t pin) {
    if (pin == 0) {
        hal_general().log("Pin invalido para el calentador.");
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

bool Heater::start_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(kHeaterQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        hal_general().log("No se pudo crear la cola del calentador.");
        return false;
    }

    if (xTaskCreate(task_entry, kHeaterTaskName, kHeaterTaskStackSize, this, kHeaterTaskPriority, nullptr) != pdPASS) {
        hal_general().log("No se pudo crear la tarea del calentador.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

bool Heater::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
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

bool Heater::publish_response(const definitions::CommandRequest& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kHeaterTimeoutMs);
}

void Heater::task_entry(void* param) {
    Heater* self = static_cast<Heater*>(param);
    definitions::CommandType command = definitions::CommandType::kHeaterOff;

    while (true) {
        if (self->queue_ == nullptr) {
            break;
        }

        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}

}  // namespace modules
