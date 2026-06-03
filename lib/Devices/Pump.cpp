#include "Pump.hpp"

#include <freertos/task.h>

#include "Config.hpp"
#include "HALGeneral.hpp"
#include "Logger.hpp"

constexpr TickType_t kPumpTaskWatchdogTicks = portMAX_DELAY;
constexpr char kPumpTaskName[] = "pump_task";

namespace modules {

static const TaggedLogger kLog("Pump");

////////////////////////////////////// Public Methods //////////////////////////////////////

Pump& Pump::instance() {
    static Pump instance;
    return instance;
}

bool Pump::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(config::kDeviceQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        kLog.error("No se pudo crear la cola de la bomba.");
        return false;
    }

    if (xTaskCreatePinnedToCore(task_entry, kPumpTaskName, config::kPumpTaskStackSize, this, config::kDeviceTaskPriority, nullptr,
                                config::kI2cCoreId) != pdPASS) {
        kLog.error("No se pudo crear la tarea de la bomba.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

void Pump::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool Pump::begin(uint32_t pin) {
    if (pin == 0) {
        kLog.error("Pin invalido para la bomba.");
        return false;
    }

    pin_ = pin;
    hal_general().pin_mode(pin_, PIN_MODE_OUTPUT);
    off();
    return true;
}

void Pump::on() {
    hal_general().digital_write(pin_, PIN_HIGH);
    status_ = true;
}

void Pump::off() {
    hal_general().digital_write(pin_, PIN_LOW);
    status_ = false;
}

bool Pump::status() const {
    return status_;
}

uint32_t Pump::pin() const {
    return pin_;
}

bool Pump::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr) {
        return false;
    }

    return xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) == pdPASS;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Pump::Pump() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

bool Pump::process_command(definitions::CommandType command) {
    switch (command) {
        case definitions::CommandType::kPumpOn:
            on();
            publish_response({definitions::CommandType::kPumpOn, 0});
            return true;

        case definitions::CommandType::kPumpOff:
            off();
            publish_response({definitions::CommandType::kPumpOff, 0});
            return true;

        case definitions::CommandType::kPumpState:
            publish_response({definitions::CommandType::kPumpState, 0});
            return true;

        default:
            return false;
    }
}

bool Pump::publish_response(const definitions::Command& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, config::kDeviceResponseTimeoutMs);
}

void Pump::task_entry(void* param) {
    Pump* self = static_cast<Pump*>(param);
    definitions::CommandType command = definitions::CommandType::kPumpOff;

    while (true) {
        if (xQueueReceive(self->queue_, &command, kPumpTaskWatchdogTicks) == pdPASS) {
            self->process_command(command);
        }
        if (self->queue_ == nullptr) {
            break;
        }
    }
}

}  // namespace modules
