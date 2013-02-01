/*
 alphafive.cpp
 
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

#include "alphafive.h"

// Stored data (including global arrays) take up roughly 20% of our 4096 bytes of SRAM.
//
// Note 1: While it is obviously possible to store font tables in program flash, storing it in SRAM
//   allows characters to be redefined on the fly, in software.
// Note 2: Retreving things from Progmem takes time.  Yes, SRAM is a precious resource, but so is CPU time.
// Note 3: We are using multiple video buffers to smooth off-screen drawing, and to enable
//   real-time fades between complete, arbitrary-brightness video buffer sets.
//   It would be possible to eliminate nearly
//   half of the SRAM usage by eliminating off-scree drawing, or if we were to eliminate fading, or if
//   we were to only allow ASCII display on the screen, at a fixed brightness.


// The Video Buffer takes up 90 bytes of SRAM. (That's about 2% of our available 4096 bytes.)
#define a5_VIDBUFLENGTH 90
byte a5_vidBuf[a5_VIDBUFLENGTH];       // Array contains brightness of individual segments (5 * 18 segments = 90)

// Off-screen buffer ("a5_OSB") takes an additional 90 bytes of SRAM. It's used for fading and compositing.
int8_t a5_OSB[a5_VIDBUFLENGTH];

// Three additional off-screen buffers for automatic grayscale fades.  271 bytes for these three, together.
int8_t a5_LastOSB[a5_VIDBUFLENGTH];  // Cache of last OSB
int8_t a5_FadeFrom[a5_VIDBUFLENGTH]; // Snapshot of what we're fading away from.
int8_t a5_FadeTo[a5_VIDBUFLENGTH];   // Snapshot of what we're fading towards.
int8_t a5_FadeStage;

/*  EEPROM storage: settings variables and default values for those variables  */
#define a5_EELength 8
byte a5_EEvalues[a5_EELength];

// Internal state variables used by refresh interrupt
volatile byte a5_intensityStep;
volatile byte a5_litChar;

int8_t a5_brightLevel;
byte a5_brightMode;  // 0: low brightness mode. 1: Medium. 2: High brightness mode

// For tone duration:
volatile long a5_timer1_toggle_count;

// Short month names:
char a5_monthShortNames_P[] PROGMEM = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";


/*
 The brightness displayed by Alpha Clock Five can be set to either bright mode or dim mode.
 
 In either mode, our brightness variable ("a5_brightLevel") ranges between 0 and 19, (20 possible values).
 For fading in and out, we want to use an exponential fade, rather than a linear intensity ramp.
 
 The brightness look-up table ( a5_BLUT[] ) maps between the input levels and the 16 linear
 intensity levels, like so:
 
 y = 15 e^((x-23)/6.5)
 
 x	y	Rounded
 0	0.436	0
 1	0.508	1
 2	0.593	1
 3	0.692	1
 4	0.807	1
 5	0.941	1
 6	1.097	1
 7	1.280	1
 8	1.492	2
 9	1.741	2
 10	2.030	2
 11	2.368	2
 12	2.761	3
 13	3.221	3
 14	3.756	4
 15	4.381	4
 16	5.110	5
 17	5.959	6
 18	6.951	7
 19	8.106	8
 20	9.455	10
 21	11.027	11
 22	12.861	13
 23	15.000	15
 
 
 We actually use a hand-edited 20-step version, with sharper cutoff at tail (dim) end:
 
 x	y	Rounded
 0	0.436	0
 1	0.807	1
 2	0.941	1
 3	1.097	1
 4	1.280	1
 5	1.741	2
 6	2.030	2
 7	2.368	2
 8	2.761	3
 9	3.221	3
 10	3.756	4
 11	4.381	4
 12	5.110	5
 13	5.959	6
 14	6.951	7
 15	8.106	8
 16	9.455	10
 17	11.027	11
 18	12.861	13
 19	15.000	15
 
 */


byte a5_BLUT[] = {
    0,1,1,1,1,
    2,2,2,3,3,
    4,4,5,6,7,
    8,10,11,13,15};



// The font table takes up 70*3 = 210 bytes of SRAM.  We store the array in RAM so that it is
// *fully editable during runtime*.
// If you want to have full multiple font sets, store those sets in PROGMEM and load only one into RAM.
// The last 5 characters (ascii 'a' - 'e') are extra buffer to store special characters.

