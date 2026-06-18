#include "Flowmeter.hpp"

#include <freertos/task.h>

#include <cmath>
#include <cstring>

#include "HALGeneral.hpp"
#include "Logger.hpp"

constexpr uint32_t kFlowmeterTimeoutMs = 20;
constexpr TickType_t kFlowmeterTaskWatchdogTicks = portMAX_DELAY;
constexpr UBaseType_t kFlowmeterTaskPriority = 2;
constexpr UBaseType_t kFlowmeterQueueLength = 8;
constexpr uint32_t kFlowmeterTaskStackSize = 3072;
constexpr char kFlowmeterTaskName[] = "flowmeter_task";

static volatile uint32_t flow_pulses = 0;           ///< Raw pulse counter, incremented by isrFlowMeter.
static volatile uint32_t flow_pulses_snapshot = 0;  ///< Pulses captured in the last 1-second window by isrTimerFlow.
static portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
static hw_timer_t* flowTimer = nullptr;

namespace modules {

static const TaggedLogger kLog("Flowmeter");

//////////////////////////////////// Flow Meter Interrupt Service Routines ////////////////////////////

// Counts one pulse per rising edge on the flow sensor pin.
// Only integer arithmetic: no float in ISR context.
void IRAM_ATTR isrFlowMeter() {
    portENTER_CRITICAL_ISR(&flowMux);
    flow_pulses++;
    portEXIT_CRITICAL_ISR(&flowMux);
}

// Fires once per second. Snapshots and resets the pulse counter.
// Float conversion is intentionally deferred to readFlow() (task context)
// to avoid FPU state corruption in ISR context.
void IRAM_ATTR isrTimerFlow() {
    portENTER_CRITICAL_ISR(&flowMux);
    flow_pulses_snapshot = flow_pulses;
    flow_pulses = 0;
    portEXIT_CRITICAL_ISR(&flowMux);
}

////////////////////////////////////// Public Methods //////////////////////////////////////

Flowmeter& Flowmeter::instance() {
    static Flowmeter instance;
    return instance;
}

bool Flowmeter::create_task() {
    if (queue_ != nullptr) {
        return true;
    }

    queue_ = xQueueCreate(kFlowmeterQueueLength, sizeof(definitions::CommandType));
    if (queue_ == nullptr) {
        kLog.error("No se pudo crear la cola del caudalimetro.");
        return false;
    }

    if (xTaskCreate(task_entry, kFlowmeterTaskName, kFlowmeterTaskStackSize, this, kFlowmeterTaskPriority, nullptr) != pdPASS) {
        kLog.error("No se pudo crear la tarea del caudalimetro.");
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }

    return true;
}

void Flowmeter::set_response_queue(interfaces::ICommandResponseQueue* queue) {
    response_queue_ = queue;
}

bool Flowmeter::begin(uint32_t pinFlow_, float pulsesPerLiter) {
    // Guard against double-initialization: attaching the interrupt or
    // creating the timer a second time would corrupt hardware state.
    if (status_) {
        return true;
    }

    if (pinFlow_ == 0) {
        kLog.error("Pin invalido para el caudalimetro.");
        return false;
    }

    pin_ = pinFlow_;
    pulsesPerLiter_ = pulsesPerLiter;
    hal_general().pin_mode(pin_, PIN_MODE_INPUT);

    attachInterrupt(digitalPinToInterrupt(pinFlow_), isrFlowMeter, RISING);

    flowTimer = timerBegin(0, 80, true);  // timer 0, prescaler 80 → 1 tick = 1 µs
    timerAttachInterrupt(flowTimer, &isrTimerFlow, true);
    timerAlarmWrite(flowTimer, 1000000, true);  // 1 sec
    timerAlarmEnable(flowTimer);

    status_ = true;
    return true;
}

void Flowmeter::stop() {
    detachInterrupt(digitalPinToInterrupt(pin_));
    if (flowTimer != nullptr) {
        timerEnd(flowTimer);
        flowTimer = nullptr;
    }
    status_ = false;
}

float Flowmeter::readFlow() {
    if (!status_) {
        return NAN;
    }

    // Read the snapshot captured by the last timer ISR tick (task-safe critical section).
    uint32_t pulses;
    portENTER_CRITICAL(&flowMux);
    pulses = flow_pulses_snapshot;
    portEXIT_CRITICAL(&flowMux);

    // Convert pulses/second → liters/minute in task context (safe for FPU).
    return (static_cast<float>(pulses) / pulsesPerLiter_) * 60.0f;
}

bool Flowmeter::status() const {
    return status_;
}

uint32_t Flowmeter::pin() const {
    return pin_;
}

bool Flowmeter::queue_command(definitions::CommandType command, uint32_t timeout_ms) {
    if (queue_ == nullptr || xQueueSend(queue_, &command, pdMS_TO_TICKS(timeout_ms)) != pdPASS) {
        return false;
    }

    return true;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Flowmeter::Flowmeter() : pin_(0), status_(false), pulsesPerLiter_(450.0f), queue_(nullptr), response_queue_(nullptr) {}

bool Flowmeter::process_command(definitions::CommandType command) {
    if (command != definitions::CommandType::kFlowRead) {
        return false;
    }

    float current_flow = readFlow();
    uint32_t flow_rate_bits;
    std::memcpy(&flow_rate_bits, &current_flow, sizeof(float));
    publish_response({definitions::CommandType::kFlowRead, flow_rate_bits});
    return true;
}

bool Flowmeter::publish_response(const definitions::Command& response) {
    if (response_queue_ == nullptr) {
        return false;
    }

    return response_queue_->publish(response, kFlowmeterTimeoutMs);
}

void Flowmeter::task_entry(void* param) {
    Flowmeter* self = static_cast<Flowmeter*>(param);
    definitions::CommandType command = definitions::CommandType::kFlowRead;

    while (true) {
        if (xQueueReceive(self->queue_, &command, kFlowmeterTaskWatchdogTicks) == pdPASS) {
            self->process_command(command);
        }
        if (self->queue_ == nullptr) {
            break;
        }
    }
}

}  // namespace modules
