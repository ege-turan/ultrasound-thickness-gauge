#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

#define DEBUG_MODE true  // set true to print raw samples

// ----------------- Colors -----------------
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0

// ----------------- Pins -----------------
#define pin_output 51
#define pin_input_reference 41   // reference capture switch
#define pin_input_sleep 31

// ----------------- Constants -----------------
#define MAX_SAMPLES       50     // Samples per scan
#define RUNNING_AVG_LEN   5       // Number of scans for averaging
#define MIN_ECHO_SAMPLE   16      // Ignore first few samples (ringing)

// ----------------- Material Calibration -----------------
struct MaterialCal {
  float slope;
  float intercept;
  const char* name;
};

// Add materials here:
MaterialCal materials[] = {
  {1.45, -23.6, "Al 6061"},   // Al 6061 8/25/25 measured
  {1.30, -20.0, "Steel"},     // Steel 8/25/25 guess
  {1.10, -15.0, "Acrylic"}    // Acrylic 8/25/25 guess
};

int currentMaterial = 0; // index into materials[]

// ----------------- Globals -----------------
unsigned int values[MAX_SAMPLES];
unsigned int reference[MAX_SAMPLES] = {0};
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
  pinMode(pin_input_reference, INPUT_PULLUP); // reference button
  pinMode(pin_input_sleep, INPUT);
  digitalWrite(pin_output, LOW);

  // TFT setup
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  tft.setTextColor(BLACK, YELLOW);
  tft.fillRect(0, 0, 432, 13, YELLOW);
  tft.setCursor(80, 1);
  tft.setTextSize(1);
  tft.print("UT - DEBUG (");
  tft.print(materials[currentMaterial].name);
  tft.print(")");

  Serial.print("System Ready. Material: ");
  Serial.println(materials[currentMaterial].name);
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

    // -------- Debug: print raw samples --------
    if (DEBUG_MODE) {
      Serial.print("Scan ");
      Serial.print(j);
      Serial.println(" ADC values:");
      for (int i = 0; i < MAX_SAMPLES; i++) {
        Serial.print(i);       // sample number
        Serial.print(" ");
        Serial.println(values[i]); // ADC value
      }
      Serial.println("-----");
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
      if (delta > 0) { // upward slope
        echoIndex = i;
        break;
      }
    }

    // -------- Thickness calculation --------
    float thickness_mm = 0;
    if (echoIndex >= 0) {
      MaterialCal cal = materials[currentMaterial];
      thickness_mm = cal.slope * echoIndex + cal.intercept;
      if (thickness_mm < 0) thickness_mm = 0;

      Serial.print("Scan ");
      Serial.print(j);
      Serial.print(": Echo at sample ");
      Serial.print(echoIndex);
      Serial.print(", thickness = ");
      Serial.print(thickness_mm, 2);
      Serial.print(" mm (");
      Serial.print(cal.name);
      Serial.println(")");
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

    tft.setCursor(50, 250);
    tft.print("Avg: ");
    tft.print(runningAvg, 2);
    tft.print(" mm");

    // -------- Timing scale (right side) --------
    tft.setTextSize(1);
    int scaleHeight = 3; // pixels per sample
    tft.drawLine(440, 15, 440, 15 + MAX_SAMPLES * scaleHeight, WHITE);
    for (int i = 0; i <= MAX_SAMPLES; i += 10) {  // tick every 10 samples
      int y = 15 + i * scaleHeight;
      tft.drawLine(440, y, 445, y, WHITE);
      tft.setCursor(450, y - 5);
      tft.print(i);
    }

    // -------- Grayscale scan column --------
    for (int i = 0; i < MAX_SAMPLES; i++) {
      uint8_t gray = map(values[i], 0, 4095, 0, 255);
      uint16_t color = tft.color565(gray, gray, gray);
      tft.fillRect(j * 24, 15 + i * scaleHeight, 23, scaleHeight, color);
    }

    // -------- Sleep mode --------
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) { }
    }

    delay(1000); // Wait before next scan
  }
}
