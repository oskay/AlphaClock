/**
 * AlphaClock_SetBrightnessCharSetDaisy
 *
 * Adjust Alpha Clock Five brightness and number character set
 *  on a daisy-chained Alpha Clock Five 
 *
 * By default, this will adjust the *SECOND* daisy-chained Alpha Clock Five.
 * You can adjust the number of times that it will be relayed before being displayed with the
 * distance variable.
 *
 * Requires Alpha Clock Five firmware version 2.0 or newer
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
  text("Click mouse to alternate brightness", 5, 20);
  text("and number font used", 5, 35);
  clockmode = true;
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


void Alpha5SetNumberCharSet(int relayCount, int Charset)
{
  myPort.write(0xff); // Send header byte
  myPort.write('B');  // Adjust settings on Alpha Clock
  myPort.write(nf(relayCount, 1)); // Adjust settings on nth Alpha Clock in daisy chain
  myPort.write('1'); // Setting type to adjust: 1, Number character set
  myPort.write( nf(Charset, 1));  // Brightness value, one digit string, E.g., "9" (9, max)
  myPort.write("________");  // Eight extra characters (that will be ignored by the Alpha Clock).
}  


void mousePressed() {  

  int distance = 1;  // Number of times to relay data between Alpha Clock Five units before displaying
  // char header = 0xff;

  if (clockmode) {  
    clockmode =  false;

    Alpha5AdjustBrightness(distance, 8);
    Alpha5SetNumberCharSet(distance, 9);

    //   Here's how you would do this without the functions above:
    //    myPort.write(header); 
    //    myPort.write('B'); // Adjust settings on Alpha Clock
    //    myPort.write('1'); // Adjust settings on nth Alpha Clock in daisy chain
    //    myPort.write('0'); // Setting type to adjust: 0, Brightness
    //    myPort.write("08"); // The setting value (Two ascii characters)
    //    myPort.write("_______");  // Seven extra characters (that will be ignored by the Alpha Clock). 


    //    myPort.write(header); 
    //    myPort.write('B'); // Adjust settings on Alpha Clock
    //    myPort.write(distance); // Adjust settings on nth Alpha Clock in daisy chain
    //    myPort.write('1'); //  Setting type to adjust: 1, Character set 
    //    myPort.write('9'); // The Number Set to use (Number 9)
    //    myPort.write("01234567");  // Eight extra characters (that will be ignored by the Alpha Clock).
  }
  else
  {

    Alpha5AdjustBrightness(distance, 11);
    Alpha5SetNumberCharSet(distance, 2);

    clockmode = true;
  }
}

void serialEvent(Serial p) {
  String inString = myPort.readStringUntil(LF); 
  println(inString);  // display serial input
}

void draw() {
}

