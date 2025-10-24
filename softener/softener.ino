/*
 *  Sweeping frequency water softener
 * Created: 15/12/2020
 *  Author: moty22.co.uk
 */ 
 
#define pwm OCR2A
unsigned char ramp=0, freq=25, rate;

void setup() {

    //timer2 of atmega328 is set to fast PWM mode at frequency of 64KHz
  TCCR2A = 0x83;
  TCCR2B = 1;
  pinMode(11, OUTPUT);

}

void loop()
{
      //ramping the frequency up/down
    ++freq;
    if(freq>150){freq=1;}
    delayMicroseconds(freq);
    
    for(rate=0;rate<52;++rate){
    if(pwm > 244){ramp=0;}
    if(pwm < 12){ramp=1;}
    if(ramp){pwm += 10;}else{pwm -= 10;}
   
    }

}
