/*
defaultfirmware.ino 
 
 -- Alpha Clock Five Firmware, version 2.1 --
 
 Version 2.1.0 - January 31, 2013
 Copyright (c) 2013 Windell H. Oskay.  All right reserved.
 http://www.evilmadscientist.com/
 
 ------------------------------------------------------------
 
 Designed for Alpha Clock Five, a five letter word clock designed by
 Evil Mad Scientist Laboratories http://www.evilmadscientist.com
 
 Target: ATmega644A, clock at 16 MHz.
 
 Designed to work with Arduino 1.0.3; untested with other versions.
 
 For additional requrements, please see:
 http://wiki.evilmadscience.com/Alpha_Clock_Firmware_v2
 
 
 Thanks to Trammell Hudson for inspiration and helpful discussion.
 https://bitbucket.org/hudson/alphaclock
 
 
 Thanks to William Phelps - wm (at) usa.net, for several important 
 bug fixes.    https://github.com/wbphelps/AlphaClock
 
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
 
 
 
 Note that the two word lists included with this distribution are NOT licensed under the GPL.
 - The list in fiveletterwords.h is derived from SCOWL, http://wordlist.sourceforge.net 
 Please see README-SCOWL.txt for copyright restrictions on the use and redistribution of this word list.
 - The alternative list in fiveletterwordspd.h is in the PUBLIC DOMAIN, 
 and cannot be restricted by the GPL or other copyright licenses.    
 	  
 */


#include "alphafive.h"      // Alpha Clock Five library

// Comment out exactly one of the following two lines
#include "fiveletterwords.h"   // Standard word list --
//#include "fiveletterwordspd.h" // Public domain alternative


#include <Time.h>       // The Arduino Time library, http://www.arduino.cc/playground/Code/Time
#include <Wire.h>       // For optional RTC module
#include <DS1307RTC.h>  // For optional RTC module. (This library included with the Arduino Time library)
#include <EEPROM.h>     // For saving settings 

// "Factory" default configuration can be configured here:
#define a5brightLevelDefault 9 
#define a5HourMode24Default 0
#define a5AlarmEnabledDefault 0
#define a5AlarmHrDefault 7  
#define a5AlarmMinDefault 30
#define a5NightLightTypeDefault 0
#define a5AlarmToneDefault 2
#define a5NumberCharSetDefault 2;
#define a5DisplayModeDefault 0;

// Clock mode variables

byte HourMode24;
byte AlarmEnabled; // If the "ALARM" function is currently turned on or off. 
byte AlarmTimeHr;
byte AlarmTimeMin;
int8_t AlarmTone;

int8_t NightLightType;  
byte NightLightSign;
unsigned int NightLightStep; 



// Configuration menu:
byte menuItem;   //Current position within options menu
int8_t optionValue; 
#define MenuItemsMax 10

#define AMPM24HRMenuItem 0
#define NightLightMenuItem 1
#define AlarmToneMenuItem 2
#define SoundTestMenuItem 3
#define numberCharSetMenuItem 4
#define DisplayStyleMenuItem 5
#define SetYearMenuItem 6
#define SetMonthMenuItem 7
#define SetDayMenuItem 8
#define SetSecondsMenuItem 9 
#define AltModeMenuItem 10 



// Clock display mode:
int8_t DisplayMode;   
int8_t DisplayModeLocalLast;
byte DisplayModePhase;
byte DisplayModePhaseCount;
byte VCRmode;
byte modeShowMenu;
byte modeShowDateViaButtons;
byte modeLEDTest;
byte UpdateEE; 
int8_t numberCharSet;


// Other global variables:
byte UseRTC;
unsigned long NextClockUpdate, NextAlarmCheck;
unsigned long milliTemp;
unsigned int FLWoffset; // Counter variable for FLW (Five Letter Word) display mode

// Text Display Variables: 
unsigned long DisplayWordEndTime;
char wordCache[5];
char dpCache[5];

byte wordSequence;
byte wordSequenceStep;
byte modeShowText;


byte RedrawNow, RedrawNow_NoFade;


// Button Management:
#define ButtonCheckInterval 20    // Time delay between responding to button state, ms
#define HoldDownTime 2000         // How long to hold buttons to acces smenus requiring holding two buttons
byte buttonStateLast;
byte buttonMonitor;
unsigned long Btn1_AlrmSet_StartTime, Btn2_TimeSet_StartTime, Btn3_Plus_StartTime, Btn4_Minus_StartTime;
unsigned long NextButtonCheck, LastButtonPress;


byte UpdateAlarmState, UpdateBrightness;
byte AlarmTimeChanged, TimeChanged;
byte holdDebounce;


// Brightness steps for manual brightness adjustment
byte Brightness;
#define BrightnessMax 11
byte MBlevel[] = {
  0, 1, 5,10,15,19,15,19, 5,10,15,19}; 
byte MBmode[]  = {
  0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2};

// For fade and update management: 
byte SecLast; 
byte MinNowOnesLast;
byte MinAlarmOnesLast;


//Alarm variables
byte AlarmTimeSnoozeMin;
byte AlarmTimeSnoozeHr;
byte snoozed;
byte alarmPrimed;
byte alarmNow;

byte modeShowAlarmTime;
byte SoundSequence; 

void incrementAlarm(void)
{  // Advance alarm time by one minute 

  AlarmTimeMin += 1;
  if (AlarmTimeMin > 59)
  {
    AlarmTimeMin = 0;
    AlarmTimeHr += 1; 
    if (AlarmTimeHr > 23)
      AlarmTimeHr = 0; 
  } 
  UpdateEE = 1;
}

void decrementAlarm(void)
{  // Retard alarm time by one minute 

  if (AlarmTimeMin > 0)
    AlarmTimeMin--;
  else
  {   
    AlarmTimeMin = 59;
    if (AlarmTimeHr > 0)
      AlarmTimeHr--;
    else
      AlarmTimeHr = 23;
  }
  UpdateEE = 1;
}

void TurnOffAlarm(void)
{ // This cancels the alarm when it is going off (or snoozed).
  // It does leave the alarm enabled for next time, however.
  if (alarmNow || snoozed){
    snoozed = 0;
    alarmNow = 0;
    a5noTone();

    if (modeShowMenu == 0) 
      DisplayWordSequence(2); // Display: "ALARM OFF", EXCEPT if we are in the menus.
  }
}  

