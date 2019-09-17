
#include <UIPEthernet.h>
#include "PubSubClient.h"

#define R1 A0
#define R2 A1
#define R3 A2
#define R4 A3
#define R5 A4
#define R6 A5
#define R7 2
#define R8 3

const byte relays[8] = { A0, A1, A2, A3, A4, A5, 2, 3};

EthernetClient ethClient;
PubSubClient client;

long previousMillis;

const byte mac[6] = {0xA0,0xA1,0xA2,0xA3,0xA4,0x01};
IPAddress ip( 192, 168,   1,  203);
boolean isEthUp = false;

const char* server = "192.168.1.212";
const char* token = "
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
    digitalWrite(relays[id-1], !isOn);    
  }  
}

void setup() {
  // setup serial communication
  Serial.begin(9600);
  delay(1000);
  
  for(int i=0; i<8; i++) {
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
  if(!isEthUp) {
    Serial.print(F("Get Ethernet link ..."));
//    isEthUp = Ethernet.begin(mac);

    Ethernet.begin(mac, ip);
    isEthUp = true;

    if(isEthUp) {
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