byte a5_FontTable[] = {
    0,0,0,    // [space]  ASCII 32
    0,0,3,    // ! (Yes, lame, but it's the "standard." Better Suggestions welcome.)
    0,0,40,  // "
    63,3,255,    //#
    63,1,42,    //$
    57,3,106,    //%
    14,3,98,    //&
    0,0,64,      // '
    0,0,192,      // (
    0,2,16,      // )
    48,3,240,    // *
    48,1,32,      //+
    0,2,0,      //  ,
    48,0,0,      // -
    64,0,0,      // . (lower DP)
    0,2,64,    //  /
    15,0,15,  // 0
    0,0,3,     //1
    63,0,5,    //2
    47,0,3,    //3
    48,0,11,    //4
    63,0,10,    //5
    63,0,14,    //6
    3,0,3,      //7
    63,0,15,    //8
    63,0,11,    //9
    0,1,32,      //:
    0,2,32,      //;
    0,0,192,      //<
    60,0,0,      //=
    0,2,16,      //>
    34,1,1,      //?
    47,0,13,      //@
    51,0,15,      //A
    47,1,35,      //B
    15,0,12,      //C
    15,1,35,     //D
    31,0,12,      //E
    19,0,12,    //F
    47,0,14,      //G
    48,0,15,      //H
    15,1,32,        //I
    12,0,7,        //J
    16,0,204,      //K
    12,0,12,       //L
    0,0,95,        //M
    0,0,159,      //N
    15,0,15,      //O
    51,0,13,      //P
    15,0,143,    //Q
    51,0,141,    //R
    63,0,10,      //S
    3,1,32,      //T
    12,0,15,      //U
    0,2,76,      //V
    0,2,143,      //W
    0,2,208,      //X
    0,1,80,      //Y
    15,2,64,      //Z
    10,1,32,      // [
    0,0,144,      // \ (backslash)
    5,1,32,        // ]
    0,2,128,      // ^
    12,0,0,      // _
    0,0,16,        // `
    0,0,0,    // [available for use as special char]  ASCII 'a'
    0,0,0,    // [available for use as special char]  ASCII 'b'
    0,0,0,    // [available for use as special char]  ASCII 'c'
    0,0,0,    // [available for use as special char]  ASCII 'd'
    0,0,0     // [available for use as special char]  ASCII 'e'
};


byte a5getFontChar(char asciiChar, byte offset)
{  // Not used in example firmware, but can be handy.
//Usage:
//    byte a = a5getFontChar('%',0);
//    byte b = a5getFontChar('%',1);
//    byte c = a5getFontChar('%',2);
        
    return a5_FontTable[(3 * (asciiChar - a5_asciiOffset)) + offset];
}

void a5editFontChar(char asciiChar, byte A, byte B, byte C)
{// Usage:  a5editFontChar ('%', 57, 3, 106);
    if ((asciiChar >= ' ') && (asciiChar < 'f') )
    {  
        byte offset = (3 * (asciiChar - a5_asciiOffset));
        a5_FontTable[offset++] = A;
        a5_FontTable[offset++] = B;
        a5_FontTable[offset] = C;
    }
}

void a5loadAltNumbers (int8_t charset)
{  // Usage:  a5loadAltNumbers(1);  // Load number characters, alt set #1
    
    // First, restore default number shapes.
    a5editFontChar('0',15,0,15);   // "Square" zero, no slash
    a5editFontChar('1',0,0,3);     // Straight 1, right side
    a5editFontChar('2',63,0,5);    // Standard 2
    a5editFontChar('3',47,0,3);    // Standard 3
    a5editFontChar('4',48,0,11);   // Standard 4
    a5editFontChar('5',63,0,10);   // Standard 5
    a5editFontChar('6',63,0,14);   // Standard 6
    a5editFontChar('7',3,0,3);     // Straight-side 7
    a5editFontChar('8',63,0,15);   // Standard 8
    a5editFontChar('9',63,0,11);   // Standard 9
    
    // Charset 0:  Default charset
    // Charset 1:  Default charset w/ Slashed Zero
    // Charset 2:  Curvy 3, top-angle 7
    // Charset 3:  Curvy 3, top-angle 7, slashed zero
    // Charset 4:  w/ "Euro-style" 1, 3, 7
    // Charset 5:  w/ "Euro-style" 1, 3, 7, slashed zero
    // Charset 6:  w/ "Euro-style" 1, 3, 7, curvy 2 & 5
    // Charset 7:  w/ "Euro-style" 1, 3, 7, curvy 2 & 5, slashed zero
    // Charset 8:  Skinny charset A
    // Charset 9:  Skinny charset B
   
    if ((charset & 1) && (charset <= 7))
    {
        a5editFontChar('0', 15,2,79);  // Slashed zero
    }
    
    if (charset > 1) {
        
        if (charset <= 3) {
            // Alt Charset
            a5editFontChar('3',47,0,66);   // "curvy" 3
            a5editFontChar('7',3,1,64);    // Top-angle 7, no cross-stroke
        }
        else if (charset <= 5)
        {
            a5editFontChar('1',0,0,67);    // Serif-1
            a5editFontChar('3',47,0,66);   // "curvy" 3
            a5editFontChar('7',51,1,64);    // half-angle 7, w/ cross-stroke 
        }
        else if (charset <= 7)
        {
            a5editFontChar('1',0,0,67);    // Serif-1
            a5editFontChar('2',47,2,1);    // "curvy" 2
            a5editFontChar('3',47,0,66);   // "curvy" 3
            a5editFontChar('5',31,0,136);   // "curvy" 5
            a5editFontChar('7',51,1,64);    // half-angle 7, w/ cross-stroke
        }
        else if (charset <= 9)
        {   // Skinny set A
            a5editFontChar('0',10,1,35);   // "Skinny" zero, no slash
            a5editFontChar('2',42,1,1);    // "Skinny" 2
            a5editFontChar('3',42,0,3);    // "Skinny" 3
            a5editFontChar('4',32,0,35);   // "Skinny" 4
            a5editFontChar('5',42,0,34);   // "Skinny" 5
            a5editFontChar('6',42,1,34);   // "Skinny" 6
            a5editFontChar('7',2,0,3);     // "Skinny" 7
            a5editFontChar('8',42,1,35);   // "Skinny" 8
            a5editFontChar('9',42,0,35);   // "Skinny" 9
        }
        if (charset == 9)
        {   // Skinny set B; two little changes to the charset 8:
            a5editFontChar('1',0,0,67);    // Serif 1 
            a5editFontChar('3',42,0,66);   // "curvy" 3 
        }
    }
}

  