void checkButtons(void )
{ 
  buttonMonitor |= a5GetButtons(); 

  if (milliTemp  >=  NextButtonCheck)  // Typically, go through this every 20 ms.
  {
    NextButtonCheck = milliTemp + ButtonCheckInterval;
    /*
     #define a5alarmSetBtn  1				// Snooze/Set alarm button
     #define a5timeSetBtn   2				// Set time button
     #define a5plusBtn      4				// + button
     #define a5minusBtn     8				// - button
     */

    if (buttonMonitor){  // If any buttons have been down in last ButtonCheckInterval

      if (VCRmode)
        EndVCRmode();  // Turn off VCR-blink mode, if it was still on.


      // Check to see if any of the buttons has JUST been depressed: 

      if (( buttonMonitor & a5_alarmSetBtn) && ((buttonStateLast & a5_alarmSetBtn) == 0))
      {  // If Alarm Set button was just depressed

        Btn1_AlrmSet_StartTime = milliTemp;  

        if (alarmNow){  // If alarm is going off, this button is the SNOOZE button.

          if (modeShowMenu)
          {
            TurnOffAlarm();
          }
          else{
            alarmNow = 0;
            a5noTone();
            snoozed = 1; 

            a5editFontChar ('a', 54, 1, 37);    // Define special character
            DisplayWord ("SNaZE", 1500);  

            AlarmTimeSnoozeMin = minute() + 9;
            AlarmTimeSnoozeHr = hour();

            if  ( AlarmTimeSnoozeMin > 59){
              AlarmTimeSnoozeMin -= 60;
              AlarmTimeSnoozeHr += 1;
            }
            if (AlarmTimeSnoozeHr > 23)
              AlarmTimeSnoozeHr -= 24; 
          }

        } 
      }

      if (( buttonMonitor & a5_timeSetBtn) && ((buttonStateLast & a5_timeSetBtn) == 0)){
        // Button S2 just depressed.
        Btn2_TimeSet_StartTime = milliTemp; 
        TimeChanged = 0;
      }
      if (( buttonMonitor & a5_plusBtn) && ((buttonStateLast & a5_plusBtn) == 0))
        Btn3_Plus_StartTime = milliTemp; 
      if (( buttonMonitor & a5_minusBtn) && ((buttonStateLast & a5_minusBtn) == 0)) 
        Btn4_Minus_StartTime = milliTemp;  
    }
    else if ((buttonStateLast == 0 ) && ( buttonMonitor == 0))
    {
      // Reset some variables if all buttons are up, and have not just been released:
      TimeChanged = 0;
      AlarmTimeChanged = 0;
      holdDebounce = 1;  
    }

    if (modeShowMenu || buttonMonitor)
      LastButtonPress = milliTemp;  //Reset EEPROM Save Timer if menu shown, or if a button pressed.

    if (modeShowMenu && holdDebounce){    // Button behavior, when in Config menu mode:

      // Check to see if AlarmSet button was JUST released::
      if ( ((buttonMonitor & a5_alarmSetBtn) == 0) && (buttonStateLast & a5_alarmSetBtn))
      {  
        if ( menuItem > 0)   //Decrement current position within options menu
          menuItem--;
        else
          menuItem = MenuItemsMax;  // Wrap-around at low-end of menu
        optionValue = 0;
        TurnOffAlarm();
        DisplayMenuOptionName();
      }

      // If TimeSet button was just released::
      if (( (buttonMonitor & a5_timeSetBtn) == 0) && (buttonStateLast & a5_timeSetBtn))
      {
        if ((alarmNow) || (snoozed)){  // Just Turn Off Alarm 
          TurnOffAlarm();
        } 
        else {  // If we are in a configuration menu
          menuItem++;          
          if ( menuItem > MenuItemsMax)   //Decrement current position within options menu
            menuItem = 0;  // Wrap-around at high-end of menu
          optionValue = 0;
          TurnOffAlarm();
          DisplayMenuOptionName();
        }
      }

      if (( (buttonMonitor & a5_plusBtn) == 0) && (buttonStateLast & a5_plusBtn))
      { // The "+" button has just been released.
        optionValue = 1; 
        UpdateEE = 1;
        RedrawNow = 1;
      }

      if (( (buttonMonitor & a5_minusBtn) == 0) && (buttonStateLast & a5_minusBtn)) 
      {  // The "-" Button has just been released.
        optionValue = -1; 
        UpdateEE = 1;
        RedrawNow = 1;
      }
    }
    else
    {   // Button behavior, when NOT in Config menu mode: 

      /////////////////////////////  Time-Of-Day Adjustments  /////////////////////////////  

      // Check to see if both time-set button and plus button are both currently depressed:

      if (( buttonMonitor & a5_TimeSetPlusBtns) == a5_TimeSetPlusBtns) 
        if (TimeChanged < 2)
        {        
          adjustTime(60); // Add one minute   
          RedrawNow = 1; 
          TimeChanged = 2;  // One-time press: detected
          if (UseRTC)  
            RTC.set(now()); 
        }    
        else if ( milliTemp >= (Btn3_Plus_StartTime + 400))
        {
          adjustTime(60); // Add one minute 
          RedrawNow_NoFade = 1; 
          if (UseRTC)  
            RTC.set(now());      
        }

      // Check to see if both time-set button and minus button are both currently depressed:
      if (( buttonMonitor & a5_TimeSetMinusBtns) == a5_TimeSetMinusBtns) 
        if (TimeChanged < 2)
        {        
          adjustTime(-60); // Subtract one minute 
          RedrawNow = 1; 
          TimeChanged = 2;
          if (UseRTC)  
            RTC.set(now()); 
        }      
        else if ( milliTemp > (Btn4_Minus_StartTime + 400 ))
        {
          adjustTime(-60); // Subtract one minute 
          RedrawNow_NoFade = 1; 
          //          TimeChanged = 1;    
          if (UseRTC)  
            RTC.set(now());      
        }

      /////////////////////////////  Time-Of-Alarm Adjustments  /////////////////////////////  

      // Entering alarm mode:
      // If Alarm button has been down 40 ms, 
      //    (to avoid displaying alarm if Alarm+Time buttons are pressed at the same time)
      //    the Set Time button is not down,
      //    and no other high-priority modes are enabled...


      if (( buttonMonitor & a5_alarmSetBtn) && (modeShowAlarmTime == 0))
        if ((( buttonMonitor & a5_timeSetBtn) == 0) && (modeShowText == 0))
          if ( milliTemp >= (Btn1_AlrmSet_StartTime + 40 ))  // of those "ifs," Check hold-time LAST.
          {
            modeShowAlarmTime = 1;
            RedrawNow = 1; 
            AlarmTimeChanged = 0; 
          }

      // Check to see if both alarm-set button and plus button are both currently depressed:
      if (( buttonMonitor & a5_AlarmSetPlusBtns) == a5_AlarmSetPlusBtns)  
        if (TimeChanged < 2)
        {        
          incrementAlarm(); // Add one minute
          RedrawNow = 1; 
          TimeChanged = 2;  // One-time press: detected
          snoozed = 0;  //  Recalculating alarm time *turns snooze off.*
        }       
        else if ( milliTemp >= (Btn3_Plus_StartTime + 400))
        {
          incrementAlarm(); // Add one minute
          RedrawNow_NoFade = 1; 
          //          TimeChanged = 1;         
        }      

      // Check to see if both alarm-set button and minus button are both currently depressed:
      if (( buttonMonitor & a5_AlarmSetMinusBtns) == a5_AlarmSetMinusBtns) 
        if (TimeChanged < 2)
        {        
          decrementAlarm(); // Subtract one minute
          RedrawNow = 1; 
          TimeChanged = 2; // One-time press: detected
          snoozed = 0;  //  Recalculating alarm time *turns snooze off.*
        }      
        else if ( milliTemp >  (Btn4_Minus_StartTime + 400))
        {
          decrementAlarm(); // Subtract one minute
          RedrawNow_NoFade = 1; 
          //          TimeChanged = 1;         
        }


      // Check to see if both S1 and S2 are both currently depressed:
      if (( buttonMonitor & a5_alarmSetBtn) && ( buttonMonitor & a5_timeSetBtn))
      {  
        if (modeShowDateViaButtons == 0)
        { // Display date
          modeShowDateViaButtons = 1;
          TimeChanged  = 1; // This overrides the usual alarm on/off function of the time set button. 
          RedrawNow = 1; 
        }
      } 



      /////////////////////////////  ENTERING & LEAVING LED TEST MODE  /////////////////////////////  
      // Check to see if both S1 and S2 are both currently held down:
      if (( buttonMonitor & a5_alarmSetBtn) && ( buttonMonitor & a5_timeSetBtn))
      {
        if( (milliTemp >= (Btn1_AlrmSet_StartTime + 2 * HoldDownTime )) && (milliTemp >= (Btn2_TimeSet_StartTime + 2 * HoldDownTime )))
        {
          Btn1_AlrmSet_StartTime = milliTemp;  // Reset hold-down timer
          Btn2_TimeSet_StartTime = milliTemp;   // Reset hold-down timer
          holdDebounce = 0;
          if (modeLEDTest) // If we are currently in the LED Test mode,
          {
            modeLEDTest = 0;  //  Exit LED Test Mode    
            RedrawNow = 1; 
            DisplayWord ("-END-", 1500);
          }
          else
          { 
            // Display version and enter LED Test Mode
            modeLEDTest = 1;
            DisplayWordSequence(5); 
            SoundSequence = 0;
          }
        }
      }


      // Check to see if AlarmSet button was JUST released::
      if ( ((buttonMonitor & a5_alarmSetBtn) == 0) && (buttonStateLast & a5_alarmSetBtn))
      {  
        if (modeShowAlarmTime && holdDebounce){  
          modeShowAlarmTime = 0; 
          RedrawNow = 1;
        } 

        if (modeShowDateViaButtons == 1)
        {
          modeShowDateViaButtons = 0;
          RedrawNow = 1; 
        } 
      }


      // If TimeSet button was just released::
      if (( (buttonMonitor & a5_timeSetBtn) == 0) && (buttonStateLast & a5_timeSetBtn))
      {
        if (holdDebounce)
        { 
          if ((alarmNow) || (snoozed)){  // Just Turn Off Alarm 
            TurnOffAlarm();
          } 
          else if (TimeChanged == 0){  // If the time has just been adjusted, DO NOT change alarm status.
            RedrawNow = 1;
            UpdateEE = 1;
            if (AlarmEnabled)
              AlarmEnabled = 0; 
            else
            {
              AlarmEnabled = 1; 
            } 
          }
          else
          {
            if (UseRTC)  
              RTC.set(now()); 
          }
        }

      }


      if (( (buttonMonitor & a5_plusBtn) == 0) && (buttonStateLast & a5_plusBtn))
      { // The "+" button has just been released.
        if (holdDebounce)
        { 
          if (TimeChanged > 0)
            TimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 
          if (AlarmTimeChanged > 0)
            AlarmTimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 

          // IF no other buttons are down, increase brightness:
          if (((buttonMonitor & a5_allButtonsButPlus) == 0) && (AlarmTimeChanged + TimeChanged == 0))
            if (Brightness < BrightnessMax)
            {
              Brightness++; 
              UpdateBrightness = 1;
              UpdateEE = 1;
            } 
        }
      }

      if (( (buttonMonitor & a5_minusBtn) == 0) && (buttonStateLast & a5_minusBtn)) 
      {  // The "-" Button has just been released.
        if (holdDebounce){
          if (TimeChanged > 0)
            TimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 
          if (AlarmTimeChanged > 0)
            AlarmTimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 

          // IF no other buttons are down, and times have not been adjusted, decrease brightness:
          if(((buttonMonitor & a5_allButtonsButMinus) == 0) && (AlarmTimeChanged + TimeChanged == 0))
            if (Brightness > 0)
            {
              Brightness--; 
              UpdateBrightness = 1; 
              UpdateEE = 1;
            }  
        }
      }
    } // End not-in-config-menu statements



    /////////////////////////////  ENTERING & LEAVING CONFIG MENU  /////////////////////////////  

    // Check to see if both S3 and S4 are both currently held down:
    if (( buttonMonitor & a5_plusBtn) && ( buttonMonitor & a5_minusBtn))
    {

      if( (milliTemp >= (Btn3_Plus_StartTime + HoldDownTime )) && (milliTemp >= (Btn4_Minus_StartTime + HoldDownTime )))
      {  
        Btn3_Plus_StartTime = milliTemp;     // Reset hold-down timer
        Btn4_Minus_StartTime = milliTemp;    // Reset hold-down timer
        holdDebounce = 0;
        TurnOffAlarm();
        if (modeShowMenu) // If we are currently in the configuration menu, 
        { 
          modeShowMenu = 0;  //  Exit configuration menu     
          DisplayWord ("     ", 500); 
        }
        else
        {
          modeShowMenu = 1;  // Enter configuration menu 
          menuItem = 0; 
          DisplayWord ("     ", 500); 
        }

      }
    }

    buttonStateLast = buttonMonitor;
    buttonMonitor = 0;
  }
}


