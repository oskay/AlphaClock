/**
 * Set the time on Alpha Clock Five
 *
 * Based upon SetArduinoClock.pde, from the Arduino DateTime library
 *
 * The "portIndex" must be set to the port connected to the Arduino
 * 
 * Clicking the window sends the current time to the arduino
 * as the string value of the number of seconds since Jan 1 1970
 *  
 */

import processing.serial.*;

Serial myPort;     // Create object from Serial class

public static final short portIndex = 4;  // select the com port, 0 is the first port


public static final short LF = 10;     // ASCII linefeed
public static final short TIME_HEADER = 255; //header byte for arduino serial time message 
public static final short FONT_SIZE = 12;
color PCClock = color(153, 51, 0);
PFont fontA; 
long prevTime=0; // holds value of the previous time update

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
  text("Click mouse to set Alpha Clock time", 5,10);
}

long getTimeNow(){
  // java time is in ms, we want secs    
  GregorianCalendar cal = new GregorianCalendar();
  cal.setTime(new Date());
  int	tzo = cal.get(Calendar.ZONE_OFFSET);
  int	dst = cal.get(Calendar.DST_OFFSET);
  long now = (cal.getTimeInMillis() / 1000) ; 
  now = now + (tzo/1000) + (dst/1000); 
  return now;
}

void mousePressed() {  
  String time = String.valueOf(getTimeNow());   
  
    char header = 0xff;
  myPort.write(header);  // send header and time to arduino
  
  myPort.write('S');   
   myPort.write('T');     
  myPort.write(time);  
   
   println(time);  // display serial input that is not a time messgage

  
}

void serialEvent(Serial p) {
  String inString = myPort.readStringUntil(LF); 
  if(inString != null && inString.length() >= 13 && inString.charAt(0) == TIME_HEADER) {    
    int val = 0;
    long time = 0;
    for(int i = 1; i <= 10; i++)        
      time = time * 10 + (inString.charAt(i) - '0');  
  }         
  else if(inString != null && inString.length() > 0 )     
    println(inString);  // display serial input that is not a time messgage
}

void draw() { 

}
