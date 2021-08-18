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
#include "blynk.h"              // Only place this can appear is top level main.h

extern const int8_t debug = 2;  // Level of debug printing (2)
extern Publish pubList;
Publish pubList = Publish();
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
Adafruit_ADS1015 *ads;          // Use this for the 12-bit version; 1115 for 16-bit
Adafruit_SSD1306 *display;
bool bare_ads = false;          // If ADS to be ignored
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
  // AD
  Serial.println("Initializing SHUNT MONITOR");
  ads = new Adafruit_ADS1015;
  ads->setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  if (!ads->begin()) {
    Serial.println("FAILE to initialize ADS SHUNT MONITOR.");
    bare_ads = true;
    // while (1);
  }
  Serial.println("SHUNT MONITOR initialized");
  // Display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  Serial.println("Initializing DISPLAY");
  if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 DISPLAY allocation FAILED"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("DISPLAY allocated");
  display->display();   // Adafruit splash
  delay(2000); // Pause for 2 seconds
  display->clearDisplay();

  // Cloud
  unsigned long now = millis();
  myWifi = new Wifi(now-CHECK_INTERVAL+CONNECT_WAIT, now, false, false, Particle.connected());  // lastAttempt, lastDisconnect, connected, blynk_started, Particle.connected
  Serial.printf("Initializing CLOUD...");
  Particle.disconnect();
  myWifi->lastDisconnect = now;
  WiFi.off();
  myWifi->connected = false;
  if ( debug > 2 ) Serial.printf("wifi disconnect...");
  Serial.printf("Setting up blynk...");
  blynk_timer_1.setInterval(PUBLISH_BLYNK_DELAY, publish1);
  blynk_timer_2.setTimeout(1*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_2.setInterval(PUBLISH_BLYNK_DELAY, publish2);});
  blynk_timer_3.setTimeout(2*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_3.setInterval(PUBLISH_BLYNK_DELAY, publish3);});
  blynk_timer_4.setTimeout(3*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_4.setInterval(PUBLISH_BLYNK_DELAY, publish4);});
  if ( myWifi->connected )
  {
    Serial.printf("Begin blynk...");
    Blynk.begin(blynkAuth.c_str());
    myWifi->blynk_started = true;
  }
  Serial.printf("done CLOUD\n");

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
  static General2_Pole* VbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., 0.05, 0.80, 0.1, 20.);       // Sensor noise and general loop filter
  static General2_Pole* TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., 0.05, 0.80, 0.0, 150.);       // Sensor noise and general loop filter
  static General2_Pole* VshuntSenseFilt = new General2_Pole(double(READ_DELAY)/1000., 0.05, 0.80, -0.100, 0.100);       // Sensor noise and general loop filter
  static DS18* sensor_tbatt = new DS18(myPins->pin_1_wire);      // 1-wire temp sensor battery temp
  static Sensors *sen = new Sensors(NOMVBATT, NOMVBATT, NOMTBATT, NOMTBATT, NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT, 0, 0, bare_ads); // Manage sensor data    
  static Battery *myBatt = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells, r1_bb, r2_bb, r2c2_bb);  // Battery model
  static Battery *myBatt_tracked = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells, r1_bb, r2_bb, r2c2_bb);  // Tracked battery model
  unsigned long currentTime;                // Time result
  static unsigned long now = millis();      // Keep track of time
  static unsigned long past = millis();     // Keep track of time
  static int reset = 1;                     // Dynamic reset
  double T = 0;                             // Present update time, s
  const int bare_wait = int(1);             // To simulate peripherals sample time
  // Synchronization
  bool publishP;                            // Particle publish, T/F
  static Sync *publishParticle = new Sync(PUBLISH_PARTICLE_DELAY);
  bool read;                                // Read, T/F
  static Sync *readSensors = new Sync(READ_DELAY);
  bool publishS;                              // Serial print, T/F
  static Sync *publishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  static double soc_est = 1.0;
  static double soc_tracked = 1.0;
  static PID *pid_o = new PID(C_G, C_TAU, C_MAX, C_MIN, C_LLMAX, C_LLMIN, 0, 1, C_DB, 0, 0, 1);       // Observer PID
  static bool reset_soc = true;
  
  // Top of loop
  // Start Blynk, only if connected since it is blocking
  if ( Particle.connected() && !myWifi->blynk_started )
  {
    if ( debug>2 ) Serial.printf("Starting Blynk at %ld...  ", millis());
    Blynk.begin(blynkAuth.c_str());   // blocking if no connection
    myWifi->blynk_started = true;
    reset_soc = false;
    if ( debug>2 ) Serial.printf("completed at %ld\n", millis());
  }
  if ( myWifi->blynk_started && myWifi->connected )
  {
    Blynk.run(); blynk_timer_1.run(); blynk_timer_2.run(); blynk_timer_3.run(); blynk_timer_4.run(); 
  }

  // Request time synchronization from the Particle Cloud once per day
  if (millis() - lastSync > ONE_DAY_MILLIS)
  {
    Particle.syncTime();
    lastSync = millis();
  }

  // Frame control
  publishP = publishParticle->update(now, false);       //  now || false
  publishS = publishSerial->update(now, reset);         //  now || reset
  read = readSensors->update(now, reset);               //  now || reset
  sen->T =  double(readSensors->updateTime())/1000.0;

  // Control References
  past = now;
  now = millis();
  T = (now - past)/1e3;
  if ( bare )
  {
    delay ( bare_wait );
  }

  // Read sensors
  if ( read )
  {
    if ( debug>2 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  ", sen->T, millis());
    // Very simple soc estimation
    if ( sen->Vbatt_filt<=batt_vsat )
    {
      soc_est = max(min( soc_est + sen->Wshunt/NOM_SYS_VOLT*sen->T/3600./NOM_BATT_CAP, 1.0), 0.0);
    }
    else   // >13.7 V is decent approximation for SOC>99.7 for prototype system (constants calculated)
    {
      soc_est = max(min( BATT_SOC_SAT + (sen->Vbatt_filt-batt_vsat)/(batt_vmax-batt_vsat)*(1.0-BATT_SOC_SAT), 1.0), 0.0);
    }
    // Observer CLAW
    pid_o->update((reset>0), sen->Vbatt, sen->Vbatt_model_tracked, sen->T, 1.0, C_MAX);
    soc_tracked = pid_o->cont;
    if ( reset_soc ) soc_est = soc_tracked;
    load(reset, sen->T, sen, sensor_tbatt, VbattSenseFilt, TbattSenseFilt, VshuntSenseFilt, myPins, ads, myBatt, myBatt_tracked, soc_est, soc_tracked);
    if ( bare ) delay(41);  // Usual I2C time
    if ( debug>2 ) Serial.printf("completed load at %ld\n", millis());
    myDisplay(display);
  }

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  if ( publishP || publishS)
  {
    char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
    controlTime = decimalTime(&currentTime, tempStr);
    hmString = String(tempStr);

    pubList.now = now;
    pubList.unit = unit;
    pubList.hmString =hmString;
    pubList.controlTime = controlTime;
    pubList.Vbatt = sen->Vbatt;
    pubList.Vbatt_filt = sen->Vbatt_filt;
    pubList.Tbatt = sen->Tbatt;
    pubList.Tbatt_filt = sen->Tbatt_filt;
    pubList.Vshunt = sen->Vshunt;
    pubList.Vshunt_filt = sen->Vshunt_filt;
    pubList.Ishunt = sen->Ishunt;
    pubList.Ishunt_filt = sen->Ishunt_filt;
    pubList.Wshunt = sen->Wshunt;
    pubList.Wshunt_filt = sen->Wshunt_filt;
    pubList.numTimeouts = numTimeouts;
    pubList.SOC = myBatt->soc()*100.0;
    pubList.SOC_tracked = myBatt_tracked->soc()*100.0;
    pubList.Vbatt_model = sen->Vbatt_model;
    pubList.Vbatt_model_filt = sen->Vbatt_model_filt;
    pubList.Vbatt_model_tracked = sen->Vbatt_model_tracked;
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      static bool led_on = false;
      led_on = !led_on;
      if ( led_on ) digitalWrite(myPins->status_led, HIGH);
      else  digitalWrite(myPins->status_led, LOW);
      publish_particle(now, myWifi);
    }

    // Monitor for debug
    if ( debug>0 && publishS )
    {
      serial_print_inputs(now, T);
    }

  }

  // Initialize complete once sensors and models started
  if ( read ) reset = 0;


} // loop
