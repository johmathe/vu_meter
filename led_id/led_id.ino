#include <FastLED.h>
#include "FastLED_RGBW.h"

#define NUM_LEDS 400
#define DATA_PIN 2

int led_number = 0;      
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];

void setup() {
  Serial.begin(9600);      // Initialize Serial Port
  Serial.setTimeout(2000000);
    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB,
                                            rgb_size_from_rgbw_size(NUM_LEDS));
  FastLED.setBrightness(255);
  FastLED.show();
}
 
void loop() {
  Serial.println("LED? ");       
  while (Serial.available()==0){}             
  led_number = Serial.parseInt();                   
  for (int i = 0; i <= NUM_LEDS; i++) {
    if (i == led_number) {
      leds[i] = CRGBW(255, 255, 255, 255);
    } else {
      leds[i] = CRGBW(0, 0, 0, 0);
    }
  }

  FastLED.show();
}
