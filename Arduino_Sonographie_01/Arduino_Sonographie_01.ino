#define pin_output 51
#define pin_input_magnification 41
#define pin_input_sleep 31

unsigned long start_time;
unsigned long stop_time;
unsigned int values[600];    // Array mit den eingelesenen Spannungswerten

int i, j;
int Trigger_time;    // Dauer des Triggerpulses
int gray_value;

#define CENTRE 240

#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];

UTFT myGLCD (ILI9486,38,39,40,41);


// =========================
// ========= SETUP =========
// =========================

void setup()
   {
    Serial.begin(115200);  
    
    REG_ADC_MR = 0x10380080;              // change from 10380200 to 10380080, 1 is the PREESCALER and 8 means FREERUN
    ADC -> ADC_CHER = 0x03;               // enable ADC on pin A7
    
    pinMode(pin_output, OUTPUT);
    pinMode(pin_input_magnification, INPUT_PULLUP);    // Switch-input for magnification
    pinMode(pin_input_sleep, INPUT_PULLUP);            // Switch-input for sleep-mode
   
    digitalWrite(pin_output, LOW); 
 
    Trigger_time = 1;                     // Dauer des Triggerpulses

    // Setup the LCD
    myGLCD.InitLCD();
    myGLCD.setFont(SmallFont);    
    myGLCD.clrScr();

    myGLCD.setColor(255, 255, 0);
    myGLCD.fillRect(0, 0, 432, 13);
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 255, 0);
    myGLCD.print("Ultrasound - Ege", CENTER, 1);
   }



// ========================
// ========= LOOP =========
// ========================


void loop()
   {
    for (j = 0; j < 18; j++)   // Darstellung von 18 scans nebeneinander in x-Richtung
       {   
        // Ausgabe des Triggersignals
        // ==========================
    
        digitalWrite(pin_output,HIGH);
        //delayMicroseconds(Trigger_time);
        digitalWrite(pin_output,LOW);
        


        // Einlesen der US-Reflexionen
        // ===========================

        //start_time = micros();
        
        if (digitalRead(pin_input_magnification) == LOW)   // ohne Zeitstreckung Einlesen von 300 Werten; erfasste Zeitspanne = 120 µsec
           {
            for(i = 0; i < 300; i++)
               {
                while((ADC->ADC_ISR & 0x03)==0);  // wait for conversion
        
                values[i] = ADC->ADC_CDR[0];      //get values

                //delayMicroseconds(1);
               }
           }
        else       // mit Zeitstreckung Einlesen von 600 Werten; erfasste Zeitspanne = 240 µsec
           {
            for(i = 0; i < 600; i++)
               {
                while((ADC->ADC_ISR & 0x03)==0);  // wait for conversion
        
                values[i] = ADC->ADC_CDR[0];      //get values

                //delayMicroseconds(1);
               }
            
           }
    
        //delayMicroseconds(100);
        //delay(5);
    
        /*
        stop_time = micros();

        Serial.print("Total time for 300 values: ");
        Serial.print(stop_time-start_time);
        Serial.println(" microseconds");
        Serial.print("Average time in microseconds per conversion: ");
        Serial.println((float)(stop_time-start_time)/300);
        */
        
        /*
        Serial.println("Values: ");
        
        for(i = 0;i < 600; i++)
           {
            Serial.println(values[i]);
           }
        */
        
        // Zeichnen des scans
        // ==================

        if (digitalRead(pin_input_magnification) == LOW)   // ohne Zeitstreckung Darstellung der ersten 300 eingelesenen Werte
           {
            myGLCD.setColor(255, 255, 255);
            myGLCD.setBackColor(0, 0, 0);
            myGLCD.drawLine(440, 15, 440, 15 + 300);
            myGLCD.drawLine(440, 15, 445, 15);
            myGLCD.drawLine(440, 65, 445, 65);
            myGLCD.drawLine(440, 115, 445, 115);
            myGLCD.drawLine(440, 165, 445, 165);
            myGLCD.drawLine(440, 215, 445, 215);
            myGLCD.drawLine(440, 265, 445, 265);
            myGLCD.drawLine(440, 315, 445, 315);
                       
            myGLCD.print("0 us", 450, 10);
            myGLCD.print("20 ", 450, 60);
            myGLCD.print("40 ", 450, 110);
            myGLCD.print("60 ", 450, 160);
            myGLCD.print("80 ", 450, 210);
            myGLCD.print("100", 450, 260);
            myGLCD.print("120", 450, 310);
            
            for(i = 0; i < 300; i++)
               {
                values[i] = map(values[i], 0, 4095, 0, 255);           // Bringe die Helligkeitswerte in den Zahlenbereich [0,255]

                //values[i] = random(255);

                myGLCD.setColor(values[i], values[i], values[i]);
                myGLCD.fillRect(j * 24, 15 + i, j * 24 + 23, 15 + i);
               }
           }
        else   // mit Zeitstreckung Darstellung nur jedes zweiten Werts der 600 eingelesenen Werte
           {
            myGLCD.setColor(255, 255, 255);
            myGLCD.setBackColor(0, 0, 0);
            myGLCD.drawLine(440, 15, 440, 15 + 300);
            myGLCD.drawLine(440, 15, 445, 15);
            myGLCD.drawLine(440, 65, 445, 65);
            myGLCD.drawLine(440, 115, 445, 115);
            myGLCD.drawLine(440, 165, 445, 165);
            myGLCD.drawLine(440, 215, 445, 215);
            myGLCD.drawLine(440, 265, 445, 265);
            myGLCD.drawLine(440, 315, 445, 315);
                       
            myGLCD.print("0 us", 450, 10);
            myGLCD.print("40 ", 450, 60);
            myGLCD.print("80 ", 450, 110);
            myGLCD.print("120", 450, 160);
            myGLCD.print("160", 450, 210);
            myGLCD.print("200", 450, 260);
            myGLCD.print("240", 450, 310);
            
            for(i = 0; i < 300; i++)
               {
                values[2*i] = map(values[2*i], 0, 4095, 0, 255);         // Bringe die Helligkeitswerte in den Zahlenbereich [0,255]

                //values[2*i] = random(255);

                myGLCD.setColor(values[2*i], values[2*i], values[2*i]);
                myGLCD.fillRect(j * 24, 15 + i, j * 24 + 23, 15 + i);
               }
           }


        // Pause-Taster abfragen
        // =====================
        
        if (digitalRead(pin_input_sleep) == HIGH)
           {
            while(digitalRead(pin_input_sleep) == HIGH)
               {
                // sleeping
               }
           }
             
        delay(1000);    
       }
      
   }