void a5loadVidBuf_Ascii (char WordIn[], byte BrightIn)
{
    // Immediately update the video buffer with five ascii characters at given brightness
    // Good for basic ASCII display and simple fades.
    //
    // Execution time: ~276 us total elapsed time, assuming that refresh interrupt is enabled.
    
    byte alphaPosTemp;
    byte letterByteTemp;
    byte segment = 0;
    byte i;
    byte j = 0;
    
    byte BrightLocal = a5_BLUT[BrightIn];
    
    while (j < 5)
    {
        
        alphaPosTemp = 3 * (WordIn[4 - j] - a5_asciiOffset );
        letterByteTemp = a5_FontTable[alphaPosTemp++];
        
        // It would be slightly faster-- but less compact in the code here --to unroll these loops.
        i = 1;
        
        do {
            if (letterByteTemp & i)
            {
                a5_vidBuf[segment++] = BrightLocal;
            }
            else
            {
                a5_vidBuf[segment++] = 0;
            }
            i = i << 1;
        }
        while (i != 0);
        
        letterByteTemp = a5_FontTable[alphaPosTemp++];
        if (letterByteTemp & 1)
        {
            a5_vidBuf[segment++] = BrightLocal;
        }
        else
        {
            a5_vidBuf[segment++] = 0;
        }
        
        if (letterByteTemp & 2)
        {
            a5_vidBuf[segment++] = BrightLocal;
        }
        else
        {
            a5_vidBuf[segment++] = 0;
        }
        
        letterByteTemp = a5_FontTable[alphaPosTemp];
        i = 1;
        
        do {
            if (letterByteTemp & i)
            {
                a5_vidBuf[segment++] = BrightLocal;
            }
            else
            {
                a5_vidBuf[segment++] = 0;
            }
            i = i << 1;
        }
        while (i != 0);
        
        j++;
    }
}



void a5loadVidBuf_DP (char WordIn[], byte BrightIn)
{
    /*
     Immediately update the video buffer with upper and/or lower decimal points at indicated brightness.
     
     The decimal point string, "WordIn" describes which decimal points (if any) should be lit for each of the
     five alphanumeric LED displays, e.g., "_123_"
     
     Example: loadVidBuf_DP ("01230", 15);
     
     For each character in this string, a value '1' will light the lower DP, a value '2' will light the upper DP,
     and a value '3' will light both DPs. Any other character will not cause either of the LEDs to be lit.
     
     Note that this routine will overwrite existing decimal places with the new brightness specified.
     This can be used to make the decimal points pulse, or to (for example) overwrite an ascii period ('.') with
     a decimal point at zero brightness.
     
     Execution time: ~14 us total elapsed time, assuming that refresh interrupt is enabled.
     */
    
    byte segment;
    byte j = 0;
    
    char theLetter;
    byte BrightLocal = a5_BLUT[BrightIn];
    
    while (j < 5)
    {
        theLetter = WordIn[4 - j];
        segment = (j * 18) + 6;
        
        if ((theLetter == '1') || (theLetter == '3'))
        {
            a5_vidBuf[segment] = BrightLocal;
        }
        
        if ((theLetter == '2') || (theLetter == '3'))
        {
            segment++;
            a5_vidBuf[segment] = BrightLocal;
        }
        j++;
    }
}


