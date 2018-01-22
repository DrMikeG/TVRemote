//Turn Samsung TV on/off repeatedly to test program.
//IR LED connected to pin 3.

#include <IRremote.h>

unsigned int power[] = {4550,4400,600,1650,550,1650,600,1650,
550,550,600,500,600,550,550,550,600,500,600,1650,600,1600,600,
1650,550,550,600,500,600,550,600,500,600,500,650,450,650,1600,
600,500,650,450,650,500,600,500,600,500,600,550,600,1600,600,
500,650,1600,650,1550,650,1600,650,1550,650,1600,650,1600,600};

IRsend irsend;

void setup()
{
  pinMode (3, OUTPUT);  //output used in library
}

void loop() {

      irsend.sendRaw(power,68,38);
      
  delay (5000);

}//end of loop
