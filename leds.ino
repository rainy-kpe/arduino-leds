/*
 * Lights up the leds when motion is detected. Lights off the leds after a short delay when no motion is detected anymore.
 */

#include <FastLED.h>
#include <TimedAction.h>

#define NUM_LEDS 72
#define LED_DATA_PIN 6
#define PIR_DATA_PIN 2

CRGB leds[NUM_LEDS];

TimedAction motionAction = TimedAction(1000, checkMotion);
TimedAction ledAction = TimedAction(20, cycleLeds);
TimedAction offAction = TimedAction(2000, lightsOff);

enum State {
  NO_MOTION,
  MOTION
};

enum Lights {
  TURNING_ON,
  TURNING_OFF
}

State state = NO_MOTION;
Lights lights = TURNING_OFF;
int lightedLeds = 0;

/*
 * Sets up the leds and pins
 */
void setup() {
  pinMode(PIR_DATA_PIN, INPUT);
  FastLED.addLeds<WS2812, LED_DATA_PIN>(leds, NUM_LEDS);
  offAction.disable();
  ledAction.disable();
}

/*
 * Runs the timed actions
 */
void loop(){
  timedAction.check();
  ledAction.check();
  offAction.check();
}

/*
 * Action which sets the leds on or off
 */
void cycleLeds() {
  if (lights == TURNING_ON) {
    leds[lightedLeds++] = CHSV(hue++, 255, 255);
  } else {
    leds[lightedLeds--] = CRGB::Black;
  }
  FastLED.show();

  if (lightedLeds < 0) {
    ledAction.disable();
    lightedLeds = 0;
  } else if (lightedLeds >= NUM_LEDS) {
    ledAction.disable();
    lightedLeds = NUM_LEDS - 1;
  }
}

/*
 * Action which reads the motion detector and starts the action to turn the lights on if necessary
 */
void checkMotion() {
  int val = digitalRead(PIR_DATA_PIN);
  if (val == HIGH) {
      // Motion detected. Restart the lights off timer
      offAction.enable();
      offAction.reset();
      if (state == NO_MOTION) {
        // If this was the first time the motion was detected light up the leds
        lights = TURNING_ON;
        cycleLeds.enable();
        cycleLeds.reset();
        state = MOTION;
      }
  }
}

/*
 * Action which starts the action to turn the lights off
 */
void lightsOff() {
  offAction.disable();
  lights = TURNING_OFF;
  cycleLeds.enable();
  cycleLeds.reset();
  state = NO_MOTION;
}
