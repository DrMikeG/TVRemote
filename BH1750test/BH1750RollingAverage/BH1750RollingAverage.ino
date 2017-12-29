/*

Example of BH1750 library usage.

This example initalises the BH1750 object using the default
high resolution mode and then makes a light level reading every second.

Connection:
 VCC-5v
 GND-GND
 SCL-SCL(analog pin 5)
 SDA-SDA(analog pin 4)
 ADD-NC or GND

*/

#include <Wire.h>
#include <BH1750.h>

const int averageArrayMax = 50;
uint16_t averageArray[averageArrayMax];
int averageArrayHead = 0;
long averageArrayTotal = 0;

int statusLed = 13;
int redLed = 12; // Expects red LED on D12/Gnd
long delayInMS = 1000;

uint16_t l_threshold = 50;
int l_count = 50;

uint16_t h_threshold = 1000;
int h_count = 10;

float d_thresholdPercent = 0.5;
int d_count = 10;



BH1750 lightMeter;


void setup(){
  Serial.begin(9600);
  pinMode(statusLed, OUTPUT);   
  pinMode(redLed, OUTPUT);   
  lightMeter.begin();
  Serial.println("Running...");
  GatherNewAverage();
  
}

void GatherNewAverage()
{
      for (int i=0; i < averageArrayMax; i++)
        {
          readLightLevel();
        }  
}



void redLED(boolean on)
{
  if (on)
    digitalWrite(redLed, HIGH);
  else
    digitalWrite(redLed, LOW);
}

void redLEDTripleBlink()
{
  long blinkDelay = 50;
  for (int loop=0; loop < 5; loop++)
  {
    redLED(true);
    delay(blinkDelay);
    redLED(false);
    delay(blinkDelay);
  }
  delay(2000);
}


void checkHighThreshold(uint16_t lux )
{
        if (lux > h_threshold)
        {
                boolean remainedHigh = true;
                for (int count = 0; count < h_count; count++)
                {
                        uint16_t recheckLux = readLightLevel();
                        if (recheckLux < h_threshold)
                        {
                                remainedHigh = false;
                                break;
                        }
                        delay(10);
                }  
                if (remainedHigh)
                {
                        Serial.println("remained high - triple blink");
                        redLEDTripleBlink();
                }
        }
}

void checkLowThreshold(uint16_t lux )
{
        if (lux < l_threshold)
        {
                Serial.println("lux below threshold");
                redLED(false);
                boolean remainedLow = true;
                for (int count = 0; count < l_count; count++)
                {
                        uint16_t recheckLux = readLightLevel();
                        if (recheckLux > l_threshold)
                        {
                                remainedLow = false;
                                break;
                        }
                        delay(20);
                }  
                if (remainedLow)
                {
                        Serial.println("remained low - triple blink");
                        redLEDTripleBlink();
                }
                else
                {
                        Serial.println("false alarm, reset...");
                        redLED(false);
                }
        }
}

void checkThresholdSwing(uint16_t lux)
{

    //uint16_t d_thresholdPercent = 50;
    //int d_count = 10;
    
    if (averageArrayTotal > 0)
    {
        float avg = averageArrayTotal/(float)averageArrayMax;
        Serial.print("avg:");
        Serial.println(avg);
        float diff = ((1.0*lux) - avg);
        Serial.print("diff:");
        Serial.println(diff);

        float diffP = diff / avg;
        Serial.print("diffP:");
        Serial.println(diffP * 100.0);

        // Diff as a percentage of avg...
        if (diffP > d_thresholdPercent)
        {
          redLEDTripleBlink();                    
        }
        else if (diffP < -d_thresholdPercent)
        {
          redLEDTripleBlink();                    
        }
        
    }
}


uint16_t readLightLevel()
{
  uint16_t lux = lightMeter.readLightLevel();

  // Update the circular array
  averageArray[averageArrayHead++] = lux;
  // Manage the circular array pointer
  if (averageArrayHead == averageArrayMax) // Wrap
  {
      averageArrayHead  = 0;
      averageArrayTotal = 1;
  }
  
  // Do we have a full set of values?
  int maxIndexToUse = averageArrayHead;
  if (averageArrayTotal > 0)
  {
    maxIndexToUse = averageArrayMax;
  
    
  // Calculate total for average  
  averageArrayTotal = 0;
  for (int i=0; i < maxIndexToUse; i++)
    averageArrayTotal+=averageArray[i];

  Serial.print("Total of ");
  Serial.print(maxIndexToUse);
  Serial.print(" values is ");
  Serial.println(averageArrayTotal);
  Serial.print("Average is ");
  Serial.println(averageArrayTotal/maxIndexToUse);
  }
  // Return the original reading...
  return lux;
}

void loop() {
  digitalWrite(statusLed, HIGH);  
  redLED(true);
  
  uint16_t lux = readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  
  //checkLowThreshold(lux);
  //checkHighThreshold(lux);
  checkThresholdSwing(lux);

  
  digitalWrite(statusLed, LOW);  
  delay(delayInMS);
}
