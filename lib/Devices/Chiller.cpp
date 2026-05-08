#include "Chiller.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"

constexpr uint32_t kChillerTimeoutMs = 20;
constexpr UBaseType_t kChillerTaskPriority = 2;
constexpr UBaseType_t kChillerQueueLength = 8;
constexpr uint32_t kChillerTaskStackSize = 2048;
constexpr char kChillerTaskName[] = "chiller_task";

namespace modules {

////////////////////////////////////// Public Methods //////////////////////////////////////

Chiller& Chiller::instance() {
    static Chiller instance;
    return instance;
}

bool Chiller::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(kChillerQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        hal_general().log("No se pudo crear la cola del chiller.");
        return false;
    }

    if (xTaskCreate(task_entry, kChillerTaskName, kChillerTaskStackSize, this, kChillerTaskPriority, nullptr) != pdPASS) {
        hal_general().log("No se pudo crear la tarea del chiller.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

void Chiller::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool Chiller::begin(uint32_t pin) {
    if (pin == 0) {
        hal_general().log("Pin invalido para el chiller.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

void Chiller::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void Chiller::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool Chiller::status() const {
    return status_;
}

uint32_t Chiller::pin() const {
    return pin_;
}

bool Chiller::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Chiller::Chiller() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool Chiller::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kChillerOn:
            on();
            publish_response({definitions::CommandType::kChillerOn, 0});
            return true;

        case definitions::CommandType::kChillerOff:
            off();
            publish_response({definitions::CommandType::kChillerOff, 0});
            return true;

        case definitions::CommandType::kChillerState:
            publish_response({definitions::CommandType::kChillerState, 0});
            return true;

        default:
            return false;
    }
}

bool Chiller::publish_response(const definitions::Command& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kChillerTimeoutMs);
}

void Chiller::task_entry(void* param) {
    Chiller* self = static_cast<Chiller*>(param);
    definitions::CommandType command = definitions::CommandType::kChillerOff;

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
