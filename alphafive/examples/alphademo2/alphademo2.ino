
/*
alphademo2.ino 
 
 -- Alpha Clock Five Demo Code --
 
 Demo 2:
 Simple demo code for Alpha Clock Five
 Ramp brightness of word "HELLO," demonstrate simple program control,
 timing, and the use of decimal points. 
 
 
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


unsigned long nextTime;
byte brightness;
byte sign;

void setup() {     

  a5Init();  // Required hardware init for Alpha Clock Five library functions   -- Line 1 of 1.

  a5_brightMode = 2; // Set Alpha Five to High brightness mode

  nextTime = millis() + 100;
  brightness = 0;
  sign = 1;
}


void loop() {
  if (millis() > nextTime)
  {

    nextTime = millis() + 200;
    a5clearVidBuf();  // Clear video buffer
    a5loadVidBuf_Ascii("HELLO",brightness); // Write "HELLO" to the video buffer. 
    // The constant a5_MaxBright is equal to 19.

    if (sign != 0)  // if the "sign" variable is not equal to zero
    {

      a5loadVidBuf_DP("20000", a5_MaxBright);
      brightness++;

      if (brightness == a5_MaxBright){ 
        sign = 0;

      } 
    }
    else
    {

      a5loadVidBuf_DP("00001", a5_MaxBright);
      brightness--;

      if (brightness == 0){
        sign = 1;

      } 
    } 
  }
}


