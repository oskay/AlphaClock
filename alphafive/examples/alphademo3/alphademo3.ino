
/*
alphademo3.ino 
 
 -- Alpha Clock Five Demo Code --
 
 Simple demo code for Alpha Clock Five
 
 Demo 3:
 Draw text to the off-screen buffer ("OSB") instead of directly
 to video.  Then, fade between two different sets of displayed text.
 
 
 10/22/2012
 Copyright (c) 2012 Windell H. Oskay.  All right reserved.
 http://www.evilmadscientist.com/
 
 ------------------------------------------------------------
 
 Designed for Alpha Clock Five, a five Letter Word Clock desiged by
 Evil Mad Scientist Laboratories http://www.evilmadscientist.com
 
 Target: ATmega644A, clock at 16 MHz.
 
 Designed to work with Arduino 1.0.1; untested with other versions.
 
 For additional requrements, please see:
 http://wiki.evilmadscience.com/Alpha_Clock_Firmware_v2
 
 
 ------------------------------------------------------------
 
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this library.  If not, see <http://www.gnu.org/licenses/>.
 
 
 */



// Required includes:  3 lines of code:

#include "alphafive.h"      // Alpha Clock Five library   -- Line 1 of 3.
#include <Wire.h>       // For optional RTC module    -- Line 2 of 3.
#include <EEPROM.h>     // For saving settings  -- Line 3 of 3.


unsigned long nextChange;
unsigned long nextFadeUpdate;

byte brightness;
byte sign;

void setup() {     

  a5Init();  // Required hardware init for Alpha Clock Five library functions   -- Line 1 of 1.

  a5_brightMode = 2; // Set Alpha Five to High brightness mode
  a5_brightLevel = 19; // The brightness level used for fading.
  // Should be set to be equal to the highest brightness that will be used.


  nextChange = millis() + 10;
  nextFadeUpdate = nextChange; 

  sign = 1;
}


void loop() {

  if (millis() > nextChange)
  {
    a5clearOSB();
    nextChange = millis() + 1000;

    if (sign) // equivalent to "if (sign != 0)"
    {
      a5loadOSB_Ascii("1234P",a5_brightLevel); // Write "1234P" to the off-screen buffer.  
      a5loadOSB_DP("30303", 10);  // write decimals to the off-screen buffer
      sign = 0;
    }

    else
    {

      a5loadOSB_Ascii("1235A",a5_brightLevel); // Write "1235A" to the off-screen buffer.  
      a5loadOSB_DP("03030", 10);  // write decimals to the off-screen buffer
      sign = 1;
    }

    a5BeginFadeToOSB();
  }


  if (millis() > nextFadeUpdate)
  { 
    nextFadeUpdate = millis() + 10;  // Suggested interval: 10 ms.

    a5LoadNextFadeStage();  // Calculate next fade iteration
    a5loadVidBuf_fromOSB();  // Load next fade iteration to video display

  }

}





