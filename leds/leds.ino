  /*
 * Lights up the leds when motion is detected. Lights off the leds after a short delay when no motion is detected anymore.
 * 
 * Requires:
 * TimedAction (http://playground.arduino.cc/code/TimedAction#Download)
 * FastLed (http://fastled.io/)
 */

#include <FastLED.h>
#include <TimedAction.h>

#define NUM_LEDS 72
#define LED_DATA_PIN 6
#define PIR_DATA_PIN 2
#define PHOTORESISTOR_DATA_PIN A5

// Defines how long the leds are lit after motion is detected
#define LIGHTS_ON_DURATION 4000
// Defines how long to wait before lighting the leds after motion is detected
#define LIGHTS_ON_DELAY 1000

CRGB leds[NUM_LEDS];

void checkMotion();
void cycleLeds();
void lightsOn();
void lightsOff();

TimedAction motionAction = TimedAction(100, checkMotion);
TimedAction ledAction = TimedAction(20, cycleLeds);
TimedAction offAction = TimedAction(LIGHTS_ON_DURATION, lightsOff);
TimedAction waitAction = TimedAction(LIGHTS_ON_DELAY, lightsOn);

enum State {
  NO_MOTION,
  MOTION
};

enum Lights {
  ALL_ON,
  TURNING_ON,
  TURNING_OFF,
  ALL_OFF
};

State state = NO_MOTION;
Lights lights = ALL_OFF;
int litLeds = 0;

/*
 * Sets up the leds and pins
 */
void setup() {
  Serial.begin(9600); 

  pinMode(PIR_DATA_PIN, INPUT);
  FastLED.addLeds<WS2812, LED_DATA_PIN>(leds, NUM_LEDS);

  offAction.disable();
  ledAction.disable();
  waitAction.disable();
}

/*
 * Runs the timed actions
 */
void loop(){
  motionAction.check();
  ledAction.check();
  offAction.check();
  waitAction.check();
}

/*
 * Action which sets the leds on or off
 */
void cycleLeds() {
  if (lights == TURNING_ON) {
    leds[NUM_LEDS / 2 - litLeds - 1] = CHSV(100, 155, 155);
    leds[NUM_LEDS / 2 + litLeds] = CHSV(100, 155, 155);
    litLeds++;
  } else if (lights == TURNING_OFF) {
    leds[NUM_LEDS / 2 - litLeds - 1] = CRGB::Black;
    leds[NUM_LEDS / 2 + litLeds] = CRGB::Black;
    litLeds--;
  }
  FastLED.show();

  if (litLeds < 0) {
    ledAction.disable();
    litLeds = 0;
    lights = ALL_OFF;
  } else if (litLeds >= NUM_LEDS / 2) {
    ledAction.disable();
    litLeds = NUM_LEDS / 2 - 1;
    lights = ALL_ON;
  }
}

/*
 * Action which reads the motion detector and starts the action to turn the lights on if necessary
 */
void checkMotion() {
  int val = digitalRead(PIR_DATA_PIN);
  if (val == HIGH) {
    Serial.write("Motion detected\n");
    // Motion detected. Restart the lights off timer
    offAction.enable();
    offAction.reset();
    if (state == NO_MOTION) {
      if (lights == TURNING_OFF) {
        Serial.write("Leds are turning off. Let's relight them.\n");
        lights = TURNING_ON;
        ledAction.setInterval(20);
        ledAction.enable();
        ledAction.reset();
      } else {
        // If this was the first time the motion was detected light up the leds
        Serial.write("Lighting the leds after delay\n");
        waitAction.enable();
        waitAction.reset();
      }
      state = MOTION;
    }
  }
}

/*
 * Action which starts the action to turn the lights on
*/
void lightsOn() {
  Serial.write("Delay ended\n");
  waitAction.disable();
  
  // Turn the leds on only if it's dark
  int value = analogRead(PHOTORESISTOR_DATA_PIN);
  if (value < 500) {
    Serial.write("It's dark so lets light up the leds\n");
    lights = TURNING_ON;
    ledAction.setInterval(20);
    ledAction.enable();
    ledAction.reset();
  } else {
    Serial.write("Lights are on so no need to do anything\n");
  }
}

/*
 * Action which starts the action to turn the lights off
 */
void lightsOff() {
  Serial.write("Turning the lights off\n");
  offAction.disable();
  lights = TURNING_OFF;
  ledAction.setInterval(200);
  ledAction.enable();
  ledAction.reset();
  state = NO_MOTION;
}

