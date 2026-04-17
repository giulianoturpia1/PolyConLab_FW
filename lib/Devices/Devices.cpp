#include "Devices.hpp"

#include <Ethernet.h>
#include <SPI.h>

#include "HALGeneral.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#else
#endif

volatile uint32_t flow_pulses = 0;
portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* timer = NULL;
const float pulsesPerLiter = 450.0;  // A configurar segun el sensor
volatile float flow_rate = 0;

// -------- FLOW METER FUNCTIONS ---------

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

void initFlowMeter(uint32_t pinFlow_) {
    modules::hal_general().pin_mode(pinFlow_, modules::PIN_MODE_INPUT);
    attachInterrupt(digitalPinToInterrupt(pinFlow_), isrFlowMeter, RISING);

    timer = timerBegin(0, 80, true);  // timer 0, prescaler 80 → 1 tick = 1 µs
    timerAttachInterrupt(timer, &isrTimerFlow, true);
    timerAlarmWrite(timer, 1000000, true);  // 1 seC
    timerAlarmEnable(timer);
}

void stopFlowMeter(uint32_t pinFlow_) {
    detachInterrupt(digitalPinToInterrupt(pinFlow_));
    timerEnd(timer);
    if (timer != NULL) {
        timerEnd(timer);
        timer = NULL;
    }
    timer = NULL;
}

float readFlowMeter() {
    float current_flow;
    portENTER_CRITICAL(&flowMux);
    current_flow = flow_rate;
    portEXIT_CRITICAL(&flowMux);
    return (current_flow);
}
