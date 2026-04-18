#include "Flowmeter.hpp"

#include <freertos/task.h>

#include <cstring>

#include "HALGeneral.hpp"

constexpr uint32_t kFlowmeterTimeoutMs = 20;
constexpr UBaseType_t kFlowmeterTaskPriority = 2;
constexpr UBaseType_t kFlowmeterQueueLength = 8;
constexpr uint32_t kFlowmeterTaskStackSize = 3072;
constexpr char kFlowmeterTaskName[] = "flowmeter_task";

volatile uint32_t flow_pulses = 0;
portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* timer = NULL;
const float pulsesPerLiter = 450.0;  // A configurar segun el sensor
volatile float flow_rate = 0;

////////////////////////////////////// Public Methods //////////////////////////////////////

modules::Flowmeter& modules::Flowmeter::instance() {
    static Flowmeter instance;
    return instance;
}

void modules::Flowmeter::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

modules::Flowmeter::Flowmeter() : pin_(0), status_(false), queue_(nullptr), response_queue_(nullptr) {}

//////////////////////////////////// Flow Meter Interrupt Service Routines //////////////////////////////
void IRAM_ATTR isrFlowMeter() {
    portENTER_CRITICAL_ISR(&flowMux);
    flow_pulses++;
    portEXIT_CRITICAL_ISR(&flowMux);
}

void IRAM_ATTR isrTimerFlow() {
    portENTER_CRITICAL_ISR(&flowMux);
    unsigned int pulses = flow_pulses;
    flow_pulses = 0;
    portEXIT_CRITICAL_ISR(&flowMux);
    float liters = pulses / pulsesPerLiter;
    flow_rate = liters * 60.0;
}

void modules::Flowmeter::begin(uint32_t pinFlow_) {
    pin_ = pinFlow_;
    modules::hal_general().pin_mode(pin_, modules::PIN_MODE_INPUT);
    status_ = true;

    attachInterrupt(digitalPinToInterrupt(pinFlow_), isrFlowMeter, RISING);

    timer = timerBegin(0, 80, true);  // timer 0, prescaler 80 → 1 tick = 1 µs
    timerAttachInterrupt(timer, &isrTimerFlow, true);
    timerAlarmWrite(timer, 1000000, true);  // 1 seC
    timerAlarmEnable(timer);
}

void modules::Flowmeter::stop(uint32_t pinFlow_) {
    detachInterrupt(digitalPinToInterrupt(pinFlow_));
    if (timer != NULL) {
        timerEnd(timer);
        timer = NULL;
    }
    status_ = false;
}

float modules::Flowmeter::readFlow() {
    float current_flow;
    portENTER_CRITICAL(&flowMux);
    current_flow = flow_rate;
    portEXIT_CRITICAL(&flowMux);
    return (current_flow);
}

bool modules::Flowmeter::status() const {
    return status_;
}

uint32_t modules::Flowmeter::pin() const {
    return pin_;
}

void modules::Flowmeter::start_task() {
    if (queue_ != nullptr) {
        return;
    }

    queue_ = xQueueCreate(kFlowmeterQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        modules::hal_general().log("No se pudo crear la cola del caudalimetro.");
        return;
    }

    if (xTaskCreate(task_entry, kFlowmeterTaskName, kFlowmeterTaskStackSize, this, kFlowmeterTaskPriority, nullptr) != pdPASS) {
        modules::hal_general().log("No se pudo crear la tarea del caudalimetro.");
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

bool modules::Flowmeter::send_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr || xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

void modules::Flowmeter::task_entry(void* param) {
    Flowmeter* self = static_cast<Flowmeter*>(param);
    definitions::CommandType command = definitions::CommandType::kFlowRead;

    while (true) {
        if (xQueueReceive(self->queue_, &command, portMAX_DELAY) == pdPASS) {
            self->process_command(command);
        }
    }
}

bool modules::Flowmeter::process_command(definitions::CommandType command) {
    if (command != definitions::CommandType::kFlowBegin && command != definitions::CommandType::kFlowStop &&
        command != definitions::CommandType::kFlowRead && command != definitions::CommandType::kFlowStatus) {
        return false;
    }

    switch (command) {
        case definitions::CommandType::kFlowBegin: {
            begin(pin_);
            publish_response({definitions::CommandType::kFlowBegin, 0});
            return true;
        }

        case definitions::CommandType::kFlowStop: {
            stop(pin_);
            publish_response({definitions::CommandType::kFlowStop, 0});
            return true;
        }

        case definitions::CommandType::kFlowStatus: {
            bool current_status = status();
            publish_response({definitions::CommandType::kFlowStatus, current_status});
            return true;
        }

        case definitions::CommandType::kFlowRead: {
            float flow_rate = readFlow();
            uint32_t flow_rate_bits;
            std::memcpy(&flow_rate_bits, &flow_rate, sizeof(float));
            publish_response({definitions::CommandType::kFlowRead, flow_rate_bits});
            return true;
        }

        default:
            return false;
    }
}

bool modules::Flowmeter::publish_response(const definitions::CommandRequest& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kFlowmeterTimeoutMs);
}
