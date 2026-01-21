#include "Devices.hpp"

bool heaterStatusFlag = false;
bool pumpStatusFlag = false;
bool valvula1StatusFlag = false;
bool valvula2StatusFlag = false;
bool valvula3StatusFlag = false;

volatile unsigned int pulsos_caudal = 0;
portMUX_TYPE caudalMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *timer = NULL;
const float pulsosPorLitro = 450.0; // A configurar segun el sensor
volatile float caudal = 0;

void setupDevices() {
  // Configuracion de pines
  pinMode(RELAYHEATER, OUTPUT);
  pinMode(RELAYBOMBA, OUTPUT);
  pinMode(RELAY_AUS_1, OUTPUT);
  pinMode(RELAY_AUS_2, OUTPUT);
  pinMode(RELAY_AUS_3, OUTPUT);

  // Inicializar estados
  heaterOFF(RELAYHEATER);
  bombaOFF(RELAYBOMBA);
  valveOFF(RELAY_AUS_1);
  valveOFF(RELAY_AUS_2);
  valveOFF(RELAY_AUS_3);
}



// -------- HEATER FUNCTIONS ---------
void heaterON(int relayHeater_){
  digitalWrite(relayHeater_,HIGH);
  heaterStatusFlag = true;
}

void heaterOFF(int relayHeater_){
  digitalWrite(relayHeater_,LOW);
  heaterStatusFlag = false;
}

bool heaterStatus(){
  return(heaterStatusFlag);
}

// -------- TEMPERATURE SENSOR FUNCTIONS ---------
float leerSensorTemp(uint8_t sensor_){
  String respuesta_sensor;
  while(Serial1.available()){Serial1.read();/*limpio el buffer de datos*/}
  Serial1.println(sensor_);//envio comando al esp32 externo
  esperarLectura:
    unsigned long t_inicial = millis();
    while(!Serial1.available() && ((millis() - t_inicial) < INTERVALO_TEMP));//seria bueno agregar un timeout
    if((millis() - t_inicial) >= INTERVALO_TEMP){
      return ERR_TEMP;//codigo de error de lectura
      }
    else{
      respuesta_sensor = Serial1.readStringUntil('\n');
      if(respuesta_sensor == "Conversion not complete!" ){
        goto esperarLectura;
        }
      }
  respuesta_sensor.trim();
  float temp = respuesta_sensor.toFloat();
  Serial.println("Temperatura leida de " + String(sensor_) + ": " + String(temp) + " ºC");
  return(temp);
}

// -------- PUMP FUNCTIONS ---------
void bombaON(int relayBomba_){
  digitalWrite(relayBomba_,HIGH);
  pumpStatusFlag = true;
  return;
}

void bombaOFF(int relayBomba_){
  digitalWrite(relayBomba_,LOW);
  pumpStatusFlag = false;
  return;
}

bool statusPump(){
  return(pumpStatusFlag);
}

// -------- VALVE FUNCTIONS ---------
void valveON(int relayValvula_){
  digitalWrite(relayValvula_,HIGH);
  if(relayValvula_ == RELAY_AUS_1) {
    valvula1StatusFlag = true;
  } else if(relayValvula_ == RELAY_AUS_2) {
    valvula2StatusFlag = true;
  } else if(relayValvula_ == RELAY_AUS_3) {
    valvula3StatusFlag = true;
  }
  return;
}

void valveOFF(int relayValvula_){
  digitalWrite(relayValvula_,LOW);
  if(relayValvula_ == RELAY_AUS_1) {
    valvula1StatusFlag = false;
  } else if(relayValvula_ == RELAY_AUS_2) {
    valvula2StatusFlag = false;
  } else if(relayValvula_ == RELAY_AUS_3) {
    valvula3StatusFlag = false;
  }
  return;
}

bool valveStatus(int relayValvula_){
  if(relayValvula_ == RELAY_AUS_1) {
    return(valvula1StatusFlag);
  } else if(relayValvula_ == RELAY_AUS_2) {
    return(valvula2StatusFlag);
  } else if(relayValvula_ == RELAY_AUS_3) {
    return(valvula3StatusFlag);
  } else {
    return false; 
  }
}

// -------- FLOW METER FUNCTIONS ---------

void IRAM_ATTR isrCaudalimetro(){
  portENTER_CRITICAL_ISR(&caudalMux);
  pulsos_caudal++;
  portEXIT_CRITICAL_ISR(&caudalMux);
  }

void IRAM_ATTR isrTimerCaudal(){
  portENTER_CRITICAL_ISR(&caudalMux);
  unsigned int pulsos = pulsos_caudal;
  pulsos_caudal = 0;
  portEXIT_CRITICAL_ISR(&caudalMux);
  float litros = pulsos / pulsosPorLitro;
  caudal = litros * 60.0; // convertir a L/min
}

void initCaudalimetro(int pinQ_){
  pinMode(pinQ_,INPUT);
  attachInterrupt(digitalPinToInterrupt(pinQ_),isrCaudalimetro,RISING);

  timer = timerBegin(0, 80, true);// timer 0, prescaler 80 → 1 tick = 1 µs
  timerAttachInterrupt(timer, &isrTimerCaudal, true);
  timerAlarmWrite(timer, 1000000, true);// 1 segundo
  timerAlarmEnable(timer);
  }

void detenerCaudalimetro(int pinQ_){
  detachInterrupt(digitalPinToInterrupt(pinQ_));
  timerEnd(timer);
  if (timer != NULL) {
    timerEnd(timer);
    timer = NULL;
    }
  timer=NULL;
  }

  float leerCaudalimetro(){
  float caudal_actual;
  portENTER_CRITICAL(&caudalMux);
  caudal_actual = caudal;
  portEXIT_CRITICAL(&caudalMux);
  return(caudal_actual);
}

