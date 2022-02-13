
#include "credentials.h"

#include "Wire.h"
#include "PubSubClient.h"

// https://github.com/PlayingWithFusion/PWFusion_TMD3700
#include <PWFusion_TMD3700.h> 
#include <PWFusion_TMD3700_STRUCT.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Ticker.h>

#define durationSleep  300   //seconds

Ticker ticker;

WiFiClient espClient;
PubSubClient client(espClient);

PWFusion_TMD3700 tmd3700_snsr0(0x39);
struct var_tmd3700 tmd3700_str_ch0;

int UVsensorIn = A0; 

void tick() {
  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}

void shutDown() {
  ESP.deepSleep(durationSleep * 1000000);
}

void setup() {
  
  pinMode(UVsensorIn, INPUT);
  
  Serial.begin(9600); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\n\r \n\rConnecting to Wifi");
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
  
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
   
  client.setServer(MQTT_SERVER, MQTT_PORT);
  int mqttConnectCredit = 0;

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MQTT_LIGHTSENZORS_DEV, MQTT_USER, MQTT_PASS)) {
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

  struct var_tmd3700 *tmd3700_ptr;
  tmd3700_ptr = &tmd3700_str_ch0;
  
  Wire.begin();
  delay(100);

  while(0 == tmd3700_snsr0.Init(tmd3700_ptr));

  publishData();
    
  shutDown();

}

void loop() {
  
}

void publishData() {

  // read UV
  int uvLevel = averageAnalogRead(UVsensorIn); 
  float outputVoltage = 3.3 * uvLevel/1024;
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);
  Serial.print(" UV Intensity: ");
  Serial.print(uvIntensity);
  Serial.print(" mW/cm^2"); 
  Serial.println(); 
  Serial.println();

  // read ALS values
  struct var_tmd3700 *tmd3700_ptr;
  tmd3700_ptr = &tmd3700_str_ch0;
  tmd3700_snsr0.update_all(tmd3700_ptr);
  uint16_t cdata = tmd3700_ptr->Cdata;
  uint16_t rdata = tmd3700_ptr->Rdata;
  uint16_t gdata = tmd3700_ptr->Gdata;
  uint16_t bdata = tmd3700_ptr->Bdata;
  uint16_t pdata = tmd3700_ptr->Pdata;
  uint16_t poffs = tmd3700_ptr->Poffset;
  
  Serial.println("Cdata Rdata Gdata Bdata Pdata Poffst");
  Serial.print(cdata);
  Serial.print("   ");
  Serial.print(rdata);
  Serial.print("   ");
  Serial.print(gdata);
  Serial.print("   ");
  Serial.print(bdata);
  Serial.print("   ");
  Serial.print(pdata);
  Serial.print("   ");
  Serial.print(poffs);
  Serial.println(" ");

  pinMode(BUILTIN_LED, OUTPUT);
  ticker.attach(0.03, tick);

  String json = "{\"uv\": "+String(uvIntensity)+", \"cdata\": "+String(cdata)+", \"rdata\": "+String(rdata)+", \"gdata\": "+String(gdata)+", \"bdata\": "+String(bdata)+", \"pdata\": "+String(pdata)+", \"poffset\": "+String(poffs)+" }";
  client.publish(MQTT_LIGHTSENZORS_TOPIC, json.c_str());
  Serial.print("Published on topic: ");
  Serial.println(MQTT_LIGHTSENZORS_TOPIC);
  client.loop();

  // wait to complete the http loop
  delay(2000);

}
 
//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead) {
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 
 
  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;
 
  return(runningValue);  
 
}
 
 
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
