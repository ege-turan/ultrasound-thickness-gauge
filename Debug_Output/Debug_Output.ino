// Pin Definitions
#define TRIGGER_PIN 51       // Output pulse pin to transducer
#define ECHO_PIN 41          // Input pin to receive echo

// Speed of sound in metal (steel) ~ 5900 m/s
#define SPEED_OF_SOUND 5900.0  // m/s

// Time variables
unsigned long start_time = 0;
unsigned long stop_time = 0;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  
  // Configure pins
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Set trigger LOW initially
  digitalWrite(TRIGGER_PIN, LOW);
}

void loop() {
  // Send a single 100 ns pulse
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(1); // 1 µs > 100 ns
  digitalWrite(TRIGGER_PIN, LOW);

  // Wait for echo start
  while (digitalRead(ECHO_PIN) == LOW);
  start_time = micros();

  // Wait for echo end
  while (digitalRead(ECHO_PIN) == HIGH);
  stop_time = micros();

  // Calculate round trip time
  unsigned long echo_time = stop_time - start_time;

  // Convert time (us) to seconds
  float time_s = echo_time / 1e6;

  // Calculate one-way distance: thickness = (speed × time) / 2
  float thickness_m = (SPEED_OF_SOUND * time_s) / 2.0;
  float thickness_mm = thickness_m * 1000.0;

  // Print results
  Serial.print("Echo time: ");
  Serial.print(echo_time);
  Serial.print(" µs, Thickness: ");
  Serial.print(thickness_mm, 2);
  Serial.println(" mm");

  delay(1000); // Wait before next measurement
}
