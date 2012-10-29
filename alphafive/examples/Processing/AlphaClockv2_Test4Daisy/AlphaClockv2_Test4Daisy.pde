/**
 * AlphaClockv2_Test4Daisy
 *
 * Send test string to four daisy-chained Alpha Clock Five units
 *
 * The left number increments on each, the right number indicates which clock unit it is.  
 *
 * portIndex must be set to the port connected to the Arduino
 * 
 *  
 */

import processing.serial.*;

public static final short portIndex = 5;  // select the com port, 0 is the first port


Serial myPort;     // Create object from Serial class
public static final short LF = 10;     // ASCII linefeed
public static final short FONT_SIZE = 12;
PFont fontA; 

byte cycle;


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
  text("Sending text to four Alpha Clocks,", 5, 20);
  text("every 500 ms.", 5, 35);
  cycle = 0;
}




void draw() {  

  String DPstring = " 123 ";    // Decimal point array
  //1-> Lower DP, 2->Upper DP, 3->Both DPs. Else: ignored.

  String textToWrite = nf(cycle, 1) + "   ";

  char header = 0xff;

  myPort.write(header); 
  myPort.write('A');   
  myPort.write(nf(3, 1));     // Send to FOURTH Alpha Clock in daisy chain
  myPort.write(textToWrite + nf(4, 1));  
  myPort.write("_____");  

  myPort.write(header); 
  myPort.write('A');   
  myPort.write(nf(2, 1));     // Send to THIRD Alpha Clock in daisy chain
  myPort.write(textToWrite + nf(3, 1));  
  myPort.write("_____");  

//  delay(100);

  myPort.write(header); 
  myPort.write('A');   
  myPort.write(nf(1, 1));     // Send to SECOND Alpha Clock in daisy chain
  myPort.write(textToWrite + nf(2, 1));  
  myPort.write("_____");  


  myPort.write(header); 
  myPort.write('A');   
  myPort.write(nf(0, 1));     // Send to FIRST Alpha Clock in daisy chain
  myPort.write(textToWrite + nf(1, 1));  
  myPort.write("_____");  



  println("Writing" + textToWrite);

  cycle++;
  if (cycle > 9)
    cycle = 0;

  delay(500);
}

void serialEvent(Serial p) {
  String inString = myPort.readStringUntil(LF); 
  println(inString);  // display serial input
}

