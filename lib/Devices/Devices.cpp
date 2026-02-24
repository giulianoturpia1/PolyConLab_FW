#include "Devices.hpp"

bool heaterStatusFlag = false;
bool pumpStatusFlag = false;
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
    pinMode(RELAYHEATER, OUTPUT);
    pinMode(RELAYPUMP, OUTPUT);
    pinMode(RELAY_AUS_1, OUTPUT);
    pinMode(RELAY_AUS_2, OUTPUT);
    pinMode(RELAY_AUS_3, OUTPUT);

    // Inicializar estados
    heaterOFF(RELAYHEATER);
    pumpOFF(RELAYPUMP);
    valveOFF(RELAY_AUS_1);
    valveOFF(RELAY_AUS_2);
    valveOFF(RELAY_AUS_3);
}

// -------- HEATER FUNCTIONS ---------
void heaterON(uint32_t relayHeater_) {
    digitalWrite(relayHeater_, HIGH);
    heaterStatusFlag = true;
}

void heaterOFF(uint32_t relayHeater_) {
    digitalWrite(relayHeater_, LOW);
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
    while (!Serial1.available() && ((millis() - initial_time) < TEMP_INTERVAL));  // seria bueno agregar un timeout
    if ((millis() - initial_time) >= TEMP_INTERVAL) {
        return ERR_TEMP;  // codigo de error de lectura
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

// -------- PUMP FUNCTIONS ---------
void pumpON(uint32_t relayPump_) {
    digitalWrite(relayPump_, HIGH);
    pumpStatusFlag = true;
    return;
}

void pumpOFF(uint32_t relayPump_) {
    digitalWrite(relayPump_, LOW);  // Generar modulo drivers
    pumpStatusFlag = false;
    return;
}

bool statusPump() {
    return (pumpStatusFlag);
}

// -------- VALVE FUNCTIONS ---------
void valveON(uint32_t relayValve_) {
    digitalWrite(relayValve_, HIGH);
    if (relayValve_ == RELAY_AUS_1) {
        valve1StatusFlag = true;
    } else if (relayValve_ == RELAY_AUS_2) {
        valve2StatusFlag = true;
    } else if (relayValve_ == RELAY_AUS_3) {
        valve3StatusFlag = true;
    }
    return;
}

void valveOFF(uint32_t relayValve_) {
    digitalWrite(relayValve_, LOW);
    if (relayValve_ == RELAY_AUS_1) {
        valve1StatusFlag = false;
    } else if (relayValve_ == RELAY_AUS_2) {
        valve2StatusFlag = false;
    } else if (relayValve_ == RELAY_AUS_3) {
        valve3StatusFlag = false;
    }
    return;
}

bool valveStatus(uint32_t relayValve_) {
    if (relayValve_ == RELAY_AUS_1) {
        return (valve1StatusFlag);
    } else if (relayValve_ == RELAY_AUS_2) {
        return (valve2StatusFlag);
    } else if (relayValve_ == RELAY_AUS_3) {
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
    pinMode(pinFlow_, INPUT);
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