void DisplayMenuOptionName(void){
  // Display title of menu name after switching to new menu utem.

  switch (menuItem) {
  case NightLightMenuItem:
    DisplayWordSequence(4);  // Night Light
    break;
  case AlarmToneMenuItem:
    DisplayWordSequence(6);  // Alarm Tone
    break; 
  case SoundTestMenuItem:
    DisplayWordSequence(3); // Sound-test menu item, 3.  Display "TEST" "SOUND" "USE+-"
    break;
  case numberCharSetMenuItem:
    DisplayWordSequence(7); // Font Style
    break;
  case DisplayStyleMenuItem:
    DisplayWordSequence(8); // Clock Style
    break;
  case SetYearMenuItem:
    DisplayWord ("YEAR ", 800); 
    DisplayWordDP("___12");
    break;
  case SetMonthMenuItem:
    DisplayWord ("MONTH", 800);   
    break;      
  case SetDayMenuItem:
    DisplayWord ("DAY  ", 800);  
    DisplayWordDP("__12_");
    break;       
  case SetSecondsMenuItem:
    DisplayWord ("SECS ", 800);  
    DisplayWordDP("___12");
    break;   
  case AltModeMenuItem:  
    DisplayWordSequence(9); // "TIME AND..."
    //    DisplayWord ("ALTW/", 2000);  
    //   DisplayWordDP("__11_");
    break; 
  default:  // do nothing!
    break;
  }
}




void ManageAlarm (void) {

  if ((SoundSequence == 0) && (modeShowMenu == 0))
    DisplayWord ("ALARM", 400);  // Synchronize with sounds!  
  //RedrawNow_NoFade = 1;

  if ( (TIMSK1 & _BV(OCIE1A)) == 0)  { // If last tone has finished   

    if (AlarmTone == 0)   // X-Low Tone
    {
      if (SoundSequence < 8)
      {
        if (SoundSequence & 1) 
          a5tone( 50, 300);  
        else 
          a5tone(0, 300);  
        SoundSequence++;    
      }
      else
      { 
        a5tone(0, 1200);
        SoundSequence = 0;
      }
    }
    else if (AlarmTone == 1)   // Low Tone
    {
      if (SoundSequence < 8)
      {
        if (SoundSequence & 1) 
          a5tone( 100, 200);  
        else 
          a5tone(0, 200);  
        SoundSequence++;    
      }
      else
      { 
        a5tone(0, 1200);
        SoundSequence = 0;
      }
    }
    else  if (AlarmTone == 2) // Med Tone
    {
      if (SoundSequence < 6)
      {
        if (SoundSequence & 1) 
          a5tone( 1000, 200); 
        else 
          a5tone( 0, 200); 
        SoundSequence++;   
      }
      else
      { 
        a5tone( 0, 1400);
        SoundSequence = 0; 
      }
    }
    else  if (AlarmTone == 3) // High Tone
    {
      if (SoundSequence < 6) 
      {
        if (SoundSequence & 1) 
          a5tone( 2050, 300); 
        else 
          a5tone( 0, 200); 
        SoundSequence++;   
      }
      else
      { 
        a5tone(0, 1000);
        SoundSequence = 0;  
      }
    }
    else if (AlarmTone == 4)   // Siren Tone
    { 
      if (SoundSequence < 254)
      { 
        a5tone(20 + 4 * SoundSequence, 2); 
        SoundSequence++; 
      }
      else if (SoundSequence == 254)
      { 
        a5tone(20 + 4 * SoundSequence, 1500); 
        SoundSequence++;
      } 
      else {
        a5tone(0, 1000);
        SoundSequence = 0; 
      } 
    }
    else if (AlarmTone == 5)   // "Tink" Tone
    {
      if (SoundSequence == 0)
      {
        a5tone( 1000, 50);  // was 50 
        SoundSequence++;
      } 
      else  if (SoundSequence == 1)
      { 
        a5tone(0, 1900);
        SoundSequence++;
      }
      else
      { 
        a5tone(0, 50);
        SoundSequence = 0;  
      }
    } 
  }
}





