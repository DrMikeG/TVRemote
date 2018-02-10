/* record.ino Example sketch for IRLib2
 *  Illustrate how to record a signal and then play it back.
 */
#include <IRLibDecodeBase.h>  //We need both the coding and
#include <IRLibSendBase.h>    // sending base classes
#include <IRLib_P01_NEC.h>    //Lowest numbered protocol 1st
#include <IRLib_P02_Sony.h>   // Include only protocols you want
#include <IRLib_P03_RC5.h>
#include <IRLib_P04_RC6.h>
#include <IRLib_P05_Panasonic_Old.h>
#include <IRLib_P07_NECx.h>
#include <IRLib_HashRaw.h>    //We need this for IRsendRaw
#include <IRLibCombo.h>       // After all protocols, include this


#include <IRLibRecv.h>


// Pins to break out:
//GND
//Vin
//D2 in
//D3 out
//D9 out
//D11 in

// LED and Fade
// LED with resistor to pin D9, other leg to gnd
#define FADE_LED_PIN 9
#define LED_PIN 13
int brightness = 0;    // how bright the LED is
int fadeAmount = 4;    // how many points to fade the LED by

// IR receiver
// Facing away from you (bump away from you)
// Left leg 5V
// Mid leg GND
// Right leg D11
int RECV_PIN = 11;
IRrecv myReceiver(RECV_PIN); //pin number for the receiver

// IR LED 
// Resistor leg D3
// Black leg GND
// Storage for the recorded code
IRdecode myDecoder;
IRsend mySender; // pin 3
uint8_t codeProtocol;  // The type of code
uint32_t codeValue;    // The data bits if type is not raw
uint8_t codeBits;      // The length of the code in bits

//These flags keep track of whether we received the first code 
//and if we have have received a new different code from a previous one.
bool gotOne, gotNew; 


// Button press
// Switch connects gnd to D2
#define SWITCH_PIN 2 // Pin D2
#define STATE_NORMAL 0
#define STATE_SHORT 1
#define STATE_LONG 2
static const byte BUTTON_PIN = 2;
volatile int  resultButton = 0; // global value set by checkButton()
volatile int recordedPress = 0;

// Program state
#define STATE_NO_CODE       0
#define STATE_CODE_LEARNING 1 // Short button press
#define STATE_CODE_KNOWN    2
#define STATE_CODE_SENDING  3 // Short button press
#define STATE_CODE_CLEAR    4 // Long button press
byte m_currentState;

void setup() {
  Serial.begin(9600);

  pinMode(FADE_LED_PIN, OUTPUT);
  
  // Setup the switch pin to be input with an internal pull-up :
  pinMode(SWITCH_PIN,INPUT_PULLUP);
  //attachInterrupt(0, pin_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(2), checkButton, CHANGE);
  
  //Setup the LED :
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN, LOW );

  gotOne=false; gotNew=false;
  codeProtocol=UNKNOWN; 
  codeValue=0; 

  myReceiver.enableIRIn(); // Start the receiver
  
  ProgressToState(STATE_NO_CODE);
  
}


void SetLEDForSlowFade()
{
  brightness = 0;
  fadeAmount = 4;
}
void SetLEDForFastFade()
{
  brightness = 0;
  fadeAmount = 10;
}

void SetLEDForHalfBrightNoFade()
{
  brightness = 128;
  fadeAmount = 0;
}

void SetLEDForBrightNoFade()
{
  brightness = 254;
  fadeAmount = 0;
}

void SetLEDForOffNoFade()
{
  brightness = 0;
  fadeAmount = 0;
}


void SedLEDForState()
{
  switch (GetCurrentState())
  {
    case STATE_NO_CODE:
      SetLEDForSlowFade();
    break;
    case STATE_CODE_LEARNING:
      SetLEDForFastFade();
    break;
    case STATE_CODE_KNOWN:
      SetLEDForHalfBrightNoFade();
    break;
    case STATE_CODE_SENDING:
      SetLEDForBrightNoFade();
    break;
    case STATE_CODE_CLEAR:      
    break;
  }
}

void AdvanceLed(){
  // set the brightness of pin 9:
  analogWrite(FADE_LED_PIN, brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount ;
  }
  if (brightness > 255)
    brightness = 255;
  
  if (brightness < 0)
    brightness = 0;
  // wait for 30 milliseconds to see the dimming effect
  delay(30);
}

int GetCurrentState()
{
  return m_currentState;
}

int ProgressToState(int newState)
{
  Serial.print("new State: ");Serial.println(newState);
  m_currentState = newState;
  SedLEDForState();
}

int GetAndClearAnyButtonPress()
{
  // If there is a record of a button press, get it and clear the record of it
  int toReturn = recordedPress;
  if (recordedPress > 0)
    recordedPress  = 0;
  return toReturn;
}

void loop() {



  switch (GetCurrentState())
  {
    case STATE_NO_CODE:
      stateNoCode();
    break;
    case STATE_CODE_LEARNING:
      stateCodeLearning();
    break;
    case STATE_CODE_KNOWN:
      stateCodeKnown();
    break;
    case STATE_CODE_SENDING:
      stateCodeSending();
    break;
    case STATE_CODE_CLEAR:
      stateCodeClear();
    break;
  }  

  AdvanceLed();
  
}


void  stateNoCode(){
  boolean longButtonPressDetected = (GetAndClearAnyButtonPress() == STATE_LONG);
  if (longButtonPressDetected)
    ProgressToState(STATE_CODE_LEARNING);
}

