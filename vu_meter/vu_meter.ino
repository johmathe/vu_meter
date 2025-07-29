#include <Arduino.h>
#include <OctoWS2811.h>
#include <math.h>

// ──────────────────────────────────────────────
// CONFIGURATION
// ──────────────────────────────────────────────
#define ANALOG_INPUT_CHANNEL A3
#define VU_LEVELS            14          // number of bars
#define RED                  0x160000    // (unused but kept)

const int ledsPerStrip = 600;            // LEDs per physical strip
DMAMEM  int displayMemory[ledsPerStrip * 8];
         int drawingMemory[ledsPerStrip * 8];
const int config       = WS2811_GRBW | WS2811_800kHz;
const int led_per_bar  = ledsPerStrip / VU_LEVELS;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

const int  sampleWindow = 80;                          // ms per measurement
const int  blank_size   = max((int)round(led_per_bar * 0.10f), 1); // gap width

// ─── Silence detection thresholds ───────────────────────────────
const float SILENCE_PEAK_DB   = -35.0f;  // nothing louder than this ⇒ silence
const float SILENCE_RANGE_DB  =   3.0f;  // and dynamic range below this

// ──────────────────────────────────────────────
// ROLLING MIN / MAX (1‑minute sliding window)
// ──────────────────────────────────────────────
const unsigned long WINDOW_MS       = 60UL * 1000UL;                  // 60 s
const unsigned int  SAMPLES_PER_WIN = WINDOW_MS / sampleWindow;       // ~750

float   vu_history[SAMPLES_PER_WIN];  // circular buffer
uint16_t vu_head  = 0;                // next slot to overwrite
uint16_t vu_count = 0;                // items in buffer (≤ SAMPLES_PER_WIN)

inline void pushVu(float vu) {
  vu_history[vu_head] = vu;
  vu_head = (vu_head + 1) % SAMPLES_PER_WIN;
  if (vu_count < SAMPLES_PER_WIN) vu_count++;
}

inline void currentMinMax(float* pmin, float* pmax) {
  if (vu_count == 0) { *pmin = -20.0f; *pmax = 0.0f; return; }
  float mn = vu_history[0], mx = vu_history[0];
  for (uint16_t i = 1; i < vu_count; ++i) {
    float v = vu_history[i];
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }
  *pmin = mn; *pmax = mx;
}

// ──────────────────────────────────────────────
// LED HELPERS
// ──────────────────────────────────────────────
void setPixel(int i, byte r, byte g, byte b, byte w) {
  const int strip_id = 2;                          // adjust for your layout
  leds.setPixel(i + strip_id * ledsPerStrip, r, g, b, w);
}

void displaySignalValue(int level, int max_value) {
  const int edge_led = level * led_per_bar;

  // draw bar
  for (int i = 0; i < ledsPerStrip; ++i) {
    int color = map(i, 0, ledsPerStrip - 1, 0, 255);
    if (i < edge_led) {
      setPixel(i,
               min(2 * color, 0xff),
               0x00,
               0x00,
               max(0xff - (int)(1.5f * color), 0));
    } else {
      setPixel(i, 0, 0, 0, 0);
    }
  }

  // blank separators between bars
  for (int i = 0; i < max_value; ++i) {
    for (int j = 0; j < blank_size; ++j) {
      int led_id = led_per_bar * i + j;
      if (led_id < ledsPerStrip) setPixel(led_id, 0, 0, 0, 0);
    }
  }

  leds.show();
}

// ──────────────────────────────────────────────
// MAIN VU LOOP
// ──────────────────────────────────────────────
void VU() {
  unsigned long startMillis = millis();
  int    read_count   = 0;
  double total_sample = 0.0;
  int    last_raw     = 0;

  // acquire samples for `sampleWindow` ms
  while (millis() - startMillis < sampleWindow) {
    last_raw     = analogRead(ANALOG_INPUT_CHANNEL) - 512;  // centre mid‑rail
    total_sample += abs(last_raw);
    read_count++;
  }

  float sample = (read_count > 0) ? (total_sample / read_count) : 0.0f;
  float vu     = 20.0f * log10(sample + 1e-3f);             // avoid log10(0)

  pushVu(vu);                     // update rolling window

  float vu_min, vu_max;
  currentMinMax(&vu_min, &vu_max);
  if (fabs(vu_max - vu_min) < 0.1f) vu_max = vu_min + 0.1f; // guard

  // ─── Silence detection ────────────────────────────────────────
  bool silence = (vu_max < SILENCE_PEAK_DB) ||
                 ((vu_max - vu_min) < SILENCE_RANGE_DB);

  int led_level = silence ? VU_LEVELS
                          : constrain(map(vu, vu_min, vu_max, 0, VU_LEVELS),
                                      0, VU_LEVELS);

  // Serial Plotter output  (raw, min, max, bar, silence flag)
  // Serial.print(last_raw); Serial.print(',');
  // Serial.print(vu_min);   Serial.print(',');
  // Serial.print(vu_max);   Serial.print(',');
  // Serial.print(led_level);Serial.print(',');
  // Serial.println(silence);

  displaySignalValue(led_level, VU_LEVELS);
}

// ──────────────────────────────────────────────
// ARDUINO SETUP / LOOP
// ──────────────────────────────────────────────
void setup() {
  delay(200);
  leds.begin();
  leds.show();
  Serial.begin(115200);
  delay(200);
}

void loop() {
  VU();
}

// Optional hue helper
unsigned int Wheel(byte WheelPos) {
  return ((0xff - WheelPos) << 16) + WheelPos;
}
