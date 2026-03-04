#include "Devices.hpp"

#include <Ethernet.h>
#include <SPI.h>

#include "Definitions.hpp"
#include "HALGeneral.hpp"
#include "Pump.hpp"

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#else
#endif

bool heaterStatusFlag = false;
bool valve1StatusFlag = false;
bool valve2StatusFlag = false;

volatile uint32_t flow_pulses = 0;
portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* timer = NULL;
const float pulsesPerLiter = 450.0;  // A configurar segun el sensor
volatile float flow_rate = 0;

void setupDevices() {
    // Configuracion de pines
    // modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::kHeater), modules::PIN_MODE_OUTPUT);
    // modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::kValve1), modules::PIN_MODE_OUTPUT);
    // modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::kValve2), modules::PIN_MODE_OUTPUT);

    // // Inicializar estados
    // heaterOFF(static_cast<uint32_t>(definitions::Pins::kHeater));
    modules::Pump::instance().begin(static_cast<uint32_t>(definitions::Pins::kPump));
    // valveOFF(static_cast<uint32_t>(definitions::Pins::kValve1));
    // valveOFF(static_cast<uint32_t>(definitions::Pins::kValve2));
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
    // TODO: Implement actual sensor reading logic based on the specific temperature sensor used (e.g., DS18B20, thermocouple, etc.)
    // This is a placeholder implementation that returns a dummy value.
    return 25.0;  // Placeholder value - replace with actual sensor reading
}

// -------- VALVE FUNCTIONS ---------
void valveON(uint32_t relayValve_) {
    modules::hal_general().digital_write(relayValve_, modules::PIN_HIGH);
    switch (relayValve_) {
    case static_cast<uint32_t>(definitions::Pins::kValve1):
        valve1StatusFlag = true;
        break;
    case static_cast<uint32_t>(definitions::Pins::kValve2):
        valve2StatusFlag = true;
        break;
    default:
        break;
    }
}

void valveOFF(uint32_t relayValve_) {
    modules::hal_general().digital_write(relayValve_, modules::PIN_LOW);
    switch (relayValve_) {
    case static_cast<uint32_t>(definitions::Pins::kValve1):
        valve1StatusFlag = false;
        break;
    case static_cast<uint32_t>(definitions::Pins::kValve2):
        valve2StatusFlag = false;
        break;
    default:
        break;
    }
}

bool valveStatus(uint32_t relayValve_) {
    switch (relayValve_)
    {
    case static_cast<uint32_t>(definitions::Pins::kValve1):
        return (valve1StatusFlag);
        break;

    case static_cast<uint32_t>(definitions::Pins::kValve2):
        return (valve2StatusFlag);
        break;

    default:
        return false;
        break;
    }
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
