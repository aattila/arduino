
#include <ESP8266WiFi.h>
#include <Thread.h>
#include "SSD1306.h" 
#include <RH_ASK.h>
#include <SPI.h> 

#define PUMP_ON_DELAY   5 // seconds
#define PUMP_OFF_DELAY  3 // seconds

#define RX_PIN     16 // D0
#define RELAY1_PIN 10 // SDD3
#define RELAY2_PIN  9 // SDD2
#define PRESS_PIN  14 // D5
#define BEEP_PIN   15 // D8
#define RF_NON      1 // D10

RH_ASK rf(2000, RX_PIN, RF_NON, RF_NON); // Tx, Ptt is not used

const char ssid[] = 
const char password[] = 
IPAddress ip(192, 168, 1, 202);
IPAddress gw(192, 168, 1, 1);
IPAddress sn(255, 255, 255, 0);

WiFiServer server(80);

Thread ledThread = Thread();
Thread consoleThread = Thread();
Thread statusThread = Thread();
Thread pressThread = Thread();

bool isWifiConnected = false;

SSD1306 display(0x3c, D1, D2);
String console[5] = {"", "", "", "", ""};
int consolePos = 0;
int consoleOff = 0;

// relay digital default level
static boolean relay_level[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
// digital IO for the relay connections
static byte   relayDIO[] = {1, 2, 3, 3, 3, 3, 3, 3};
// alarms array for the triggers
static boolean  alarm[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};

bool rfRxFlag;
String rfMsg = "";
int pressOffCount = 0;
int pressOnCount = 0;
bool isShutDownPomp = false;

void blinkLed() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(20);
  digitalWrite(LED_BUILTIN, pressOffCount == 0);
}

void displayConsole() {
  String text = "";
  int len = 5;
  if(consolePos<5) {
    len = consolePos;
  }
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  for (int i = 0; i< len; i++) {
    display.drawString(0, i*13, console[i]);
  }
  display.display();
}

void print(int line, int pos, String text) {   
  if (consoleOff > 0) {
    return;
  }
  
  text.trim();
  int x = (pos-1)*16;
  int y = (line-1)*13;

  display.setColor(BLACK);
  display.fillRect(x, y, x+16, 12);
  display.setColor(WHITE);
  display.drawString(x, y, text);
  display.display();
}

void println(int pos, String line) {   
  if (consoleOff > 0) {
    return;
  }
  
  line.trim();
  console[pos-1] = line;
  int y = (pos-1)*13;
  display.setColor(BLACK);
  display.fillRect(0, y, 128, 12);
  display.setColor(WHITE);
  display.drawString(0, y, line);
  display.display();
}

void println(String line) {
   
   line.trim();

  // fill the lines
  if(consolePos < 5) {
    console[consolePos] = line;
  } else {
    for (int i = 0; i< 4; i++) {
      console[i] = console[i+1];
    }
    console[4] = line;
  }
  consolePos++;  

  displayConsole();
}

void manageConsole() {
  if (consoleOff <= 0) {
    return;
  }

  consoleOff = consoleOff - 1000;

  if (consoleOff == 0) {
    displayConsole();
  }
}

