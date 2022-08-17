#include "FastLED.h"
#include "FastLED_RGBW.h"
#include <movingAvg.h>

#define PEAK_FALL 10 // Rate of peak falling dot
#define COLOR_FROM 0
#define COLOR_TO 255
#define ANALOG_INPUT_CHANNEL A3
#define NUM_LEDS 100
#define DATA_PIN 2
#define VU_LEVELS 14  // Numbers of levels onthe vu meter

const int sampleWindow = 30;  // Sample window width in mS (50 mS = 20Hz)
const uint8_t brightness = 100;
int maximum = 1024;
int peak;
int dotCount;
unsigned int sample;

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];


void printValues(float v1, float v2, float v3) {
  Serial.print(v1);
  Serial.print(",");
  Serial.print(v2);
  Serial.print(",");
  Serial.println(v3);
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB,
                                          rgb_size_from_rgbw_size(NUM_LEDS));
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void loop() { VU(); }

int global_peak_to_peak = 0;

void displaySignalValue(int value, int max) {
  for (int i; i <= NUM_LEDS; i++) {
    int color = map(i, 0, NUM_LEDS, 0, 255);
    if (i < led) {
      leds[i] = Wheel(color);
    } else {
      leds[i] = CRGBW(0, 0, 0, 0);
    }
  }

  if (peak > 1 && peak <= NUM_LEDS - 1) {
    leds[peak] = Wheel(map(peak, 0, NUM_LEDS - 1, 0, 255));
  }

  FastLED.show();
}

void VU() {
  unsigned long startMillis = millis(); // Start of sample window
  unsigned int peakToPeak = 0;          // peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(ANALOG_INPUT_CHANNEL);
    if (sample < 1024) // toss out spurious readings
    {
      if (sample > signalMax) {
        signalMax = sample; // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample; // save just the min levels
      }
    }
  }

  peakToPeak = signalMax - signalMin;
  global_peak_to_peak = ALPHA * peakToPeak + (1 - ALPHA) * global_peak_to_peak;
  int led = map(global_peak_to_peak, 0, maximum, 0, NUM_LEDS) - 1;
  printValues(sample, global_peak_to_peak, led);
  if (led > peak) {
    peak = led; // Keep 'peak' dot at top
  }

  displaySignalValue(led, maximum);

  if (peak > 1 && peak <= NUM_LEDS - 1) {
    leds[peak] = Wheel(map(peak, 0, NUM_LEDS - 1, 0, 255));
  }

  FastLED.show();

  if (++dotCount >= PEAK_FALL) { // fall rate
    if (peak > 0) {
      peak--;
    }
    dotCount = 0;
  }
}

CRGBW Wheel(byte WheelPos) { return CRGBW(0, 255 - WheelPos, 0, WheelPos); }
