
#define CLK_PIN 2
#define  DT_PIN 11
#define  SW_PIN 3

volatile boolean turnDetected;
volatile boolean isUp;


void rotaryEncoder ()  {
  turnDetected = true;
  isUp = (digitalRead(CLK_PIN) == digitalRead(DT_PIN));
}

void rotarySwitch()  {
  Serial.println("Switched");
}

void setup() {
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  attachInterrupt (digitalPinToInterrupt(CLK_PIN), rotaryEncoder, CHANGE);
  attachInterrupt (digitalPinToInterrupt(SW_PIN), rotarySwitch, FALLING);
}

void loop() {
  
  if(turnDetected) {
    Serial.print("Rotated: ");
    if(isUp) {
      Serial.println("UP");
    } else {
      Serial.println("DOWN");
    }
    turnDetected = false;
  }

  
  delay(500);

}
