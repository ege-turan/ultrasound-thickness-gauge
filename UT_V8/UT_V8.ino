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
#define SPEED_OF_SOUND_MM_PER_US 6.224    // 6061 Aluminum
#define CONSTANT_DELAY_US 6.93            // System delay to subtract
#define ECHO_THRESHOLD 4000               // ADC threshold to detect echo
#define SAMPLE_PERIOD_US 0.2              // ADC sample period in microseconds

// ----------------- Globals -----------------
unsigned int values[600];
int Trigger_time;
int i, j;

void setup() {
  Serial.begin(115200);

  // ADC config for Due
  REG_ADC_MR = 0x10380080;  // freerun, prescaler
  ADC->ADC_CHER = 0x03;     // enable ADC on channels 0 and 1

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
  tft.print("UT - Ege V8");

  Serial.println("System Ready.");
}

void loop() {
  for (j = 0; j < 18; j++) {
    // Trigger pulse
    digitalWrite(pin_output, HIGH);
    digitalWrite(pin_output, LOW);

    // Sample data
    int numSamples = (digitalRead(pin_input_magnification) == LOW) ? 300 : 600;
    for (i = 0; i < numSamples; i++) {
      while ((ADC->ADC_ISR & 0x03) == 0);
      values[i] = ADC->ADC_CDR[0];
    }

    // --- Serial Plotter-friendly output ---
    Serial.print("Scan ");
    Serial.print(j);
    Serial.println(" ADC values:");
    for (i = 0; i < numSamples; i++) {
      Serial.print(i);       // sample number
      Serial.print(" ");
      Serial.println(values[i]);
    }
    Serial.println("-----");

    // Find echo index by threshold starting only after constant delay samples
    int echoIndex = -1;
    int minSearchIndex = 10;

    for (i = minSearchIndex; i < numSamples; i++) {
      if (values[i] > ECHO_THRESHOLD) {
        echoIndex = i;
        break;
      }
    }

    if (echoIndex >= 0) {
      unsigned long echoTime = echoIndex * SAMPLE_PERIOD_US;
      float correctedTime = echoTime - CONSTANT_DELAY_US;
      if (correctedTime < 0) correctedTime = 0;

      float thickness_mm = (correctedTime * SPEED_OF_SOUND_MM_PER_US) / 2.0;

      Serial.print("Scan ");
      Serial.print(j);
      Serial.print(": Echo at sample ");
      Serial.print(echoIndex);
      Serial.print(", time = ");
      Serial.print(echoTime);
      Serial.print(" us, corrected = ");
      Serial.print(correctedTime);
      Serial.print(" us, thickness = ");
      Serial.print(thickness_mm, 2);
      Serial.println(" mm");

      // Display thickness higher so it's not cut off
      tft.fillRect(0, 295, 432, 24, YELLOW);
      tft.setTextColor(BLACK, YELLOW);
      tft.setTextSize(2);
      tft.setCursor(10, 297);
      tft.print("Thickness: ");
      tft.print(thickness_mm, 2);
      tft.print(" mm");
    } else {
      Serial.print("Scan ");
      Serial.print(j);
      Serial.println(": No echo detected.");

      tft.fillRect(0, 295, 432, 24, YELLOW);
      tft.setTextColor(BLACK, YELLOW);
      tft.setTextSize(2);
      tft.setCursor(10, 297);
      tft.print("No Echo");
    }

    // Draw timing scale
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(1);
    tft.drawLine(440, 15, 440, 15 + 300, WHITE);
    for (int y = 15, t = 0; y <= 315; y += 50, t += 20) {
      tft.drawLine(440, y, 445, y, WHITE);
      tft.setCursor(450, y - 5);
      tft.print(t);
      tft.print(" us");
    }

    // Draw scan grayscale
    if (digitalRead(pin_input_magnification) == LOW) {
      for (i = 0; i < 300; i++) {
        uint8_t gray = map(values[i], 0, 4095, 0, 255);
        uint16_t color = tft.color565(gray, gray, gray);
        tft.fillRect(j * 24, 15 + i, 23, 1, color);
      }
    } else {
      for (i = 0; i < 300; i++) {
        uint8_t gray = map(values[2 * i], 0, 4095, 0, 255);
        uint16_t color = tft.color565(gray, gray, gray);
        tft.fillRect(j * 24, 15 + i, 23, 1, color);
      }
    }

    // Sleep mode check
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) {}
    }

    delay(1000);
  }
}
