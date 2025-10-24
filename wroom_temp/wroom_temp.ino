
// Hardware: WeMos D1 ESP-Wroom-02 Nodemcu ESP8266
// Board: LOLIN(WEMOS) D1 D2 & mini 

#include <credentials.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "PubSubClient.h"

const int oneWireBus = D4;   // GPIO where the DS18B20 is connected to

WiFiClient espClient;
PubSubClient client(espClient);

OneWire onewire(oneWireBus);
DallasTemperature sensors(&onewire);


void shutDown() {
  Serial.println(F("Going to sleep"));
  delay(100);
  ESP.deepSleep(900000000, WAKE_RF_DEFAULT); // 15 minutes
  delay(100);
  Serial.println(F("If you are seeing this something is wong with deep sleep"));
}

void setup() {
  Serial.begin(115200);
  sensors.begin();

  Serial.println();
  Serial.println(F("Booting"));

  pinMode(D5, INPUT_PULLUP);

//  if(digitalRead(D5) == HIGH) {
//    WiFi.begin(WIFI_PUB_SSID, WIFI_PASSWORD);
//    Serial.print("Connecting to Wifi " + String(WIFI_PUB_SSID));
//  } else {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);    
    Serial.print("Connecting to Wifi " + String(WIFI_SSID));
//  }
  
  
  int wifiConnectCredit = 0;

  // 10 credits for connection or shut down
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiConnectCredit++;
    if(wifiConnectCredit >= 10) {
      shutDown();
    }
  }
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

   if(digitalRead(D5) == HIGH) {
     client.setServer(MQTT_PUB_SERVER, MQTT_PORT);
     Serial.print("Connecting to " + String(MQTT_PUB_SERVER) + " ... ");
   } else {
     client.setServer(MQTT_SERVER, MQTT_PORT);
     Serial.print("Connecting to " + String(MQTT_SERVER) + " ... ");
   }

  int mqttConnectCredit = 0;

  // 5 credits for MQTT connection or shut down
  while (!client.connected()) {
    if (client.connect(MQTT_DEV_CAB1, MQTT_DEV_CAB1_TOKEN, NULL)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // 5 credits for connection or shut down
      mqttConnectCredit++;
      if(mqttConnectCredit >= 5) {
        shutDown();
      }
      
      delay(5000);
    }
  }

  sensors.requestTemperaturesByIndex(0);
  float t = sensors.getTempCByIndex(0);
  if (isnan(t)) {
   Serial.println(F("Failed to read from sensor!"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.print(t);
    Serial.println(F(" C"));
    
    String json = "{\"temperature\": "+String(t)+" }";
    client.publish(MQTT_TELEMETRY_TOPIC, json.c_str());
    Serial.print("published: ");
    Serial.println(json);
    client.loop();

    // wait to complete the http loop
    delay(2000);
      
  }
   
  shutDown();
}

void loop() {
}
