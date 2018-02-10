#define SWITCH_PIN 2 // Pin D2
#define LED_PIN 13
#define FADE_LED_PIN 9

#define STATE_NO_CODE       0
#define STATE_CODE_LEARNING 1 // Short button press
#define STATE_CODE_KNOWN    2
#define STATE_CODE_SENDING  3 // Short button press
#define STATE_CODE_CLEAR    4 // Long button press

#define STATE_NORMAL 0
#define STATE_SHORT 1
#define STATE_LONG 2

static const byte BUTTON_PIN = 2;
volatile int  resultButton = 0; // global value set by checkButton()


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

  m_currentState = STATE_NO_CODE;
  
}

int brightness = 0;    // how bright the LED is
int fadeAmount = 4;    // how many points to fade the LED by

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

void loop() {

  if (resultButton == STATE_NORMAL)
    SetLEDForOffNoFade();
    
  if (resultButton == STATE_SHORT)
    SetLEDForFastFade();
  
  if (resultButton == STATE_LONG)
    SetLEDForHalfBrightNoFade();


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
  boolean longButtonPressDetected = false;
  if (longButtonPressDetected)
    ProgressToState(STATE_CODE_LEARNING);
}

void  stateCodeLearning(){
  boolean newCodeLearnt = false;
  if (newCodeLearnt)
    ProgressToState(STATE_CODE_KNOWN);
}

void  stateCodeKnown(){
  boolean shortButtonPressDetected = false;
  if (shortButtonPressDetected)
    ProgressToState(STATE_CODE_SENDING);  

  boolean longButtonPressDetected = false;
  if (longButtonPressDetected)
    ProgressToState(STATE_CODE_CLEAR);
}

void  stateCodeSending(){
  // Send code
  // Transmit
  // Return to code known
  ProgressToState(STATE_CODE_KNOWN);  
}
  
void  stateCodeClear(){
  // Clear any saved state
  ProgressToState(STATE_NO_CODE);
}


void pin_ISR() {
  int value = digitalRead(SWITCH_PIN);
  Serial.println(value);   
  if ( value == LOW ) {
    digitalWrite(LED_PIN, HIGH );
  } 
  else {
    digitalWrite(LED_PIN, LOW );
  }
}


//*****************************************************************
void checkButton() {
  /*
  * This function implements software debouncing for a two-state button.
  * It responds to a short press and a long press and identifies between
  * the two states. Your sketch can continue processing while the button
  * function is driven by pin changes.
  */

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
  } else if (timeoutShort && Released) {                          // short timeout has occurred (and not long timeout) and button was just released
      resultButton = STATE_SHORT | resultButton;     // ensure the button result reflects a short press
      Serial.println("Short press detected");
  } else {                                                                                  // else there is no change in status, return the normal state
      resultButton = STATE_NORMAL | resultButton; // with no change in status, ensure no change in button status
  }
}

