#include "Devices.hpp"
#include "HALGeneral.hpp"
#include "Pump.hpp"
#include "Definitions.hpp"

#include <Ethernet.h>
#include <SPI.h>

#if defined(PLC_38AR)
#include "PLC38AR.hpp"
#elif defined(PLC_19R)
#include "PLC19R.hpp"
#else
#endif

bool heaterStatusFlag = false;
bool valve1StatusFlag = false;
bool valve2StatusFlag = false;
bool valve3StatusFlag = false;

volatile uint32_t flow_pulses = 0;
portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* timer = NULL;
const float pulsesPerLiter = 450.0;  // A configurar segun el sensor
volatile float flow_rate = 0;

void setupDevices() {
    // Configuracion de pines
    //modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::RelayHeater), modules::PIN_MODE_OUTPUT);
    // modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::RelayAus1), modules::PIN_MODE_OUTPUT);
    // modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::RelayAus2), modules::PIN_MODE_OUTPUT);
    // modules::hal_general().pin_mode(static_cast<uint32_t>(definitions::Pins::RelayAus3), modules::PIN_MODE_OUTPUT);

    // // Inicializar estados
    //heaterOFF(static_cast<uint32_t>(definitions::Pins::RelayHeater));
    modules::Pump::instance().begin(static_cast<uint32_t>(definitions::Pins::RelayPump));
    //valveOFF(static_cast<uint32_t>(definitions::Pins::RelayAus1));
    //valveOFF(static_cast<uint32_t>(definitions::Pins::RelayAus2));
    //valveOFF(static_cast<uint32_t>(definitions::Pins::RelayAus3));
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
    String sensor_response;
    while (Serial1.available()) {
        Serial1.read(); /*limpio el buffer de datos*/
    }
    Serial1.println(sensor_);  // envio comando al esp32 externo
awaitReading:
    unsigned long initial_time = millis();
    while (!Serial1.available() && ((millis() - initial_time) < static_cast<unsigned long>(definitions::Constants::TEMP_INTERVAL_MS)));  // seria bueno agregar un timeout
    if ((millis() - initial_time) >= static_cast<unsigned long>(definitions::Constants::TEMP_INTERVAL_MS)) {
        return static_cast<float>(definitions::ErrorCode::ERR_TEMP);  // codigo de error de lectura
    } else {
        sensor_response = Serial1.readStringUntil('\n');
        if (sensor_response == "Conversion not complete!") {
            goto awaitReading;
        }
    }
    sensor_response.trim();
    float temp = sensor_response.toFloat();
    Serial.println("Temperatura leida de " + String(sensor_) + ": " + String(temp) + " ºC");
    return (temp);
}

// -------- VALVE FUNCTIONS ---------
void valveON(uint32_t relayValve_) {
    modules::hal_general().digital_write(relayValve_, modules::PIN_HIGH);
    if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus1)) {
        valve1StatusFlag = true;
    } else if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus2)) {
        valve2StatusFlag = true;
    } else if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus3)) {
        valve3StatusFlag = true;
    }
    return;
}

void valveOFF(uint32_t relayValve_) {
    modules::hal_general().digital_write(relayValve_, modules::PIN_LOW);
    if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus1)) {
        valve1StatusFlag = false;
    } else if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus2)) {
        valve2StatusFlag = false;
    } else if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus3)) {
        valve3StatusFlag = false;
    }
    return;
}

bool valveStatus(uint32_t relayValve_) {
    if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus1)) {
        return (valve1StatusFlag);
    } else if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus2)) {
        return (valve2StatusFlag);
    } else if (relayValve_ == static_cast<uint32_t>(definitions::Pins::RelayAus3)) {
        return (valve3StatusFlag);
    } else {
        return false;
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
