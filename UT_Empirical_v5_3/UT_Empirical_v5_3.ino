#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <math.h>  // for log/exp

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
#define MAX_SAMPLES       25     // Samples per scan
#define RUNNING_AVG_LEN   5      // Number of scans for averaging
#define MIN_ECHO_SAMPLE   16     // Ignore first few samples (ringing)

// Reference impulse region
#define IMP_START 15
#define IMP_END   18

// ----------------- Material Calibration -----------------
struct MaterialCal {
  float slope;
  float intercept;
  const char* name;
};

// Add materials here:
MaterialCal materials[] = {
  {0.967, -12.5, "Al 6061"},   // Al 6061 8/25/25 measured
  {1.30, -20.0, "Steel"},      // Steel 8/25/25 guess
  {1.10, -15.0, "Acrylic"}     // Acrylic 8/25/25 guess
};

int currentMaterial = 0; // index into materials[]

// ----------------- Globals -----------------
unsigned int rawSamples[MAX_SAMPLES];     // keep raw before correction
unsigned int values[MAX_SAMPLES];         // corrected samples
unsigned int reference[MAX_SAMPLES] = {0};
bool referenceSet = false;

float thicknessBuffer[RUNNING_AVG_LEN] = {0};
int bufferIndex = 0;
int numFilled = 0;

// ----------------- Functions -----------------
void fitReferenceExponential(const unsigned int raw[], unsigned int out_ref[], int n) {
  // 1) Estimate baseline (c) as the average of the last few samples
  const int tailStart = max(IMP_END + 1, n - 4);
  long sumTail = 0; int cntTail = 0;
  for (int i = tailStart; i < n; i++) { sumTail += raw[i]; cntTail++; }
  float c = (cntTail > 0) ? (float)sumTail / cntTail : 0.0f;

  // 2) Log-linear fit of y = a*exp(b*i) + c
  double Sx = 0, Sy = 0, Sxx = 0, Sxy = 0;
  int m = 0;
  for (int i = 0; i < n; i++) {
    if (i >= IMP_START && i <= IMP_END) continue;  // skip impulse region
    float y = (float)raw[i] - c;
    if (y < 1.0f) y = 1.0f;                        // clamp
    double X = i;
    double Y = log((double)y);
    Sx += X; Sy += Y; Sxx += X * X; Sxy += X * Y;
    m++;
  }

  double den = m * Sxx - Sx * Sx;
  double slope = 0.0, intercept = 0.0;
  if (m >= 2 && fabs(den) > 1e-9) {
    slope = (m * Sxy - Sx * Sy) / den;
    intercept = (Sy - slope * Sx) / m;
  }
  double A = exp(intercept);

  // 3) Build the fitted reference
  for (int i = 0; i < n; i++) {
    if (i >= IMP_START && i <= IMP_END) {
      out_ref[i] = 0;  // nullify impulse region
    } else {
      double yfit = A * exp(slope * i) + c;
      if (yfit < 0) yfit = 0;
      if (yfit > 4095) yfit = 4095;
      out_ref[i] = (unsigned int)(yfit + 0.5);
    }
  }
}

// ----------------- Setup -----------------
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

// ----------------- Loop -----------------
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
    for (int i = 0; i < MAX_SAMPLES; i++) rawSamples[i] = values[i];  // snapshot raw

    // -------- Reference capture --------
    if (digitalRead(pin_input_reference) == LOW) {  // button pressed
      fitReferenceExponential(rawSamples, reference, MAX_SAMPLES);
      referenceSet = true;
      Serial.println("Reference captured: exp-fit applied, samples 15–18 nulled.");
    }

    // -------- Reference subtraction --------
    if (referenceSet) {
      for (int i = 0; i < MAX_SAMPLES; i++) {
        int corrected = (int)rawSamples[i] - (int)reference[i];
        if (corrected < 0) corrected = 0;
        values[i] = corrected;
      }
    } else {
      for (int i = 0; i < MAX_SAMPLES; i++) values[i] = rawSamples[i];
    }

    // -------- Debug: print raw, reference, and corrected samples --------
    if (DEBUG_MODE) {
      Serial.print("Scan ");
      Serial.print(j);
      Serial.println(" Samples Table:");
      Serial.println("Idx\tRaw\tRefFit\tCorrected");

      for (int i = 0; i < MAX_SAMPLES; i++) {
        int corrected = referenceSet ? max(0, (int)rawSamples[i] - (int)reference[i])
                                     : (int)rawSamples[i];
        Serial.print(i);
        Serial.print("\t");
        Serial.print(rawSamples[i]);         // raw
        Serial.print("\t");
        if (referenceSet) Serial.print(reference[i]);
        else Serial.print("-");
        Serial.print("\t");
        Serial.println(corrected);
      }
      Serial.println("-----");
    }

    // -------- Echo detection (first upward spike) --------
    int echoIndex = -1;
    for (int i = MIN_ECHO_SAMPLE; i < MAX_SAMPLES; i++) {
      int delta = values[i] - values[i - 1];
      if (delta > 0) {
        echoIndex = i;
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
    for (int i = 0; i <= MAX_SAMPLES; i += 10) {
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