void a5loadVidBuf_fromOSB_noCache (void)
{
    // Immediately update the video buffer, loading it with the contents
    //   of the off-screen buffer (a5_OSB). (a5_LastOSB is NOT updated.)
    //
    // Slightly faster than a5loadVidBuf_fromOSB().
    
    int8_t *bufPtr = &a5_OSB[0];
    byte *vPtr = &a5_vidBuf[0];
    
    byte i = a5_VIDBUFLENGTH;
    do {
        
        *vPtr++ = a5_BLUT[*bufPtr++];
        
        // This two lines are essentially equivalent to:
        //a5vidBuf[i] = a5_BLUT[a5_OSB[i]];
        //i++;
        
        i--;
    }
    while (i > 0);
}


void a5loadVidBuf_fromOSB (void)
{
    // Immediately update the video buffer, loading it with the contents
    //   of the off-screen buffer (a5_OSB).
    // Store a snapshot of the off-screen buffer (a5_OSB) in a5_LastOSB.
    
    int8_t *bufPtr = &a5_OSB[0];
    int8_t *OldbufPtr = &a5_LastOSB[0];
    byte *vPtr = &a5_vidBuf[0];
    
    byte i = a5_VIDBUFLENGTH;
    do {
        
        *OldbufPtr++ = *bufPtr;
        *vPtr++ = a5_BLUT[*bufPtr++];
        
        // These two lines are essentially equivalent to:
        //a5_LastOSB[i] = a5_OSB[i];
        //a5vidBuf[i] = a5_BLUT[a5_OSB[i]];
        //i++;
        
        i--;
    }
    while (i > 0);
}

void a5BeginFadeToOSB (void)
{
    // Begin process of fading FROM the data presently shown on the LED display
    //   TO the contents of the off-screen buffer (a5_OSB).
    // The current contents of the LED display are assumed to be loaded into a5_LastOSB.
    //    (a5_LastOSB is updated each time that you call a5loadVidBuf_fromOSB.)
    
    int8_t *OldbufPtr = &a5_LastOSB[0]; // Current display contents
    int8_t *fromPtr = &a5_FadeFrom[0];  // Snapshot of buffer contents that we're fading away from.
    
    int8_t *bufPtr = &a5_OSB[0];     // Current contents of OSB
    int8_t *toPtr = &a5_FadeTo[0];   // Snapshot of buffer contents that we're fading towards.
    
    a5_FadeStage = 0;
    
    byte i = a5_VIDBUFLENGTH;
    do {
        
        *fromPtr++ = *OldbufPtr++;
        *toPtr++ = *bufPtr++;
        
        // These two lines are essentially equivalent to:
        //a5_FadeFrom[i] = a5_LastOSB[i];
        //a5_FadeTo[i] = a5_OSB[i];
        //i++;
        
        i--;
    }
    while (i > 0);
}


void a5LoadNextFadeStage (void)
{
    // If a fade is presently in progress, update the OSB with the next iteration.
    // usage:
    // a5LoadNextFadeStage();  // Calculate next fade iteration
    // a5loadVidBuf_fromOSB();  // Load next fade iteration to video display
      
    if (a5_FadeStage >= 0){
        
        int8_t *fromPtr = &a5_FadeFrom[0];
        int8_t *toPtr = &a5_FadeTo[0];
        int8_t *bufPtr = &a5_OSB[0];     // Current contents of OSB
        byte i = a5_VIDBUFLENGTH;
        
        if (a5_FadeStage < a5_brightLevel) {
            
            do {
                int8_t segmentBrightnessFrom = (*fromPtr++);
                int8_t segmentBrightnessTo = (*toPtr++);
                
                if (segmentBrightnessTo > segmentBrightnessFrom) {
                    int8_t temp = (segmentBrightnessFrom + a5_FadeStage);
                    if (segmentBrightnessTo > temp) {
                        *bufPtr++ = temp;
                    }
                    else
                    {*bufPtr++ = segmentBrightnessTo;} 
                }
                else // segmentBrightnessFrom > segmentBrightnessTo
                { 
                    if (segmentBrightnessFrom > (segmentBrightnessTo + a5_FadeStage)) {
                        *bufPtr++ = segmentBrightnessFrom - a5_FadeStage;
                    }
                    else
                    {*bufPtr++ = segmentBrightnessTo;} 
                }
                i--;
            }
            while (i > 0);
            
            a5_FadeStage++;
        }
        else { // i.e., if (InverseFadeStage == 0)
            // When we finish the fade, we load the original "to" buffer into OSB.
            
            do {
                *bufPtr++ = *toPtr++;
                i--;
            }
            while (i > 0);
            a5_FadeStage = -1;
        }
    }
}
 