void  stateCodeLearning(){
  boolean newCodeLearnt = false;
  //Serial.println("stateCodeLearning");


 if (myReceiver.getResults()) {
    myDecoder.decode();
    storeCode();
    newCodeLearnt = true;
    myReceiver.enableIRIn(); // Re-enable receiver
  }
  
  if (newCodeLearnt)
    ProgressToState(STATE_CODE_KNOWN);

  myReceiver.enableIRIn(); // Start the receiver
 
}

void  stateCodeKnown(){
  int buttonPress = GetAndClearAnyButtonPress();
  boolean shortButtonPressDetected = (buttonPress == STATE_SHORT);
  boolean longButtonPressDetected = (buttonPress == STATE_LONG);
  
  if (shortButtonPressDetected)
    ProgressToState(STATE_CODE_SENDING); 
  
  if (longButtonPressDetected)
    ProgressToState(STATE_CODE_CLEAR);
}

void  stateCodeSending(){

  if(gotOne) {
    sendCode();
    myReceiver.enableIRIn(); // Re-enable receiver
  }
    
  ProgressToState(STATE_CODE_KNOWN);  
}
  
void  stateCodeClear(){
  // Clear any saved state
  ProgressToState(STATE_NO_CODE);
}


/**
 * IR Code
 * 
 */
// Stores the code for later playback
void storeCode(void) {
  gotNew=true;    gotOne=true;
  codeProtocol = myDecoder.protocolNum;
  Serial.print(F("Received "));
  Serial.print(Pnames(codeProtocol));
  if (codeProtocol==UNKNOWN) {
    Serial.println(F(" saving raw data."));
    myDecoder.dumpResults();
    codeValue = myDecoder.value;
  }
  else {
    if (myDecoder.value == REPEAT_CODE) {
      // Don't record a NEC repeat value as that's useless.
      Serial.println(F("repeat; ignoring."));
    } else {
      codeValue = myDecoder.value;
      codeBits = myDecoder.bits;
    }
    Serial.print(F(" Value:0x"));
    Serial.println(codeValue, HEX);
  }
}
void sendCode(void) {
  if( !gotNew ) {//We've already sent this so handle toggle bits
    if (codeProtocol == RC5) {
      codeValue ^= 0x0800;
    }
    else if (codeProtocol == RC6) {
      switch(codeBits) {
        case 20: codeValue ^= 0x10000; break;
        case 24: codeValue ^= 0x100000; break;
        case 28: codeValue ^= 0x1000000; break;
        case 32: codeValue ^= 0x8000; break;
      }      
    }
  }
  gotNew=false;
  if(codeProtocol== UNKNOWN) {
    //The raw time values start in decodeBuffer[1] because
    //the [0] entry is the gap between frames. The address
    //is passed to the raw send routine.
    codeValue=(uint32_t)&(recvGlobal.decodeBuffer[1]);
    //This isn't really number of bits. It's the number of entries
    //in the buffer.
    codeBits=recvGlobal.decodeLength-1;
    Serial.println(F("Sent raw"));
  }
  mySender.send(codeProtocol,codeValue,codeBits);
  if(codeProtocol==UNKNOWN) return;
  Serial.print(F("Sent "));
  Serial.print(Pnames(codeProtocol));
  Serial.print(F(" Value:0x"));
  Serial.println(codeValue, HEX);
}





/**
 *  Button
 * 
 */

void checkButton() {
  const unsigned long LONG_DELTA = 1000ul;               // hold seconds for a long press
  const unsigned long DEBOUNCE_DELTA = 30ul;        // debounce time
  static int lastButtonStatus = HIGH;                                   // HIGH indicates the button is NOT pressed
  int buttonStatus;                                                                    // button atate Pressed/LOW; Open/HIGH
  static unsigned long longTime = 0ul, shortTime = 0ul; // future times to determine is button has been poressed a short or long time
  boolean Released = true, Transition = false;                  // various button states
  boolean timeoutShort = false, timeoutLong = false;    // flags for the state of the presses

  buttonStatus = digitalRead(BUTTON_PIN);                // read the button state on the pin "BUTTON_PIN"
  timeoutShort = (millis() > shortTime);                          // calculate the current time states for the button presses
  timeoutLong = (millis() > longTime);

  if (buttonStatus != lastButtonStatus) {                          // reset the timeouts if the button state changed
      shortTime = millis() + DEBOUNCE_DELTA;
      longTime = millis() + LONG_DELTA;
  }

  Transition = (buttonStatus != lastButtonStatus);        // has the button changed state
  Released = (Transition && (buttonStatus == HIGH)); // for input pullup circuit

  lastButtonStatus = buttonStatus;                                     // save the button status

  if ( ! Transition) {                                                                //without a transition, there's no change in input
  // if there has not been a transition, don't change the previous result
       resultButton =  STATE_NORMAL | resultButton;
       return;
  }

  if (timeoutLong && Released) {                                      // long timeout has occurred and the button was just released
       resultButton = STATE_LONG | resultButton;       // ensure the button result reflects a long press
       Serial.println("Long press detected");
       recordedPress = STATE_LONG;
  } else if (timeoutShort && Released) {                          // short timeout has occurred (and not long timeout) and button was just released
      resultButton = STATE_SHORT | resultButton;     // ensure the button result reflects a short press
      Serial.println("Short press detected");
      recordedPress = STATE_SHORT;
  } else {                                                                                  // else there is no change in status, return the normal state
      resultButton = STATE_NORMAL | resultButton; // with no change in status, ensure no change in button status
  }
}

