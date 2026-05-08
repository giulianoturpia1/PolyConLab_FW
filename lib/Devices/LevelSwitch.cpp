#include "LevelSwitch.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"

constexpr uint32_t kLevelSwitchTimeoutMs = 20;
constexpr UBaseType_t kLevelSwitchTaskPriority = 2;
constexpr UBaseType_t kLevelSwitchQueueLength = 8;
constexpr uint32_t kLevelSwitchTaskStackSize = 2048;
constexpr char kLevelSwitchTaskName[] = "level_switch_task";

namespace modules {

////////////////////////////////////// Public Methods //////////////////////////////////////

LevelSwitch& LevelSwitch::instance() {
    static LevelSwitch instance;
    return instance;
}

bool LevelSwitch::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(kLevelSwitchQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        hal_general().log("No se pudo crear la cola del interruptor de nivel.");
        return false;
    }

    if (xTaskCreate(task_entry, kLevelSwitchTaskName, kLevelSwitchTaskStackSize, this, kLevelSwitchTaskPriority, nullptr) != pdPASS) {
        hal_general().log("No se pudo crear la tarea del interruptor de nivel.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

void LevelSwitch::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool LevelSwitch::begin(uint32_t pin) {
    if (pin == 0) {
        hal_general().log("Pin invalido para el interruptor de nivel.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_INPUT);
    status_ = hal_general().digital_read(pin_);
    return true;
}

bool LevelSwitch::status() const {
    return hal_general().digital_read(pin_);
}

uint32_t LevelSwitch::pin() const {
    return pin_;
}

bool LevelSwitch::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

LevelSwitch::LevelSwitch() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool LevelSwitch::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kLevelSwitchState:
            publish_response({definitions::CommandType::kLevelSwitchState, 0});
            return true;

        default:
            return false;
    }
}

bool LevelSwitch::publish_response(const definitions::CommandRequest& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kLevelSwitchTimeoutMs);
}

void LevelSwitch::task_entry(void* param) {
    LevelSwitch* self = static_cast<LevelSwitch*>(param);
    definitions::CommandType command = definitions::CommandType::kLevelSwitchState;

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
