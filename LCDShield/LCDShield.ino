#include <LiquidCrystal.h>

#define IR_DATA     13 //PWM
#define IR_SYNC     2  //INT0

int x = 0;
int backLight = 10; // LCD Panel Backlight LED connected to digital pin 10
int lightOn = true;
int clicks = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

bool state[8] = { false, false, false, false, false, false, false, false };
bool star = false;

void irEvent() { //ISR
  clicks++;
  printClicks(false);

  uint16_t data = pulseIn(IR_DATA, HIGH);
  printPWM(data, false);
  if(data > 75 && data < 80 ) { //1
      state[0] = !state[0];
      printR(0, state[0]);
  } else if(data > 150 && data < 160) { //2
      state[1] = !state[1];
      printR(1, state[1]);    
  } else if(data > 230 && data < 240) { //3
      state[2] = !state[2];
      printR(2, state[2]);
  } else if(data > 300 && data < 310) { //4
      state[3] = !state[3];
      printR(3, state[3]);
  } else if(data > 380 && data < 390) { //5
      state[4] = !state[4];
      printR(4, state[4]);
  } else if(data > 460 && data < 470) { //6
      state[5] = !state[5];
      printR(5, state[5]);
  } else if(data > 530 && data < 540) { //7
      state[6] = !state[6];
      printR(6, state[6]);
  } else if(data > 610 && data < 620) { //8
      state[7] = !state[7];
      printR(7, state[7]);
  } else if(data > 690 && data < 700) { //9
  } else if(data > 760 && data < 775) { //0
  } else if(data > 840 && data < 850) { //STAR
      star = !star;
      for (int i = 0; i < 4; i++) {
        printR(i, star);
      }
  } else if(data > 920 && data < 930) { //SHARP
      state[7] = !state[7];
      printR(7, state[7]);
  } else if(data > 1001 && data < 1010) { //UP
  } else if(data > 1070 && data < 1080) { //DOWN
  } else if(data > 1150 && data < 1160) { //LEFT
  } else if(data > 1230 && data < 1240) { //RIGHT
  } else if(data > 1305 && data < 1315) { //OK
      star = false;
      for (int i = 0; i < 8; i++) {
        state[i] = false;
        printR(i, false);
      }
  }
}

void printR(int r, bool isOn) {
  lcd.setCursor(r * 2, 0);
  if (isOn) {
    lcd.print(r + 1);
  } else {
    lcd.print(" ");
  }
}

void printClicks(bool isDelete) {
  lcd.setCursor(0, 1);
  lcd.print("          ");
  if (!isDelete) {
    lcd.setCursor(0, 1);
    lcd.print(clicks);
  } else {
    clicks = 0;
  }
}

void printPWM(uint16_t pwm, bool isDelete) {
  lcd.setCursor(10, 1);
  lcd.print("          ");
  if (!isDelete) {
    lcd.setCursor(10, 1);
    lcd.print(pwm);
  }
}


void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(IR_DATA, INPUT_PULLUP);
  pinMode(IR_SYNC, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IR_SYNC), irEvent, CHANGE);
}

void loop() {
  x = analogRead(A0); // the buttons are read from the analog0 pin

  if (x > 720 && x < 730) {
    // Select
    lightOn = !lightOn;
    if (lightOn) {
      analogWrite(backLight, 255);
    } else {
      analogWrite(backLight, 0);
    }
    delay(1000);
  } else if (x > 470 && x < 490) {
    // Left
  } else if (x < 10) {
    // Right
    printClicks(true);
    printPWM(0, true);
  } else if (x > 120 && x < 140) {
    // Up
  } else if (x > 300 && x < 355) {
    // Down
  }

  delay(100);

}
