/* FastLED RGBW Example Sketch
 *
 * Example sketch using FastLED for RGBW strips (SK6812). Includes
 * color wipes and rainbow pattern.
 *
 * Written by David Madison
 * http://partsnotincluded.com
*/
#include "FastLED.h"
#include "FastLED_RGBW.h"
#include <movingAvg.h>   

#define PEAK_FALL 10  // Rate of peak falling dot
#define PEAK_FALL1 4 // Rate of peak falling dot
#define PEAK_FALL2 0 // Rate of peak falling dot
#define SAMPLES 100  // Length of buffer for dynamic level adjustment
#define COLOR_FROM 0
#define COLOR_TO 255
const int sampleWindow = 30;  // Sample window width in mS (50 mS = 20Hz)
const int sampleWindow1 = 10; // Sample window width in mS (50 mS = 20Hz)
int maximum = 1024;
int peak;
int peak1;
int dotCount;
int dotCount1;
unsigned int sample;
unsigned int sample1;
bool gReverseDirection = false;
#define COLOR_START 0
movingAvg foo(6);
#define NUM_LEDS 100
#define DATA_PIN 2
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];
const uint8_t brightness = 100;

void setup() { 
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, rgb_size_from_rgbw_size(NUM_LEDS));
  FastLED.setBrightness(brightness);
  FastLED.show();
}
void loop(){
  VU();
}


float ALPHA = 1.0;
int global_value = 512;
void rainbowLoop(){
  
  int value = analogRead(A3);
  value = max(512, value);
  int filtered_value = ALPHA* value + (1-ALPHA) * global_value;
  //foo.reading(value);
  //int value2 = foo.getAvg();
  int value_mapped = map(filtered_value, 512, 1024, 0, NUM_LEDS-1);
  value_mapped = value_mapped * 5;
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (i < value_mapped) {
      leds[i] = CRGBW(0, 0, 0, 255);
    } else {
      leds[i] = CRGBW(0, 0, 0, 0);
    }
  }
  Serial.print(value);
  Serial.print(",");
  Serial.print(filtered_value);
  Serial.print(",");
  Serial.println(value_mapped);
  FastLED.show();
}

int global_peak_to_peak = 0;

void VU() {
  unsigned long startMillis = millis(); // Start of sample window
  unsigned int peakToPeak = 0;          // peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(A3);
    if (sample < 1024) // toss out spurious readings
    {
      if (sample > signalMax) {
        signalMax = sample; // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample; // save just the min levels
      }
    }
  }

   peakToPeak = signalMax - signalMin; // max - min = peak-peak amplitude
   global_peak_to_peak = ALPHA * peakToPeak + (1-ALPHA) * global_peak_to_peak;
   int led = map(global_peak_to_peak, 0, maximum, 0, NUM_LEDS) - 1;
  Serial.print(sample);
  Serial.print(",");
   Serial.print(global_peak_to_peak);
  Serial.print(",");
  Serial.println(led);
  if (led > peak) {
    peak = led; // Keep 'peak' dot at top
  }

  for (int i; i <= NUM_LEDS; i++) {
    int color = map(i, 0, NUM_LEDS, 0, 255);
    if (i < led) {
      leds[i] = Wheel(color);
    } else {
      leds[i] = CRGBW(0, 0, 0, 0);
    }
  }



  if (led > peak) {
    peak = led; // Keep 'peak' dot at top
  }

  if (peak > 1 && peak <= NUM_LEDS - 1)
  {
   leds[peak] = Wheel(map(peak, 0, NUM_LEDS - 1, 0, 255));
  }

  FastLED.show();

  if (++dotCount >= PEAK_FALL) { // fall rate
    if (peak > 0) {
      peak--;
    }
    dotCount = 0;
  }
  // EDITED//
}


CRGBW Wheel(byte WheelPos) {
  return CRGBW(0, 255 - WheelPos, 0, WheelPos);
}
