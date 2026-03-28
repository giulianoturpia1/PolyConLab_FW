///
/// @file Heater.cpp
/// @author F. Galietta, G. Turpia, S. Miranda (proyectopolyconlab@gmail.com)
/// @brief Heater control implementation for the reactor system.
/// @version 0.1
/// @date 24-02-2026
///
/// @copyright Copyright (c) 2026
///
///

#include "Heater.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"
#include "StateMachine.hpp"

constexpr UBaseType_t kHeaterTaskPriority = 2;

////////////////////////////////////// Task Function //////////////////////////////////////

void modules::Heater::task_entry(void* param) {
    Heater* self = static_cast<Heater*>(param);
    definitions::CommandType command = definitions::CommandType::kHeaterOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Heater& modules::Heater::instance() {
    static Heater instance;
    return instance;
}

modules::Heater::Heater() : pin_(0), status_(false), queue_(nullptr) {}

void modules::Heater::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_OUTPUT);
    off();
}

void modules::Heater::on() {
    modules::hal_general().digital_write(pin_, modules::PIN_HIGH);
    status_ = true;
}

void modules::Heater::off() {
    modules::hal_general().digital_write(pin_, modules::PIN_LOW);
    status_ = false;
}

bool modules::Heater::status() const {
    return status_;
}

uint32_t modules::Heater::pin() const {
    return pin_;
}

void modules::Heater::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(8, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola del calentador.");
        return;
    }

    if (xTaskCreate(task_entry, "heater_task", 2048, this, kHeaterTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea del calentador.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::Heater::send_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr || xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

bool modules::Heater::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kHeaterOn:
            on();
            modules::StateMachine::instance().enqueue_action({definitions::CommandType::kHeaterOn, 0}, 10);
            return true;

        case definitions::CommandType::kHeaterOff:
            off();
            modules::StateMachine::instance().enqueue_action({definitions::CommandType::kHeaterOff, 0}, 10);
            return true;

        default:
            return false;
    }
}
