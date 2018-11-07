#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_SSD1306.h>
#include <Math.h>

// Callibration
// please read your ADXL345 maximum and minimum values for each cordinates and put it here
#define MIN_X -253
#define MAX_X 259
#define MIN_Y -263
#define MAX_Y 248
#define MIN_Z -281
#define MAX_Z 226

// please calculate the slope and the intercept values after you have the callibration data by using the equations below:
// slope = 2 / (max_reading - min_reading)
// intercept = 1 - slope * max_reading
#define slopeX      0.00390625
#define interceptX -0.01171875 
#define slopeY      0.00391389   
#define interceptY  0.02935528
#define slopeZ      0.00394477   
#define interceptZ  0.10848198

Adafruit_SSD1306 display = Adafruit_SSD1306();
Adafruit_ADXL345_Unified adxl;

void setup() {

//  Serial.begin(115200);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  adxl.begin();
  adxl.setRange(ADXL345_RANGE_4_G);
  adxl.setDataRate(ADXL345_DATARATE_50_HZ);
}

String format(double x) {
  String sx = String(x,2);
  if(x>=0 && x<10) {
    sx = " " + sx;
  }
  if(x<=0) {
    if(x>=-10) {
      sx = " " + sx.substring(1);
    } else {
      sx = sx.substring(1);
    }
  }
  return sx;
}

void triangle(int x, int y, int h, int w, bool isReverse) {
  for (int i=0; i < h; i++){
    int s, ox;
    if(isReverse) {
      s = 1+(i*2);
      ox = x-i;
    } else {
      s = w-(i*2);
      ox = x+i;
    }
    display.drawFastHLine(ox, y+i, s, WHITE);
  }
}

void loop() {
   
  int16_t x = adxl.getX();
  int16_t y = adxl.getY();
  int16_t z = adxl.getZ();

  double rx = x * slopeX + interceptX;
  double ry = y * slopeY + interceptY;
  double rz = z * slopeZ + interceptZ;

  rx = map(rx*10000, -10000, 10000, -9000, 9000);
  rx = rx/100;
  ry = map(ry*10000, -10000, 10000, -9000, 9000);
  ry = ry/100;
  rz = map(rz*10000, -10000, 10000, -9000, 9000);
  rz = rz/100;

//  Used to read data for callibration
//  Serial.print(x);
//  Serial.print("\t");
//  Serial.print(y);
//  Serial.print("\t");
//  Serial.print(z);
//  Serial.print("\n");


  display.clearDisplay();

//  display.setCursor(96, 0);
//  display.print("x");
//  display.setCursor(96, 18);
//  display.print("y");
  display.setTextSize(2);

  display.setCursor(0, 0);
  display.print(format(rx));
  display.setCursor(0, 18);
  display.print(format(ry));

  // drawing X axis assets
  display.drawFastHLine(72, 15, 56, WHITE);
  if(rx != 0) {
    if(rx>0) {
      triangle(72, 18, 4, 7,false);
      triangle(124, 9, 4, 7, true);  
    } else {
      triangle(75, 9, 4, 7, true);
      triangle(121, 18, 4, 7, false);  
    }
  }

  // drawing Y axis assets
  display.drawLine(85, 24, 109, 8, WHITE);
  if(ry != 0) {
    if(ry>0) {
      triangle(81, 27, 3, 5,false);
      triangle(111, 4, 3, 5,true);
    } else {
      triangle(84, 22, 3, 5,true);
      triangle(108, 8, 3, 5,false);
    }
  }

  display.display();
  delay(100);
}
