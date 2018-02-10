#include <IRremote.h>

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
IRrecv irrecv(RECV_PIN);

// IR LED 
// Resistor leg D3
// Black leg GND
IRsend irsend;
decode_results results;


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

  irrecv.enableIRIn(); // Start the receiver
  
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

 
  if (irrecv.decode(&results)) {
    //Serial.println("decoded");
    storeCode(&results);
    irrecv.resume(); // resume receiver    
    newCodeLearnt = true;
  }
  
  if (newCodeLearnt)
    ProgressToState(STATE_CODE_KNOWN);

  irrecv.enableIRIn(); // Start the receiver
 
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
/*
    // Working
  for (int i = 0; i < 3; i++) {
    irsend.sendSAMSUNG(0xE0E040BF, 32);    
    delay(40);
  }
*/

    for (int repeat = 0; repeat < 1; repeat++)
    {
      sendCode(repeat != 0);
      delay(5); // Wait a bit between retransmissions
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
// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the co
int toggle = 0; // The RC5/6 toggle state

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  //int count = results->rawlen;
  if (codeType == UNKNOWN) {
    Serial.println("Received unknown code, saving as raw");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) {
      if (i % 2) {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
        Serial.print(" m");
      } 
      else {
        // Space
        rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
        Serial.print(" s");
      }
      Serial.print(rawCodes[i - 1], DEC);
    }
    Serial.println("");
  }
  else {
    if (codeType == NEC) {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        // Assuming Samsung TV off...
        
        return;
      }
    } 
    else if (codeType == SONY) {
      Serial.print("Received SONY: ");
    } 
    else if (codeType == PANASONIC) {
      Serial.print("Received PANASONIC: ");
    }
    else if (codeType == JVC) {
      Serial.print("Received JVC: ");
    }
    else if (codeType == RC5) {
      Serial.print("Received RC5: ");
    } 
    else if (codeType == RC6) {
      Serial.print("Received RC6: ");
    } 
    else if (codeType == SAMSUNG) {
      Serial.print("Received SAMSUNG: ");
    } 
    else {
      Serial.print("Unexpected codeType ");
      Serial.print(codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX);
    codeValue = results->value;
    codeLen = results->bits;
  }
}

void sendCode(int repeat) {
  if (codeType == NEC) {
    if (repeat) {
      irsend.sendNEC(REPEAT, codeLen);
      Serial.println("Sent NEC repeat");
    } 
    else {
      irsend.sendNEC(codeValue, codeLen);
      Serial.print("Sent NEC ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == SONY) {
    irsend.sendSony(codeValue, codeLen);
    Serial.print("Sent Sony ");
    Serial.println(codeValue, HEX);
  } 
  else if (codeType == PANASONIC) {
    irsend.sendPanasonic(codeValue, codeLen);
    Serial.print("Sent Panasonic");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == JVC) {
    irsend.sendJVC(codeValue, codeLen, false);
    Serial.print("Sent JVC");
    Serial.println(codeValue, HEX);
  }
  else if (codeType == RC5 || codeType == RC6) {
    if (!repeat) {
      // Flip the toggle bit for a new button press
      toggle = 1 - toggle;
    }
    // Put the toggle bit into the code to send
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) {
      Serial.print("Sent RC5 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC5(codeValue, codeLen);
    } 
    else {
      irsend.sendRC6(codeValue, codeLen);
      Serial.print("Sent RC6 ");
      Serial.println(codeValue, HEX);
    }
  } 
  else if (codeType == UNKNOWN /* i.e. raw */) {
    // Assume 38 KHz
    irsend.sendRaw(rawCodes, codeLen, 38);
    Serial.println("Sent raw");
  }
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

