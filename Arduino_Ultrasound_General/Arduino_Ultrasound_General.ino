#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

// Pin definitions
#define pin_output 51
#define pin_input_magnification 41
#define pin_input_sleep 31

unsigned long start_time;
unsigned long stop_time;
unsigned int values[600]; // ADC readings

int i, j;
int Trigger_time; // Trigger pulse duration

#define CENTRE 240

// Common 16-bit colors (RGB565)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0

void setup() {
  Serial.begin(115200);

  // ADC setup for Arduino Due
  REG_ADC_MR = 0x10380080; // freerun mode
  ADC->ADC_CHER = 0x03;    // enable ADC on pins A6/A7

  pinMode(pin_output, OUTPUT);
  pinMode(pin_input_magnification, INPUT_PULLUP);
  pinMode(pin_input_sleep, INPUT_PULLUP);

  digitalWrite(pin_output, LOW);

  Trigger_time = 1;

  // TFT init
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1); // landscape
  tft.fillScreen(BLACK);

  // Title bar
  tft.fillRect(0, 0, 480, 14, YELLOW);
  tft.setTextColor(BLACK);
  tft.setTextSize(1);
  tft.setCursor(150, 3);
  tft.print("Ege's Ultrasound - General");
}

void drawTimeScale(bool magnified) {
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(1);

  tft.drawFastVLine(440, 15, 300, WHITE);

  if (!magnified) {
    tft.drawFastHLine(440, 15, 5, WHITE);
    tft.drawFastHLine(440, 65, 5, WHITE);
    tft.drawFastHLine(440, 115, 5, WHITE);
    tft.drawFastHLine(440, 165, 5, WHITE);
    tft.drawFastHLine(440, 215, 5, WHITE);
    tft.drawFastHLine(440, 265, 5, WHITE);
    tft.drawFastHLine(440, 315, 5, WHITE);

    tft.setCursor(450, 10);  tft.print("0 us");
    tft.setCursor(450, 60);  tft.print("20");
    tft.setCursor(450, 110); tft.print("40");
    tft.setCursor(450, 160); tft.print("60");
    tft.setCursor(450, 210); tft.print("80");
    tft.setCursor(450, 260); tft.print("100");
    tft.setCursor(450, 310); tft.print("120");
  } else {
    tft.drawFastHLine(440, 15, 5, WHITE);
    tft.drawFastHLine(440, 65, 5, WHITE);
    tft.drawFastHLine(440, 115, 5, WHITE);
    tft.drawFastHLine(440, 165, 5, WHITE);
    tft.drawFastHLine(440, 215, 5, WHITE);
    tft.drawFastHLine(440, 265, 5, WHITE);
    tft.drawFastHLine(440, 315, 5, WHITE);

    tft.setCursor(450, 10);  tft.print("0 us");
    tft.setCursor(450, 60);  tft.print("40");
    tft.setCursor(450, 110); tft.print("80");
    tft.setCursor(450, 160); tft.print("120");
    tft.setCursor(450, 210); tft.print("160");
    tft.setCursor(450, 260); tft.print("200");
    tft.setCursor(450, 310); tft.print("240");
  }
}

void loop() {
  bool magnified = (digitalRead(pin_input_magnification) == HIGH);

  drawTimeScale(magnified);

  for (j = 0; j < 18; j++) {
    // Trigger pulse
    digitalWrite(pin_output, HIGH);
    digitalWrite(pin_output, LOW);

    // Read ADC
    if (!magnified) { // 300 values
      for (i = 0; i < 300; i++) {
        while ((ADC->ADC_ISR & 0x03) == 0);
        values[i] = ADC->ADC_CDR[0];
      }
    } else { // 600 values
      for (i = 0; i < 600; i++) {
        while ((ADC->ADC_ISR & 0x03) == 0);
        values[i] = ADC->ADC_CDR[0];
      }
    }

    // Draw scan line
    for (i = 0; i < 300; i++) {
      uint8_t val;
      if (!magnified) {
        val = map(values[i], 0, 4095, 0, 255);
      } else {
        val = map(values[2 * i], 0, 4095, 0, 255);
      }
      uint16_t color = tft.color565(val, val, val);
      tft.fillRect(j * 24, 15 + i, 23, 1, color);
    }

    // Pause if sleep switch is pressed
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) {
        // Wait in sleep mode
      }
    }

    delay(1000);
  }
}
