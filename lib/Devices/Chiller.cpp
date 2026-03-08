#include "Chiller.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"

constexpr UBaseType_t kChillerTaskPriority = 2;

void modules::Chiller::task_entry(void* param) {
    Chiller* self = static_cast<Chiller*>(param);
    definitions::CommandType command = definitions::CommandType::kChillerOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            if (command == definitions::CommandType::kChillerOn) {
                self->on();
            } else {
                self->off();
            }
        }
    }
}

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Chiller& modules::Chiller::instance() {
    static Chiller instance;
    return instance;
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

    queue_ = xQueueCreate(8, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola del chiller.");
        return;
    }

    if (xTaskCreate(task_entry, "chiller_task", 2048, this, kChillerTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea del chiller.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::Chiller::send_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (command != definitions::CommandType::kChillerOn && command != definitions::CommandType::kChillerOff) {
        return false;
    }

    if (queue_ == nullptr) {
        if (command == definitions::CommandType::kChillerOn) {
            on();
        } else {
            off();
        }
        return false;
    }

    if (xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        if (command == definitions::CommandType::kChillerOn) {
            on();
        } else {
            off();
        }
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::Chiller::Chiller() : pin_(0), status_(false), queue_(nullptr) {}
