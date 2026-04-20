#include "Chiller.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"

constexpr uint32_t kChillerTimeoutMs = 20;
constexpr UBaseType_t kChillerTaskPriority = 2;
constexpr UBaseType_t kChillerQueueLength = 8;
constexpr uint32_t kChillerTaskStackSize = 2048;
constexpr char kChillerTaskName[] = "chiller_task";

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Chiller& modules::Chiller::instance() {
    static Chiller instance;
    return instance;
}

void modules::Chiller::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

void modules::Chiller::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_OUTPUT);
    off();
}

void modules::Chiller::on() {
    modules::hal_general().digital_write(pin_, modules::PIN_HIGH);
    status_ = true;
}

void modules::Chiller::off() {
    modules::hal_general().digital_write(pin_, modules::PIN_LOW);
    status_ = false;
}

bool modules::Chiller::status() const {
    return status_;
}

uint32_t modules::Chiller::pin() const {
    return pin_;
}

void modules::Chiller::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(kChillerQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola del chiller.");
        return;
    }

    if (xTaskCreate(task_entry, kChillerTaskName, kChillerTaskStackSize, this, kChillerTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea del chiller.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::Chiller::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::Chiller::Chiller() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool modules::Chiller::process_command(definitions::CommandType command) {
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

bool modules::Chiller::publish_response(const definitions::CommandRequest& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kChillerTimeoutMs);
}

void modules::Chiller::task_entry(void* param) {
    Chiller* self = static_cast<Chiller*>(param);
    definitions::CommandType command = definitions::CommandType::kChillerOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}
