/*
 * Project Vent_Photon
  * Description:
  * Combine digital pot output in parallel with manual pot
  * to control an ECMF-150 TerraBloom brushless DC servomotor fan.
  * 
  * By:  Dave Gutz January 2021
  * 07-Jan-2021   A tinker version
  * 18-Feb-2021   Cleanup working version
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
SYSTEM_THREAD(ENABLED); // Make sure code always run regardless of network status
#include <Arduino.h>     // Used instead of Print.h - breaks Serial
#else
#undef PHOTON
using namespace std;
#undef max
#undef min
#endif

#include "constants.h"
#include "myAuth.h"
/* This file myAuth.h is not in Git repository because it contains personal information.
Make it yourself.   It should look like this, with your personal authorizations:
(Note:  you don't need a valid number for one of the blynkAuth if not using it.)
#ifndef BARE_PHOTON
  const   String      blynkAuth     = "4f1de4949e4c4020882efd3e61XXX6cd"; // Photon thermostat
#else
  const   String      blynkAuth     = "d2140898f7b94373a78XXX158a3403a1"; // Bare photon
#endif
*/

// Dependent includes.   Easier to debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"
#include "myCloud.h"
#include "blynk.h"              // Only place this can appear is top level main.h

extern const int8_t debug = 2;  // Level of debug printing (3)
extern Publish pubList;
Publish pubList = Publish();
extern int badWeatherCall;      // webhook lookup counter
extern long updateweatherhour;  // Last hour weather updated
extern bool weatherGood;        // webhook OAT lookup successful, T/F
int badWeatherCall  = 0;
long updateweatherhour;         // Last hour weather updated
bool weatherGood;               // webhook OAT lookup successful, T/F

extern BlynkParticle Blynk;      // Blynk object
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4; // Time Blynk events
BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;        // Time Blynk events

// Global locals
char buffer[256];               // Serial print buffer
int numTimeouts = 0;            // Number of Particle.connect() needed to unfreeze
String hmString = "00:00";      // time, hh:mm
double controlTime = 0.0;       // Decimal time, seconds since 1/1/2021
unsigned long lastSync = millis();// Sync time occassionally.   Recommended by Particle.
Pins *myPins;                   // Photon hardware pin mapping used

Adafruit_ADS1015 ads;     /* Use this for the 12-bit version; 1115 for 16-bit */

// Setup
void setup()
{
  // Serial
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:
  Serial.flush();
  delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  Serial.println("Hello!");

  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 0.256V (1 bit = 0.125mV/ADS1015, 0.1875mV/ADS1115)");
  ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  // Peripherals
  myPins = new Pins(D6, D2, D7, A1, A2, A3);
  if ( !bare )
  {
    // Status
    pinMode(myPins->status_led, OUTPUT);
    digitalWrite(myPins->status_led, LOW);

    // I2C
    if ( !bare )
    {
      Wire.setSpeed(CLOCK_SPEED_100KHZ);
      Wire.begin();
    }
  }
  else
  {
    // Status
    pinMode(myPins->status_led, OUTPUT);
    digitalWrite(myPins->status_led, LOW);
  }

  // Begin
  Particle.connect();
  Particle.function("HOLD", particleHold);
  Particle.function("SET",  particleSet);
  blynk_timer_1.setInterval(PUBLISH_DELAY, publish1);
  blynk_timer_2.setTimeout(1*PUBLISH_DELAY/4, [](){blynk_timer_2.setInterval(PUBLISH_DELAY, publish2);});
  blynk_timer_3.setTimeout(2*PUBLISH_DELAY/4, [](){blynk_timer_3.setInterval(PUBLISH_DELAY, publish3);});
  blynk_timer_4.setTimeout(3*PUBLISH_DELAY/4, [](){blynk_timer_4.setInterval(PUBLISH_DELAY, publish4);});
  Blynk.begin(blynkAuth.c_str());

  #ifdef PHOTON
    if ( debug>1 ) { sprintf(buffer, "Particle Photon.  bare = %d,\n", bare); Serial.print(buffer); };
  #else
    if ( debug>1 ) { sprintf(buffer, "Arduino Mega2560.  bare = %d,\n", bare); Serial.print(buffer); };
  #endif

  // Header for debug print
  if ( debug>1 )
  { 
    print_serial_header();
  }

  if ( debug>3 ) { Serial.print(F("End setup debug message=")); Serial.println(F(", "));};

} // setup


