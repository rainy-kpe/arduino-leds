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
#include <LowPower.h>

// #define DEBUG 1

#define NUM_LEDS 72
#define LED_DATA_PIN 6
#define PIR_DATA_PIN 2
#define PHOTORESISTOR_DATA_PIN A5
#define BUILTIN_LED_PIN 13

// Defines how quickly individual leds are turned off
#define LIGHTS_ON_INTERVAL 20
// Defines how quickly individual leds are turned off
#define LIGHTS_OFF_INTERVAL 400
// Defines the limit of dark in the room before lighting up the leds (smaller value is darker)
#define PHOTO_RESISTOR_LIMIT 5

CRGB leds[NUM_LEDS];

void checkMotion();
void cycleLeds();
void checkDark();

bool lightsOn();
void lightsOff(bool fast);

TimedAction motionAction = TimedAction(500, checkMotion);
TimedAction ledAction = TimedAction(20, cycleLeds);
TimedAction darkAction = TimedAction(500, checkDark);

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
bool lightGuard = false;

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
  FastLED.clear();
  FastLED.show();

  ledAction.disable();
}

/*
 * Runs the timed actions
 */
void loop(){
  ledAction.check();
#ifdef DEBUG
  motionAction.check();
  sensorsAction.check();
  darkAction.check();
#else
  // Go to the low power mode if there is no motion detected
  if (lights == ALL_OFF) {
    checkMotion();
    LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  } else {
    motionAction.check();
    checkDark();
  }
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
    digitalWrite(PIR_DATA_PIN, LOW);
    // digitalWrite(BUILTIN_LED_PIN, val);
    
    if (state == NO_MOTION || lights == ALL_OFF) {
#ifdef DEBUG
      Serial.write("CheckMotion() -> New motion detected\n");
#endif
      state = MOTION;
      if (!lightsOn()) {
        // If lights were not lit because it wasn't dark. Do not lit the lights until there is no motion detected anymore.
        lightGuard = true;
      }
    }
  } else {
    if (state == MOTION) {
#ifdef DEBUG
      Serial.write("CheckMotion(NOT) -> No more motion detected\n");
#endif
      state = NO_MOTION;
      lightsOff(false);
    }
    lightGuard = false;   // From now on the lights can be lit again
  }
}

/*
 * Action which turns lights off if it's not dark anymore
*/
void checkDark() {
  if (lights == ALL_ON && !isDark()) {
    lightsOff(true);
  }
}

/*
 * Action which starts the action to turn the lights on
*/
bool lightsOn() {  
  // Turn the leds on only if it's dark
  if (!lightGuard && isDark()) {
#ifdef DEBUG
    Serial.write("LightsOn() -> It's dark so lets light up the leds\n");
#endif
    lights = TURNING_ON;
    ledAction.setInterval(LIGHTS_ON_INTERVAL);
    ledAction.enable();
    ledAction.reset();
    return true;
  } else {
#ifdef DEBUG
    Serial.write("LightsOn(NOT) -> It's not dark (or it wasn't when the motion was detected) so no need to do anything\n");
#endif
    return false;
  }
}

/*
 * Action which starts the action to turn the lights off
 */
void lightsOff(bool fast) {
    if (lights != ALL_OFF) {
#ifdef DEBUG
    Serial.write("LightsOff() -> Turning the lights off\n");
#endif
    lights = TURNING_OFF;
    ledAction.setInterval(fast ? LIGHTS_ON_INTERVAL : LIGHTS_OFF_INTERVAL);
    ledAction.enable();
    ledAction.reset();
  } else {
#ifdef DEBUG
    Serial.write("LightsOff(NOT) -> All lights are already off so no need to do anything\n");
#endif
  }
}

/*
 * Returns true if its datk
 */
#define AVERAGE_LENGTH 3
bool isDark() {
  int total = 0;
  for (int i = 0; i < AVERAGE_LENGTH; i++) {
    total += analogRead(PHOTORESISTOR_DATA_PIN);
    if (i != AVERAGE_LENGTH - 1) {
      delay(10);
     }
  }
  int average = total / AVERAGE_LENGTH;
#ifdef DEBUG
  Serial.write("Photoresistor average: ");
  Serial.print(average);
  Serial.write("\n");
#endif
  return (average < PHOTO_RESISTOR_LIMIT);
}

#ifdef DEBUG
void readSensors() {
  int valuePR = analogRead(PHOTORESISTOR_DATA_PIN);
  int valuePIR = digitalRead(PIR_DATA_PIN);

  Serial.print(millis());
  Serial.write(" - Photoresistor: ");
  Serial.print(valuePR);
  Serial.write(" - PIR motion sensor: ");
  Serial.print(valuePIR);
  Serial.write(" - Lights: ");
  Serial.print(lights);
  Serial.write(" - State: ");
  Serial.print(state);
  Serial.write(" - Leds: ");
  Serial.print(litLeds);
  Serial.write("\n");
}
#endif