void DisplayWordSequence (byte sequence)
{  // Usage:  // DisplayWordSequence(1); // displays "HELLO" "WORLD"

  if (sequence != wordSequence)
  {
    wordSequence = sequence;
    wordSequenceStep = 0;
  }

  DisplayWordDP("_____"); // Blank DPs unless stated otherwise.
  wordSequenceStep++;

  switch (sequence) {
  case 1:     //Display "HELLO" "WORLD"
    if (wordSequenceStep == 1)
      DisplayWord ("HELLO", 800);
    else if (wordSequenceStep == 3)
      DisplayWord ("WORLD", 800);
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 300);
    else
      wordSequence = 0;
    break;
  case 2:  //Display "ALARM" " OFF "
    if (wordSequenceStep == 1)
      DisplayWord ("ALARM", 800);
    else if (wordSequenceStep == 3)
      DisplayWord (" OFF ", 800);
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 100);
    else
      wordSequence = 0;
    break;
  case 3:  //Display "TEST" "SOUND" "USE+-"
    if (wordSequenceStep == 1)
      DisplayWord ("TEST ", 600); 
    else if (wordSequenceStep == 3)
      DisplayWord ("SOUND", 600);
    else if (wordSequenceStep == 5)
      DisplayWord ("USE+-", 600);
    else if (wordSequenceStep < 7)
      DisplayWord ("     ", 200);
    else
      wordSequence = 0;
    break;
  case 4: //Display "NIGHT" "LIGHT"
    if (wordSequenceStep == 1)
      DisplayWord ("NIGHT", 600); 
    else if (wordSequenceStep == 3)
      DisplayWord ("LIGHT", 600);
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 100);
    else
      wordSequence = 0;
    break; 
  case 5: //Display "VER21" " LED " "TEST "  // Display software version number, 2.1

    if (wordSequenceStep == 1){
      DisplayWord ("VER21", 2000);
      DisplayWordDP("___1_");
    }
    else if (wordSequenceStep == 3)
      DisplayWord (" LED ", 1000);
    else if (wordSequenceStep == 5)
      DisplayWord ("TEST ", 1000);
    else if (wordSequenceStep < 7)
      DisplayWord ("     ", 200); 
    else
      wordSequence = 0; 
    break;
  case 6:     //Display "ALARM" "TONE"
    if (wordSequenceStep == 1)
      DisplayWord ("ALARM", 700);
    else if (wordSequenceStep == 3)
      DisplayWord (" TONE", 700);
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 100);
    else
      wordSequence = 0;
    break;

  case 7:     //Display "FONT " "STYLE"
    if (wordSequenceStep == 1)
      DisplayWord ("FONT ", 700);
    else if (wordSequenceStep == 3)
      DisplayWord ("STYLE", 700);
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 100);
    else
      wordSequence = 0;
    break;

  case 8:     //Display "CLOCK" "STYLE"
    if (wordSequenceStep == 1)
      DisplayWord ("CLOCK", 700);
    else if (wordSequenceStep == 3)
      DisplayWord ("STYLE", 700);
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 100);
    else
      wordSequence = 0;
    break;    
  case 9:     //Display "TIME" "AND..."
    if (wordSequenceStep == 1)
      DisplayWord ("TIME ", 900); 
    else if (wordSequenceStep == 3){ 
      DisplayWord ("AND  ", 900);
      DisplayWordDP("__111"); 
    }
    else if (wordSequenceStep < 5)
      DisplayWord ("     ", 100);
    else
      wordSequence = 0;
    break;    




  default: 
    // Turn off word sequences. (Catches case 0.)
    wordSequence = 0;
    wordSequenceStep = 0;
  }
}



void DisplayWord (char WordIn[], unsigned int duration)
{ // Usage: DisplayWord ("ALARM", 500); 

  modeShowText = 1;  
  wordCache[0] = WordIn[0];
  wordCache[1] = WordIn[1];
  wordCache[2] = WordIn[2];
  wordCache[3] = WordIn[3];
  wordCache[4] = WordIn[4];

  DisplayWordEndTime = milliTemp + duration;
  RedrawNow = 1; 
}

void DisplayWordDP (char WordIn[])
{
  // Usage: DisplayWord ("_123_"); 
  // Add or edit decimals for text displayed via DisplayWord().
  // Call in conjuction with DisplayWord, just before or after.

  dpCache[0] = WordIn[0];
  dpCache[1] = WordIn[1];
  dpCache[2] = WordIn[2];
  dpCache[3] = WordIn[3];
  dpCache[4] = WordIn[4];
}





void  EndVCRmode(){ 
  if (VCRmode){
    a5_brightLevel = MBlevel[Brightness];  
    RedrawNow_NoFade = 1;
    VCRmode = 0;

    randomSeed(now());  // Either a button press or RTC time 
  }
}



void setup() {     

  a5Init();  // Required hardware init for Alpha Clock Five library functions

  VCRmode = 1;

  Serial.println("\nHello, World.");
  Serial.println("Alpha Clock Five here, reporting for duty!");   

  EEReadSettings(); // Read settings stored in EEPROM

  if (Brightness == 0)
    Brightness = 1;       // If display is fully dark at reset, turn it up to minimum brightness.

  UseRTC = a5CheckForRTC();
  if (UseRTC)   
  {
    setSyncProvider(RTC.get);   // Function to get the time from the RTC (e.g., Chronodot)
    if(timeStatus()!= timeSet) { 
      Serial.println("RTC detected, *but* I can't seem to sync to it. ;("); 
      UseRTC = 0;
    }
    else {
      Serial.println("System time: Set by RTC.  Rock on!");  
      EndVCRmode();
    }
  }
  else
    Serial.println("RTC not detected. I don't know what time it is.  :(");  

  if ( UseRTC == 0)
  {
    Serial.println("Setting the date to 2013. I didn't exist in 1970.");   
    setTime(0,0,0,1, 1, 2013);
  }

  SerialPrintTime(); 
  NextClockUpdate = millis() + 1;

  buttonMonitor = 0;  
  holdDebounce = 0;

  modeShowAlarmTime = 0;
  modeShowDateViaButtons = 0; // for button-press date display
  modeShowMenu = 0;
  modeShowText = 0;
  modeLEDTest = 0;

  // Alarm Setup:
  snoozed = 0;
  alarmPrimed = 0;
  alarmNow = 0; 
  SoundSequence = 0; 

  NextButtonCheck = NextClockUpdate;
  NextAlarmCheck =  NextClockUpdate;

  UpdateEE = 0;
  LastButtonPress = NextClockUpdate;

  wordSequence = 0;
  wordSequenceStep = 0;

  RedrawNow = 1;
  RedrawNow_NoFade = 0;
  UpdateBrightness = 0;



  DisplayWordSequence(1);  // Display: Hello world

  buttonMonitor = a5GetButtons(); 
  if (( buttonMonitor & a5_alarmSetBtn) && ( buttonMonitor & a5_timeSetBtn))
  {
    // If Alarm button and Time button (LED Test buttons) held down at turn on, reset to defaults.

    Brightness = a5brightLevelDefault; 
    HourMode24 = a5HourMode24Default; 
    AlarmEnabled = a5AlarmEnabledDefault; 
    AlarmTimeHr = a5AlarmHrDefault; 
    AlarmTimeMin = a5AlarmMinDefault; 
    AlarmTone = a5AlarmToneDefault; 
    NightLightType = a5NightLightTypeDefault;   
    numberCharSet = a5NumberCharSetDefault; 
    DisplayMode = a5DisplayModeDefault;       

    wordSequenceStep = 0;
    DisplayWord ("*****", 1000); 
  }


  a5_brightLevel = MBlevel[Brightness];
  a5_brightMode = MBmode[Brightness]; 

  a5loadAltNumbers(numberCharSet);

  FLWoffset = 0;

  NightLightSign = 1;
  NightLightStep = 0; 
  updateNightLight();

  DisplayModePhase = 0;
  DisplayModePhaseCount = 0;

}

void loop() {

  milliTemp = millis();
  checkButtons();

  if (UpdateBrightness)
  {
    UpdateBrightness = 0;  // Reset the flag that triggered this clause.

    if (a5_brightMode == MBmode[Brightness])
    {
      a5_brightLevel = MBlevel[Brightness];          
      UpdateDisplay (1); // Force update of display data, with new brightness level 
    }
    else
    {
      a5_brightLevel = 0;
      UpdateDisplay (1); // Force update of display data, with temporary brightness level
      a5loadVidBuf_fromOSB();

      a5_brightLevel = MBlevel[Brightness];          
      UpdateDisplay (1); // Force update of display data, with new brightness level  
      a5_brightMode = MBmode[Brightness];  
    } 
  }

  if (VCRmode) 
  {
    if (modeShowText == 0){
      byte temp = second() & 1; 
      if((temp) && (VCRmode == 1))
      {
        a5_brightLevel = 0;  
        RedrawNow_NoFade = 1;
        VCRmode = 2;
      }
      if((temp == 0) && (VCRmode == 2))
      {
        a5_brightLevel = MBlevel[Brightness];  
        RedrawNow_NoFade = 1;
        VCRmode = 1;
      }
    }
  }


  if (RedrawNow || RedrawNow_NoFade)
  { 
    NextClockUpdate = milliTemp + 10; // Reset auto-redraw timer.

    UpdateDisplay (1);   // Force redraw
    if (RedrawNow_NoFade)   // Explicitly do not fade.  Takes priority over redraw with fade.
      a5_FadeStage = -1;
    a5LoadNextFadeStage(); 
    a5loadVidBuf_fromOSB(); 

    RedrawNow = 0;
    RedrawNow_NoFade = 0;
  }  
  else if (milliTemp >= NextClockUpdate)  // Update at most 100 times per second
  {  
    NextClockUpdate = milliTemp + 10; // Reset auto-redraw timer.
    UpdateDisplay (0); // Argument 0: Only update if display data has changed.
    a5LoadNextFadeStage();
    a5loadVidBuf_fromOSB(); 

    if (NightLightType >= 4)  // Only in pulse mode do we need to regularly update
      updateNightLight();

    if (UpdateEE)   // Don't need to check this more than 100 times/second.
      EESaveSettings();





  }

  // Check for alarm:
  if  (milliTemp >= NextAlarmCheck)
  {
    NextAlarmCheck = milliTemp +  500;  // Check again in 1/2 second. 

    if (AlarmEnabled)  {
      byte hourTemp = hour();
      byte minTemp = minute();

      if ((AlarmTimeHr == hourTemp ) && (AlarmTimeMin == minTemp ))
      {
        if (alarmPrimed){ 
          alarmPrimed = 0;
          alarmNow = 1;
          snoozed = 0; 
          SoundSequence = 0; 
        }
      }
      else{
        alarmPrimed = 1;  
        // Prevent alarm from going off twice in the same minute, after being turned off and back on.
      }

      if (snoozed)
        if  ((AlarmTimeSnoozeHr == hourTemp ) && (AlarmTimeSnoozeMin == minTemp ))
        {
          alarmNow = 1;
          snoozed = 0; 
          SoundSequence = 0; 
        }
    }
  }


  if (alarmNow)
    ManageAlarm();



  if(Serial.available() ) 
  { 
    processSerialMessage();
  } 


}



