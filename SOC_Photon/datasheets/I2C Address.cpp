// Reviewed in the United Kingdom on March 28, 2021
// Size: LCD 1602 16 x 2Color: BlueVerified Purchase
// I purchased four units and they all worked fine.

// If the screen appears blank when you connect.... try rotating the potentiometer at the rear. DON'T FORCE IT.

// Remember, their hexadecimal addresses may not be set to your default address. My addresses all required changing. Changing the address is quite easy. If you do not understand these instructions, there are some interesting YouTube tutorials.

// 1. Connect the I2C board to 5v VCC and also to ground.
// 2. Connect the SDA of the I2C board to the SDA of your Arduino
// 3. Connect the SCL of the I2C board to the SCL of your Arduino
// 4. Copy this sketch to the Arduino IDE then upload it to your Arduino
// 5. Open the IDE Serial Monitor and you should be able to see a list of the used addresses
// 6. Your LCD1602 will be listed here
// 7. Once you find the the number, change your Sketch
// 8. Example...... LiquidCrystal_I2C lcd(0x27, 16, 2);
// 9. If you can't count in hexadecimal, Google can help
// 10. It's the 0x27 number which will require changing

// I2CScanner.ino -- I2C bus scanner for Arduino
#include "Wire.h"
extern "C" {
#include "utility/twi.h" // from Wire library, so we can do bus scanning
}

// Scan the I2C bus between addresses from_addr and to_addr.
// On each address, call the callback function with the address and result.
// If result==0, address was found, otherwise, address wasn't found
// (can use result to potentially get other status on the I2C bus, see twi.c)
// Assumes Wire.begin() has already been called
void scanI2CBus(byte from_addr, byte to_addr,
void(*callback)(byte address, byte result) )
{
byte rc;
byte data = 0; // not used, just an address to feed to twi_writeTo()
for( byte addr = from_addr; addr <= to_addr; addr++ ) {
rc = twi_writeTo(addr, &data, 0, 1, 0);
callback( addr, rc );
}
}

// Called when address is found in scanI2CBus()
// Feel free to change this as needed
// (like adding I2C comm code to figure out what kind of I2C device is there)
void scanFunc( byte addr, byte result ) {
Serial.print("addr: ");
Serial.print(addr,DEC);
Serial.print( (result==0) ? " found!":" ");
Serial.print( (addr%4) ? "\t":"\n");
}

byte start_address = 8; // lower addresses are reserved to prevent conflicts with other protocols
byte end_address = 119; // higher addresses unlock other modes, like 10-bit addressing

// standard Arduino setup()
void setup()
{
Wire.begin();

Serial.begin(9600); // Changed from 19200 to 9600 which seems to be default for Arduino serial monitor
Serial.println("\nI2CScanner ready!");

Serial.print("starting scanning of I2C bus from ");
Serial.print(start_address,DEC);
Serial.print(" to ");
Serial.print(end_address,DEC);
Serial.println("...");

// start the scan, will call "scanFunc()" on result from each address
scanI2CBus( start_address, end_address, scanFunc );

Serial.println("\ndone");

// Set pin mode so the loop code works ( Not required for the functionality)
pinMode(13, OUTPUT);
}

// standard Arduino loop()
void loop()
{
// Nothing to do here, so we'll just blink the built-in LED
digital_write(13,HIGH);
delay(300);
digital_write(13,LOW);
delay(300);
}
