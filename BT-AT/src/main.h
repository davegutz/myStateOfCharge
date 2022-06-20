/*
There is a pushbutton near to the EN pin of Bluetooth Module. Press that button and hold. Then connect
the Arduino Uno to computer. Then release the button.

Then the LED on the Bluetooth Module start blinking with the inter well of 2 seconds.  This indicates
now the Bluetooth Module in command mode.

Next open the serial monitor on Arduino Uno. And set the baud rate as 38400 and set output mode to
"Both NL & CR". (box near the baud rate).

Here we use the AT commands. First type AT on the serial monitor and press send. You will see a OK message
when everything is fine.

I am going to set my Bluetooth Module name as Hackster
Type AT+NAME=Hackster and press Send. It will return a OK message.

The default password of HC-05 is 1234. And here I am going to change it to 7806.

Type AT+PSWD="7806" and press Send. Alternatively you can change it to any password. Password must be in
double quotes. Otherwise you will get an error. Once it was fine, OK message will return.
*/

// hc-06
// https://mcuoneclipse.com/2013/06/19/using-the-hc-06-bluetooth-module/
// AT+BAUD4;    OK9600
// AT+BAUD8;    OK115200
// AT+VERSION;  OKlinvorV1.8
// AT; 	OK 	Used to verify communication
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

HC-05
AT;   OK


*/

// Make this selection!!!
#undef HC05

// For Photon
#if (PLATFORM_ID==6)
  #define PHOTON
  //#define BOOT_CLEAN      // Use this to clear 'lockup' problems introduced during testing using Talk
  #include "application.h"  // Should not be needed if file ino or Arduino
  SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status
  #include <Arduino.h>      // Used instead of Print.h - breaks Serial
#else
  #undef PHOTON
  using namespace std;
  #undef max
  #undef min
#endif


void serialEvent();
boolean string_complete = false;
String input_string;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);	/* Define baud rate for serial communication */
  Serial1.begin(9600); /* Define baud rate for serial1 communication */
}

void loop()
{
  if (string_complete)
  {
    Serial.printf("%s\n", input_string.c_str());
    Serial1.write(input_string.c_str());
    input_string = "";
    string_complete = false;
  }
  if (Serial1.available()) Serial.write(Serial1.read());
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