#define a5_COMM_MSG_LEN  13   // time sync to PC is HEADER followed by unix time_t as ten ascii digits  (Was 11)
#define a5_COMM_HEADER  255   // Header tag for serial sync messages

void SerialSendDataDaisyChain (char DataIn[])
{ 
  char outputBuffer[13];
  char *toPtr = &outputBuffer[0];
  char *fromPtr = &DataIn[0]; 

  *toPtr++ = 255;
  *toPtr++ = *fromPtr++;  
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;

  *toPtr++ = *fromPtr++;  
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;

  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr = *fromPtr; 

  Serial1.write(outputBuffer);
}


void processSerialMessage() {

  char c,c2;
  byte i, temp, temp2;
  int8_t paramNo, valueNo;
  char OutputCache[13]; 



  // if time sync available from serial port, update time and return true
  while(Serial.available() >=  a5_COMM_MSG_LEN ){  // time message consists of a header and ten ascii digits

    if( Serial.read() == a5_COMM_HEADER) { 

      c = Serial.read() ; 
      c2 = Serial.read();

      if( c == 'S' )
      {
        if (c2 == 'T')
        {  // COMMAND: ST, SET TIME     
          time_t pctime = 0;
          for( i=0; i < 10; i++){   
            c = Serial.read();          
            if( c >= '0' && c <= '9'){   
              pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
            }
          }   
          setTime(pctime);   // Sync Arduino clock to the time received on the serial port
          DisplayWord ("SYNCD", 900);
          DisplayWordDP("____2"); 
          Serial.println("PC Time Sync Signal Received.");
          SerialPrintTime(); 
          if (UseRTC)  
            RTC.set(now());
          EndVCRmode(); 
        } 
      }
      else if( c == 'B' )
      {
        if ((c2 == '0') || (c2 == 0)) // B0, with either ASCII or Binary 0.
        { // COMMAND: B0, Set Parameters 

          c = Serial.read();   // B0 command: Which setting to adjust
          c2 = Serial.read();  // Read first char of additional data

          if (c == '2')
          {
            // edit font character  
            // c2 : Idicates which ASCII character location to edit 
            // Read in 8 more ASCII chars:
            // [___][_][___] <- "A", "B", "C" values, ASCII text

            i = 100 * (Serial.read() - '0');
            i += 10 * (Serial.read() - '0');
            i += (Serial.read() - '0');

            temp = (Serial.read() - '0');          

            temp2 = 100 * (Serial.read() - '0');
            temp2 += 10 * (Serial.read() - '0');
            temp2 += (Serial.read() - '0');

            a5editFontChar(c2, i, temp, temp2); 

            Serial.read();  // Empty input buffer, char 10 of 10
          }
          else{         
            if (c == '0')
            {
              // Set brightness
              c = Serial.read();  // Read input buffer, char 3 of 10

              Brightness = (10 * (c2 - '0') + (c - '0'));
              UpdateBrightness = 1; 
            }
            if (c == '1')
            {// Load altnernate number set
              a5loadAltNumbers(c2 - '0'); 
              Serial.read();  // Empty input buffer, char 3 of 10
            }

            for( i=3; i < 10; i++){   
              Serial.read();  // Empty input buffer
            }   
          }

          RedrawNow = 1;  
          EndVCRmode();
        }
        else { // Daisy chaining: With Bx, where x is less than 48 or x is less than 10:
          if (c2 <= '9')  
          { // if we're here, c2 is <= '9', c2 != 0, and c2 != '0'.   

            OutputCache[0] = 'B';
            OutputCache[1] = c2 - 1;

            for( i=2; i < 12; i++){   
              OutputCache[i] = Serial.read();  
            }   
            SerialSendDataDaisyChain (OutputCache);            
          }

        }
      }
      else if( c == 'A' )
      {
        if ((c2 == '0') || (c2 == 0)) // A0, with either ASCII or Binary 0.
        { // COMMAND: A0, DISPLAY ASCII DATA  
          // ASCII display mode, first 5 chars will be displayed, second 5: decimals

          for( i=0; i < 10; i++){   
            c = Serial.read();     
            if (i < 5) 
              wordCache[i] = c; 
            else 
              dpCache[i - 5] = c;  
          }             
          modeShowText = 3;   
          RedrawNow = 1; 
          EndVCRmode();
        }
        else { // Daisy chaining: With Ax, where x is less than 48 or x is less than 10:
          if (c2 <= '9') 
          { // if we're here, c2 is <= '9', c2 != 0, and c2 != '0'.   

            OutputCache[0] = 'A';
            OutputCache[1] = c2 - 1;

            for( i=2; i < 12; i++){   
              OutputCache[i] = Serial.read();  
            }   
            SerialSendDataDaisyChain (OutputCache);            
          }
        }
      }

      else if( c == 'M' )  // Mode setting commands
      {// Eventually, it would be nice to have all settings and functions
        // accessible through the remote interface.

        if (c2 == 'T')   { // Command: 'MT' : Mode: Time
          modeShowAlarmTime = 0;
          modeShowMenu = 0;
          modeShowText = 0;
          modeLEDTest = 0;

          EndVCRmode();
        }
      }


    }
  }
}



void updateNightLight(void)
{
  if  (NightLightType == 4)  
  { // "Sleep" mode
    unsigned int temp = 0; 

    NightLightStep++;  

    if (NightLightStep <= 255) { 
      if (NightLightSign) 
        temp = NightLightStep; 
      else 
        temp = 255 - NightLightStep;    
    }
    else 
    { 
      if (NightLightSign) 
        temp = 255; 
      else 
        temp = 0; 

      if (NightLightStep > 280)
      {
        NightLightStep = 0; 
        if (NightLightSign)
          NightLightSign = 0;
        else 
          NightLightSign = 1; 
      }
    }

    temp = (temp * temp) >> 8;
    if (temp > 252) {
      temp = 252;
    }

    temp += 3;	  
    a5nightLight(temp);
  }
  else if (NightLightType == 0)  
    a5nightLight(0);   // OFF
  else if (NightLightType == 1) 
    a5nightLight(5);   // LOW
  else if (NightLightType == 2) 
    a5nightLight(50);   // MED  
  else if (NightLightType == 3) 
    a5nightLight(255);  // HIGH  
}


