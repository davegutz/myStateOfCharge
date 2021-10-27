/*
 * Project SOC_Photon
  * Description:
  * 
  * By:  Dave Gutz October 2021
  * 01-Oct-2021   Cleanup working version
  * 
//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// See README.md
*/

// For Photon
#if (PLATFORM_ID == 6)
#define PHOTON
#include "application.h" // Should not be needed if file ino or Arduino
//SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED); // Make sure code always run regardless of network status
#include <Arduino.h>     // Used instead of Print.h - breaks Serial
#else
#undef PHOTON
using namespace std;
#undef max
#undef min
#endif

#include "constants.h"

// Dependent includes.   Easier to debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"

extern int8_t debug;              // Level of debug printing (2)
extern Publish pubList;
Publish pubList = Publish();
extern String inputString;        // a string to hold incoming data
extern boolean stringComplete;    // whether the string is complete
extern boolean enable_wifi;     // Enable wifi

// Global locals
int8_t debug = 2;
String inputString = "";
boolean stringComplete = false;
boolean enable_wifi = false;

char buffer[256];               // Serial print buffer
int numTimeouts = 0;            // Number of Particle.connect() needed to unfreeze
String hmString = "00:00";      // time, hh:mm
double controlTime = 0.0;       // Decimal time, seconds since 1/1/2021
unsigned long lastSync = millis();// Sync time occassionally.   Recommended by Particle.
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_ADS1015 *ads;          // Use this for the 12-bit version; 1115 for 16-bit
Adafruit_ADS1015 *ads_amp;      // Use this for the 12-bit version; 1115 for 16-bit; amplified; different address
bool bare_ads = false;          // If ADS to be ignored
bool bare_ads_amp = false;      // If ADS to be ignored
Wifi *myWifi;                   // Manage Wifi

// Setup
void setup()
{
  // Serial
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:
  Serial.flush();
  delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  Serial.println("Hello!");

  // Peripherals
  myPins = new Pins(D6, D7, A1);

  // Status
  pinMode(myPins->status_led, OUTPUT);
  digitalWrite(myPins->status_led, LOW);

  // I2C
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();

  // AD
  Serial.println("Initializing SHUNT MONITORS");
  ads = new Adafruit_ADS1015;
  ads->setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  ads_amp = new Adafruit_ADS1015;
  ads_amp->setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  if (!ads->begin()) {
    Serial.println("FAILED to initialize ADS SHUNT MONITOR.");
    bare_ads = true;
  }
  if (!ads_amp->begin((0x49))) {
    Serial.println("FAILED to initialize ADS AMPLIFIED SHUNT MONITOR.");
    bare_ads_amp = true;
  }
  Serial.println("SHUNT MONITORS initialized");

  // Cloud
  unsigned long now = millis();
  myWifi = new Wifi(now-CHECK_INTERVAL+CONNECT_WAIT, now, false, false, Particle.connected());  // lastAttempt, lastDisconnect, connected, blynk_started, Particle.connected
  Serial.printf("Initializing CLOUD...");
  Particle.disconnect();
  myWifi->lastDisconnect = now;
  WiFi.off();
  myWifi->connected = false;
  if ( debug > 2 ) Serial.printf("wifi disconnect...");
  Serial.printf("done CLOUD\n");

  #ifdef PHOTON
    if ( debug>1 ) { sprintf(buffer, "Particle Photon\n"); Serial.print(buffer); } //Serial1.print(buffer); }
  #else
    if ( debug>1 ) { sprintf(buffer, "Arduino Mega2560\n"); Serial.print(buffer); } //Serial1.print(buffer); }
  #endif

  // Header for debug print
  if ( debug>1 ) print_serial_header();
  if ( debug>3 ) { Serial.print(F("End setup debug message=")); Serial.println(F(", "));};

} // setup


// Loop
void loop()
{

  // 1-wire temp sensor battery temp
  static DS18* sensor_tbatt = new DS18(myPins->pin_1_wire);

  // Sensor conversions
  static Sensors *sen = new Sensors(NOMVBATT, NOMVBATT, NOMTBATT, NOMTBATT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        0, 0, bare_ads, bare_ads_amp); // Manage sensor data    

  unsigned long currentTime;                // Time result
  static unsigned long now = millis();      // Keep track of time
  static unsigned long past = millis();     // Keep track of time
  static int reset = 1;                     // Dynamic reset
  double T = 0;                             // Present update time, s

  // Synchronization
  bool read;                                // Read, T/F
  static Sync *readSensors = new Sync(READ_DELAY);
  bool publishS;                            // Serial print, T/F
  static Sync *publishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  static bool reset_free = false;

  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Keep track of time
  past = now;
  now = millis();
  T = (now - past)/1e3;

  // Read sensors and update filters
  read = readSensors->update(millis(), reset);               //  now || reset
  sen->T =  double(readSensors->updateTime())/1000.0;
  if ( read )
  {
    if ( debug>2 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  ", sen->T, millis());

    // Load and filter
    load(reset_free, sen, sensor_tbatt, myPins, ads, ads_amp, readSensors->now());

  }

  publishS = publishSerial->update(millis(), reset);          //  now || reset
  if ( publishS)
  {
    char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
    controlTime = decimalTime(&currentTime, tempStr);
    hmString = String(tempStr);
    assignPubList(&pubList, publishSerial->now(), unit, hmString, controlTime, sen, numTimeouts);
 
    // Monitor for debug
    if ( debug>0 )
    {
      serial_print(publishSerial->now(), T);
    }

  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  talk();


  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = 0;

} // loop
