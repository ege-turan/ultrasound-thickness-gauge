#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

// ----------------- Colors -----------------
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0

// ----------------- Pins -----------------
#define pin_output 51
#define pin_input_magnification 41
#define pin_input_sleep 31

// ----------------- Constants for Thickness Calculation -----------------
#define SPEED_OF_SOUND_MM_PER_US 7.684   // 6061 Aluminum speed of sound in mm/us
#define SAMPLE_PERIOD_US 0.23             // ADC sample period in microseconds
#define CONSTANT_DELAY_US 2.7           // Additional constant delay to subtract
#define MAX_SAMPLES 100                  // Limit to first 100 samples per scan
#define RUNNING_AVG_LENGTH 5             // Number of scans to average
#define MIN_ECHO_SAMPLE 5

// ----------------- Globals -----------------
unsigned int values[MAX_SAMPLES];
int Trigger_time;
int i, j;

// Running average buffer
float thicknessBuffer[RUNNING_AVG_LENGTH] = {0};
int bufferIndex = 0;
int numFilled = 0; // number of valid entries in buffer

void setup() {
  Serial.begin(115200);

  // Configure ADC for Arduino Due in free-run mode
  REG_ADC_MR = 0x10380080;  // freerun, prescaler
  ADC->ADC_CHER = 0x03;     // enable ADC channels 0 and 1 (adjust if needed)

  // Pin setup
  pinMode(pin_output, OUTPUT);
  pinMode(pin_input_magnification, INPUT);
  pinMode(pin_input_sleep, INPUT);
  digitalWrite(pin_output, LOW);
  Trigger_time = 1;

  // TFT setup
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(BLACK, YELLOW);
  tft.fillRect(0, 0, 432, 13, YELLOW);
  tft.setCursor(120, 1);
  tft.setTextSize(1);
  tft.print("UT - DEBUG");

  Serial.println("System Ready.");
}

void loop() {
  for (j = 0; j < 18; j++) {
    // ----------------- Trigger ultrasonic pulse -----------------
    digitalWrite(pin_output, HIGH);
    digitalWrite(pin_output, LOW);

    // ----------------- Acquire ADC samples -----------------
    for (i = 0; i < MAX_SAMPLES; i++) {
      while ((ADC->ADC_ISR & 0x03) == 0);   // Wait for ADC ready
      values[i] = ADC->ADC_CDR[0];          // Read ADC value
    }

    // ----------------- Output for Serial Plotter -----------------
    // Serial.print("Scan ");
    // Serial.print(j);
    // Serial.println(" ADC values:");
    // for (i = 0; i < MAX_SAMPLES; i++) {
    //   Serial.print(i);        // Sample number
    //   Serial.print(" ");
    //   Serial.println(values[i]); // ADC value
    // }
    // Serial.println("-----");

    // ----------------- Echo detection (second upward spike) -----------------
    int echoIndex = -1;
    int spikeCount = 0;

    for (i = MIN_ECHO_SAMPLE; i < MAX_SAMPLES; i++) {
      int delta = values[i] - values[i - 1];

      if (delta > 0) { // only upward spikes
        spikeCount++;
        if (spikeCount == 1) { // spike num
          echoIndex = i;
          break;
        }
      }
    }

    // ----------------- Thickness calculation -----------------
    float thickness_mm = 0;
    float correctedTime = 0;
    if (echoIndex >= 0) {
      float echoTime = echoIndex * SAMPLE_PERIOD_US;         // Raw time (us)
      correctedTime = echoTime - CONSTANT_DELAY_US;          // Apply delay correction
      if (correctedTime < 0) correctedTime = 0;             // Avoid negative time

      thickness_mm = (correctedTime * SPEED_OF_SOUND_MM_PER_US) / 2.0;

      Serial.print("Scan ");
      Serial.print(j);
      Serial.print(": Echo at sample ");
      Serial.print(echoIndex);
      Serial.print(", time = ");
      Serial.print(correctedTime);
      Serial.print(" us, thickness = ");
      Serial.print(thickness_mm, 2);
      Serial.println(" mm");

    } else {
      Serial.print("Scan ");
      Serial.print(j);
      Serial.println(": No echo detected.");
    }

    // ----------------- Update running average -----------------
    thicknessBuffer[bufferIndex] = thickness_mm;
    bufferIndex = (bufferIndex + 1) % RUNNING_AVG_LENGTH;
    if (numFilled < RUNNING_AVG_LENGTH) numFilled++;

    float runningAvg = 0;
    for (i = 0; i < numFilled; i++) {
      runningAvg += thicknessBuffer[i];
    }
    runningAvg /= numFilled;

    // ----------------- Display thickness, echo sample/time, and running average on TFT -----------------
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(2);
    tft.setCursor(50, 200);
    tft.print("Echo S: ");
    tft.print(echoIndex);
    tft.setCursor(50, 225);
    tft.print("Time: ");
    tft.print(correctedTime, 2);
    tft.print(" us");

    tft.setCursor(50, 250);
    tft.print("T: ");
    tft.print(thickness_mm, 2);
    tft.print(" mm");

    tft.setCursor(200, 250);
    tft.print("Avg: ");
    tft.print(runningAvg, 2);
    tft.print(" mm");

    // ----------------- Draw timing scale (right side) -----------------
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(1);
    tft.drawLine(440, 15, 440, 15 + MAX_SAMPLES, WHITE);
    for (int y = 15, t = 0; y <= 15 + MAX_SAMPLES; y += 20, t += 4) {
      tft.drawLine(440, y, 445, y, WHITE);
      tft.setCursor(450, y - 5);
      tft.print(t);
      tft.print(" us");
    }

    // ----------------- Draw grayscale scan column -----------------
    for (i = 0; i < MAX_SAMPLES; i++) {
      uint8_t gray = map(values[i], 0, 4095, 0, 255);
      uint16_t color = tft.color565(gray, gray, gray);
      tft.fillRect(j * 24, 15 + i, 23, 1, color);
    }

    // ----------------- Sleep mode check -----------------
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) {
        // Pause until released
      }
    }

    delay(1000); // Wait before next scan
  }
}
