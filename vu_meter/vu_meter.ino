#include <FastLED.h>
#include "FastLED_RGBW.h"

#define PEAK_FALL 10 // Rate of peak falling dot
#define COLOR_FROM 0
#define COLOR_TO 255
#define ANALOG_INPUT_CHANNEL A0
#define NUM_LEDS 30
#define DATA_PIN 2
#define VU_LEVELS 14  // Numbers of levels onthe vu meter

const int sampleWindow = 100;  // Sample window width in mS (50 mS = 20Hz)
const uint8_t brightness = 100;
unsigned int maximum = 1024;
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
  FastLED.setBrightness(20);
  FastLED.show();
  delay(200);
}

void loop() { VU(); }
//
//int global_peak_to_peak = 0;
//
void displaySignalValue(int value, int max) {
  const int led_per_bar = NUM_LEDS / max;
  const int edge_led = value * led_per_bar;
  for (int i = 0; i <= NUM_LEDS; i++) {
     int color = map(i, 0, NUM_LEDS, 0, 255);
    if (i < edge_led) {
      leds[i] = Wheel(color);
    } else {
      leds[i] = CRGBW(0, 0, 0, 0);
    }
  }
}

void VU() {
  unsigned long startMillis = millis(); // Start of sample window
 
  // collect data for 50 mS
  int read_count = 0;
  double total_sample = 0;
  while (millis() - startMillis < sampleWindow) {
    total_sample += analogRead(ANALOG_INPUT_CHANNEL);
    read_count++;
  }
  sample = total_sample / read_count;
  
  float vu = 20*log10( sample*0.0049/1.2*2 );
  int vu_max = 0;
  int vu_min = -10;
  vu = max(vu_min, vu);
  vu = min(vu_max, vu);
  int led = map(vu, vu_min, vu_max, 0, VU_LEVELS);
  printValues(vu, 0, 0); //led*300/VU_LEVELS);
  displaySignalValue(led, VU_LEVELS);
  FastLED.setBrightness(20);                
  FastLED.show();
}

CRGBW Wheel(byte WheelPos) { return CRGBW(0, min(2*WheelPos, 255), 0, max(255 - 2*WheelPos, 0)); }