void a5loadOSB_Ascii (char WordIn[], byte BrightIn)
{
    // Add five ascii characters at given brightness to the Off-Screen Buffer (OSB).
    // Note that this routine is strictly additive; it can be used for compositing and cross-fading.
    // Execution time: ~178 us total elapsed time, assuming that refresh interrupt is enabled.
    
    byte alphaPosTemp;
    byte letterByteTemp;
    byte segment = 0;
    byte i;
    byte j = 0;
    
    while (j < 5)
    {
        alphaPosTemp = 3 * (WordIn[4 - j] - a5_asciiOffset );
        letterByteTemp = a5_FontTable[alphaPosTemp++];
        
        i = 1;
        do {
            if (letterByteTemp & i)
            {
                a5_OSB[segment] += BrightIn;
            }
            segment++;
            i = i << 1;
        }
        while (i != 0);
        
        letterByteTemp = a5_FontTable[alphaPosTemp++];
        if (letterByteTemp & 1)
        {
            a5_OSB[segment] += BrightIn;
        }
        segment++ ;
        
        if (letterByteTemp & 2)
        {
            a5_OSB[segment] += BrightIn;
        }
        segment++ ;
        
        letterByteTemp = a5_FontTable[alphaPosTemp];
        i = 1;
        do {
            if (letterByteTemp & i)
            {
                a5_OSB[segment] += BrightIn;
            }
            segment++;
            i = i << 1;
        }
        while (i != 0); 
        j++;
    }
}




void a5loadOSB_DP (char WordIn[], byte BrightIn)
{
    /*
     Add decimal points at given brightness to the Off-Screen Buffer (a5_OSB).
     
     The decimal point string, "WordIn" describes which decimal points (if any) should be lit for each of the
     five alphanumeric LED displays, e.g., "_123_"
     
     Example: a5loadOSB_DP ("01230", 15);
     
     For each character in this string, a value '1' will light the lower DP, a value '2' will light the upper DP,
     and a value '3' will light both DPs. Any other character will not cause either of the LEDs to be lit.
     
     Note that this routine is strictly additive, so that it can be used for compositing and cross-fading.
     BUT, remember to CLEAR the OSB before using it and remember that the contents will not be visible until you load
     them into the video buffer.
     
     Execution time: ~24 us total elapsed time, assuming that refresh interrupt is enabled.
     */
    
    byte segment;
    char theLetter;
    byte j = 0;
    while (j < 5)
    {
        theLetter = WordIn[4 - j];
        segment = (j * 18) + 6;
        
        if ((theLetter == '1') || (theLetter == '3'))
        {
            a5_OSB[segment] += BrightIn;
        }
        
        if ((theLetter == '2') || (theLetter == '3'))
        {
            segment++;
            a5_OSB[segment] += BrightIn;
        }
        j++;
    }
}


void a5loadOSB_Segment (byte segment, byte BrightIn)
{
    /*
     Add brightness to an individual segment of the Off-Screen Buffer (a5_OSB).
     
     byte segment: Which segment to update
     byte BrightIn: brightness level to display
     
     Example: a5loadOSB_Segment (45, 19);
     
     Note that this routine is strictly additive, so that it can be used for compositing and cross-fading.
     BUT, remember to CLEAR the OSB before using it and remember that the contents will not be visible until you load
     them into the video buffer.
     
     */
    
    a5_OSB[segment] += BrightIn;
    
}




void a5clearVidBuf (void)
{ // Empty video buffer.
    // Execution time: ~84-113 us, assuming that refresh interrupt is enabled.
    // Note: Some sacrifices have been made for performance at the expense of readability.
    byte *Ptr = &a5_vidBuf[0];
    
    byte i = a5_VIDBUFLENGTH;
    do {
        *Ptr++ = 0;
        i--;
    }
    while (i != 0);
}





void a5clearOSB (void)
{ // Empty off-screen video buffer.
    // Execution time: ~74 us, assuming that refresh interrupt is enabled.
    
    byte i = 0;
    do {
        a5_OSB[i++] = 0;
    }
    while (i < a5_VIDBUFLENGTH);
}


void a5nightLight(byte Brightness)
{  // Might make a better macro than a function.
    OCR2B = Brightness;
}

 

byte a5GetButtons(void)
{
    return (~(PINB) & a5_BUTTONMASK);
}

