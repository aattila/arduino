
#include <WS2812B.h>
#include <irmp.h>
#define SERIALX Serial1

#define IR_1      69
#define IR_2      70
#define IR_3      71
#define IR_4      68
#define IR_5      64
#define IR_6      67
#define IR_7      7
#define IR_8      21
#define IR_9      9
#define IR_STAR   22
#define IR_0      25
#define IR_SHARP  13
#define IR_UP     24
#define IR_LEFT   8
#define IR_OK     28
#define IR_RIGHT  90
#define IR_DOWN   82
#define SNOOZE    99
#define NOPE      0

#define ALS_PIN   PA6
#define LED_PIN   PB12
#define MIC_PIN   PB1
#define PIR_PIN   PB14
#define IR_PIN    PC15
#define NUM_LEDS 90

IRMP_DATA irmp_data;
 
WS2812B strip = WS2812B(NUM_LEDS);
uint8_t LEDGamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

volatile bool refreshSet = false;
volatile int  ir_key = 0;
volatile int  last_ir_key = 0;
volatile bool isSwitchedOff = true;
volatile byte okCounter = 0;
volatile int  loopCount = 0;
volatile bool snoozeLightOn = false;
volatile int movementCount = 0;

bool isWhiteSelected = false;
byte wpos;
uint8 currentIntensity = 40;
uint8 currentColor = 127;
uint16 c_addr = 0x00;
uint16 i_addr = 0x10;
uint16 l_addr = 0x20;
uint16 w_addr = 0x30;

float rawRange = 1024; // 3.3v
float logRange = 5.0; // 3.3v = 10^5 lux

HardwareTimer timer(2);

void timer2_init () {
  timer.pause();
  timer.setPrescaleFactor( ((F_CPU / F_INTERRUPTS)/8) - 1);
  timer.setOverflow(7);
  timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
  timer.attachCompare1Interrupt(TIM2_IRQHandler);
  timer.refresh();
  timer.resume();
}

void TIM2_IRQHandler() {
  (void) irmp_ISR(); // call irmp ISR
}

void handleMic() {
  if(digitalRead(LED_PIN)) {
    detachInterrupt(PIR_PIN);
    digitalWrite(LED_PIN, LOW);
    Serial.println("Mic/PIR is disarmed");
  } else {
    attachInterrupt(PIR_PIN, capturePIR, CHANGE);
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Mic/PIR is armed");
  }
}

// Illuminance              Example
// 0.002 lux                Moonless clear night sky
// 0.2 lux                  Design minimum for emergency lighting (AS2293).
// 0.27 - 1 lux             Full moon on a clear night
// 3.4 lux                  Dark limit of civil twilight under a clear sky
// 50 lux                   Family living room
// 80 lux                   Hallway/toilet
// 100 lux                  Very dark overcast day
// 300 - 500 lux            Sunrise or sunset on a clear day. Well-lit office area.
// 1,000 lux                Overcast day; typical TV studio lighting 
// 10,000 - 25,000 lux      Full daylight (not direct sun)
// 32,000 - 130,000 lux     Direct sunlight
float ambientInLux() {
  int raw = analogRead(ALS_PIN) >> 2;
  float logLux = raw * logRange / rawRange;
  return pow(10, logLux); 
}
      
void capturePIR() {
  Serial.println("Movement detected");
  if(movementCount >= 50000) {
    doSnoozeLight();
    movementCount = 0;
  } else {
    movementCount = movementCount + 10000;
  }
}

void captureIR() {
  if (irmp_get_data (&irmp_data)) {  
    ir_key = irmp_data.command;
    switch(ir_key) {
      case IR_UP:
        if(!isSwitchedOff) {
          if(currentIntensity < 255) {
            currentIntensity++;
          }
          ir_key = last_ir_key;
        }
        break;
      case IR_LEFT:
        if(!isSwitchedOff) {
          if(currentColor > 0) {
            currentColor--;
          }
          ir_key = last_ir_key;
        }
        break;
      case IR_OK:
        refreshSet = true;
        if(okCounter<10) {
          okCounter++;
        }
        if(okCounter >= 10) {
          handleMic();
          okCounter = 0;          
        }
        break;
      case IR_RIGHT:
        if(!isSwitchedOff) {
          if(currentColor < 255) {
            currentColor++;
          }
        }
        ir_key = last_ir_key;
        break;
      case IR_DOWN:
        if(!isSwitchedOff) {
          if(currentIntensity > 0) {
            currentIntensity--;
          }
          ir_key = last_ir_key;
        }
        break;
      default:
        isSwitchedOff = false;
        refreshSet = ir_key != irmp_data.command;
        last_ir_key = ir_key;
        break;
    }
  }
}

void xdelay(int millisec) {
  if(refreshSet) {
    return;
  }
  if(millisec < 10) {
    delay(millisec);
    return;
  }
  int peace = millisec/10;
  for (int i = 0; i< peace; i++) {
    if(refreshSet) {
      break;
    }
    delay(10);
  }
}

