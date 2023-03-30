/*
  Software serial multple serial test

 Receives from the hardware serial, sends to software serial.
 Receives from software serial, sends to hardware serial.

 The circuit:
 * RX is digital pin 2 (connect to TX of other device)
 * TX is digital pin 3 (connect to RX of other device)
*/
#include <SoftwareSerial.h>
SoftwareSerial Serial1(2, 3); // RX to TX on HC-06 | TX to RX on HC-06
String command = ""; // Stores response from HC-06 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);       //monitor
  Serial1.begin(9600);      //bluetooth 

  Serial.print("AT      ");  
  Serial1.print("AT");                  //PING
  if (Serial1.available()) {
    while(Serial1.available()) { // While there is more to be read, keep reading.
    delay(3);
    char c = Serial1.read();
      command += c;    
    }
  }
  delay(2000);
  Serial.println(command);
  command = ""; // No repeats

  Serial.print("AT+NAMEpro1a      "); 
  Serial1.print("AT+NAMEpro1a");        //CHANGE NAME
  if (Serial1.available()) {
    while(Serial1.available()) { // While there is more to be read, keep reading.
        delay(3);
      command += (char)Serial1.read();  
    }
  }
  delay(2000);
  Serial.println(command);
  command = ""; // No repeats

  Serial.println("AT+PIN1234");
  Serial1.print("AT+PIN1234");        //CHANGE PASSWORD
  if (Serial1.available()) {
    while(Serial1.available()) { // While there is more to be read, keep reading.
        delay(3);
      command += (char)Serial1.read();  
    }
  }
  delay(2000);   
  Serial.println(command);
  command = ""; // No repeats

  Serial.print("AT+BAUD8      ");  
  Serial1.print("AT+BAUD8");               //CHANGE SPEED TO 115K
  if (Serial1.available()) {
    while(Serial1.available()) { // While there is more to be read, keep reading.
      command += (char)Serial1.read();    
    } 
  } 
delay(2000);       
Serial.println(command);
}

void loop(){
}   //one-shot - nothing here
