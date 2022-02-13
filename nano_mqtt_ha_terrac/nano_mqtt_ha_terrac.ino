
#include <credentials.h>

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

byte relays[8] = { R1, R2, R3, R4, R5, R6, R7, R8};

EthernetClient ethClient;
PubSubClient client;

const byte mac[6] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0x01};
boolean isEthUp = false;

void callback(char* topic, byte* payload, unsigned int length) {

  int len = strlen(topic);
  char relay = topic[len-1]; 
  
  byte id = relay - '0';
  boolean isOn = ((char)payload[1] == 'n');
  Serial.print(F("Relay "));
  Serial.print(id);
  Serial.print(F(": "));
  Serial.println(isOn);
  digitalWrite(relays[id - 1], !isOn);
}


void setup() {
  // setup serial communication
  Serial.begin(9600);
  delay(1000);

  for (int i = 0; i < 8; i++) {
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i], HIGH);
  }

}

void reconnect() {
  if (!isEthUp) {
    Serial.print(F("Get Ethernet link ..."));
    isEthUp = Ethernet.begin(mac);

    if (isEthUp) {
      Serial.println(F("UP"));
      // setup mqtt client
      client.setClient(ethClient);
      client.setServer(MQTT_SERVER, MQTT_PORT);
      client.setCallback(callback);
      Serial.println(F("MQTT client configured"));
      delay(1000);
    } else {
      Serial.println(F("DOWN"));
      delay(5000);
    }
  }

  while (isEthUp and !client.connected()) {
    Serial.print(F("Connecting to MQTT ..."));
    if ( client.connect(MQTT_DEV_TERRACE, MQTT_USER, MQTT_PASS) ) {
      client.subscribe("terrace/relay/1");
      client.subscribe("terrace/relay/2");
      client.subscribe("terrace/relay/3");
      client.subscribe("terrace/relay/4");
      client.subscribe("terrace/relay/5");
      client.subscribe("terrace/relay/6");
      client.subscribe("terrace/relay/7");
      client.subscribe("terrace/relay/8");
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