void doSnoozeLight() {
  if(isSwitchedOff) {
    refreshSet = true;
    ir_key = SNOOZE;  
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
  strip.begin();
  strip.show();
  strip.setBrightness(255); 
  irmp_init(); 
  timer2_init();

  pinMode(ALS_PIN, INPUT_ANALOG);
  pinMode(MIC_PIN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW); // !ledState

  delay(2000);

  attachInterrupt(IR_PIN,  captureIR, FALLING);
  handleMic();

}

void loop() {
  if(okCounter > 0) {
    loopCount++;
    if(loopCount > 5) {
      okCounter = 0;
      loopCount = 0;
    }
  }
  
  switch(ir_key) {
    case IR_1:
      rainbowCycle(40);
      break;
    case IR_2:
      rainbow(40);
      break;
    case IR_3:
      whiteOverRainbow(20,75,5);  
      break;
    case IR_4:
      pixelRainbow();
      break;
    case IR_5:
      snowSparkle(10, 10, 10, 20, random(200,1000));
      break;  
    case IR_6:
      randomColors();
      xdelay(5000);
      break;               
    case IR_7:
      lava();
      xdelay(500);
      break;  
    case IR_8:
      police();
      break;  
    case IR_9:
      Serial.println("9");
      break;  
    case IR_0:
      chillFade();
      xdelay(700);
      break;
    case IR_STAR:
      isWhiteSelected = false;
      lightUp(isWhiteSelected, currentIntensity);
      xdelay(500);
      break;  
    case IR_SHARP:
      isWhiteSelected = true;
      lightUp(isWhiteSelected, currentIntensity);
      xdelay(500);
      break;
    case IR_OK:
      if(!isSwitchedOff) {
        Serial.println("Switching Off");
        delay(100);
        switchOff();
        delay(2000);
        isSwitchedOff = true;
        okCounter = 0;
      }
      ir_key = NOPE;
      break;
    case SNOOZE:
      if(digitalRead(LED_PIN)) {
        isSwitchedOff = false;
        Serial.print("The current ambient light is: ");
        Serial.print(ambientInLux());
        Serial.println(" lux");
        if(ambientInLux() > 3) {
          Serial.println("There is not enough dark to activate snooze light");        
        } else {
          Serial.println("The Snooze light is activated");
          fadeIn(currentIntensity, isWhiteSelected, 300);
          if(!refreshSet) {
              xdelay(120000); // 2 min delay
          }
          if(!refreshSet) {
            fadeOut(currentIntensity, isWhiteSelected, 300); 
          }
        }
        ir_key = NOPE;
        isSwitchedOff = !refreshSet;
      }
      break;
    default:
      if(digitalRead(LED_PIN)) {
        int noise = analogRead(PB1);
        if(noise > 2250 or noise < 1750) {
          Serial.print("Noise detected: ");
          Serial.println(noise);
          doSnoozeLight();
        }
        if (movementCount >= 1000) {
          movementCount = movementCount - 1000;
        }
//        Serial.print("Movement count: ");
//        Serial.println(movementCount);
      }
      delay(1000);

      break;
  }
  refreshSet = false;
}


void lava() {
  uint32_t r = applyIntensity(strip.Color(255, 0, 0));
  uint32_t b = applyIntensity(strip.Color(0, 0, 255));
  for(uint16_t i=0; i<45; i++) {
    strip.setPixelColor(i, r);
  }
  for(uint16_t i=45; i<90; i++) {
    strip.setPixelColor(i, b);
  }
  delay(10);
  strip.show();
}

void police() {
  uint32_t r = Wheel(wpos);

  uint32_t x = applyIntensity(strip.Color(255, 0, 0));
  uint32_t g = applyIntensity(strip.Color(0, 255, 0));
  uint32_t b = applyIntensity(strip.Color(0, 0, 255));
  uint32_t o = applyIntensity(strip.Color(0, 0, 0));
  uint32_t mask[8][8] = {
    {r,o,o,o,o,o,o,o},
    {o,r,o,o,o,o,o,o},
    {o,o,r,o,o,o,o,o},
    {o,o,o,r,o,o,o,o},
    {o,o,o,o,r,o,o,o},
    {o,o,o,o,o,r,o,o},
    {o,o,o,o,o,o,r,o},
    {o,o,o,o,o,o,o,r},
  };
//  uint32_t mask[8][8] = {
//    {r,r,g,g,b,b,o,o},
//    {o,o,r,r,g,g,b,b},
//    {b,o,r,r,g,g,b,b},
//    {b,o,o,r,r,g,g,b},
//    {b,b,o,o,r,r,g,g},
//    {g,b,b,o,o,r,r,g},
//    {g,g,b,b,o,o,r,r},
//    {r,g,g,b,b,o,o,r},
//  };
  for(uint8_t i=0; i<8; i++) {
    for(uint8_t j=0; j<8; j++) {
      setSlice(j, mask[i][j]);
    }
    xdelay(500);
    wpos++;
  }
}

void pixelRainbow() {
  int r = random(3);
  uint32_t c = strip.Color(255, 255, 255);
  switch(r) {
    case 0:
      c = strip.Color(255, 0, 0);
      break;
    case 1:
      c = strip.Color(0, 255, 0);
      break;
    case 2:
      c = strip.Color(0, 0, 255);
      break;
  }
  int pixel = random(strip.numPixels());
  strip.setPixelColor(pixel, applyIntensity(c));
  delay(40);
  strip.show();
  pixel = random(strip.numPixels());
//  strip.setPixelColor(pixel, strip.Color(0, 0, 0));
//  delay(40);
//  strip.show();
}

void chillFade() {
  uint32_t c = Wheel(wpos);
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, applyIntensity(c));
  }
  wpos++;
  delay(20);
  strip.show();
}

