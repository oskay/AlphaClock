/**
 * AlphaClock_CustomChars
 *
 * Display five characters of text- with custom character set 
 *
 * Requires Alpha Clock Five firmware version 2.0 or newer
 * 
 *
 * Partly Based upon SetArduinoClock.pde, from the Arduino DateTime library
 *
 * portIndex must be set to the port connected to the Alpha Clock Five
 * 
 * Clicking the window sends the text string; clicking again resumes clock display.
 *  
 */

import processing.serial.*;

Serial myPort;     // Create object from Serial class
public static final short LF = 10;     // ASCII linefeed
public static final short FONT_SIZE = 12;
public static final short portIndex = 5;  // select the com port, 0 is the first port
PFont fontA; 

boolean clockmode;


void setup() {  
  size(300, 300);
  println(Serial.list());
  println(" Connecting to -> " + Serial.list()[portIndex]);
  myPort = new Serial(this, Serial.list()[portIndex], 19200);

  fontA =  createFont("Arial", FONT_SIZE);  
  textFont(fontA); 
  textSize( FONT_SIZE);
  stroke(255);
  smooth();
  background(0);
  text("Click mouse to toggle custom text", 5, 20); 
  clockmode = true;
}



// Utility functions:

void Alpha5ReplaceFontChar(int relayCount, char theChar, int Aval, int Bval, int Cval)
{
  myPort.write(0xff); // Send header byte
  myPort.write('B'); // Adjust settings on Alpha Clock
  myPort.write(nf(relayCount, 1)); // Adjust settings on nth Alpha Clock in daisy chain
  myPort.write('2'); // Setting type to adjust: Edit font character    
  myPort.write(theChar); // The ASCII char that we are replacing  

  myPort.write( nf(Aval, 3));  // Segment data "A".   Three digits string, with leading-zero padding. E.g., "006"
  myPort.write( nf(Bval, 1)); //  Segment data "B".   One digit string.  E.g., "3"
  myPort.write( nf(Cval, 3)); //  Segment data "C".   Three digits string, with leading-zero padding. E.g., "058"

  myPort.write("_");    // Final padding character, which will be ignored. (to fill the 13 byte format).
}




void Alpha5AdjustBrightness(int relayCount, int bright)
{
  myPort.write(0xff); // Send header byte
  myPort.write('B');  // Adjust settings on Alpha Clock
  myPort.write(nf(relayCount, 1)); // Adjust settings on nth Alpha Clock in daisy chain
  myPort.write('0'); // Setting type to adjust: 0, Brightness
  myPort.write( nf(bright, 2));  // Brightness value, two digits string, with leading-zero padding. E.g., "010" (11, max)
  myPort.write("_______");  // Seven extra characters (that will be ignored by the Alpha Clock).
}
    
  

void mousePressed() {  

    int distance = 0;  // Number of times to relay data between Alpha Clock Five units before displaying


  //These three strings must be exactly 5 characters long:

  String textToWrite = "abcde";       // The ascii locations where we store our custom chars
  String textToWrite2 = "ABCDE";       // The ascii locations where we store our custom chars
  String DPstring = "     ";    // Decimal point array (empty, here) 

  char header = 0xff;

  if (clockmode) {  
    clockmode =  false;

    // Replace ASCII locations a, b, c, d, and e with custom characters:        

    Alpha5ReplaceFontChar(distance, 'a', 16, 0, 156);
    Alpha5ReplaceFontChar(distance, 'b', 0, 0, 64);
    Alpha5ReplaceFontChar(distance, 'c', 32, 0, 0);
    Alpha5ReplaceFontChar(distance, 'd', 0, 1, 0);
    Alpha5ReplaceFontChar(distance, 'e', 16, 0, 0);


    delay(100);  // Delay for input buffer to clear up.

    Alpha5AdjustBrightness(distance, 8);
 
    // Now, send Ascii text, to display our custom chars:
    myPort.write(header); 
    myPort.write('A'); // Send text to Alpha Clock
    myPort.write(nf(distance, 1)); // Adjust text on nth Alpha Clock in daisy chain
    myPort.write(textToWrite);  
    myPort.write(DPstring);
  }
  else
  {
    // Toggle brightness to indicate when command is received

    Alpha5ReplaceFontChar(distance, 'A', 63, 0, 7);

    Alpha5AdjustBrightness(distance, 11);

    // Now, send Ascii text, to display our custom chars:
    myPort.write(header); 
    myPort.write('A'); // Send text to Alpha Clock
    myPort.write(nf(distance, 1)); // Talk to nth Alpha Clock in daisy chain
    myPort.write(textToWrite2);  
    myPort.write(DPstring);     


    clockmode = true;
  }
}

void serialEvent(Serial p) {
  String inString = myPort.readStringUntil(LF); 
  println(inString);  // display serial input
}

void draw() {
}

