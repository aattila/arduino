
#include <Thread.h>
#include <RH_ASK.h>
#include <SPI.h> 
#include <DHT.h>;

#define LED 13

//#define FIFO_ARRAYS   8
//#define FIFO_ENTRIES  20
//int fifo[FIFO_ARRAYS][FIFO_ENTRIES];

Thread ledThread = Thread();
Thread dhtThread = Thread();
Thread rfThread = Thread();
//Thread fifoThread = Thread();

RH_ASK rf(2000, 10, 12);


#define DHTPIN 9
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float hum;
float temp;

boolean error;


// live LED
void blinkLed() {
  digitalWrite(LED, HIGH);
  if(!error) {
    delay(20);
    digitalWrite(LED,LOW);
  }
}

// read the DHT senzor
void dhtRun() {
  hum = dht.readHumidity();
  temp= dht.readTemperature();
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

void sendRfData(String msg) {
  Serial.println(msg);
  int strLen = msg.length()+1;
  uint8_t dataArray[strLen];
  msg.toCharArray(dataArray, strLen);
  rf.send(dataArray, strLen);
  rf.waitPacketSent();  
}

// RF transmit data
void rfTransmit() {
  double Vcc = readVcc()/1000.0;
  double t = readTemp()/10000.0;

  unsigned int ADCValue;
  double Voltage;

  String msg1 = "1: " + String(Vcc) + " " + String(t) + " " + String(hum) + " " + String(temp);
  sendRfData(msg1);
  String msg2 = "2: ";
  for(int i=0; i<8; i++) {
    ADCValue = analogRead(i);
    Voltage = (ADCValue / 1024.0) * Vcc;
    msg2 += String(Voltage) + " ";
  }
  msg2.trim();
  sendRfData(msg2);
}


void setup() {
  Serial.begin(115200);

  // setup the live LED
  pinMode(LED, OUTPUT);

  // setup the threads
  ledThread.onRun(blinkLed);
  ledThread.setInterval(3000);

  dhtThread.onRun(dhtRun);
  dhtThread.setInterval(5000);

  rfThread.onRun(rfTransmit);
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

  // setup the RF transmitter module
  error = !rf.init();

  // start the DHT senzor
  dht.begin();
  
}

void loop() { 

  if(ledThread.shouldRun()) {
    ledThread.run();
  }
  if(dhtThread.shouldRun()) {
    dhtThread.run();
  }
  if(rfThread.shouldRun()) {
    rfThread.run();
  }
//  if(fifoThread.shouldRun()) {
//    fifoThread.run();
//  }
  
}
