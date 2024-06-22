/*
Then the LED on the Bluetooth Module start blinking with the interval of 2 seconds.  This indicates
now the Bluetooth Module in command mode.

Next open the serial monitor. And set the baud rate as 9600 and set output mode to
"Both NL & CR". (box near the baud rate).

Here we use the AT commands. First type AT on the serial monitor and press send. You will see a OK message
when everything is fine.

I am going to set my Bluetooth Module name as Hackster
Type AT+NAME=Hackster and press Send. It will return a OK message.

*/

// hc-06
// https://mcuoneclipse.com/2013/06/19/using-the-hc-06-bluetooth-module/

#define SBAUD 230400
// #define STEP 1
#define STEP 2

// Step-by-step
// 1.  Uncomment line 'STEP1' and comment line 'STEP2.'  Compile and flash.
// 2.  Run following using serial that is running at SBAUD (be sure to include ';' below):
//    >AT;    // answers 'OK'
//    >AT+NAMEpro3p2;  // answers 'OKsetname'
//    >AT+BAUD9; // use BAUD<n> from table below that matches SBAUD answers 'OK230400'
//       // This setting makes the device unavailable.  It seems you need to recompile with new baud (STEP 2) ti recommunicate
// 3.  If you need to run again, comment out '#define STEP 1' and uncomment '#define STEP 2'
// 4.  On android pair classic BT device.  pwd=1234
//
//  Notes:  
//  a.  Any bad characters corrected by keyboard 'back' will show the special back character
//  b.  I've had to do the NAME step a couple times, de-powering the device completely between steps

#if STEP==1
  #define S1BAUD 9600
#elif STEP==2
  #define S1BAUD SBAUD
#endif

//  
//  >AT;
//     <<answers 'OK'
//  >AT+BAUD9;
//     <<answers 'OK230400
//  >AT+NAMEsoc1a;

// >AT+BAUD4;    OK9600
// >AT+BAUD8;    OK115200
// >AT+VERSION;  OKlinvorV1
// >AT; 	OK 	Used to verify communication



/*
AT+VERSION 	OKlinvorV1.8 	The firmware version (version might depend on firmware)
AT+NAMExyz 	OKsetname 	Sets the module name to “xyz”
AT+PIN1234 	OKsetPIN 	Sets the module PIN to 1234
AT+BAUD1 	OK1200 	Sets the baud rate to 1200
AT+BAUD2 	OK2400 	Sets the baud rate to 2400
AT+BAUD3 	OK4800 	Sets the baud rate to 4800
AT+BAUD4 	OK9600 	Sets the baud rate to 9600
AT+BAUD5 	OK19200 	Sets the baud rate to 19200
AT+BAUD6 	OK38400 	Sets the baud rate to 38400
AT+BAUD7 	OK57600 	Sets the baud rate to 57600
AT+BAUD8 	OK115200 	Sets the baud rate to 115200
AT+BAUD9 	OK230400 	Sets the baud rate to 230400
AT+BAUDA 	OK460800 	Sets the baud rate to 460800
AT+BAUDB 	OK921600 	Sets the baud rate to 921600
AT+BAUDC 	OK1382400 	Sets the baud rate to 1382400




*/

// Make this selection!!!
#undef HC05

// For Photon/Argon
#define PHOTON
#include "application.h"  // Should not be needed if file ino or Arduino
SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status
#include <Arduino.h>      // Used instead of Print.h - breaks Serial

void serialEvent();
boolean string_complete = false;
String input_string;

// Non-blocking delay
void delay_no_block(const unsigned long int interval)
{
  unsigned long int previousMillis = millis();
  unsigned long currentMillis = previousMillis;
  while( currentMillis - previousMillis < interval )
  {
    currentMillis = millis();
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(SBAUD);	/* Define baud rate for serial communication */
  Serial1.begin(S1BAUD); /* Define baud rate for serial1 communication */
  // Serial1.begin(115200); /* Define baud rate for serial1 communication */
  // Serial1.begin(38400); /* Define baud rate for serial1 communication */
  ////////IF YOU DON'T GET ANY JOY WITH 115200 TRY 38400 INSTEAD/////////
  ////// ALSO, WAIT A REALLY LONG TIME FOR PHONE TO FIND NEW DEVICE (MAYBE UP TO 1 MIN)
}

void loop()
{
  if (string_complete)
  {
    Serial1.write(input_string.c_str());
    Serial.printf("\nwrote '%s' to Serial1; waiting response...\n", input_string.c_str());
    input_string = "";
    string_complete = false;
  }
  // delay_no_block(100);
  if (Serial1.available())
  {
    Serial.write(Serial1.read());

  }
  // delay_no_block(100);
}


/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the cp.input_string:
    input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      string_complete = true;
     // Remove whitespace
      input_string.trim();
      input_string.replace("\0","");
      input_string.replace("\r","");
      input_string.replace("\n","");
      input_string.replace(";","");
      input_string.replace(",","");
      input_string.replace(" ","");

      // for hc-05
      #ifdef HC05
        input_string += '\r';
        input_string += '\n';
      #endif

      break;  // enable reading multiple inputs
    }
  }
}
