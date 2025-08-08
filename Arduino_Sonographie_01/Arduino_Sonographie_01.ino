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

// ----------------- Globals -----------------
unsigned int values[600];
int Trigger_time;
int i, j;

void setup() {
  Serial.begin(115200);

  // ADC config for Due
  REG_ADC_MR = 0x10380080;  // freerun, prescaler
  ADC->ADC_CHER = 0x03;     // enable ADC on A7

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
  tft.print("Sonographie - stoppi");
}

void loop() {
  for (j = 0; j < 18; j++) {
    // Trigger pulse
    digitalWrite(pin_output, HIGH);
    digitalWrite(pin_output, LOW);

    // Sample data
    if (digitalRead(pin_input_magnification) == LOW) { // normal mode
      for (i = 0; i < 300; i++) {
        while ((ADC->ADC_ISR & 0x03) == 0);
        values[i] = ADC->ADC_CDR[0];
      }
    } else { // magnified mode
      for (i = 0; i < 600; i++) {
        while ((ADC->ADC_ISR & 0x03) == 0);
        values[i] = ADC->ADC_CDR[0];
      }
    }

    // Draw timing scale (right side)
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(1);
    tft.drawLine(440, 15, 440, 15 + 300, WHITE);
    for (int y = 15, t = 0; y <= 315; y += 50, t += 20) {
      tft.drawLine(440, y, 445, y, WHITE);
      tft.setCursor(450, y - 5);
      tft.print(t);
      tft.print(" us");
    }

    // Draw scan column
    if (digitalRead(pin_input_magnification) == LOW) { // normal
      for (i = 0; i < 300; i++) {
        uint8_t gray = map(values[i], 0, 4095, 0, 255);
        uint16_t color = tft.color565(gray, gray, gray);
        tft.fillRect(j * 24, 15 + i, 23, 1, color);
      }
    } else { // magnified
      for (i = 0; i < 300; i++) {
        uint8_t gray = map(values[2 * i], 0, 4095, 0, 255);
        uint16_t color = tft.color565(gray, gray, gray);
        tft.fillRect(j * 24, 15 + i, 23, 1, color);
      }
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
