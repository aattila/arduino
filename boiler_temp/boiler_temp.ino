

#include <TimerOne.h>
#include <Wire.h>
#include <MultiFuncShield.h>  // https://www.mpja.com/download/hackatronics-arduino-multi-function-shield.pdf
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DEBUG 0

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

#define RX_PIN    5
#define TX_PIN    6
#define WATER_FLOW_PIN 2
#define ONE_WIRE_PIN   4

SoftwareSerial RS485Serial(RX_PIN, TX_PIN);
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature DS18B20(&oneWire);

String prevMsg = "";

float currentDebit = 0.0;
float currentTemp  = 0.0;
unsigned long lastEnvRead;

void flow_ISR() {
  currentDebit = currentDebit + 0.5;
}

void setup() {
  
  Serial.begin(9600);
  RS485Serial.begin(9600);
  DS18B20.begin();  
  
  Timer1.initialize();
  MFS.initialize(&Timer1); 

  pinMode(WATER_FLOW_PIN, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(WATER_FLOW_PIN), flow_ISR, FALLING);

// 4 short beeps, repeated 1 time
//  MFS.beep(5, // beep for 50 milliseconds
//           5, // silent for 50 milliseconds
//           4, // repeat above cycle 4 times
//           1, // loop 1 time
//           50 // wait 500 milliseconds between loop
//  );

  debugln("");
  debugln("Setup is Done.");
}

void loop() {
  
  String msg = "3:{\"temp\":" + String(currentTemp, 2) +",\"flow\":"+ String(currentDebit, 1) +"}";
  if(prevMsg != msg) {
    debugln(msg);
    RS485Serial.println(msg);  
    prevMsg = msg;      
  }

  // soft delayed sensor read tuned to 10s
  if (millis() > lastEnvRead + 10000) {
    readSensor();
    lastEnvRead = millis(); 
  }
}

void readSensor() {
  DS18B20.requestTemperatures();
  currentTemp = DS18B20.getTempCByIndex(0);
  debug("The temperature is: ");
  debugln(currentTemp);
  MFS.write(currentTemp, 2);
}


