#include "Devices.hpp"

#include <Ethernet.h>
#include <SPI.h>

#include "Chiller.hpp"
#include "Definitions.hpp"
#include "HALGeneral.hpp"
#include "Pump.hpp"
#include "SolenoidValve.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#else
#endif

bool heaterStatusFlag = false;

volatile uint32_t flow_pulses = 0;
portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* timer = NULL;
const float pulsesPerLiter = 450.0;  // A configurar segun el sensor
volatile float flow_rate = 0;

void setupDevices() {
    // Configuracion de pines y estado inicial de los dispositivos
    modules::Pump::instance().begin(static_cast<uint32_t>(definitions::Pins::kPump));
    modules::Chiller::instance().begin(static_cast<uint32_t>(definitions::Pins::kChiller));
    uint32_t valve_pins[] = {static_cast<uint32_t>(definitions::Pins::kValve1),
                             static_cast<uint32_t>(definitions::Pins::kValve2)};
    modules::SolenoidValve::instance().begin(valve_pins, 2);
}

// -------- HEATER FUNCTIONS ---------
void heaterON(uint32_t relayHeater_) {
    modules::hal_general().digital_write(relayHeater_, modules::PIN_HIGH);
    heaterStatusFlag = true;
}

void heaterOFF(uint32_t relayHeater_) {
    modules::hal_general().digital_write(relayHeater_, modules::PIN_LOW);
    heaterStatusFlag = false;
}

bool heaterStatus() {
    return (heaterStatusFlag);
}

// -------- TEMPERATURE SENSOR FUNCTIONS ---------
float readTempSensor(uint32_t sensor_) {
    // TODO: Implement actual sensor reading logic based on the specific temperature sensor used (e.g., DS18B20,
    // thermocouple, etc.) This is a placeholder implementation that returns a dummy value.
    return 25.0;  // Placeholder value - replace with actual sensor reading
}

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
