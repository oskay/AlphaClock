/**
 * AlphaClockv2_SendText_Daisy
 *
 * Display five characters of text- with optional decimal points
 *  on daisy-chained Alpha Clock Five units, through the serial interface.
 * 
 * By default, this will display the text on the *SECOND* daisy-chained Alpha Clock Five.
 * You can adjust the number of times that it will be relayed before being displayed with the
 * distance variable.
 *
 *  For proper display, the Alpha Clock Five should run firmware v 2.0 or newer.
 * 
 *
 * Partly Based upon SetArduinoClock.pde, from the Arduino DateTime library
 *
 * portIndex must be set to the port connected to the Arduino
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
  myPort = new Serial(this,Serial.list()[portIndex], 19200);

  fontA =  createFont("Arial", FONT_SIZE);  
  textFont(fontA); 
  textSize( FONT_SIZE);
  stroke(255);
  smooth();
  background(0);
  text("Click mouse to send Text", 5,20);
  text("or resume time display", 5,35);
  clockmode = true;
}




void mousePressed() {  


  //These four strings must be 5 characters long:

  String textToWrite = "*EVIL";       // The actual text to be displayed
  String DPstring = " 123 ";    // Decimal point array
  //1-> Lower DP, 2->Upper DP, 3->Both DPs. Else: ignored.

  String textToWrite2 = "ALPHA";       // The actual text to be displayed
  String DPstring2 = "3   3";    // Decimal point array

  int distance = 1;  // Number of times to relay data between Alpha Clock Five units before displaying


  char header = 0xff;

  if (clockmode) {  
    myPort.write(header); 
    myPort.write('A');   
    myPort.write(nf(distance, 1));     // Send to SECOND Alpha Clock in daisy chain
    myPort.write(textToWrite);  
    myPort.write(DPstring);  

    println("Writing" + textToWrite);
    clockmode =  false;
  }
  else
  {  
    myPort.write(header); 
    myPort.write('A');   
    myPort.write(nf(distance, 1));     // Send to SECOND Alpha Clock in daisy chain
    myPort.write(textToWrite2);  
    myPort.write(DPstring2);  

    println("Writing" + textToWrite2);
    clockmode =  true;
  }
}

void serialEvent(Serial p) {
  String inString = myPort.readStringUntil(LF); 
  println(inString);  // display serial input
}

void draw() {
}

