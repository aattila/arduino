

#include <TimerOne.h>
#include <Wire.h>
#include <MultiFuncShield.h>  // https://www.mpja.com/download/hackatronics-arduino-multi-function-shield.pdf
#include <SoftwareSerial.h>

#define RX_PIN    5
#define TX_PIN    6
#define RELAY_PIN 9
#define PRESS_PIN A5

SoftwareSerial RS485Serial(RX_PIN, TX_PIN);
String prevMsg = "";

const float pressureZero = 97.0; // analog reading of pressure transducer at 0psi
const float pressureMax  = 920.7; // analog reading of pressure transducer at 100psi
const int   sensorMax    = 12;    // Bar value of transducer being used
const int   readStep     = 20;

const float minPress = 1.5;
const float maxPress = 3.0;
const float segment = (maxPress-minPress)/4;
const float segment1 = minPress+segment;
const float segment2 = minPress+segment*2;
const float segment3 = minPress+segment*3;
const float alarmPress = maxPress+segment;

float currentPressure = 0.0;

bool manualOn = false;
bool relayOn = false;

const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

void setup() {
  
  Serial.begin(9600);
  RS485Serial.begin(9600);
  
  Timer1.initialize();
  MFS.initialize(&Timer1); 

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  
  // 4 short beeps, repeated 1 time
//  MFS.beep(5, // beep for 50 milliseconds
//           5, // silent for 50 milliseconds
//           4, // repeat above cycle 4 times
//           1, // loop 1 time
//           50 // wait 500 milliseconds between loop
//  );

  // initialize the smoothing buffer
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

}

void loop() {
  
  readSensor();
  handleButtons();
  
  if(!manualOn) {
    handleLevel();  
    MFS.write(currentPressure, 2);
  }

  String msg = "2:{\"pressure\":" + String(currentPressure, 2) +",\"relay\":"+ String(isRelayOn()) +",\"alarm\":" + String(currentPressure >= alarmPress) + "}";
  if(prevMsg != msg) {
    Serial.println(msg);
    RS485Serial.println(msg);  
    prevMsg = msg;      
  }

  delay(200); 
}

void readSensor() {
  total = total - readings[readIndex];
  int v = analogRead(PRESS_PIN);
  
  // negative values not allowed
  if(v < 0) {
    v = 0;
  }

  // handle read steps to filter out analog hazard
  if(v > (average+readStep) or v < (average-readStep)) {
    readings[readIndex] = v;
  }
  
//  Serial.println(v);
//  Serial.println(readings[readIndex]);
  total = total + readings[readIndex];

  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  average = total / numReadings;
//  Serial.println(average);
  currentPressure = ((average-pressureZero)*sensorMax)/(pressureMax-pressureZero);
}

void handleLevel() {
  if(currentPressure < minPress) {
    MFS.blinkLeds(LED_ALL, OFF);
    MFS.writeLeds(LED_ALL, OFF);
    handleRelay(true);
  } else if(currentPressure >= minPress and currentPressure < segment1) {
    MFS.writeLeds(LED_4, ON);
    MFS.writeLeds(LED_1 | LED_2 | LED_3, OFF);
    if(isRelayOn()) {
      MFS.blinkLeds(LED_4, ON);
    }
  } else if(currentPressure >= segment1 and currentPressure < segment2) {
    MFS.writeLeds(LED_3 | LED_4, ON);
    MFS.writeLeds(LED_1 | LED_2, OFF);
    if(isRelayOn()) {
      MFS.blinkLeds(LED_3 | LED_4, ON);
    }
  } else if(currentPressure >= segment2 and currentPressure < segment3) {
    MFS.writeLeds(LED_2 | LED_3 | LED_4, ON);
    MFS.writeLeds(LED_1, OFF);
    if(isRelayOn()) {
      MFS.blinkLeds(LED_2 | LED_3 | LED_4, ON);
    }
  } else if(currentPressure >= maxPress) {
    MFS.blinkLeds(LED_ALL, OFF);
    MFS.writeLeds(LED_ALL, ON);
    handleRelay(false);
  } else if(currentPressure >= alarmPress) {
    MFS.blinkLeds(LED_ALL, ON);
    // short beep for 200 milliseconds (non blocking)
    MFS.beep();
    handleRelay(false);
  }
}

void handleButtons() {
  byte btn = MFS.getButton();
  if (btn) {
     byte buttonNumber = btn & B00111111;
     byte buttonAction = btn & B11000000;
  
//     Serial.print("BUTTON_");
//     Serial.write(buttonNumber + '0');
//     Serial.print("_");

     if (buttonAction == BUTTON_PRESSED_IND) {
//       Serial.println("PRESSED");
       if(buttonNumber == 1) {
         manualOn = !manualOn;
         handleManual();
       }
     } else if (buttonAction == BUTTON_SHORT_RELEASE_IND) {
//       Serial.println("SHORT_RELEASE");
     } else if (buttonAction == BUTTON_LONG_PRESSED_IND) {
//       Serial.println("LONG_PRESSED");
     } else if (buttonAction == BUTTON_LONG_RELEASE_IND) {
//       Serial.println("LONG_RELEASE");
     }
  }  
}

void handleManual() {
  if(manualOn) {
    MFS.write("on"); 
    MFS.blinkDisplay(DIGIT_ALL, ON);
    handleRelay(true);
  } else {
    MFS.blinkDisplay(DIGIT_ALL, OFF);
    MFS.write("off"); 
    handleRelay(false);
    delay(1000);
    MFS.write(""); 
  }
}

void handleRelay(bool isEnabled) {
  // smoothing buffer fillup, no relay action meanwhile
  if (millis() < 30000) {
    digitalWrite(RELAY_PIN, HIGH);
    return;
  }

  if(isEnabled) {
    digitalWrite(RELAY_PIN, LOW);
//    delay(2000); // filtering out the pressure spike 
  } else {
    digitalWrite(RELAY_PIN, HIGH);
  }
}

bool isRelayOn() {
  return !digitalRead(RELAY_PIN);
}
