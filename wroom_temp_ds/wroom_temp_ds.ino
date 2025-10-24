
// Hardware: WeMos D1 ESP-Wroom-02 Nodemcu ESP8266
// Board: LOLIN(WEMOS) D1 D2 & mini 

#include <credentials.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <CayenneMQTTESP8266.h>


const int oneWireBus = D4;   // GPIO where the DS18B20 is connected to

OneWire onewire(oneWireBus);
DallasTemperature sensors(&onewire);

float temp;

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
  Cayenne.begin(CAYENNE_USER_1, CAYENNE_PASS_1, CAYENNE_CLIENTID_1, WIFI_SSID, WIFI_PASSWORD);  
  readTemp();
  for(int i=0; i<10; i++) {
    Cayenne.loop();    
  }
  shutDown();
}

void loop() {
}

CAYENNE_OUT_DEFAULT() {
  Cayenne.celsiusWrite(1, temp);
}

void readTemp() {
  sensors.requestTemperaturesByIndex(0);
  float t = sensors.getTempCByIndex(0);
  if (isnan(t)) {
   Serial.println(F("Failed to read from sensor!"));
   return;
  }
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.println(F(" C"));
  temp = t;
}
