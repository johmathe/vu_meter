

#define PEAK_FALL 10 // Rate of peak falling dot
#define COLOR_FROM 0
#define COLOR_TO 255
#define ANALOG_INPUT_CHANNEL A0
#define NUM_LEDS 300
#define DATA_PIN 2
#define VU_LEVELS 14  // Numbers of levels onthe vu meter

const int sampleWindow = 20;  // Sample window width in mS (50 mS = 20Hz)
const uint8_t brightness = 100;
unsigned int maximum = 1024;
int peak;
int dotCount;
unsigned int sample;



void printValues(float v1, float v2, float v3) {
  Serial.println(v1);
  //Serial.print(",");
 // Serial.println(v2);
//  Serial.print(",");
//  Serial.println(v3);
}

void setup() {
  Serial.begin(115200);
  pinMode(ANALOG_INPUT_CHANNEL, INPUT);
}

void loop() { VU(); }

int global_peak_to_peak = 0;

void VU() {
  unsigned long startMillis = millis(); // Start of sample window
  unsigned int peakToPeak = 0;          // peak-to-peak level
  unsigned int signalMax = 0;
  unsigned int signalMin = maximum;

  int read_count = 0;
  double total_sample = 0;
  while (millis() - startMillis < sampleWindow) {
    total_sample += analogRead(ANALOG_INPUT_CHANNEL);
    read_count++;
  }
  sample = total_sample / read_count;
  peakToPeak = signalMax - signalMin;
  float vu = 20*log10( sample*0.0049/1.2*1 );
  
  // global_peak_to_peak = ALPHA * peakToPeak + (1 - ALPHA) * global_peak_to_peak;
  vu = max(-20, vu);
  vu = min(3, vu);
  int led = map(vu, -20, 3, 0, VU_LEVELS);
  printValues(vu, peakToPeak, 0); //led*300/VU_LEVELS);
  
}
