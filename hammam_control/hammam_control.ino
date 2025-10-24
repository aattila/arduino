

#include <credentials.h>
#include <SPI.h>
#include <Ethernet.h>
#include "PubSubClient.h"

#define DEBUG 0

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

unsigned char relayPin[4] = {4,5,6,7};
unsigned long lastRenew;

byte mac[] = { 0x00, 0x00, 0x01, 0x01, 0x01, 0x01 };

void setup() {
  int i;
  for(i = 0; i < 4; i++) {
    pinMode(relayPin[i],OUTPUT);
  }

  
  Serial.begin(9600);
  while (!Serial) {
  }
  debugln("Hammam Controler Setup");

  Ethernet.init(10);
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    debugln("Ethernet shield is not installed or it is not funcional!");
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    debugln("Ethernet cable is not connected!");
  }
  debugln("");
  debug("DHCP responds with: ");
  debugln(Ethernet.localIP());

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);

  while (!mqttClient.connected()) {
    debug("Attempting MQTT connection...");
    if (mqttClient.connect("Hammam", MQTT_USER, MQTT_PASS)) {
      debugln("connected");
      mqttClient.subscribe("hammam/set");
      mqttClient.subscribe("hammam/get");
    } else {
      debug("failed, rc=");
      debug(mqttClient.state());
      debugln(" try again in 5 seconds");
            
      delay(5000);
    }
  }

}

void loop() {

   // DHCP maintain to keep the lease 2 min
  if (millis() > lastRenew + 120000) {
    Ethernet.maintain();
    lastRenew = millis(); 
  }

  mqttClient.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  debug("Message arrived [");
  debug(topic);
  debug("]: ");

  if(String(topic) == "hammam/set") {
    for (int i=0;i<length;i++) {
      char relayState = (char)payload[i];
      if(relayState == '0' || relayState == '1') {
        // char to number (48 = "0", 49 = "1")
        byte value = 48 - payload[i];
        digitalWrite(relayPin[i], value);
      }
      debug(relayState);
    }
  }

  if(String(topic) == "hammam/get") {
    byte status[] = { 48+digitalRead(relayPin[0]), 48+digitalRead(relayPin[1]), 48+digitalRead(relayPin[2]), 48+digitalRead(relayPin[3]) };
    mqttClient.publish("hammam/status", status, 4);
  }

  debugln();
}
