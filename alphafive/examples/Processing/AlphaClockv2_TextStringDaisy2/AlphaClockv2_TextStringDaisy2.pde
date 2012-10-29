/**
 * AlphaClockv2_TextStringDaisy2
 *
 * Write text across multiple daisy-chained Alpha Clock Five units, through the serial interface.
 *
 * Edit the text string in the TextString location below, and edit the A5Count variable
 * to indicate how many Alpha Clock Five units are daisy-chained together.
 *
 * portIndex must be set to the port connected to the Arduino
 * 
 *  
 */

import processing.serial.*;

public static final short portIndex = 5;  // select the com port, 0 is the first port
String TextString = "ALPHACLOCKFIVE V 20 ";
String DPString   = "_______________1_1__";



int A5Count = 4;    // The number of daisy-chained Alpha Clock Five units


Serial myPort;     // Create object from Serial class
public static final short LF = 10;     // ASCII linefeed
public static final short FONT_SIZE = 12;
PFont fontA; 

int offset;
int TextStrLength;



void setup() {  
  size(500, 300);
  println(Serial.list());
  println(" Connecting to -> " + Serial.list()[portIndex]);
  myPort = new Serial(this, Serial.list()[portIndex], 19200);

  fontA =  createFont("Arial", FONT_SIZE);  
  textFont(fontA); 
  textSize( FONT_SIZE);
  stroke(255);
  smooth();
  background(0);
  text("Scrolling text:", 5, 20);
  text(TextString, 5, 35);
  offset = 5*(A5Count - 1);
  
  TextStrLength = TextString.length();
}

//  TextString.substring(beginIndex, endIndex) 


void draw() {  
 
  char header = 0xff;

String textToWrite = "     ";
String DPsToWrite  = "_____";


int n = A5Count - 1;

while (n >= 0) {

if (( offset >= (5 * n)) && (offset <= (TextStrLength + (n-1) * 5)) ){
 textToWrite = TextString.substring(offset - n * 5, offset - (n-1) * 5) ;
 DPsToWrite  = DPString.substring(offset - n * 5, offset - (n-1) * 5) ;
}
else
textToWrite = "_____";

  myPort.write(header); 
  myPort.write('A');   
  myPort.write(nf(n, 1));     // Send to nth Alpha Clock in daisy chain
  myPort.write(textToWrite);  
  myPort.write(DPsToWrite);  
  
  n--;
}

  println("Writing: " + textToWrite);
 
  delay(400); // 400 ms delay recommended.
}

void serialEvent(Serial p) {
  String inString = myPort.readStringUntil(LF); 
  println(inString);  // display serial input
}