void randomColors() {
  uint32_t c = Wheel(random(256));
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, applyIntensity(c));
  }
  delay(10);
  strip.show();
}

void snowSparkle(byte r, byte g, byte b, int sparkleDelay, int speedDelay) {
  setAll(r, g, b);
  int pixel = random(strip.numPixels());
  strip.setPixelColor(pixel, strip.Color(255, 255, 255));
  delay(20);
  strip.show();
  xdelay(sparkleDelay);
  strip.setPixelColor(pixel, strip.Color(r, g, b));
  delay(20);
  strip.show();
  xdelay(speedDelay);
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, applyIntensity(c));
    delay(10);
    strip.show();
    xdelay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    xdelay(wait);
  }
}


void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for(j=0; j<256*5; j++) { 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    xdelay(wait);
  }
}

void whiteOverRainbow(uint8_t wait, uint8_t whiteSpeed, uint8_t whiteLength ) {
  
  if(whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int head = whiteLength - 1;
  int tail = 0;

  int loops = 3;
  int loopNum = 0;

  static unsigned long lastTime = 0;


  for(int j=0; j<256; j++) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      if((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ){
        strip.setPixelColor(i, strip.Color(255,255,255) );
      }
      else{
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      }
      
    }

    if(millis() - lastTime > whiteSpeed) {
      head++;
      tail++;
      if(head == strip.numPixels()){
        loopNum++;
      }
      lastTime = millis();
    }

    if(loopNum == loops) return;
  
    head%=strip.numPixels();
    tail%=strip.numPixels();
      strip.show();
      xdelay(wait);
  }
  
}

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 

  WheelPos -= 170;
  return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
}

void switchOff() {
  setAll(0, 0, 0);
}

void lightUp(bool isWhite, uint16_t intensity) {
  for(uint16_t j=0; j<strip.numPixels(); j++) {
    uint32_t color;
    if(isWhite) {
      color = strip.Color(255,255,50);
    } else {
      color = Wheel(currentColor);
    }
    strip.setPixelColor(j, applyIntensity(color, intensity));
  }
  strip.show();
}

void fadeIn(uint16_t intensity, bool isColor, uint16_t stepDelay) {
  for(uint16_t i=0; i<intensity; i++) {
    lightUp(isColor, i);
    xdelay(stepDelay);
  }
}

void fadeOut(uint16_t intensity, bool isColor, uint16_t stepDelay) {
  for(uint16_t i=intensity; i>0; i--) {
    lightUp(isColor, i);
    xdelay(stepDelay);
  }
}

void setAll(byte red, byte green, byte blue) {
  for(uint16_t j=0; j<strip.numPixels(); j++) {
    strip.setPixelColor(j, strip.Color(red,green,blue));
  }
  delay(10);
  strip.show();
}

void setAll(uint16_t color) {
  for(uint16_t j=0; j<strip.numPixels(); j++) {
    strip.setPixelColor(j, color);
  }
  delay(10);
  strip.show();
}

void setSlice(byte slice, uint32_t color) {
  switch(slice) {
    case(0):
      for(uint16_t i=0; i<13; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(1):
      for(uint16_t i=60; i<70; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(2):
      for(uint16_t i=26; i<38; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(3):
      for(uint16_t i=80; i<90; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(4):
      for(uint16_t i=13; i<26; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(5):
      for(uint16_t i=50; i<60; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(6):
      for(uint16_t i=38; i<50; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
    case(7):
      for(uint16_t i=70; i<80; i++) {
        strip.setPixelColor(i, applyIntensity(color));
      }
      break;
  }
  delay(5);
  strip.show();
}

uint32_t applyIntensity(uint32_t color) {
  applyIntensity(color, currentIntensity);
}


uint32_t applyIntensity(uint32_t color, uint16_t intensity) {
  uint8_t r = setIntensity(red(color), intensity);
  uint8_t g = setIntensity(green(color), intensity);
  uint8_t b = setIntensity(blue(color), intensity);
  return strip.Color(r,g,b);
}

uint8_t setIntensity(uint8_t c) {
  return setIntensity(c, currentIntensity);
}

uint8_t setIntensity(uint8_t c, uint16_t intensity) {
  return (c * intensity) >> 8;
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}