byte a5CheckForRTC()
{ // Check for presence of RTC module.
    // send request to receive data (just a byte!) starting at register 0
    
    byte status = 0;
    Wire.beginTransmission(104); // 104 is DS3231 device address
    Wire.write(0); // start at register 0
    Wire.endTransmission();
    Wire.requestFrom(104, 1); // request one byte (seconds)
    
    int dummy;
    
    while(Wire.available())
    {
        status = 1;
        dummy = Wire.read(); // get seconds
    }
    return status;
} 

 
void a5writeEEPROM(byte address, byte value)
{
    // Disable interrupts while writing to EEPROM, to avoid
    // possible EEPROM corruption that can result from not doing so.
    
    byte oldSREG = SREG;
    cli();
    EEPROM.write(address, value);
    SREG = oldSREG;
}


// a5tone: Adapted from Arduino Tone library.
// A special case, for our available speaker pin, and using hardware PWM. :)
// frequency (in hertz) and duration (in milliseconds).
// Also, add zero-frequency case: Silent delay of specified period.

void a5tone(unsigned int frequency, unsigned long duration)
{
    long toggle_count = 0;
    uint32_t ocr = 0;
    byte TCCR1Atemp = 0;
    byte TCCR1Btemp = 0;
    
     if (frequency > 0) 
    {
        TCCR1Atemp = _BV(COM1A0);
    } 
    
    // Fixed scale for the 16 bit timer: ck/8 
    
    if (frequency > 15000L)
        frequency = 15000L;   // Max 15 kHz
    
    if (frequency < 16)
        frequency = 16;   // Min 16 Hz
    
    // ocr = F_CPU / frequency / 2 / 8 - 1; 
    ocr = 1000000L / frequency - 1;
     
    
    TCCR1Btemp = (_BV(WGM12) | _BV(CS11));
    
    // Calculate the toggle count
    if (duration > 0)
    {
        toggle_count = 2 * (frequency * duration / 1000); 
    }
    else
    {
        toggle_count = -1;
    }
    
    // Set the OCR for the given timer,
    // set the toggle count,
    // then turn on the interrupts
    
    TCCR1A = TCCR1Atemp;
    TCCR1B = TCCR1Btemp;
    OCR1A = ocr;
    
    a5_timer1_toggle_count = toggle_count;
    bitWrite(TIMSK1, OCIE1A, 1);
    
}


void a5noTone (void)
{
    PORTD |= 32; // Set the speaker's I/O pin high
    TCCR1A = 0;  // Disconnect timer from the I/O pin
    bitWrite(TIMSK1, OCIE1A, 0);  // Disable timer interrupt
}


ISR(TIMER1_COMPA_vect)
{
    if (a5_timer1_toggle_count != 0)
    { 
        if (a5_timer1_toggle_count > 0)
            a5_timer1_toggle_count--;
    }
    else
    { 
        PORTD |= 32; // Set the speaker's I/O pin high
        TCCR1A = 0;  // Disconnect timer from the I/O pin
        bitWrite(TIMSK1, OCIE1A, 0);  // Disable timer interrupt
    }
}
 


void a5Init (void)
{
    randomSeed(analogRead(0));
    
    PORTA = 95;             //PA0 - PA4 high: turn off chars 0-4. (#31), PA6 high:
    //         Blank all LED drivers (#64).  31 + 64 = 95.
    DDRA = 95;              // Outputs: PA0-PA5 ("rows") and Blank pin, PA6
    
    PORTB = a5_BUTTONMASK;   // Pull-up resistors for buttons
    PORTC = 3;              // PULL UPS for I2C
    PORTD |= _BV(5) ;        // Speaker location
    
    DDRB = _BV(4) | _BV(5) | _BV(7);    // SS, SCK, MOSI are outputs
    DDRC = _BV(2);                      // Latch pin, PC2
    DDRD = _BV(5) | _BV(6) | _BV(7);    //Speaker output (PD5), LED outputs on PD6, PD7
    
    //ENABLE SPI, MASTER, CLOCK RATE fck/4:
    SPCR = _BV(SPE) | _BV(MSTR);    // Initialize SPI, fast!
    SPSR |= 1;                      // enable double-speed mode!
    
    a5nightLight(0);            // Nightlight off
    a5clearVidBuf();            // Empty video buffer
    a5clearOSB();               // Empty off-screen buffer, too.
    a5loadVidBuf_fromOSB();
    
    Serial.begin(19200);   // Initialize serial port.  19200 baud default matches Alpha5 library examples.
    Serial1.begin(19200);  // Initialize serial port.  19200 baud default matches Alpha5 library examples.
    
#ifndef HybridScanMode
    a5_intensityStep = 1;  // Needs to start at 1, not (default initialization of) 0.
#endif
    
#ifdef HybridScanMode
    SubIntensity = 1;   // used only in HybridScanMode
    BaseIntensity = 0;  // used only in HybridScanMode
#endif
    
    a5_brightMode = 0;  // Initialize brightness mode variable
    a5_brightLevel = 1; // Initialize main brightness variable
    a5_FadeStage = -1;  // No fade underway
    
    
    
    TCCR2A = (_BV(WGM20) | _BV(COM2B1)); // PWM, phase correct, top: FF, overflow at bottom
    // Drive output OC2B (nightlight) with value OCR2B
    
    TCCR2B = (_BV(CS20)); // System clock w/o prescaler.
    // so overflow happens at 16 MHz/2*256 = 31.250 kHz

  //  TCCR2B = (_BV(CS21)); // System clock / 8, for overdrive mode

    
    TIMSK2 = (1<<TOIE2);	// Begin interrupt on timer overflow compare match
    
}


