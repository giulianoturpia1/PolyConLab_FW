#include "Pump.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"
#include "StateMachine.hpp"

constexpr UBaseType_t kPumpTaskPriority = 2;
constexpr UBaseType_t kPumpQueueLength = 8;
constexpr uint32_t kPumpTaskStackSize = 2048;
constexpr char kPumpTaskName[] = "pump_task";

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Pump& modules::Pump::instance() {
    static Pump instance;
    return instance;
}

void modules::Pump::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_OUTPUT);
    off();
}

void modules::Pump::on() {
    modules::hal_general().digital_write(pin_, modules::PIN_HIGH);
    status_ = true;
}

void modules::Pump::off() {
    modules::hal_general().digital_write(pin_, modules::PIN_LOW);
    status_ = false;
}

bool modules::Pump::status() const {
    return status_;
}

uint32_t modules::Pump::pin() const {
    return pin_;
}

void modules::Pump::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(kPumpQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola de la bomba.");
        return;
    }

    if (xTaskCreate(task_entry, kPumpTaskName, kPumpTaskStackSize, this, kPumpTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea de la bomba.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::Pump::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::Pump::Pump() : pin_(0), status_(false), queue_(nullptr) {}

bool modules::Pump::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kPumpOn:
            on();
            modules::StateMachine::instance().queue_action({definitions::CommandType::kPumpOn, 0}, 10);
            return true;

        case definitions::CommandType::kPumpOff:
            off();
            modules::StateMachine::instance().queue_action({definitions::CommandType::kPumpOff, 0}, 10);
            return true;

        case definitions::CommandType::kPumpState:
            modules::StateMachine::instance().queue_action({definitions::CommandType::kPumpState, 0}, 10);
            return true;

        default:
            return false;
    }
}

void modules::Pump::task_entry(void* param) {
    Pump* self = static_cast<Pump*>(param);
    definitions::CommandType command = definitions::CommandType::kPumpOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}
