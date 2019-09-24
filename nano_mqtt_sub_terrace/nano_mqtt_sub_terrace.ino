
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

//10, 11, 12, 13 is for eth shield

#define IR_DOUT1    4
#define IR_DOUT2    5
#define IR_DOUT3    6
#define IR_SYNC     2 //INT0

const byte relays[8] = { R1, R2, R3, R4, R5, R6, R7, R8};

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

void irEvent() { //ISR
  //handle inputs as bits
  bool b1 = digitalRead(IR_DOUT1);
  bool b2 = digitalRead(IR_DOUT2);
  bool b3 = digitalRead(IR_DOUT3);
  bool ID[3] = {b1, b2, b3};
  int recivedID = ID[0] | (ID[1] << 1) | (ID[2] << 2);
  switch (recivedID) {
    case 0:
      digitalWrite(relays[0], !digitalRead(relays[0]));
      break;
    case 1:
      digitalWrite(relays[1], !digitalRead(relays[1]));
      break;
    case 2:
      digitalWrite(relays[2], !digitalRead(relays[2]));
      break;
    case 3:
      digitalWrite(relays[3], !digitalRead(relays[3]));
      break;
    case 4:
      digitalWrite(relays[4], !digitalRead(relays[4]));
      break;
    case 5:
      for (int i = 0; i < 4; i++) {
        digitalWrite(relays[i], !digitalRead(relays[i]));
      }
      break;
    case 6:
      digitalWrite(relays[7], !digitalRead(relays[7]));
      break;
    case 7:
      for (int i = 0; i < 8; i++) {
        digitalWrite(relays[i], HIGH);
      }
      break;
  }
}

void setup() {
  // setup serial communication
  Serial.begin(9600);
  delay(1000);

  pinMode(IR_SYNC, INPUT_PULLUP);
  pinMode(IR_DOUT1, INPUT_PULLUP);
  pinMode(IR_DOUT2, INPUT_PULLUP);
  pinMode(IR_DOUT3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IR_SYNC), irEvent, CHANGE);

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
  if (!isEthUp or !client.connected() ) {
    reconnect();
  }
  client.loop();
}
