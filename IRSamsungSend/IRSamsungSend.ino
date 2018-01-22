/*
 * IRremote: IRsendDemo - demonstrates sending IR codes with IRsend
 * An IR LED must be connected to Arduino PWM pin 3.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */


#include <IRremote.h>
int led = 13;
IRsend irsend;

//FYI: on our samsung TV the protocol used is 32 bit NEC and the code for power is 0xF4BA2988.


void setup()
{
    pinMode(led, OUTPUT);     
}

void loop() {

  // Working
	for (int i = 0; i < 3; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level
		irsend.sendSAMSUNG(0xE0E040BF, 32);    
		delay(40);
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
	}
  
	delay(5000); //5 second delay between each signal burst
}