ISR(TIMER2_OVF_vect)
{
    /*
     Automatic refresh routine for 5-character alphanumeric LED display with 16 levels of grayscale,
     in either the high or low brightness mode.
     
     there are 15 passes through this interrupt for each row per frame.
     (15 * 5) = 75 times per frame.
     
     There are two basic brightness modes: low and high.  Only one may be active at a time.
     In the low brightness mode, the LEDs are on *only* during the interrupt.
     In the high brightness mode, the LEDs are on between the interrupts.
     
     There are 15 passes through this interrupt, so for *either high or low brightness*:
     if an LED is off for every step, the perceived brightness is 0/15
     if an LED is on for every step, the perceived brightness is 15/15
     giving a total of 16 brightness levels: 0 (unlit) plus 15 levels of nonzero brightness.
     
     The brightness of the two registers is tuned such that 15 brightness on the low register
     is *slightly under* 1 brightness on the high register.
     
     This interrupt executes 31250 times per second, every 32 us.
     
     Execution time: Typically 16-17 us; about 50% of CPU time.
     This routine has had some degree of optimization; it uses local copies of
     global variables and indirect array addressing to speed execution.
     Sacrifices have been made for performance at the expense of readability.
     */
    
    byte segment = a5_litChar * 18;
    byte *pointer = &a5_vidBuf[segment];
    
    byte Intensity = a5_intensityStep;  // Using a local variable actually saves a *huge* amount of time.
    
    byte bufferTemp = 0;
    byte bufferTemp2 = 0;
    byte bufferTemp3 = 0;
    byte PAbackup = PORTA | 95;
    
    PAbackup &= ~(1 << a5_litChar); // All pins allowed high except row driver.
    
    PORTA |= 95;   // Turn off LED driver and row (if previously in bright mode)
    
    if ( *pointer++ >= Intensity )
        bufferTemp  = 1;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 2;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 4;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 8;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 16;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 32;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 64;
    if ( *pointer++ >= Intensity )
        bufferTemp |= 128;
    
    
    SPDR = bufferTemp;    // Initiate SPI transmission of 1st byte
    
    if ( *pointer++ >= Intensity )
        bufferTemp2  = 1;
    if ( *pointer++ >= Intensity )
        bufferTemp2 |= 2;
    
    if ( *pointer++ >= Intensity )
        bufferTemp3  = 1;
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 2;
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 4;
    
    
    
    SPDR = bufferTemp2;    // Initiate SPI transmission of 2nd byte
    
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 8;
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 16;
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 32;
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 64;
    if ( *pointer++ >= Intensity )
        bufferTemp3 |= 128;
    
    
    
    bufferTemp = SPSR;  // CLEAR SPIF FLAG
    SPDR = bufferTemp3;    // Initiate SPI transmission of 3rd byte
    PAbackup &= 191;  // Prepare to enable LED driver (PA6 goes low).
    
    //  Iterate across intensity steps on the "inner" loop, and across characters more slowly.
    
    if (a5_brightMode == 0)
    {
        a5_intensityStep++;
        if (Intensity > 56)  // Extra-dim mode
        {
            a5_intensityStep = 1;
            a5_litChar++;
            
            if (a5_litChar > 4)
                a5_litChar = 0;
        }
        
        loop_until_bit_is_set(SPSR, SPIF) ;  //Wait for transmission of 3rd byte to complete
        
        PORTC |= 4;       // Latch shift registers
        PORTC &= 251;     //  End latch
        
        PORTA = PAbackup; //  Enable LED row (character)
        
        asm("nop;");
        asm("nop;");
        asm("nop;");
        asm("nop;");
        PORTA |= 95; // Turn off LED driver and row
    }
    else if (a5_brightMode == 1)
        
    {
        a5_intensityStep++;
        if (Intensity > 14)
        {
            a5_intensityStep = 1;
            a5_litChar++;
            
            if (a5_litChar > 4)
                a5_litChar = 0;
        }
        
        loop_until_bit_is_set(SPSR, SPIF) ;  //Wait for transmission of 3rd byte to complete
        
        PORTC |= 4;       // Latch shift registers
        PORTC &= 251;     //  End latch
        
        PORTA = PAbackup; //  Enable LED row (character)
        
        asm("nop;");
        asm("nop;");
        asm("nop;");
        asm("nop;");
        PORTA |= 95; // Turn off LED driver and row
    }
    else
    {
        a5_intensityStep++;
        if (Intensity > 14) //14
        {
            a5_intensityStep = 1;
            a5_litChar++;
            
            if (a5_litChar > 4)
                a5_litChar = 0;
        }
        
        loop_until_bit_is_set(SPSR, SPIF) ;  //Wait for transmission of 3rd byte to complete
        
        PORTC |= 4;       // Latch shift registers
        PORTC &= 251;     //  End latch
        
        PORTA = PAbackup; //  Enable LED row (character)
        
    }
    
    
    
    
}


