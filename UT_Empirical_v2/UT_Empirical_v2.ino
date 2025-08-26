#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

// ----------------- Colors -----------------
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0

// ----------------- Pins -----------------
#define pin_output 51
#define pin_input_reference 41   // repurposed: reference capture switch
#define pin_input_sleep 31

// ----------------- Constants -----------------
#define MAX_SAMPLES       40     // Samples per scan
#define RUNNING_AVG_LEN   5       // Number of scans for averaging
#define MIN_ECHO_SAMPLE   17      // Ignore first few samples (ringing)

// ----------------- Globals -----------------
unsigned int values[MAX_SAMPLES];
unsigned int reference[MAX_SAMPLES] = {0};   // store transmit-only signal
bool referenceSet = false;

float thicknessBuffer[RUNNING_AVG_LEN] = {0};
int bufferIndex = 0;
int numFilled = 0;

void setup() {
  Serial.begin(115200);

  // Configure ADC (Arduino Due freerun mode)
  REG_ADC_MR = 0x10380080;
  ADC->ADC_CHER = 0x03;

  // Pin setup
  pinMode(pin_output, OUTPUT);
  pinMode(pin_input_reference, INPUT_PULLUP); // button to set reference
  pinMode(pin_input_sleep, INPUT);
  digitalWrite(pin_output, LOW);

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
  for (int j = 0; j < 18; j++) {

    // -------- Trigger pulse --------
    digitalWrite(pin_output, HIGH);
    digitalWrite(pin_output, LOW);

    // -------- Acquire samples --------
    for (int i = 0; i < MAX_SAMPLES; i++) {
      while ((ADC->ADC_ISR & 0x03) == 0); // Wait ADC ready
      values[i] = ADC->ADC_CDR[0];
    }

    // -------- Reference capture --------
    if (digitalRead(pin_input_reference) == LOW) {  // button pressed
      for (int i = 0; i < MAX_SAMPLES; i++) {
        reference[i] = values[i];
      }
      referenceSet = true;
      Serial.println("Reference (no object) captured.");
    }

    // -------- Reference subtraction --------
    if (referenceSet) {
      for (int i = 0; i < MAX_SAMPLES; i++) {
        int corrected = (int)values[i] - (int)reference[i];
        if (corrected < 0) corrected = 0;
        values[i] = corrected;
      }
    }

    // -------- Echo detection (first upward spike) --------
    int echoIndex = -1;
    for (int i = MIN_ECHO_SAMPLE; i < MAX_SAMPLES; i++) {
      int delta = values[i] - values[i - 1];
      if (delta > 10) { // upward slope
        echoIndex = i;
        break;
      }
    }

    // -------- Thickness calculation --------
    float thickness_mm = 0;
    if (echoIndex >= 0) {
      thickness_mm = 0.967 * echoIndex - 12.5;   // Empirical formula
      if (thickness_mm < 0) thickness_mm = 0;

      Serial.print("Scan ");
      Serial.print(j);
      Serial.print(": Echo at sample ");
      Serial.print(echoIndex);
      Serial.print(", thickness = ");
      Serial.print(thickness_mm, 2);
      Serial.println(" mm");
    } else {
      Serial.print("Scan ");
      Serial.print(j);
      Serial.println(": No echo detected.");
    }

    // -------- Running average --------
    thicknessBuffer[bufferIndex] = thickness_mm;
    bufferIndex = (bufferIndex + 1) % RUNNING_AVG_LEN;
    if (numFilled < RUNNING_AVG_LEN) numFilled++;

    float runningAvg = 0;
    for (int i = 0; i < numFilled; i++) runningAvg += thicknessBuffer[i];
    runningAvg /= numFilled;

    // -------- Display results --------
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(2);

    tft.setCursor(50, 200);
    tft.print("Echo S: ");
    tft.print(echoIndex);

    tft.setCursor(50, 225);
    tft.print("T: ");
    tft.print(thickness_mm, 2);
    tft.print(" mm");

    tft.setCursor(200, 250);
    tft.print("Avg: ");
    tft.print(runningAvg, 2);
    tft.print(" mm");

    // -------- Timing scale (right side) --------
    tft.setTextSize(1);
    tft.drawLine(440, 15, 440, 15 + MAX_SAMPLES, WHITE);
    for (int y = 15, t = 0; y <= 15 + MAX_SAMPLES; y += 20, t += 4) {
      tft.drawLine(440, y, 445, y, WHITE);
      tft.setCursor(450, y - 5);
      tft.print(t);
      tft.print(" us");
    }

    // -------- Grayscale scan column --------
    for (int i = 0; i < MAX_SAMPLES; i++) {
      uint8_t gray = map(values[i], 0, 4095, 0, 255);
      uint16_t color = tft.color565(gray, gray, gray);
      tft.fillRect(j * 24, 15 + i, 23, 1, color);
    }

    // -------- Sleep mode --------
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) { }
    }

    delay(1000); // Wait before next scan
  }
}
