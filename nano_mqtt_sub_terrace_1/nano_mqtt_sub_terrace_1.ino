
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

#define IR_DATA     5  //PWM

byte relays[8] = { R1, R2, R3, R4, R5, R6, R7, R8};
volatile uint16_t data = 0;
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


void setRelay(byte relay) {
  digitalWrite(relays[relay], !digitalRead(relays[relay]));
  Serial.println(relay + 1);
}

void setStar() {
  bool val = LOW;
  for (int i = 0; i < 4; i++) {
    if (digitalRead(relays[i]) == LOW) {
      val = HIGH;
      break;
    }
  }
  Serial.println("*");
  for (int i = 0; i < 4; i++) {
    digitalWrite(relays[i], val);
  }
}

void shutDown() {
  Serial.println("OK");
  for (int i = 0; i < 8; i++) {
    digitalWrite(relays[i], HIGH);
  }
}

void setup() {
  // setup serial communication
  Serial.begin(9600);
  delay(1000);

  pinMode(IR_DATA, INPUT_PULLUP);

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

  data = pulseIn(IR_DATA, HIGH);
  if ( (millis() - lastDebounceTime) > debounceDelay) {
    if (data > 0) {
      if (data > 60 && data < 90 ) { //1
        setRelay(0);
      } else if (data > 140 && data < 160) { //2
        setRelay(1);
      } else if (data > 220 && data < 250) { //3
        setRelay(2);
      } else if (data > 290 && data < 330) { //4
        setRelay(3);
      } else if (data > 350 && data < 400) { //5
        setRelay(4);
      } else if (data > 440 && data < 490) { //6
        setRelay(5);
      } else if (data > 510 && data < 560) { //7
        setRelay(6);
      } else if (data > 600 && data < 640) { //8
        setRelay(7);
      } else if (data > 670 && data < 720) { //9
      } else if (data > 750 && data < 800) { //0
      } else if (data > 820 && data < 870) { //STAR
        setStar();
      } else if (data > 900 && data < 950) { //SHARP
        setRelay(7);
      } else if (data > 990  && data < 1030) { //UP
      } else if (data > 1060 && data < 1110) { //DOWN
      } else if (data > 1130 && data < 1180) { //LEFT
      } else if (data > 1210 && data < 1260) { //RIGHT
      } else if (data > 1290 && data < 1340) { //OK
        shutDown();
      }
    }
    lastDebounceTime = millis();
  }

  client.loop();
}
