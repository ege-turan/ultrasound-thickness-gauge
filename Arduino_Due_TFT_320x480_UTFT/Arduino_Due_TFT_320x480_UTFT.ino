/*************************************************************************
**  AH_MCP4922.h - Library for get out analog voltage          	        **
**  Created by A. Hinkel 2012-01-05					**
**  download from "http://www.alhin.de/arduino"  			**
**									**
**  Based on code from website: 					**
**  http://www.sinneb.net/2010/06/mcp4921-12bit-dac-voltage-controller/ **
**									**
**  Released into the public domain.  		                    	**
*************************************************************************/

#include "AH_MCP4922.h"
#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];

UTFT myGLCD (ILI9486,38,39,40,41);

int xPos, yPos;         // Abtastposition
int value;              // Grauwert
int pushButton = 2;     // Port für den Startknopf

#define sensorPin A0    // analoger Eingang für das Photodiodensignal



// ***************************************
// **************** SETUP ****************
// ***************************************

void setup()
   { 
    Serial.begin(9600);                          // Init serial interface 
    
    // Setup the LCD
    myGLCD.InitLCD();
    myGLCD.setFont(SmallFont);    
    myGLCD.clrScr();
   } 



// *******************************************
// ************** HAUPTSCHLEIFE **************
// *******************************************

void loop()
   {
    myGLCD.setColor(255, 255, 0);
    myGLCD.fillRect(0, 0, 479, 13);
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 255, 0);
    myGLCD.print("Sonographie - stoppi", CENTER, 1); 
        
    for (yPos = 0; yPos < 101; yPos++)
       {
        for (xPos = 0; xPos < 160; xPos++)
           {
            value = random(1023);
            
            value = map(value,0,1023,0,255);
            
            myGLCD.setColor(value, value, value);
            myGLCD.fillRect(xPos * 3, 15 + yPos * 3, xPos * 3 + 2, 15 + yPos * 3 + 2);
            //myGLCD.drawPixel(xPos, 15 + yPos);
           }       
        
        
        yPos++;
        
        for (xPos = 159; xPos >= 0; xPos--)
           {
            value = random(1023);
    
            value = map(value,0,1023,0,255);
       
            myGLCD.setColor(value, value, value);
            myGLCD.fillRect(xPos * 3, 15 + yPos * 3, xPos * 3 + 2, 15 + yPos * 3 + 2);
            //myGLCD.drawPixel(xPos, 15 + yPos);
       
           }       
        
       } 
       
   }
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
