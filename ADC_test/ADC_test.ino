void setup() {
  Serial.begin(115200);

  // Configure ADC for free-running mode on channel A7 (ADC channel 7)
  REG_ADC_MR = 0x10380080;   // FREERUN = 1, PRESCAL=8
  ADC->ADC_CHER = (1 << 7);  // Enable ADC channel 7 (A7)

  // Wait a moment before starting
  delay(1000);
  Serial.println("Starting ADC free-run sampling on A7...");
}

void loop() {
  // Wait until new conversion complete on channel 7
  while ((ADC->ADC_ISR & (1 << 7)) == 0) {
    // wait
  }
  
  // Read conversion result from channel 7
  uint16_t adcValue = ADC->ADC_CDR[7];
  
  Serial.println(adcValue);
  
  delay(10);  // Adjust delay to control print speed
}