void serveClient(WiFiClient client) {
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  client.flush();

  // Invalid request
  if ( req.indexOf("/status") == -1) {
    client.stop();
    return;
  }

  // Get time
  String timeValue;
  int timeIdx = req.indexOf("time=");
  if ( timeIdx != -1) {
    timeValue = req.substring(timeIdx+5, timeIdx+5+5);
    timeValue.replace(".", ":");
  }


  String dateValue;
  int dateIdx = req.indexOf("date=");
  if ( dateIdx != -1) {
    dateValue = req.substring(dateIdx+5, dateIdx+5+10);
  }

  consoleOff = 5000;
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 0, timeValue);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 28, dateValue);
  display.display();

  client.flush();

  // Prepare the response
  String s = "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Pragma: no-cache\r\n"
    "\r\n" 
    "{" 
    "\"rf\" : \"$rfMsg\", "  
    "\"alarms\" : [ $a1, $a2, $a3, $a4, $a5, $a6, $a7, $a8 ] "  
    "}";

  s.replace("$rfMsg", rfMsg);
  s.replace("$a1", String(alarm[0]));
  s.replace("$a2", String(alarm[1]));
  s.replace("$a3", String(alarm[2]));
  s.replace("$a4", String(alarm[3]));
  s.replace("$a5", String(alarm[4]));
  s.replace("$a6", String(alarm[5]));
  s.replace("$a7", String(alarm[6]));
  s.replace("$a8", String(alarm[7]));

    
  // Send the response to the client
  client.print(s);
  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed  

}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void printStatus() {
  if(rfRxFlag) {
    print(4, 8, "RX");  
    print(5, 1, rfMsg.substring(12));  
    rfRxFlag = false;
  } else {
    print(4, 8, "");  
    print(5, 1, "");  
    print(5, 2, "");  
    print(5, 3, "");  
    print(5, 4, "");  
    print(5, 5, "");  
    print(5, 6, "");  
    print(5, 7, "");  
    print(5, 8, "");  
  }

//  String h = getValue(rfMsg, ' ', 0);
//  String t = getValue(rfMsg, ' ', 1);
//  print(5, 1, h);  
//  print(5, 3, t);  

//  print(5, 1, rfMsg);  

  
//  digitalWrite(BEEP_PIN, HIGH);
//  delay(1000);
//  digitalWrite(BEEP_PIN, LOW);
}

void shutDownPomp() {
  setPump(false);
  isShutDownPomp = true;
}

void setPump(bool isOn) {
  if(!isShutDownPomp) {
    digitalWrite(RELAY1_PIN, !isOn);
  }
}

void managePump() {

  // press switch enabled and the pomp is not working - signal pump start
  if(!digitalRead(PRESS_PIN) && digitalRead(RELAY1_PIN)) {
    pressOffCount++;
    pressOnCount = 0;
  } 

  // press switch disabled and the pump is working - signal pump stop
  if(digitalRead(PRESS_PIN) && !digitalRead(RELAY1_PIN)) {
    pressOnCount++;
    pressOffCount = 0;
  }

  // pump enable 
  if(pressOffCount >= PUMP_ON_DELAY) {
    pressOffCount = 0;
    pressOnCount = 0;
    setPump(true);
  }
  
  // pump disable 
  if(pressOnCount >= PUMP_OFF_DELAY) {
    pressOffCount = 0;
    pressOnCount = 0;
    setPump(false);
  }
}

void setup() {

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  ledThread.onRun(blinkLed);
  ledThread.setInterval(3000);

  consoleThread.onRun(manageConsole);
  consoleThread.setInterval(1000);

  statusThread.onRun(printStatus);
  statusThread.setInterval(1000);

  pressThread.onRun(managePump);
  pressThread.setInterval(1000);

  Serial.begin(115200);

  pinMode(A0, INPUT);
//  ADC_MODE(ADC_VCC);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(PRESS_PIN, HIGH);
  pinMode(PRESS_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY2_PIN, HIGH);
  pinMode(BEEP_PIN, OUTPUT);
  digitalWrite(BEEP_PIN, LOW);
  
  WiFi.mode(WIFI_STA);
  WiFi.hostname("D-Duino");
  WiFi.config(ip, gw, sn);
  WiFi.begin(ssid, password);

  println("Booting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  isWifiConnected = true;
  println("Connected to: "+String(ssid));
  
  // Start the server
  server.begin();
  IPAddress ip = WiFi.localIP();
  
  println("IP: "+String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3]));
  
  if (!rf.init()) {
     println("RF init Failed!");
  } else {
     println("RF init Sucess:");
  }
  
}

void loop() {

  if(ledThread.shouldRun()) {
    ledThread.run();
  }
  if(consoleThread.shouldRun()) {
    consoleThread.run();
  }
  if(pressThread.shouldRun()) {
    pressThread.run();
  }
  if(statusThread.shouldRun()) {
    statusThread.run();
  }


  // handle RF received message
  String rfCurrMsg = "";
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  uint8_t i;
  if (rf.recv(buf, &buflen)) {
    for(i=0; i<(buflen-1); i++) {
      rfCurrMsg += (char)buf[i];
    }
    rfRxFlag = true;
    rfMsg = rfCurrMsg;
  } 

  
  // handle HTTP request
  WiFiClient client = server.available();
  if (client) {
    while(!client.available()) {
      delay(1);
    }
    serveClient(client);
  }
  

}

