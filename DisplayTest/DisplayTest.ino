#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

// Define common 16-bit color constants (RGB565 format)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

void setup() {
  Serial.begin(9600);
  uint16_t ID = tft.readID();
  Serial.print("TFT ID = 0x");
  Serial.println(ID, HEX);

  tft.begin(ID);
  tft.setRotation(1);  // Landscape orientation

  tft.fillScreen(BLACK);
  delay(500);
  tft.fillScreen(RED);
  delay(500);
  tft.fillScreen(GREEN);
  delay(500);
  tft.fillScreen(BLUE);
  delay(500);

  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, 50);
  tft.println("ILI9486 Test");
  tft.setCursor(50, 80);
  tft.println("Arduino Due");
}

void loop() {
  static uint16_t colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE};
  static uint8_t idx = 0;
  tft.fillRect(100, 120, 120, 80, colors[idx]);
  idx = (idx + 1) % (sizeof(colors) / sizeof(colors[0]));
  delay(500);
}
