#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

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

MaterialCal materials[] = {
  {1.45, -23.6, "Al 6061"},
  {1.30, -20.0, "Steel"},
  {1.10, -15.0, "Acrylic"}
};

int currentMaterial = 0; // index into materials[]

// ----------------- Globals -----------------
unsigned int values[MAX_SAMPLES] = {0};
unsigned int corrected[MAX_SAMPLES] = {0};
unsigned int reference[MAX_SAMPLES] = {0};
bool referenceSet = false;

float thicknessBuffer[RUNNING_AVG_LEN] = {0};
int bufferIndex = 0;
int numFilled = 0;

#define DEBUG_MODE true   // set true to enable Serial print of samples
bool lastSleepState = LOW; // track previous sleep switch state

void setup() {
  Serial.begin(115200);

  // Configure ADC (Arduino Due freerun mode)
  REG_ADC_MR = 0x10380080;
  ADC->ADC_CHER = 0x03;

  // Pin setup
  pinMode(pin_output, OUTPUT);
  pinMode(pin_input_reference, INPUT_PULLUP);
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
      while ((ADC->ADC_ISR & 0x03) == 0);
      values[i] = ADC->ADC_CDR[0];
    }

    // -------- Reference capture --------
    if (digitalRead(pin_input_reference) == LOW) {
      for (int i = 0; i < MAX_SAMPLES; i++) reference[i] = values[i];
      referenceSet = true;
      Serial.println("Reference captured.");
    }

    // -------- Reference subtraction --------
    if (referenceSet) {
      for (int i = 0; i < MAX_SAMPLES; i++) {
        int correct = (int)values[i] - (int)reference[i];
        if (correct < 0) correct = 0;
        corrected[i] = correct;
      }
    }

    // -------- Echo detection --------
    int echoIndex = -1;
    for (int i = MIN_ECHO_SAMPLE; i < MAX_SAMPLES; i++) {
      int delta = values[i] - values[i - 1];
      if (delta > 0) {
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
    }

    // -------- Running average --------
    thicknessBuffer[bufferIndex] = thickness_mm;
    bufferIndex = (bufferIndex + 1) % RUNNING_AVG_LEN;
    if (numFilled < RUNNING_AVG_LEN) numFilled++;
    float runningAvg = 0;
    for (int i = 0; i < numFilled; i++) runningAvg += thicknessBuffer[i];
    runningAvg /= numFilled;

    // -------- Display --------
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(2);
    tft.setCursor(50, 200);
    tft.print("Echo S: "); tft.print(echoIndex);
    tft.setCursor(50, 225);
    tft.print("T: "); tft.print(thickness_mm, 2); tft.print(" mm");
    tft.setCursor(200, 250);
    tft.print("Avg: "); tft.print(runningAvg, 2); tft.print(" mm");

    // -------- Grayscale scan column --------
    int scaleHeight = 2;
    for (int i = 0; i < MAX_SAMPLES; i++) {
      uint8_t gray = map(values[i], 0, 4095, 0, 255);
      uint16_t color = tft.color565(gray, gray, gray);
      tft.fillRect(j * 24, 15 + i * scaleHeight, 23, scaleHeight, color);
    }

    // -------- Serial CSV logging --------
    if (digitalRead(pin_input_reference) == HIGH) {
      Serial.println("Idx,Raw,Ref,Corrected,EchoIdx,Thickness_mm");
      for (int i = 0; i < MAX_SAMPLES; i++) {
        int rawVal = values[i];
        int refVal = referenceSet ? reference[i] : 0;
        int correct = corrected[i];
        Serial.print(i); Serial.print(",");
        Serial.print(rawVal); Serial.print(",");
        Serial.print(refVal); Serial.print(",");
        Serial.print(correct); Serial.print(",");
        Serial.print(echoIndex); Serial.print(",");
        Serial.println(thickness_mm, 2);
      }
      Serial.println("-----"); // end of scan block
    }

    // -------- Sleep mode --------
    if (digitalRead(pin_input_sleep) == HIGH) {
      while (digitalRead(pin_input_sleep) == HIGH) { }
    }

    delay(1000);
  }
}