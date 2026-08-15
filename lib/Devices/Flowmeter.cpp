#include "Flowmeter.hpp"

#include <cmath>
#include <cstring>

#include "Definitions.hpp"
#include "HALGeneral.hpp"
#include "Logger.hpp"

static volatile uint32_t flow_pulses = 0;           ///< Raw pulse counter, incremented by isrFlowMeter.
static volatile uint32_t flow_pulses_snapshot = 0;  ///< Pulses captured in the last 1-second window by isrTimerFlow.
static portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
static hw_timer_t* flowTimer = nullptr;
int timeWindowSeconds = 1;
long secondfromScaler = 1000000;
long timeWindowfromScaler = timeWindowSeconds * secondfromScaler;  // timeWindowSeconds seconds

namespace modules {

static const TaggedLogger kLog("Flowmeter");

//////////////////////////////////// Flow Meter Interrupt Service Routines ////////////////////////////

// Counts one pulse per falling edge on the flow sensor pin.
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
    hal_general().pin_mode(pin_, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(27), isrFlowMeter, FALLING);

    flowTimer = timerBegin(0, 80, true);  // timer 0, prescaler 80 → 1 tick = 1 µs
    if (flowTimer == nullptr) {
        kLog.error("No se pudo inicializar el timer del caudalimetro.");
        return false;
    }

    timerAttachInterrupt(flowTimer, &isrTimerFlow, true);
    timerAlarmWrite(flowTimer, timeWindowfromScaler, true);
    timerAlarmEnable(flowTimer);

    status_ = true;
    return true;
}

bool Flowmeter::accepts(definitions::CommandType type) const {
    return type == definitions::CommandType::kFlowRead;
}

////////////////////////////////////// Private Methods //////////////////////////////////////

Flowmeter::Flowmeter() : pin_(0), status_(false), pulsesPerLiter_(450.0f) {}

bool Flowmeter::execute_command(const definitions::Command& cmd, definitions::Command& out_response) {
    if (cmd.type != definitions::CommandType::kFlowRead) {
        return false;
    }

    float current_flow = readFlow();
    uint32_t flow_rate_bits;
    std::memcpy(&flow_rate_bits, &current_flow, sizeof(float));
    kLog.info("Publicando respuesta del flujo:" + std::string(" L/min ") + std::to_string(current_flow));
    out_response = {definitions::CommandType::kFlowRead, flow_rate_bits};
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

    // Read both counters in a single critical section for a consistent snapshot.
    uint32_t pulses, pulses_raw;
    portENTER_CRITICAL(&flowMux);
    pulses = flow_pulses_snapshot;
    pulses_raw = flow_pulses;
    portEXIT_CRITICAL(&flowMux);

    // Convert pulses/second → liters/minute in task context (safe for FPU).
    kLog.debug("Calculando flujo: snapshot=" + std::to_string(pulses) + " raw=" + std::to_string(pulses_raw) + " pulsos");
    float pulseFrequency = static_cast<float>(pulses) / static_cast<float>(timeWindowSeconds);  // pulses per second

    return float((pulseFrequency * definitions::kReferenceFlow) / definitions::kReferenceFrequencyofPulsesPerSecond) *
           60.0f;  // Convert to liters per minute
}

bool Flowmeter::status() const {
    return status_;
}

uint32_t Flowmeter::pin() const {
    return pin_;
}

}  // namespace modules
