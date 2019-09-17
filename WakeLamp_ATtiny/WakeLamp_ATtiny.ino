
#include <avr/io.h>
#include <stdint.h> 
#include <avr/interrupt.h>
#include <Adafruit_NeoPixel.h>


#define CLK_PIN 4
#define  DT_PIN 2
#define  SW_PIN 3
#define LED_PIN 1
#define NUM_LEDS 8

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);


volatile bool refresh = false;
volatile bool isWhite = false;
volatile bool isSwitchedOn = false;
volatile bool isSetupOn = false;
volatile bool isSetupPreset = false;
volatile bool switchPreset = false;

uint8_t setupCounter = 0;
uint16_t currentIntensity = 40;
uint8_t currentColor = 127;
boolean prevCLK = HIGH;
boolean prevDT = HIGH;


void modifyColor(boolean isUp) {
  if(isUp) {
      currentColor++;
  } else {
      currentColor--;
  }
  isWhite = currentColor > 250;
}

void modifyIntensity(boolean isUp) {
  if(isUp) {
    if(currentIntensity<10) {
      currentIntensity = currentIntensity+1;
    } else if(currentIntensity<90) {
      currentIntensity = currentIntensity+5;
    } else {
      if(currentIntensity<255) {
        currentIntensity = currentIntensity+10;
      } else {
        currentIntensity = 255;
      }
    }
  } else {
    if(currentIntensity<10) {
      if(currentIntensity>2) {
        currentIntensity = currentIntensity-1;
      }
    } else if(currentIntensity<90) {
      currentIntensity = currentIntensity-5;
    } else {
      currentIntensity = currentIntensity-10;
    }
  }
}

void rotaryEncoder(boolean isUp)  {
  if(isSetupOn) {
    modifyColor(isUp);      
  } else {
    modifyIntensity(isUp);
  }
  refresh = true;
}

void rotarySwitch()  {
  if(!isSetupOn) {
    if(isSetupPreset) {
      isSwitchedOn = true;
      isSetupPreset = false;
      isSetupOn = true;
//      Serial.println("Entering in setup mode");
    } else {
//      Serial.println("Switched");
      isSwitchedOn = !isSwitchedOn;
    }
    setupCounter = 0;
  } else {
    isSetupOn = false;
    isSwitchedOn = true;
//    Serial.println("Exit from setup mode");
  }
  refresh = true;
}

ISR(PCINT0_vect) {
  if(isSwitchedOn) {
    boolean dt = digitalRead(DT_PIN);
    boolean clk = digitalRead(CLK_PIN);
    if(prevCLK != clk) {
      boolean isUp = clk != dt;
      rotaryEncoder(isUp);
      prevCLK = clk;
    }
  }
  
  if(digitalRead(SW_PIN)) {
    if(switchPreset) {
      rotarySwitch();
      refresh = true;
      switchPreset = false;
    }
  } else {
    switchPreset = true;
  }
}

void setup() {
  cli();
  GIMSK = 0b00100000;    // turns on pin change interrupts
  PCMSK = 0b00011000;    // turn on interrupts on pins PB3, PB4
  sei(); 

//  Serial.begin(9600);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  strip.begin();
  strip.show();
  strip.setBrightness(255); 

//  attachInterrupt (digitalPinToInterrupt(CLK_PIN), isr, CHANGE);
//  attachInterrupt (digitalPinToInterrupt(SW_PIN), isr, CHANGE);
}

void loop() {
  if(isSwitchedOn && !digitalRead(SW_PIN)) {
    delay(100);
    setupCounter++;
    if(setupCounter>20) {
      switchOff();
      isSetupOn = false;
      isSetupPreset = true;
    }
  } else {
  
    if(refresh) {
      if(isSwitchedOn) {
        lightUp(isWhite, currentIntensity);
      } else {
        switchOff();
      }
      refresh = false;
    }    
    delay(200);    
    
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
      color = strip.Color(200,255,230); 
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
    delay(stepDelay);
  }
}

void fadeOut(uint16_t intensity, bool isColor, uint16_t stepDelay) {
  for(uint16_t i=intensity; i>0; i--) {
    lightUp(isColor, i);
    delay(stepDelay);
  }
}

void setAll(byte red, byte green, byte blue) {
  setAll(strip.Color(red,green,blue));
}

void setAll(uint16_t color) {
  for(uint16_t j=0; j<strip.numPixels(); j++) {
    strip.setPixelColor(j, color);
  }
  delay(10);
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
