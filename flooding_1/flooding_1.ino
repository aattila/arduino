
#include "LedControl.h"
#include <EtherCard.h>
#include <Thread.h>

Thread sensorThread = Thread();
Thread alarmThread = Thread();

/*
 pin 7 is connected to the DataIn 
 pin 6 is connected to the CLK 
 pin 5 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc = LedControl(7,6,5,1);
unsigned long delaytime = 1000;

// alarm triggering levels based on the display's 8 leds
static byte   triggers[] = {1, 4, 4, 4, 4, 4, 4, 4};
// alarm reference HIGH means the alarm is triggered when the level less then, LOW means when the level more then
static boolean    refs[] = {HIGH, HIGH, LOW, LOW, LOW, LOW, LOW, LOW};
// analog input default level
static boolean analog_level[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
// relay digital default level
static boolean relay_level[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
// digital IO for the relay connections
static byte   relayDIO[] = {1, 2, 3, 3, 3, 3, 3, 3};
// alarms array for the triggers
static boolean  alarms[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};

static byte values[] = {0, 0, 0, 0, 0, 0, 0, 0};
static byte levels[] = {0, 0, 0, 0, 0, 0, 0, 0};


// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x00,0x00,0xAA,0xDD,0x00,0x01 };
static byte myip[] = { 192,168,1,204 };
static byte gwip[] = { 192,168,1,1 };

byte Ethernet::buffer[500];
BufferFiller bfill;


void displayLevel(int senzor, int level) {
  // show the level info
  for(int i=0;i<level;i++) {
    // the display is positioned in analog_level so this is way 7-sensor
    lc.setLed(0, i, 7-senzor, true);    
  }
}

int calculateLevel(int analogValue) {
  
    int level;
    // 0-0.25V 
    if(analogValue < 51) {
      level = 0;
    }
    // 0.25-1V this is basically noise
    else if(analogValue >= 51 and analogValue < 204 ) {
      level = 1;
    }
    // 1-1.5V
    else if(analogValue >= 204 and analogValue < 306 ) {
      level = 2;
    }
    // 1.5-2V
    else if(analogValue >= 306 and analogValue < 408 ) {
      level = 3;
    }
    // 2-2.5V
    else if(analogValue >= 408 and analogValue < 510 ) {
      level = 4;
    }
    // 2.5-3V
    else if(analogValue >= 510 and analogValue < 612 ) {
      level = 5;
    }
    // 3-3.5V
    else if(analogValue >= 612 and analogValue < 714 ) {
      level = 6;
    }
    // 3.5-4V
    else if(analogValue >= 714 and analogValue < 816 ) {
      level = 7;
    }
    // bigger than 4V
    else if(analogValue >= 816 ) {
      level = 8;
    }

  return level;
}

void levelCheck() {
  lc.clearDisplay(0);
  
  for (int senzor=0; senzor<8; senzor++) {
    int analogValue = analogRead(senzor);
    if(analog_level[senzor] == HIGH) {
      analogValue = 1023 - analogValue;
    }
    values[senzor] = analogValue;
    levels[senzor] = calculateLevel(analogValue);
//    levels[senzor] = map(analogValue, 0, 1023, 1, 8);
    
    displayLevel(senzor, levels[senzor]);
    if ( refs[senzor] == HIGH) { 
      alarms[senzor] = levels[senzor] <= triggers[senzor];
    }
    else {
      alarms[senzor] = levels[senzor] >= triggers[senzor];
    }
  }
}

void alarmCheck() {
  
  for (int relay=0; relay<8; relay++) {
    int io = relayDIO[relay];
    bool needsSet = false;
    for (int senzor=0; senzor<8; senzor++) {
      if(relayDIO[senzor] == io and alarms[senzor] == true) {
        needsSet = true;
        break; 
      }
    }

    if(relay_level[relay] == HIGH) {
      digitalWrite(relayDIO[relay], !needsSet);
    } else {
      digitalWrite(relayDIO[relay], needsSet);
    }
  }

}

static word homePage() {
  
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "{"
    "\"alarms\" : [ $D, $D, $D, $D, $D, $D, $D, $D ], "
    "\"alarm_digital_pins\" : [ $D, $D, $D, $D, $D, $D, $D, $D ], "
    "\"digital_out_values\" : [ $D, $D, $D, $D, $D, $D, $D, $D ], "
    "\"trigger_values\" : [ $D, $D, $D, $D, $D, $D, $D, $D ], "
    "\"trigger_references\" : [ $D, $D, $D, $D, $D, $D, $D, $D ], "
    "\"analog_in_values\" : [ $D, $D, $D, $D, $D, $D, $D, $D ], "
    "\"levels\" : [ $D, $D, $D, $D, $D, $D, $D, $D ]"
    "}"
    ),
    alarms[0], alarms[1], alarms[2], alarms[3], alarms[4], alarms[5], alarms[6], alarms[7],
    relayDIO[0], relayDIO[1], relayDIO[2], relayDIO[3], relayDIO[4], relayDIO[5], relayDIO[6], relayDIO[7],
    digitalRead(relayDIO[0]), digitalRead(relayDIO[1]), digitalRead(relayDIO[2]), digitalRead(relayDIO[3]), digitalRead(relayDIO[4]), digitalRead(relayDIO[5]), digitalRead(relayDIO[6]), digitalRead(relayDIO[7]),
    triggers[0], triggers[1], triggers[2], triggers[3], triggers[4], triggers[5], triggers[6], triggers[7],
    refs[0], refs[1], refs[2], refs[3], refs[4], refs[5], refs[6], refs[7],
    values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7],
    levels[0], levels[1], levels[2], levels[3], levels[4], levels[5], levels[6], levels[7]
  );
  return bfill.position();
}


void setup() {
  lc.shutdown(0,false);
  lc.setIntensity(0,1);
  lc.clearDisplay(0);

  for(int i=0;i<8;i++) {
    pinMode(relayDIO[i], OUTPUT);
    digitalWrite(relayDIO[i], LOW);
  }
  
  sensorThread.onRun(levelCheck);
  sensorThread.setInterval(1000);

  alarmThread.onRun(alarmCheck);
  alarmThread.setInterval(2000);

  ether.begin(sizeof Ethernet::buffer, mymac);
  ether.staticSetup(myip, gwip);

}

void loop() { 
  if(sensorThread.shouldRun()) {
    sensorThread.run();
  }
  if(alarmThread.shouldRun()) {
    alarmThread.run();
  }

  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  if (pos)  // check if valid tcp data is received
    ether.httpServerReply(homePage()); // send web page data

//  levelSimulator();
}
