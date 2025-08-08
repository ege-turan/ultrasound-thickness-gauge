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

// ----------------- Constants -----------------
#define SPEED_OF_SOUND_MM_PER_US 5.9     // Speed of sound in steel (mm/us)
#define ECHO_DELTA_THRESHOLD 3         // ADC delta threshold for echo detection
#define SAMPLE_PERIOD_US 1                // ADC sample period in microseconds
#define MAX_SAMPLES 20                   // Number of samples per scan to cover 50 mm max

// ----------------- Globals -----------------
unsigned int values[MAX_SAMPLES];
int Trigger_time;
int i, j;

void setup() {
  Serial.begin(115200);

  // ADC config for Arduino Due
  REG_ADC_MR = 0x10380080;  // freerun mode, prescaler
  ADC->ADC_CHER = 0x03;     // enable ADC channels 0 and 1 (adjust if needed)

  pinMode(pin_output, OUTPUT);
  pinMode(pin_input_magnification, INPUT);
  pinMode(pin_input_sleep, INPUT);

  digitalWrite(pin_output, LOW);
  Trigger_time = 1;

  // TFT init
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(BLACK, YELLOW);
  tft.fillRect(0, 0, 432, 13, YELLOW);
  tft.setCursor(120, 1);
  tft.setTextSize(1);
  tft.print("UT - Ege V6 thin");

  Serial.println("System Ready.");
}

void loop() {
  for (j = 0; j < 18; j++) {
    // Trigger pulse
    digitalWrite(pin_output, HIGH);
    digitalWrite(pin_output, LOW);

    // Collect samples
    for (i = 0; i < MAX_SAMPLES; i++) {
      while ((ADC->ADC_ISR & 0x03) == 0);  // wait for ADC conversion ready
      values[i] = ADC->ADC_CDR[0];
    }

    // Debug print ADC values
    Serial.print("Scan ");
    Serial.print(j);
    Serial.println(" ADC values:");
    for (i = 0; i < MAX_SAMPLES; i++) {
      Serial.println(values[i]);
    }
    Serial.println("-----");

    // Find minimum ADC value and index
    int minValue = 4096;
    int minIndex = 0;
    for (i = 0; i < MAX_SAMPLES; i++) {
      if (values[i] < minValue) {
        minValue = values[i];
        minIndex = i;
      }
    }

    // Detect echo after minimum index
    int echoIndex = -1;
    for (i = minIndex; i < MAX_SAMPLES; i++) {
      if (values[i] > minValue + ECHO_DELTA_THRESHOLD) {
        echoIndex = i;
        break;
      }
    }

    // Calculate thickness and print
    if (echoIndex >= 0) {
      unsigned long echoTime = echoIndex * SAMPLE_PERIOD_US;
      float thickness_mm = (echoTime * SPEED_OF_SOUND_MM_PER_US) / 2.0;

      Serial.print("Scan ");
      Serial.print(j);
      Serial.print(": Echo at sample ");
      Serial.print(echoIndex);
      Serial.print(", time = ");
      Serial.print(echoTime);
      Serial.print(" us, thickness = ");
      Serial.print(thickness_mm, 2);
      Serial.println(" mm");
    } else {
      Serial.print("Scan ");
      Serial.print(j);
      Serial.println(": No echo detected.");
    }

    // Draw timing scale (right side)
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(1);
    tft.drawLine(440, 15, 440, 15 + MAX_SAMPLES, WHITE);
    for (int y = 15, t = 0; y <= 15 + MAX_SAMPLES; y += 5, t += 5) {
      tft.drawLine(440, y, 445, y, WHITE);
      tft.setCursor(450, y - 5);
      tft.print(t);
      tft.print(" us");
    }

    // Draw grayscale scan column
    for (i = 0; i < MAX_SAMPLES; i++) {
      uint8_t gray = map(values[i], 0, 4095, 0, 255);
      uint16_t color = tft.color565(gray, gray, gray);
      tft.fillRect(j * 24, 15 + i, 23, 1, color);
    }

    // Sleep mode check
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) {
        // paused
      }
    }

    delay(1000);
  }
}
