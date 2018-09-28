  /*
 * Lights up the leds when motion is detected. Lights off the leds after a short delay when no motion is detected anymore.
 * 
 * Test board: Arduino One
 * Prod board: Arduino Nano 3.0
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
// Defines the limit of dark in the room before lighting up the leds (smaller value is darker)
#define PHOTO_RESISTOR_LIMIT 200

//     cxxx#define DEBUG 1

CRGB leds[NUM_LEDS];

void checkMotion();
void cycleLeds();
void lightsOn();
void lightsOff();

TimedAction motionAction = TimedAction(1000, checkMotion);
TimedAction ledAction = TimedAction(20, cycleLeds);
TimedAction offAction = TimedAction(LIGHTS_ON_DURATION, lightsOff);
TimedAction waitAction = TimedAction(LIGHTS_ON_DELAY, lightsOn);

#ifdef DEBUG
void readSensors();
TimedAction sensorsAction = TimedAction(1000, readSensors);
#endif

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
#ifdef DEBUG
  Serial.begin(9600); 
  Serial.write("Program started\n");
#endif

  pinMode(PIR_DATA_PIN, INPUT);
  digitalWrite(PIR_DATA_PIN, LOW);
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
#ifdef DEBUG
  sensorsAction.check();
#endif
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
#ifdef DEBUG
    Serial.write("Motion detected\n");
#endif
    digitalWrite(PIR_DATA_PIN, LOW);
    // Motion detected. Restart the lights off timer
    offAction.enable();
    offAction.reset();
    if (state == NO_MOTION) {
      if (lights == TURNING_OFF) {
#ifdef DEBUG
        Serial.write("Leds are turning off. Let's relight them.\n");
#endif
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
#ifdef DEBUG
  Serial.write("Delay ended\n");
#endif
  waitAction.disable();
  
  // Turn the leds on only if it's dark
  int value = analogRead(PHOTORESISTOR_DATA_PIN);
  if (value < PHOTO_RESISTOR_LIMIT) {
#ifdef DEBUG
    Serial.write("It's dark so lets light up the leds\n");
#endif
    lights = TURNING_ON;
    ledAction.setInterval(20);
    ledAction.enable();
    ledAction.reset();
  } else {
#ifdef DEBUG
    Serial.write("Lights are on so no need to do anything\n");
#endif
  }
}

/*
 * Action which starts the action to turn the lights off
 */
void lightsOff() {
#ifdef DEBUG
  Serial.write("Turning the lights off\n");
#endif
  offAction.disable();
  lights = TURNING_OFF;
  ledAction.setInterval(200);
  ledAction.enable();
  ledAction.reset();
  state = NO_MOTION;
}

/*
 * Action which outputs the sensor values
 */
#ifdef DEBUG
void readSensors() {
  int valuePR = analogRead(PHOTORESISTOR_DATA_PIN);
  int valuePIR = digitalRead(PIR_DATA_PIN);

  Serial.write("Photoresistor: ");
  Serial.print(valuePR);
  Serial.write(" - PIR motion sensor: ");
  Serial.print(valuePIR);
  Serial.write("\n");
}
#endif