// Loop
void loop()
{
  Serial.println("-----------------------------------------------------------");
  Serial.print("AIN0_1: "); Serial.print(adc0_1); Serial.print("  "); Serial.printf("%7.6f", volts0_1); Serial.println("V");
  static General2_Pole* VbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., 0.05, 0.80, 0.0, 120.);       // Sensor noise and general loop filter
  static General2_Pole* TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., 0.05, 0.80, 0.0, 120.);       // Sensor noise and general loop filter
  static General2_Pole* VshuntSenseFilt = new General2_Pole(double(READ_DELAY)/1000., 0.05, 0.80, 0.0, 120.);       // Sensor noise and general loop filter
  static DS18* sensor_tbatt = new DS18(myPins->pin_1_wire);
  static Sensors *sen = new Sensors(NOMSET, NOMSET, NOMSET, NOMSET, 32, 0, 0, 0, NOMSET, 0,
                              NOMSET, 999, true, true, true, NOMSET, POT, 0, 0, 0);                                      // Sensors

  unsigned long currentTime;                // Time result
  static unsigned long now = millis();      // Keep track of time
  static unsigned long past = millis();     // Keep track of time
  static int reset = 1;                     // Dynamic reset
  double T = 0;                             // Present update time, s
  const int bare_wait = int(1);             // To simulate peripherals sample time
  bool checkPot = false;                    // Read the POT, T/F
  // Synchronization
  bool publishP;                            // Particle publish, T/F
  static Sync *publishParticle = new Sync(PUBLISH_PARTICLE_DELAY);
  bool readTp;                              // Special sequence to read Tp affected by PWM noise with duty>0, T/F
  static Sync *readPlenum = new Sync(READ_TBATT_DELAY);
  bool read;                                // Read, T/F
  static Sync *readSensors = new Sync(READ_DELAY);
  bool query;                               // Query schedule and OAT, T/F
  static Sync *queryWeb = new Sync(QUERY_DELAY);
  bool serial;                              // Serial print, T/F
  static Sync *serialDebug = new Sync(SERIAL_DELAY);
  bool control;                             // Control sequence, T/F
  static Sync *controlFrame = new Sync(CONTROL_DELAY);

  // Top of loop
  // Start Blynk
  Blynk.run(); blynk_timer_1.run(); blynk_timer_2.run(); blynk_timer_3.run(); blynk_timer_4.run(); 

  // Request time synchronization from the Particle Cloud once per day
  if (millis() - lastSync > ONE_DAY_MILLIS)
  {
    Particle.syncTime();
    lastSync = millis();
  }

  // Frame control
  publishP = publishParticle->update(now, false);
  readTbatt = readTbatt->update(now, reset);
  read = readSensors->update(now, reset, !publishP);
  sen->T =  double(readSensors->updateTime())/1000.0;
  query = queryWeb->update(reset, now, !read);
  serial = serialDebug->update(false, now, !query);

  // Control References
  past = now;
  now = millis();
  T = (now - past)/1e3;
  control = controlFrame->update(reset, now, true);
  if ( bare )
  {
    delay ( bare_wait );
  }

  if ( true ) digitalWrite(myPins->status_led, HIGH);
  else  digitalWrite(myPins->status_led, LOW);

  // Read sensors
  if ( read )
  {
    if ( debug>2 ) Serial.printf("Read update=%7.3f\n", sen->T);
    load(reset, sen->T, sen, sensor_tbatt, VbattSenseFilt, TbattSenseFilt, VshuntSenseFilt, myPins);
    if ( bare ) delay(41);  // Usual I2C time
  }

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  if ( publishP || serial)
  {
    pubList.now = now;
    pubList.unit = unit;
    pubList.hmString =hmString;
    pubList.controlTime = controlTime;
    pubList.Tp = sen->Tp;
    pubList.Ta = sen->Ta;
    pubList.T = sen->T;
    pubList.OAT = sen->OAT;
    pubList.Ta_obs = sen->Ta_obs;
    pubList.I2C_status = sen->I2C_status;
    pubList.pcnt_pot = sen->pcnt_pot;
    pubList.Ta_filt = sen->Ta_filt;
    pubList.hum = sen->hum;
    pubList.numTimeouts = numTimeouts;
    pubList.held = sen->held;
    pubList.mdot = sen->mdot;
    pubList.mdot_lag = sen->mdot_lag;
    sen->webHold = pubList.webHold;
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      if ( debug>2 ) Serial.println(F("publish"));
      publish_particle(now);
    }

    // Monitor for debug
    if ( debug>0 && serial )
    {
      serial_print_inputs(now, T);
    }

  }

  // Initialize complete once sensors and models started
  if ( read ) reset = 0;


} // loop
