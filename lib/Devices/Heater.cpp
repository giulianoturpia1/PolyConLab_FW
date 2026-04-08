#include "Heater.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"
#include "StateMachine.hpp"

constexpr UBaseType_t kHeaterTaskPriority = 2;
constexpr UBaseType_t kHeaterQueueLength = 8;
constexpr uint32_t kHeaterTaskStackSize = 2048;
constexpr char kHeaterTaskName[] = "heater_task";

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Heater& modules::Heater::instance() {
    static Heater instance;
    return instance;
}

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

    queue_ = xQueueCreate(kHeaterQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola del calentador.");
        return;
    }

    if (xTaskCreate(task_entry, kHeaterTaskName, kHeaterTaskStackSize, this, kHeaterTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea del calentador.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::Heater::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::Heater::Heater() : pin_(0), status_(false), queue_(nullptr) {}

bool modules::Heater::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kHeaterOn:
            on();
            modules::StateMachine::instance().queue_action({definitions::CommandType::kHeaterOn, 0}, 10);
            return true;

        case definitions::CommandType::kHeaterOff:
            off();
            modules::StateMachine::instance().queue_action({definitions::CommandType::kHeaterOff, 0}, 10);
            return true;

        case definitions::CommandType::kHeaterState:
            modules::StateMachine::instance().queue_action({definitions::CommandType::kHeaterState, 0}, 10);
            return true;

        default:
            return false;
    }
}

void modules::Heater::task_entry(void* param) {
    Heater* self = static_cast<Heater*>(param);
    definitions::CommandType command = definitions::CommandType::kHeaterOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}
