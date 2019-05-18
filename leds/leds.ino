/*
 * Lights up the leds when motion is detected. Lights off the leds after a short delay when no motion is detected anymore.
 * 
 * Test board: Arduino One
 * Prod board: Arduino Nano 3.0
 * 
 * Requires:
 * FastLed (http://fastled.io/)
 * LowPower (https://github.com/rocketscream/Low-Power)
 */

#include <FastLED.h>
#include <LowPower.h>

#define NUM_LEDS 72
#define LED_DATA_PIN 6
#define PIR_DATA_PIN 2
#define PHOTORESISTOR_DATA_PIN A5
#define BUILTIN_LED_PIN 13

// Defines the limit of dark in the room before lighting up the leds (smaller value is darker)
#define PHOTO_RESISTOR_LIMIT 4

enum State
{
  LEDS_IDLE,
  INITIAL_DELAY,
  LEDS_TURNING_ON,
  LEDS_ON,
  LEDS_TURNING_OFF,
  LEDS_QUICK_OFF,
  POST_IDLE
};

// Globals
CRGB leds[NUM_LEDS];
State state = LEDS_IDLE;
int counter = 0;
int litLeds = 0;

/*
 * Returns true if lights are on
 */
#define AVERAGE_LENGTH 3
bool checkLights()
{
  int total = 0;
  for (int i = 0; i < AVERAGE_LENGTH; i++)
  {
    total += analogRead(PHOTORESISTOR_DATA_PIN);
    if (i != AVERAGE_LENGTH - 1)
    {
      LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
    }
  }
  int average = total / AVERAGE_LENGTH;
#ifdef DEBUG
  Serial.write("Photoresistor average: ");
  Serial.print(average);
  Serial.write("\n");
#endif
  return (average > PHOTO_RESISTOR_LIMIT);
}

/*
 * Reads the motion detector
 */
bool checkMotion()
{
  int val = digitalRead(PIR_DATA_PIN);
  if (val == HIGH)
  {
    digitalWrite(PIR_DATA_PIN, LOW);
    // digitalWrite(BUILTIN_LED_PIN, val);
    return true;
  }
  return false;
}

/*
 * Sets up the leds and pins
 */
void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
  Serial.write("Program started\n");
#endif

  pinMode(PIR_DATA_PIN, INPUT);
  digitalWrite(PIR_DATA_PIN, LOW);
  FastLED.addLeds<WS2812, LED_DATA_PIN>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

/*
 * Runs the state machine
 */
void loop()
{
  switch (state)
  {
  case LEDS_IDLE:
    if (checkMotion())
    {
      state = INITIAL_DELAY;
    }
    break;

  case INITIAL_DELAY:
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
    if (checkLights() || !checkMotion())
    {
      state = INITIAL_DELAY;
    }
    else
    {
      state = LEDS_TURNING_ON;
    }
    break;

  case LEDS_TURNING_ON:
    if (litLeds < 0)
    {
      litLeds = 0;
    }

    for (; litLeds < NUM_LEDS / 2; litLeds++)
    {
      leds[NUM_LEDS / 2 - litLeds - 1] = CHSV(100, 155, 155);
      leds[NUM_LEDS / 2 + litLeds] = CHSV(100, 155, 155);
      FastLED.show();
      LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    }
    state = LEDS_ON;
    break;

  case LEDS_ON:
    if (checkLights())
    {
      state = LEDS_QUICK_OFF;
    }
    else if (!checkMotion())
    {
      state = LEDS_TURNING_OFF;
    }
    break;

  case LEDS_TURNING_OFF:
    if (litLeds >= NUM_LEDS / 2)
    {
      litLeds = NUM_LEDS / 2 - 1;
    }
    
    for (; litLeds >= 0; litLeds--)
    {
      leds[NUM_LEDS / 2 - litLeds - 1] = CRGB::Black;
      leds[NUM_LEDS / 2 + litLeds] = CRGB::Black;
      FastLED.show();
      LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);

      if (checkLights())
      {
        state = LEDS_QUICK_OFF;
        break;
      }
      if (checkMotion())
      {
        state = LEDS_TURNING_ON;
        break;
      }
    }

    if (state == LEDS_TURNING_OFF) 
    {
      state = POST_IDLE;
    }
    counter = 0;
    break;

  case LEDS_QUICK_OFF:
    if (litLeds >= NUM_LEDS / 2)
    {
      litLeds = NUM_LEDS / 2 - 1;
    }
    
    for (; litLeds >= 0; litLeds--)
    {
      leds[NUM_LEDS / 2 - litLeds - 1] = CRGB::Black;
      leds[NUM_LEDS / 2 + litLeds] = CRGB::Black;
      FastLED.show();
      LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
    }
    state = LEDS_IDLE;
    break;

  case POST_IDLE:
    if (!checkLights() && checkMotion())
    {
      state = LEDS_TURNING_ON;
    }
    if (counter > 120)
    {
      state = LEDS_IDLE;
    }
    break;
  }

  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  counter++;
}
