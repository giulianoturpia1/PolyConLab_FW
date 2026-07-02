#include "LevelSwitch.hpp"

#include <freertos/task.h>

#include "Config.hpp"
#include "HALGeneral.hpp"
#include "Logger.hpp"

constexpr char kLevelSwitchTaskName[] = "level_switch_task";

namespace modules {

static const TaggedLogger kLog("LevelSwitch");

////////////////////////////////////// Public Methods //////////////////////////////////////

LevelSwitch& LevelSwitch::instance() {
    static LevelSwitch instance;
    return instance;
}

bool LevelSwitch::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(config::kDeviceQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        kLog.error("No se pudo crear la cola del interruptor de nivel.");
        return false;
    }

    if (xTaskCreatePinnedToCore(task_entry, kLevelSwitchTaskName, config::kLevelSwitchTaskStackSize, this, config::kDeviceTaskPriority,
                                nullptr, config::kI2cCoreId) != pdPASS) {
        kLog.error("No se pudo crear la tarea del interruptor de nivel.");
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
        kLog.error("Pin invalido para el interruptor de nivel.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_INPUT);
    status_ = hal_general().digital_read(pin_);
    return true;
}

bool LevelSwitch::status() const {
    // El sensor de nivel está activo-bajo: 0V significa cerrado/alto lógico.
    return hal_general().digital_read(pin_) == PIN_LOW;
}

uint32_t LevelSwitch::pin() const {
    return pin_;
}

bool LevelSwitch::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kLevelSwitchState;
}

bool LevelSwitch::queue_command(const definitions::Command& cmd, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }
    return xQueueSend(queue_, &cmd.type, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

LevelSwitch::LevelSwitch() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool LevelSwitch::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kLevelSwitchState: {
            // Invertimos la lectura física porque el sensor es activo-bajo.
            const uint32_t level_state = hal_general().digital_read(pin_) == PIN_LOW ? PIN_HIGH : PIN_LOW;
            publish_response({definitions::CommandType::kLevelSwitchState, level_state});
            return true;
        }

        default:
            return false;
    }
}

bool LevelSwitch::publish_response(const definitions::Command& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, config::kDeviceResponseTimeoutMs);
}

void LevelSwitch::task_entry(void* param) {
    LevelSwitch* self = static_cast<LevelSwitch*>(param);
    definitions::CommandType command = definitions::CommandType::kLevelSwitchState;

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
