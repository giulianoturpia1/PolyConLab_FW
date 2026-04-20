#include "LevelSwitch.hpp"

#include <freertos/task.h>

#include "HALGeneral.hpp"

constexpr uint32_t kLevelSwitchTimeoutMs = 20;
constexpr UBaseType_t kLevelSwitchTaskPriority = 2;
constexpr UBaseType_t kLevelSwitchQueueLength = 8;
constexpr uint32_t kLevelSwitchTaskStackSize = 2048;
constexpr char kLevelSwitchTaskName[] = "level_switch_task";

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::LevelSwitch& modules::LevelSwitch::instance() {
    static LevelSwitch instance;
    return instance;
}

void modules::LevelSwitch::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

void modules::LevelSwitch::begin(uint32_t pin) {
    pin_ = pin;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_INPUT);
    status_ = modules::hal_general().digital_read(pin_);
}

bool modules::LevelSwitch::status() const {
    return modules::hal_general().digital_read(pin_);
}

uint32_t modules::LevelSwitch::pin() const {
    return pin_;
}

void modules::LevelSwitch::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(kLevelSwitchQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola del interruptor de nivel.");
        return;
    }

    if (xTaskCreate(task_entry, kLevelSwitchTaskName, kLevelSwitchTaskStackSize, this, kLevelSwitchTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea del interruptor de nivel.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::LevelSwitch::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

modules::LevelSwitch::LevelSwitch() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool modules::LevelSwitch::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kLevelSwitchState:
            publish_response({definitions::CommandType::kLevelSwitchState, 0});
            return true;

        default:
            return false;
    }
}

bool modules::LevelSwitch::publish_response(const definitions::CommandRequest& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kLevelSwitchTimeoutMs);
}

void modules::LevelSwitch::task_entry(void* param) {
    LevelSwitch* self = static_cast<LevelSwitch*>(param);
    definitions::CommandType command = definitions::CommandType::kLevelSwitchState;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}
