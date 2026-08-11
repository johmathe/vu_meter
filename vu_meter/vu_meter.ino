#include <Arduino.h>
#include <OctoWS2811.h>
#include <math.h>

// ──────────────────────────────────────────────
// CONFIGURATION
// ──────────────────────────────────────────────
#define ANALOG_INPUT_CHANNEL A3
#define VU_LEVELS            14          // number of bars
#define STRIP_ID             2           // OctoWS2811 output driving the meter

const int ledsPerStrip = 600;            // LEDs per physical strip
DMAMEM  int displayMemory[ledsPerStrip * 8];
         int drawingMemory[ledsPerStrip * 8];
const int config       = WS2811_GRBW | WS2811_800kHz;
const int led_per_bar  = ledsPerStrip / VU_LEVELS;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

const int  sampleWindow = 80;                          // ms per measurement
const int  blank_size   = max((int)round(led_per_bar * 0.10f), 1); // gap width

// ─── ADC / signal conditioning ──────────────────────────────────
const int   ADC_RESOLUTION_BITS = 12;
const float DC_EMA_ALPHA        = 0.0005f; // per-sample; sub-Hz cutoff, tracks bias drift
const float VU_FLOOR            = 1.0f;    // counts; keeps log10 defined, vu >= 0 dB

// ─── Silence detection thresholds ───────────────────────────────
const float SILENCE_PEAK_DB   = 15.0f;   // nothing louder than this ⇒ silence
                                         // (12-bit scale; tune to your noise floor)
const float SILENCE_RANGE_DB  =  3.0f;   // and dynamic range below this

// ─── Auto-scaling (decaying min/max envelopes) ──────────────────
const float ENVELOPE_DECAY_DB = 0.05f;   // per frame (~0.6 dB/s at 12.5 fps)

// ─── Meter ballistics ───────────────────────────────────────────
const float ATTACK_ALPHA  = 0.60f;   // fast rise
const float RELEASE_ALPHA = 0.10f;   // slow fall
const float PEAK_FALL     = 0.15f;   // peak marker fall rate, bar levels per frame

// ──────────────────────────────────────────────
// LED HELPERS
// ──────────────────────────────────────────────
void setPixel(int i, byte r, byte g, byte b, byte w) {
  leds.setPixel(i + STRIP_ID * ledsPerStrip, r, g, b, w);
}

// level is in bar units (0..VU_LEVELS, fractional); peak_led < 0 hides the marker
void displaySignalValue(float level, int peak_led) {
  const int edge_led = constrain((int)lroundf(level * led_per_bar), 0, ledsPerStrip);

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
  for (int i = 0; i < VU_LEVELS; ++i) {
    for (int j = 0; j < blank_size; ++j) {
      int led_id = led_per_bar * i + j;
      if (led_id < ledsPerStrip) setPixel(led_id, 0, 0, 0, 0);
    }
  }

  // falling peak-hold marker (white dot above the bar)
  if (peak_led >= edge_led && peak_led >= 0 && peak_led < ledsPerStrip) {
    setPixel(peak_led, 0, 0, 0, 0xff);
  }

  leds.show();
}

// ──────────────────────────────────────────────
// MAIN VU LOOP
// ──────────────────────────────────────────────
void VU() {
  static float dc_offset    = 2048.0f;  // EMA of raw ADC; adapts to mid-rail bias
  static float vu_min       = NAN;      // decaying envelopes, seeded on 1st frame
  static float vu_max       = NAN;
  static float level_smooth = 0.0f;     // displayed bar level (0..VU_LEVELS)
  static float peak_level   = 0.0f;     // falling peak-hold marker

  unsigned long startMillis  = millis();
  int           read_count   = 0;
  double        total_sample = 0.0;

  // acquire samples for `sampleWindow` ms
  while (millis() - startMillis < sampleWindow) {
    int raw = analogRead(ANALOG_INPUT_CHANNEL);
    dc_offset    += DC_EMA_ALPHA * (raw - dc_offset);
    total_sample += fabsf(raw - dc_offset);
    read_count++;
  }

  float sample = (read_count > 0) ? (float)(total_sample / read_count) : 0.0f;
  float vu     = 20.0f * log10f(max(sample, VU_FLOOR));

  // ─── Auto-scaling: decaying min/max envelopes ─────────────────
  if (isnan(vu_min)) { vu_min = vu; vu_max = vu; }
  vu_max = max(vu, vu_max - ENVELOPE_DECAY_DB);
  vu_min = min(vu, vu_min + ENVELOPE_DECAY_DB);

  // ─── Silence detection ────────────────────────────────────────
  bool silence = (vu_max < SILENCE_PEAK_DB) ||
                 ((vu_max - vu_min) < SILENCE_RANGE_DB);

  if (silence) {
    // ambient mode: full bar, no peak marker
    displaySignalValue((float)VU_LEVELS, -1);
    return;
  }

  // ─── Map to bar level (float), then apply ballistics ──────────
  float span   = max(vu_max - vu_min, 0.1f);
  float target = constrain((vu - vu_min) / span, 0.0f, 1.0f) * VU_LEVELS;

  float alpha = (target > level_smooth) ? ATTACK_ALPHA : RELEASE_ALPHA;
  level_smooth += alpha * (target - level_smooth);

  peak_level = max(level_smooth, peak_level - PEAK_FALL);

  displaySignalValue(level_smooth, (int)lroundf(peak_level * led_per_bar));
}

// ──────────────────────────────────────────────
// ARDUINO SETUP / LOOP
// ──────────────────────────────────────────────
void setup() {
  delay(200);
  analogReadResolution(ADC_RESOLUTION_BITS);
  analogReadAveraging(4);
  leds.begin();
  leds.show();
  Serial.begin(115200);
  delay(200);
}

void loop() {
  VU();
}