void UpdateDisplay (byte forceUpdate) { 

  byte temp, remainder;

  if (modeShowText)  //Text Display
  { 
    if ((milliTemp >= DisplayWordEndTime) && (modeShowText == 1))
    {
      modeShowText = 0;
      if (wordSequence)
        DisplayWordSequence(wordSequence);  
      // If the word sequence is finished, return to clock display:
      if (wordSequence == 0) 
        RedrawNow = 1; 
    } 
    else
    {
      if(forceUpdate)
      { 
        a5clearOSB();    
        a5loadOSB_Ascii(wordCache,a5_brightLevel);
        a5loadOSB_DP(dpCache,a5_brightLevel);
        a5BeginFadeToOSB();   
      }
    }
  }
  else  if (modeLEDTest)  //LED Test Mode
  { 
    if (milliTemp > DisplayWordEndTime)
    {  
      forceUpdate = 1;
      SoundSequence++; 
      DisplayWordEndTime = milliTemp + 350;  // Advance every 350 ms.
    } 

    if(forceUpdate)
    { 
      // Borrow SoundSequence as a dummy variable:
      if (SoundSequence > 91)
        SoundSequence = 1;

      remainder = SoundSequence - 1;
      temp = 4;

      while (remainder >= 18){ 
        remainder -= 18;   // remainder
        temp -= 1;   // (4 - modulo)
      }

      byte map[] = {
        7,0,1,10,11,3,2,12,13,14,15,16,5,17,8,9,4,6
      };  

      a5clearOSB();   
      a5_OSB[18 * temp + map[remainder]] = a5_brightLevel; 
      a5BeginFadeToOSB();  
      RedrawNow = 1;
    }  

  }

  else if (modeShowMenu)
  {

    DisplayWordDP("_____");
    byte ExtendTextDisplay = 0; 

    if (menuItem == AMPM24HRMenuItem)  // Hour mode: 12Hr / 24 Hr
    {   
      if (optionValue != 0)
      {
        if (HourMode24)
          HourMode24 = 0;
        else
          HourMode24 = 1;
        optionValue = 0;
      }

      if (HourMode24) 
        DisplayWord ("24 HR", 500);
      else
        DisplayWord ("AM/PM", 500);  

      ExtendTextDisplay = 1;
    }
    else if (menuItem == NightLightMenuItem)  // Night Light
    {  
      NightLightType += optionValue;

      if (NightLightType < 0)
        NightLightType = 4;
      if (NightLightType > 4)
        NightLightType = 0;

      if (optionValue != 0){
        if  (NightLightType == 4) 
        {
          NightLightStep = 0;
          NightLightSign = 1;  
        }
        updateNightLight();
      }
      optionValue = 0;

      if (NightLightType == 0) 
        DisplayWord (" NONE", 500);
      else if (NightLightType == 1) 
        DisplayWord (" LOW ", 500);  
      else if (NightLightType == 2) 
        DisplayWord (" MED ", 500);  
      else if (NightLightType == 3) 
        DisplayWord (" HIGH", 500);  
      else  // (NightLightType == 4) 
      DisplayWord ("SLEEP", 500);  
      ExtendTextDisplay = 1;
    }    
    else if (menuItem == AlarmToneMenuItem)  // Alarm Tone: 2
    {  
      AlarmTone += optionValue;
      optionValue = 0;
      if (AlarmTone < 0)
        AlarmTone = 5;
      if (AlarmTone > 5)
        AlarmTone = 0;

      if (AlarmTone == 0) 
        DisplayWord ("X LOW", 500);
      else if (AlarmTone == 1) 
        DisplayWord (" LOW ", 500);  
      else if (AlarmTone == 2) 
        DisplayWord (" MED ", 500);  
      else if (AlarmTone == 3) 
        DisplayWord (" HIGH", 500);   
      else if (AlarmTone == 4) 
        DisplayWord ("SIREN", 500);   
      else 
        DisplayWord (" TINK", 500);   
      ExtendTextDisplay = 1;
    }   
    else if (menuItem == SoundTestMenuItem)  // Alarm Test: 3
    {   
      DisplayWord (" +/- ", 500);   
      if (optionValue != 0)
      { 
        if (alarmNow == 0)
          alarmNow = 1;
        else        
          TurnOffAlarm();
        optionValue = 0;
      }
      ExtendTextDisplay = 1;
    }   

    else if (menuItem == numberCharSetMenuItem)  
    {
      numberCharSet += optionValue;
      if (optionValue != 0){
        optionValue = 0;
        if (numberCharSet < 0)
          numberCharSet = 9;
        if (numberCharSet > 9)
          numberCharSet = 0;
        a5loadAltNumbers(numberCharSet);
      }

      DisplayWord ("01237", 500); // Sample font display
      ExtendTextDisplay = 1;
    }   

    else if (menuItem == DisplayStyleMenuItem) 
    {  
      temp = (DisplayMode & 3U);

      if (optionValue != 0){ 
        if (optionValue == 1) 
          temp = (temp + 1) & 3U;  
        else if (temp == 0) 
          temp = 3; 
        else
          temp--;

        DisplayMode = (DisplayMode & 12U) | (temp);
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(DisplayMode & 3, forceUpdate); // Show clock time, in appropriate style
    }
    else if (menuItem == AltModeMenuItem)  // Alternate with seconds or date:
    {  
      // if (TimeDisplay & 4): Alternate date with time
      // if (TimeDisplay & 8): Alternate date with seconds
      // if (TimeDisplay & 16): Alternate date with words

      if (optionValue != 0)
      {
        temp = 1;
        if ( DisplayMode & 4)
          temp = 2;
        if ( DisplayMode & 8)
          temp = 3;
        if ( DisplayMode & 16)
          temp = 4;    

        temp += optionValue;

        if (temp == 0) 
          temp = 4; // Wrap around (low side)
        else if (temp == 5)
          temp = 0;  // wrap around (high side) 

        DisplayMode &= 3U;

        if (temp > 1)
          DisplayMode |= (1 << temp);
        // if temp is 0 or 1, display time only.

        DisplayModePhaseCount = 0;  
        optionValue = 0; 

      }

      if (DisplayMode & 4U)
        DisplayWord ("DATE ", 500);
      else if (DisplayMode & 8U){
        DisplayWord ("SECS ", 500);
        DisplayWordDP("___1_"); 
      }
      else if (DisplayMode & 16U){
        DisplayWord ("WORDS", 500);
      }     
      else
        DisplayWord (" NONE", 500);

      ExtendTextDisplay = 1;

    } 
    else if (menuItem == SetYearMenuItem) 
    {  
      if (optionValue != 0){
        AdjDayMonthYear(0,0,optionValue); // Day, Month, Year
        optionValue = 0;
        forceUpdate = 1; 
      }    
      TimeDisplay(35, forceUpdate); // Show clock time, in appropriate style
    }

    else if (menuItem == SetMonthMenuItem) 
    {  
      if (optionValue != 0){  
        AdjDayMonthYear(0,optionValue,0); // Day, Month, Year
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(33, forceUpdate); // Show clock time, in appropriate style
    } 
    else if (menuItem == SetDayMenuItem) 
    {  
      if (optionValue != 0){ 
        AdjDayMonthYear(optionValue,0,0); // Day, Month, Year
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(33, forceUpdate); // Show clock time, in appropriate style
    }   

    else if (menuItem == SetSecondsMenuItem) 
    {  
      if (optionValue != 0){ 
        adjustTime(optionValue); // Adjust by +/- 1 second
        if (UseRTC)  
          RTC.set(now()); 
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(32, forceUpdate); // Show clock time, seconds
    }    

    if(forceUpdate && ExtendTextDisplay)
    {  
      if (menuItem != DisplayStyleMenuItem) 
      {   
        a5clearOSB();    
        a5loadOSB_Ascii(wordCache,a5_brightLevel);
        a5loadOSB_DP(dpCache,a5_brightLevel);
        a5BeginFadeToOSB();   
      }
    }
  }
  else if (modeShowDateViaButtons) 
  { 
    TimeDisplay(33, forceUpdate); // Show date
  }
  else if (modeShowAlarmTime) 
  { 
    TimeDisplay(20, forceUpdate); // Show alarm time 
  }

  else
  {
    // Time Display Mode!  Possibly with aux. display.


    if ((DisplayMode > 3) && (DisplayMode < 32))
    {


      if (buttonMonitor) 
      {
        // Do not use alternate display modes when buttons are pressed.
        DisplayModePhase = 0;
        DisplayModePhaseCount = 0;
      }
      else
        if (DisplayModePhaseCount >= 7) // Alternate display every 7 seconds
        { 

          DisplayModePhaseCount = 0;

          DisplayModePhase++;
          if (DisplayModePhase > 1)
            DisplayModePhase = 0; 

          forceUpdate = 1;

          DisplayWord ("     ", 400);  // Blank out between display phases 
          if (AlarmEnabled)  
            DisplayWordDP("2____");
          else
            DisplayWordDP("_____"); 

        }




      if (DisplayModePhase == 0)
      {
        TimeDisplay(DisplayMode & 3, forceUpdate);  // "Normal" time display
      }
      else
      { // Alternate display modes: "Time and ... "
        if (DisplayMode & 4)
          TimeDisplay(33, forceUpdate); // if Bit 4 is set: Show date
        else if (DisplayMode & 8)
          TimeDisplay(32, forceUpdate); // if Bit 8 is set: Show seconds 
        else if (DisplayMode & 16)
          TimeDisplay(36, forceUpdate); // if Bit 16 is set:  Show five letter words
      }
    }
    else 
      TimeDisplay(DisplayMode, forceUpdate);

  }
}
 


void AdjDayMonthYear (int8_t AdjDay, int8_t AdjMonth, int8_t AdjYear)
{
  // From Time library: API starts months from 1, this array starts from 0
  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; 

   time_t timeTemp = now();
      
  int yrTemp = year(timeTemp) + (int) AdjYear;  
  
  int moTemp = month(timeTemp) + AdjMonth;  // Avoid changing year, unless requested
  if (moTemp < 1)
      moTemp = 12;
   if (moTemp > 12)
      moTemp = 1;
      
  int dayTemp = day(timeTemp) + AdjDay;  // avoid changing month, unless requested
  
  if (dayTemp < 1)
     dayTemp = monthDays[moTemp - 1]; 
     
  if (dayTemp > monthDays[moTemp - 1])
    if (AdjDay > 0)
      {  // Roll over day-of-month to 1, if explicitly requesting increase in date.  
         dayTemp = 1;
      }
      else
      { // Otherwise, we should "truncate" the date to last day of month.
       dayTemp = monthDays[moTemp - 1];
      }
     
  setTime(hour(timeTemp),minute(timeTemp),second(timeTemp),
      dayTemp, moTemp, yrTemp);
  if (UseRTC)  
    RTC.set(now()); 
}




void TimeDisplay (byte DisplayModeLocal, byte forceUpdateCopy)  {
  byte temp;
  char units;
  char WordIn[] = {
    "     "                                                                                                                                      };
  byte SecNowTens,  SecNowOnes;
  byte SecNow;

  SecNow = second();

  if (SecLast != SecNow){
    forceUpdateCopy = 1;
    DisplayModePhaseCount++;
  }

  if ((DisplayModeLocal <= 4) || (DisplayModeLocal == 20))
  { // Normal time display OR Alarm time display
    // DisplayModeLocal 0: Standard-mode time-of-day display
    // DisplayModeLocal 1: Time-of-day w/ seconds spinner
    // DisplayModeLocal 2: Standard-mode time-of-day display + flashing separator
    // DisplayModeLocal 3: Time-of-day w/ seconds spinner + flashing separator

    // DisplayModeLocal 20: Standard-mode alarm-time display

    byte HrNowTens,  HrNowOnes, MinNowTens,  MinNowOnes;

    if (DisplayModeLocal == 20)
      temp = AlarmTimeHr;  
    else
      temp = hour(); 

    if (HourMode24) 
      units = 'H';  
    else
    {  
        units = 'A'; 
       
      if (temp >= 12){
        units = 'P';   
      }       
      
      if (temp > 12){ 
        temp -= 12;   //
      }
       
      if (temp == 0)  // Represent 00:00 as 12:00
          temp += 12;
    }

    HrNowTens = U8DIVBY10(temp);    //i.e.,  HrNowTens = temp / 10;
    HrNowOnes = temp - 10 * HrNowTens;

    if (DisplayModeLocal == 20)
      temp = AlarmTimeMin;
    else
      temp = minute(); 

    MinNowTens = U8DIVBY10(temp);      //i.e.,  MinNowTens = temp / 10;
    MinNowOnes = temp - 10 * MinNowTens;

    if (MinNowOnesLast != MinNowOnes)
      forceUpdateCopy = 1;

    SecNow = second();    

    if (SecLast != SecNow)
      forceUpdateCopy = 1;

    if (DisplayModeLocal & 1) // Seconds Spinner Mode
    {


      // binary tree for 8 cases:  three tests max, rather than 8.
      // Split seconds into octants: 0-6,7-14,15-22,23-29,30-36,37-44,45-52,53-59

      if (SecNow < 30)
      {// temp in range 0-29
        if (SecNow < 15)
        {// temp in range 0-14
          if (SecNow < 7)
          { // temp in range 0-6 
            temp = 15;   //  a5editFontChar('a',0,0,32);    // N arrow
          }
          else 
          { // temp in range 7-14
            temp = 16; // a5editFontChar('a',0,0,64);    // NE arrow
          } 
        }
        else
        {// temp in range 15-29
          if (SecNow < 23)
          {  // temp in range 15-22
            temp = 5;  // a5editFontChar('a',32,0,0);    // E arrow 
          }
          else
          {  // temp in range 23-29
            temp = 17;  // a5editFontChar('a',0,0,128);    // SE arrow
          }   
        }
      }
      else
      {// temp in range 30-59
        if (SecNow < 45)
        {// temp in range 30-44 
          if (SecNow < 37)
          {  // temp in range 30-36
            temp = 8;  // a5editFontChar('a',0,1,0);    // S arrow
          }
          else 
          {   // temp in range 37-44
            temp = 9;  // a5editFontChar('a',0,2,0);    // SW arrow
          }  
        }
        else
        {// temp in range 45-59
          if (SecNow < 53)
          {  // temp in range 45-52
            temp = 4;  //  a5editFontChar('a',16,0,0);    // W arrow
          }
          else 
          {   // temp in range 53-59
            temp = 14;  // a5editFontChar('a',0,0,16);    // NW arrow     
          }     
        }
      } 
    }

    if ((HourMode24) || (HrNowTens > 0))
      WordIn[0] =  HrNowTens + a5_integerOffset;   // Blank leading zero unless in 24-hour mode.

    WordIn[1] =  HrNowOnes  + a5_integerOffset;
    WordIn[2] =  MinNowTens + a5_integerOffset;
    WordIn[3] =  MinNowOnes + a5_integerOffset;

    if (DisplayModeLocal & 1)  // Spinner
      WordIn[4] =  ' ';
    else
      WordIn[4] =  units;

    if(forceUpdateCopy)
    { 
      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (DisplayModeLocal & 1)
      {
        a5loadOSB_Segment (temp, a5_brightLevel);
        if (units == 'P')
          a5loadOSB_DP("___1_",a5_brightLevel);   // DP dot in DisplayMode 1.        
      }

      if (AlarmEnabled)
        a5loadOSB_DP("2____",a5_brightLevel);     

      if ((DisplayModeLocal < 20) && (DisplayModeLocal & 2) && (SecNow & 1)){ 
        // no HOUR:MINUTE separators
      }
      else
        a5loadOSB_DP("01200",a5_brightLevel);    

      a5BeginFadeToOSB();  
    }  

    MinNowOnesLast = MinNowOnes;  

  }

  else if (DisplayModeLocal == 32)  //Seconds only
  { 

    temp = SecNow;

    SecNowTens = U8DIVBY10(temp);      //i.e.,  SecNowTens = temp / 10;
    SecNowOnes = temp - 10 * SecNowTens;

    WordIn[2] =  SecNowTens + a5_integerOffset;
    WordIn[3] =  SecNowOnes + a5_integerOffset; 
    if(forceUpdateCopy)
    { 
      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmEnabled)
        a5loadOSB_DP("21200",a5_brightLevel);     
      else
        a5loadOSB_DP("01200",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    } 
    SecLast = SecNow; 
  }

  else if (DisplayModeLocal == 33)  //Month, Day
  {  

    if(forceUpdateCopy)
    {  
      temp = day();  

      byte monthTemp = 3 * ( month() - 1);  
      //Month name (short):
      //      char a5monthShortNames_P[] PROGMEM = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
      WordIn[0] = pgm_read_byte(&(a5_monthShortNames_P[monthTemp++]));  
      WordIn[1] = pgm_read_byte(&(a5_monthShortNames_P[monthTemp++]));
      WordIn[2] = pgm_read_byte(&(a5_monthShortNames_P[monthTemp]));

      byte divtemp =  U8DIVBY10(temp);  //i.e.,  divtemp = day / 10;

      WordIn[3] =   divtemp + a5_integerOffset;  
      WordIn[4] =  ( temp - 10 * divtemp) + a5_integerOffset;   

      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmEnabled)
        a5loadOSB_DP("20100",a5_brightLevel);     
      else
        a5loadOSB_DP("00100",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    }  
    SecLast = SecNow; 
  }
  else if (DisplayModeLocal == 35)  //Year
  { 

    unsigned int yeartemp = year();  
    unsigned int divtemp =  U16DIVBY10(yeartemp);  //i.e.,  divtemp = yeartemp / 10;

    WordIn[4] =   yeartemp - 10 * divtemp + a5_integerOffset;  
    yeartemp = U16DIVBY10(divtemp); 
    WordIn[3] =   divtemp - 10 * yeartemp + a5_integerOffset;  
    divtemp =  U16DIVBY10(yeartemp); 
    WordIn[2] =   yeartemp - 10 * divtemp + a5_integerOffset;  
    yeartemp = U16DIVBY10(divtemp); 
    WordIn[1] =   divtemp - 10 * yeartemp + a5_integerOffset;  

    if(forceUpdateCopy)
    { 
      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmEnabled)
        a5loadOSB_DP("20000",a5_brightLevel);     
      else
        a5loadOSB_DP("00000",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    }   
  }
  else if (DisplayModeLocal == 36)  //FLW - FIVE LETTER WORD mode
  {



    if(forceUpdateCopy)
    {  
      if (DisplayModeLocalLast != 36)
      {  // Pick new display word, but only when first entering mode 36.

        // Uncomment exactly one of the following two lines:
        FLWoffset = random(fiveLetterWordsMax);  // Random word order!
        // FLWoffset += 1;  // Alphebetical word order!

      }

      if (FLWoffset >= fiveLetterWordsMax)
        FLWoffset = 0;

      unsigned int index = 4 * FLWoffset; 

      WordIn[1] = pgm_read_byte(&(fiveLetterWords[index++]));
      WordIn[2] = pgm_read_byte(&(fiveLetterWords[index++]));
      WordIn[3] = pgm_read_byte(&(fiveLetterWords[index++]));
      WordIn[4] = pgm_read_byte(&(fiveLetterWords[index]));

      temp = 0; 

      while (temp < 25){
        index = pgm_read_word(&(fiveLetterPosArray[temp]));
        if (FLWoffset < index){
          WordIn[0] = 'A' + temp;
          temp = 50;
        } 
        temp++;
      }

      if (temp < 50)
        WordIn[0] = 'Z'; 



      a5clearOSB(); 
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmEnabled)
        a5loadOSB_DP("20000",a5_brightLevel);     
      else
        a5loadOSB_DP("00000",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    }   
  }

  DisplayModeLocalLast = DisplayModeLocal;
  SecLast = SecNow; 

}


void SerialPrintTime(){
  //   Print time over serial interface.   Adapted from Time library.

 time_t timeTmp = now();  

  Serial.print(hour(timeTmp));
  printDigits(minute(timeTmp));
  printDigits(second(timeTmp));
  Serial.print(" ");
  Serial.print(dayStr(weekday(timeTmp)));
  Serial.print(" ");
  Serial.print(day(timeTmp));
  Serial.print(" ");
  Serial.print(monthShortStr(month(timeTmp)));
  Serial.print(" ");
  Serial.print(year(timeTmp)); 
  Serial.println(); 

}

void printDigits(int digits){
  // utility function for digital clock serial output: prints preceding colon and leading 0
  // borrowed from Time library.
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void ApplyDefaults (void) {
  // VARIABLES THAT HAVE EEPROM STORAGE AND DEFAULTS...

  a5_brightLevel =  a5brightLevelDefault;
  HourMode24 =      a5HourMode24Default;
  AlarmEnabled =    a5AlarmEnabledDefault;
  AlarmTimeHr =     a5AlarmHrDefault;
  AlarmTimeMin =    a5AlarmMinDefault;
  AlarmTone =       a5AlarmToneDefault;
  NightLightType =  a5NightLightTypeDefault;  
  numberCharSet =   a5NumberCharSetDefault;
}



void EEReadSettings (void) {  
  // Check values for sanity at THIS stage.
  byte value = 255;
  value = EEPROM.read(0);      

  if ((value > 100 + BrightnessMax) || (value < 100))
    Brightness = a5brightLevelDefault;
  else  
    Brightness = value - 100;   

  value = EEPROM.read(1);
  if (value > 1)
    HourMode24 = a5HourMode24Default;
  else  
    HourMode24 = value;

  value = EEPROM.read(2);
  if (value > 1)
    AlarmEnabled = a5AlarmEnabledDefault;
  else  
    AlarmEnabled = value;

  value = EEPROM.read(3); 
  if ((value > 123) || (value < 100))
    AlarmTimeHr = a5AlarmHrDefault;
  else  
    AlarmTimeHr = value - 100;   

  value = EEPROM.read(4);

  if ((value > 159) || (value < 100))
    AlarmTimeMin = a5AlarmMinDefault;
  else  
    AlarmTimeMin = value - 100;   

  value = EEPROM.read(5);
  if (value > 5)
    AlarmTone = a5AlarmToneDefault;
  else  
    AlarmTone = value;   

  value = EEPROM.read(6);
  if (value > 4)
    NightLightType = a5NightLightTypeDefault;  
  else  
    NightLightType = value;    


  value = EEPROM.read(7);  
  if (value > 9)   
  {
    numberCharSet = a5NumberCharSetDefault;
  }
  else
    numberCharSet = value;       

  value = EEPROM.read(8);   
  if (value > 31) 
  {
    DisplayMode = a5DisplayModeDefault;  
  }
  else  
    DisplayMode = value;       



}


void EESaveSettings (void){ 

  // If > 4 seconds since last button press, and
  // we suspect that we need to change the stored settings:

  byte value; 
  byte indicateEEPROMwritten = 0;

  if (milliTemp >= (LastButtonPress + 4000))
  {

    // Careful if you use this function: EEPROM has a limited number of write
    // cycles in its life.  Good for human-operated buttons, bad for automation.
    // Also, no error checking is provided at this, the write EEPROM stage.

    value = EEPROM.read(0);  
    if (Brightness != (value - 100))  {
      a5writeEEPROM(0, Brightness + 100);  

      //NOTE:  Do not blink LEDs off to indicate saving of this value
    }
    value = EEPROM.read(1);  
    if (HourMode24 != value)  { 
      a5writeEEPROM(1, HourMode24); 
      indicateEEPROMwritten = 1;
    } 
    value = EEPROM.read(2);  
    if (AlarmEnabled != value)  {
      a5writeEEPROM(2, AlarmEnabled);  
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    } 
    value = EEPROM.read(3);  
    if (AlarmTimeHr != (value - 100))  { 
      a5writeEEPROM(3, AlarmTimeHr + 100); 
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    }
    value = EEPROM.read(4);  
    if (AlarmTimeMin != (value - 100)){
      a5writeEEPROM(4, AlarmTimeMin + 100); 
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    }
    value = EEPROM.read(5);  
    if (AlarmTone != value){ 
      a5writeEEPROM(5, AlarmTone);
      indicateEEPROMwritten = 1;
    }
    value = EEPROM.read(6);  
    if (NightLightType != value){
      a5writeEEPROM(6, NightLightType);  
      indicateEEPROMwritten = 1;
    }
    value = EEPROM.read(7);  
    if (numberCharSet != value){
      a5writeEEPROM(7, numberCharSet);  
      indicateEEPROMwritten = 1;
    }
    value = EEPROM.read(8);  
    if (DisplayMode != value){
      a5writeEEPROM(8, DisplayMode);  
      indicateEEPROMwritten = 1;
    }      

    if (indicateEEPROMwritten) { // Blink LEDs off to indicate when we're writing to the EEPROM 
      DisplayWord ("     ", 100);  
    }

    UpdateEE = 0;
    if (UseRTC)  
      RTC.set(now());  // Update time at RTC, in case time was changed in settings menu
  }
} 



