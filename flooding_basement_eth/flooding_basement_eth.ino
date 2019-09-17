
#include <Thread.h>
#include <DHT.h>
#include <EtherCard.h>

#include "ThingsBoard.h"

#define TOKEN               
#define THINGSBOARD_SERVER  "mqttbroker"

EtherCard ethClient;
ThingsBoard tb(ethClient);

byte Ethernet::buffer[500];
BufferFiller bfill;
static byte mymac[] = { 0x00,0x00,0xAA,0xDD,0x00,0x01 };
static byte myip[] = { 192,168,1,203 };
static byte gwip[] = { 192,168,1,1 };

String values[] = {"", "", "", "", "", "", "", ""};
String Vcc = "";
String boardTemp = "";


//#define FIFO_ARRAYS   8
//#define FIFO_ENTRIES  20
//int fifo[FIFO_ARRAYS][FIFO_ENTRIES];

Thread ledThread = Thread();
Thread dhtThread = Thread();
Thread rfThread = Thread();
//Thread fifoThread = Thread();


#define DHTPIN 9
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
String hum = "";
String temp = "";

boolean error;


// read the DHT senzor
void dhtRun() {
  hum = String(dht.readHumidity());
  temp= String(dht.readTemperature());
}

// read the power supply value 
// https://web.archive.org/web/20140712024348/http://code.google.com/p/tinkerit/wiki/SecretThermometer
long readTemp() {
  long result;
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(20); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = (result - 125) * 1075;
  return result;
}

// read the power supply value 
// https://web.archive.org/web/20150218055034/http://code.google.com:80/p/tinkerit/wiki/SecretVoltmeter
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

// move the buffer entries and add the analog entries into
//void readAnalogInputs() {
//  unsigned int ADCValue;
//  double Voltage;
//  double Vcc;
//
//  for(int i=0; i<FIFO_ARRAYS; i++) {
//    for(int j=1; j<FIFO_ENTRIES; j++) {
//      fifo[i][j-1] = fifo[i][j];
//    }
//    
//    Vcc = readVcc()/1000.0;
//    ADCValue = analogRead(i);
//    Voltage = (ADCValue / 1024.0) * Vcc;
//    
//    fifo[i][FIFO_ENTRIES-1] = Voltage;
//  }
//}

// calculates the AVG of a fifo buffer
//int getBufferAvg(int buff) {
//  int val = 0;
//  for(int j=0; j<FIFO_ENTRIES; j++) {
////    Serial.print(fifo[buff][j]);
////    Serial.print(" ");
//    val += fifo[buff][j];
//  }
////  Serial.println();
//  return val/FIFO_ENTRIES;
//}


// read data for transmission
void readValues() {
  double v = readVcc()/1000.0;
  Vcc = String(v);
  boardTemp = String(readTemp()/10000.0);

  unsigned int ADCValue;
  double Voltage;
  for(int i=0; i<8; i++) {
    ADCValue = analogRead(i);
    Voltage = (ADCValue / 1024.0) * v;
    values[i] = String(Voltage);
  }
}
 
static word homePage() {
  Serial.println(Vcc);
  bfill = ether.tcpOffset();

  String rsp = "{ \"Vcc\" : "+ Vcc  + ", \"board_temp\" : " + boardTemp + 
        ", \"humidity\" : " + hum + ", \"temperature\" : " + temp +
        ", \"analog_values\" : [ "+values[0]+", "+values[1]+", "+values[2]+", "+values[3]+", "+values[4]+", "+values[5]+", "+values[6]+", "+values[7]+" ] }";

  int str_len = rsp.length() + 1;
  const char char_array[str_len];
  rsp.toCharArray(char_array, str_len);

  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "$S"
    ),
    char_array
  );

  return bfill.position();
}


void setup() {
  Serial.begin(115200);

  // setup the threads
  dhtThread.onRun(dhtRun);
  dhtThread.setInterval(5000);

  rfThread.onRun(readValues);
  rfThread.setInterval(3000);

//  analogReference(INTERNAL);
  
//  fifoThread.onRun(readAnalogInputs);
//  fifoThread.setInterval(2000);

//  // initializing the fifo buffers
//  for(int i=0; i<FIFO_ARRAYS; i++) {
//    for(int j=0; j<FIFO_ENTRIES; j++) {
//      fifo[i][j] = 5;
//    }
//  }


  // start the DHT senzor
  dht.begin();

  // start the Ethernet card
  ether.begin(sizeof Ethernet::buffer, mymac);
  ether.staticSetup(myip, gwip);

}

void loop() { 

  if(dhtThread.shouldRun()) {
    dhtThread.run();
  }
  if(rfThread.shouldRun()) {
    rfThread.run();
  }
//  if(fifoThread.shouldRun()) {
//    fifoThread.run();
//  }

  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  if (pos)  // check if valid tcp data is received
    ether.httpServerReply(homePage()); // send web page data

}
