/*
 alphafive.h
 
 Part of the Alpha Five library for Arduino
 Version 2.1 - 1/31/2013
 Copyright (c) 2013 Windell H. Oskay.  All right reserved.
 
 This library is free software: you can redistribute it and/or modify
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

#ifndef alphafive_h
#define alphafive_h

#include "Arduino.h"
#include <Wire.h>       // For optional RTC module
#include <EEPROM.h>     // For saving settings


// Division routines from: http://codereview.blogspot.com/2009/06/division-of-integers-by-constants.html
// Approximately 5 times as fast as regular division routine, for U8 /10:
#define U8DIVBY10(A)  (uint8_t)( (((uint16_t)(A) * (uint16_t)(0xCDu)) >> 8u) >> (3u) )
#define U16DIVBY10(A) (uint16_t)((((uint32_t)(A) * (uint32_t)(0xCCCDu)) >> 16u) >> (3u))


#define a5_MaxBright 19                 // 20 levels, 0-19

// Hardware location shortcuts
#define a5_BUTTONMASK   15              // Locations of physical pushbuttons, PB0, PB1, PB2, PB3
#define a5_alarmSetBtn  1				// Snooze/Set alarm button
#define a5_timeSetBtn   2				// Set time button
#define a5_plusBtn	    4				// + button
#define a5_minusBtn     8				// - button

#define a5_allButtonsButAlarmSet 14
#define a5_allButtonsButTimeSet  13
#define a5_allButtonsButPlus     11
#define a5_allButtonsButMinus     7

#define a5_TimeSetPlusBtns   6
#define a5_TimeSetMinusBtns 10
#define a5_AlarmSetPlusBtns  5
#define a5_AlarmSetMinusBtns 9
extern int8_t a5_OSB[];
extern int8_t a5_FadeStage;


// Starting offset of our ASCII array:
#define a5_asciiOffset 32
// Starting offset for number zero:
#define a5_integerOffset 48

extern int8_t a5_brightLevel;
extern byte a5_brightMode;  // 0: low brightness mode. 1: Medium. 2: High brightness mode
extern char a5_monthShortNames_P[];

byte a5getFontChar(char asciiChar, byte offset);
void a5editFontChar(char asciiChar, byte A, byte B, byte C);
void a5loadAltNumbers (int8_t charset);
void a5loadVidBuf_Ascii (char WordIn[], byte BrightIn);
void a5loadVidBuf_DP (char WordIn[], byte BrightIn);
void a5loadVidBuf_fromOSB_noCache (void);
void a5loadVidBuf_fromOSB (void);
void a5BeginFadeToOSB (void);
void a5LoadNextFadeStage (void);
void a5loadOSB_Ascii (char WordIn[], byte BrightIn);
void a5loadOSB_DP (char WordIn[], byte BrightIn);
void a5loadOSB_Segment (byte segment, byte BrightIn);
void a5clearVidBuf (void);
void a5clearOSB (void);
void a5nightLight(byte Brightness);
byte a5GetButtons(void);
byte a5CheckForRTC();
void a5writeEEPROM(byte address, byte value);
void a5tone(unsigned int frequency, unsigned long duration);
void a5noTone (void);
void a5Init (void);

#endif



























