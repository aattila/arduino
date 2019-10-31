
#include <UIPEthernet.h>
#include "PubSubClient.h"

#define R1 A0
#define R2 A1
#define R3 A2
#define R4 A3
#define R5 A4
#define R6 A5
#define R7 8
#define R8 9

//10, 11, 12, 13 reserved for thr eth shield

#define IR_SYNC     3  //INT1
#define IR_DATA     5  //PWM

byte relays[8] = { R1, R2, R3, R4, R5, R6, R7, R8};
volatile uint16_t data = 0;
volatile unsigned long last_micros;
long lastDebounceTime = 0;  // the last time of the ISR event
long debounceDelay = 1000;  // the debounce time;

EthernetClient ethClient;
PubSubClient client;

const byte mac[6] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0x01};
IPAddress ip( 192, 168,   1,  203);
boolean isEthUp = false;

const char* server = "192.168.1.212";
const char* token = ""; //device token goes here
const char* device = "Terrace";
const char* attr_topic = "v1/devices/me/attributes";
const char* tele_topic = "v1/devices/me/telemetry";


void callback(char* topic, byte* payload, unsigned int length) {
  if ((char)payload[2] == 'R') {
    byte id = (char)payload[3] - '0';
    boolean isOn = ((char)payload[6] == 't');
    Serial.print(F("Relay "));
    Serial.print(id);
    Serial.print(F(": "));
    Serial.println(isOn);
    digitalWrite(relays[id - 1], !isOn);
  }
}


void irEvent() {
  if ((long)(micros() - last_micros) >= 2000 * 1000) { // 2 sec debouncing
    data = pulseIn(IR_DATA, HIGH);
    Serial.print("ISR: ");
    Serial.println(data);
    last_micros = micros();
  }
}

void setRelay(byte relay) {
  digitalWrite(relays[relay], !digitalRead(relays[relay]));
}

void setStar() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(relays[i], LOW);
  }
}

void shutDown() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(relays[i], HIGH);
  }
}

void setup() {
  // setup serial communication
  Serial.begin(9600);
  delay(1000);

  pinMode(IR_DATA, INPUT_PULLUP);
  pinMode(IR_SYNC, INPUT_PULLUP);
  //  attachInterrupt(digitalPinToInterrupt(IR_SYNC), irEvent, CHANGE);

  for (int i = 0; i < 8; i++) {
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i], HIGH);
  }


  // setup ethernet communication using DHCP
  //  if(Ethernet.begin(mac) == 0) {
  //    Serial.println(F("Ethernet configuration using DHCP failed"));
  //    for(;;);
  //  }
  // Ethernet.begin(mac, ip);


}

void reconnect() {
  if (!isEthUp) {
    Serial.print(F("Get Ethernet link ..."));
    //    isEthUp = Ethernet.begin(mac);

    Ethernet.begin(mac, ip);
    isEthUp = true;

    if (isEthUp) {
      Serial.println(F("UP"));
      // setup mqtt client
      client.setClient(ethClient);
      client.setServer(server, 1883);
      client.setCallback(callback);
      Serial.println(F("MQTT client configured"));
      delay(1000);
    } else {
      Serial.println(F("DOWN"));
      delay(5000);
    }
  }

  while (isEthUp and !client.connected()) {
    Serial.print(F("Connecting to ThingsBoard node ..."));
    if ( client.connect(device, token, NULL) ) {
      client.subscribe(attr_topic);
      Serial.println( F("[DONE]") );
    } else {
      Serial.print( F("[FAILED] [ rc = ") );
      Serial.print( client.state() );
      Serial.println( F(" : retrying in 5 seconds]") );
      delay( 5000 );
    }
  }
}

void loop() {
  //  if (!isEthUp or !client.connected() ) {
  //    reconnect();
  //  }

  if ( (millis() - lastDebounceTime) > debounceDelay) {
    data = pulseIn(IR_DATA, HIGH);
    if (data > 0) {
      Serial.print("PROC: ");
      Serial.println(data);
      if (data > 75 && data < 80 ) { //1
        setRelay(0);
      } else if (data > 150 && data < 160) { //2
        setRelay(1);
      } else if (data > 230 && data < 240) { //3
        setRelay(2);
      } else if (data > 305 && data < 315) { //4
        setRelay(3);
      } else if (data > 380 && data < 390) { //5
        setRelay(4);
      } else if (data > 460 && data < 470) { //6
        setRelay(5);
      } else if (data > 535 && data < 545) { //7
        setRelay(6);
      } else if (data > 615 && data < 625) { //8
        setRelay(7);
      } else if (data > 690 && data < 700) { //9
      } else if (data > 770 && data < 780) { //0
      } else if (data > 845 && data < 855) { //STAR
        setStar();
      } else if (data > 920 && data < 930) { //SHARP
        setRelay(7);
      } else if (data > 1001 && data < 1010) { //UP
      } else if (data > 1080 && data < 1090) { //DOWN
      } else if (data > 1155 && data < 1165) { //LEFT
      } else if (data > 1230 && data < 1240) { //RIGHT
      } else if (data > 1310 && data < 1320) { //OK
        shutDown();
      }
      data = 0;
    }
    lastDebounceTime = millis();
  } else {
    data = 0;
  }

  //  client.loop();
}
