#include "OctoSK6812.h"

#define NUM_LEDS 10
#define DATA_PIN 2

int led_number = 0;      
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];

void setup() {
  Serial.begin(9600);      // Initialize Serial Port
  Serial.setTimeout(2000000);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB,
                                          rgb_size_from_rgbw_size(NUM_LEDS));
  
  FastLED.show();
}
 
void loop() {
  Serial.println("LED? ");       
  while (Serial.available()==0){}             
  led_number = Serial.parseInt();   
  FastLED.setBrightness(led_number);                
  for (int i = 0; i <= NUM_LEDS; i++) {
    //if (i == led_number) {
      leds[i] = CRGBW(0, 0, 0, 255);
    //} else {
    // leds[i] = CRGBW(0, 0, 0, 0);
  }
  int milli  = millis();
  FastLED.show();
  int time  = millis() - milli;
  Serial.print("time:");
  Serial.println(time);
}