//ISR(TIMER2_OVF_vect)  /* OVERDRIVE */
//{
//    /*
//     Automatic refresh routine for 5-character alphanumeric LED display w/o grayscale.
//     Slightly brighter, but less elegant.
//     */
//    
//    byte segment = a5_litChar * 18;
//    byte *pointer = &a5_vidBuf[segment];
//    
//    byte Intensity = a5_intensityStep;  // Using a local variable actually saves a *huge* amount of time.
//    
//    byte bufferTemp = 0;
//    byte bufferTemp2 = 0;
//    byte bufferTemp3 = 0;
//    byte PAbackup = PORTA | 95;
//    
//    PAbackup &= ~(1 << a5_litChar); // All pins allowed high except row driver.
//    
//    if( Intensity == 1)
//    PORTA |= 95;   // Turn off LED driver and row (if previously in bright mode)
//    
//    if ( *pointer++)
//        bufferTemp  = 1;
//    if ( *pointer++)
//        bufferTemp |= 2;
//    if ( *pointer++)
//        bufferTemp |= 4;
//    if ( *pointer++)
//        bufferTemp |= 8;
//    if ( *pointer++)
//        bufferTemp |= 16;
//    if ( *pointer++)
//        bufferTemp |= 32;
//    if ( *pointer++)
//        bufferTemp |= 64;
//    if ( *pointer++ )
//        bufferTemp |= 128;
//    
//   if( Intensity == 1)
//    SPDR = bufferTemp;    // Initiate SPI transmission of 1st byte
//    
//    if ( *pointer++)
//        bufferTemp2  = 1;
//    if ( *pointer++)
//        bufferTemp2 |= 2;
//    
//    if ( *pointer++)
//        bufferTemp3  = 1;
//    if ( *pointer++)
//        bufferTemp3 |= 2;
//    if ( *pointer++)
//        bufferTemp3 |= 4;
//    
//    
//  if( Intensity == 1)
//    SPDR = bufferTemp2;    // Initiate SPI transmission of 2nd byte
//    
//    if ( *pointer++)
//        bufferTemp3 |= 8;
//    if ( *pointer++)
//        bufferTemp3 |= 16;
//    if ( *pointer++)
//        bufferTemp3 |= 32;
//    if ( *pointer++)
//        bufferTemp3 |= 64;
//    if ( *pointer++)
//        bufferTemp3 |= 128;
//    
//    bufferTemp = SPSR;  // CLEAR SPIF FLAG
//    if( Intensity == 1)
//    SPDR = bufferTemp3;    // Initiate SPI transmission of 3rd byte
//    PAbackup &= 191;  // Prepare to enable LED driver (PA6 goes low).
//    
//    //  Iterate across intensity steps on the "inner" loop, and across characters more slowly.
//    {
//        a5_intensityStep++;
//        if (Intensity > 2) //14
//        {
//            a5_intensityStep = 1;
//            a5_litChar++;
//            
//            if (a5_litChar > 4)
//                a5_litChar = 0;
//        }
//        
//        if( Intensity == 1)
//        {
//        loop_until_bit_is_set(SPSR, SPIF) ;  //Wait for transmission of 3rd byte to complete
//        
//        PORTC |= 4;       // Latch shift registers
//        PORTC &= 251;     //  End latch
//        
//        PORTA = PAbackup; //  Enable LED row (character)
//        }
//    } 
//}


