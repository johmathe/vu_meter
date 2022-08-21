#include <OctoWS2811.h>

#define COLOR_FROM 0
#define COLOR_TO 255
#define ANALOG_INPUT_CHANNEL A3
#define VU_LEVELS 14  // Numbers of levels onthe vu meter
#define RED    0x160000

const int ledsPerStrip = 600;
DMAMEM int displayMemory[ledsPerStrip*8]; // 8 ints is 32 bytes
int drawingMemory[ledsPerStrip*8]; // RGBW needs 32 bytes, RGB needs only 24
const int config = WS2811_GRBW | WS2811_800kHz;
const int led_per_bar = ledsPerStrip / VU_LEVELS;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);


const int sampleWindow = 70;  // Sample window width in mS (50 mS = 20Hz)
const int blank_size = max(floor(led_per_bar * 0.20), 1);
unsigned long int learning_window_ms = 1000; 
const unsigned long int max_learning_window_ms = 10 * 60 * 1000;
const float default_min = -20;
const float default_max = 0;
unsigned int sample;

const int delta_t_max_min_ms = 10 * 60 * 1000;

void printValues(float v1, float v2, float v3) {
  Serial.print(v1);
  Serial.print(",");
  Serial.print(v2);
  Serial.print(",");
  Serial.println(v3);
}

void setup() {
  delay(200);
  leds.begin();
  leds.show();
  Serial.begin(115200);
  delay(200);
}

void loop() { VU(); }

void setPixel(int i, byte r, byte g, byte b, byte w) {
  int strip_id = 2;
  leds.setPixel(i+strip_id*ledsPerStrip, r, g, b, w) ;
}
void displaySignalValue(int value, int max_value) {
  
  const int edge_led = value * led_per_bar;
  for (int i = 0; i <= ledsPerStrip; i++) {
     int color = map(i, 0, ledsPerStrip, 0, 255);
    if (i < edge_led) {
      setPixel(i, min(2*color, 0xff), 0x00, 0x00, max(0xff-1.5*color, 0)) ;
    } else {
      setPixel(i, 0, 0, 0, 0) ;
    }
  }
  // little blank spaces
  for (int i = 0; i < max_value; ++i) {
    for (int j = 0; j < blank_size; ++j) {
      int led_id = led_per_bar*i + j;
      setPixel(led_id, 0, 0, 0, 0);
    }
  }
  leds.show();
}

// Stateful!
void get_history_min_max(float sample, float* min_output, float* max_output) {
  static float previous_min = default_min;
  static float previous_max = default_max;
  static float max_over_dt = default_min;
  static float min_over_dt = default_max;
  static unsigned long int time_since_reset = 0;
  unsigned long int current_time = millis();
  if (current_time > (time_since_reset + learning_window_ms)) {
    previous_max = max_over_dt;
    previous_min = min_over_dt;
    time_since_reset = current_time;
    max_over_dt = default_min;
    min_over_dt = default_max;
    learning_window_ms = min(max_learning_window_ms, learning_window_ms*2);
  }
  if (sample >= max_over_dt) {
    max_over_dt = sample;
  }
  if (sample <= min_over_dt) {
    min_over_dt = sample;
  }
  *min_output = previous_min;
  *max_output = previous_max;
}

void VU() {
  unsigned long startMillis = millis();
  int read_count = 0;
  double total_sample = 0;
  while (millis() - startMillis < sampleWindow) {
    total_sample += analogRead(ANALOG_INPUT_CHANNEL);
    read_count++;
  }
  sample = total_sample / read_count;

  
  
  float vu = 20*log10( sample*0.0049/1.2 );
  float vu_min = 0;
  float vu_max = 0;
  get_history_min_max(vu, &vu_min, &vu_max);
  vu_min = -17;
//  Serial.print(vu);
//  Serial.print(",");
//  Serial.print(vu_min);
//  Serial.print(",");
//  Serial.print(vu_max);
//  Serial.println(",");
  
  int led = map(vu, vu_min, vu_max, 0, VU_LEVELS);
  displaySignalValue(led, VU_LEVELS);   
}

unsigned int Wheel(byte WheelPos) { return ((0xff - WheelPos) << 16) + WheelPos; }
