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
#include "mySummary.h"

extern int8_t debug;              // Level of debug printing (2)
extern Publish pubList;
Publish pubList = Publish();
extern BlynkParticle Blynk;       // Blynk object
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4; // Time Blynk events
BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;        // Time Blynk events
extern String inputString;        // a string to hold incoming data
extern boolean stringComplete;    // whether the string is complete
extern boolean stepping;          // active step adder
extern double stepVal;            // Step size
extern boolean vectoring;         // Active battery test vector
extern int8_t vec_num;            // Active vector number
extern unsigned long vec_start;   // Start of active vector
extern boolean enable_wifi;       // Enable wifi
extern double socu_free;           // Free integrator state

// Global locals
int8_t debug = 2;
String inputString = "";
boolean stringComplete = false;
boolean stepping = false;
double stepVal = -2;
boolean vectoring = false;
int8_t vec_num = 1;
unsigned long vec_start = 0UL;
boolean enable_wifi = false;

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
const int nsum = 154;           // Number of summary strings, 17 Bytes per isum
retained int isum = -1;         // Summary location.   Begins at -1 because first action is to increment isum
retained Sum_st mySum[nsum];    // Summaries
retained double socu_free = 0.5;// Coulomb Counter state

// Setup
void setup()
{
  // Serial
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:
  Serial.flush();
  delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  Serial.println("Hello!");
  // Serial1.begin(9600); // initialize serial communication at 115200 bits per second:
  // Serial1.flush();
  // delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  // Serial1.println("Hello!");

  // Peripherals
  myPins = new Pins(D6, D7, A1);

  // Status
  pinMode(myPins->status_led, OUTPUT);
  digitalWrite(myPins->status_led, LOW);

  // I2C
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();

  // AD
  Serial.println("Initializing SHUNT MONITOR");
  ads = new Adafruit_ADS1015;
  ads->setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  if (!ads->begin()) {
    Serial.println("FAILE to initialize ADS SHUNT MONITOR.");
    bare_ads = true;
  }
  Serial.println("SHUNT MONITOR initialized");
  // Display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  Serial.println("Initializing DISPLAY");
  if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) // Seems to return true even if depowered
  {
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
    if ( debug>1 ) { sprintf(buffer, "Particle Photon\n"); Serial.print(buffer); } //Serial1.print(buffer); }
  #else
    if ( debug>1 ) { sprintf(buffer, "Arduino Mega2560\n"); Serial.print(buffer); } //Serial1.print(buffer); }
  #endif

  // Summary
  System.enableFeature(FEATURE_RETAINED_MEMORY);
  print_all(mySum, isum, nsum);

  // Header for debug print
  if ( debug>1 ) print_serial_header();
  if ( debug>3 ) { Serial.print(F("End setup debug message=")); Serial.println(F(", "));};

} // setup


// Loop
void loop()
{
  // Sensor noise filters.   Obs filters 0.5 --> 0.1 to eliminate digital instability.   Also rate limited the observer belts+suspenders
  static General2_Pole* VbattSenseFiltObs = new General2_Pole(double(READ_DELAY)/1000., F_O_W, F_O_Z, 0.4*double(NOM_SYS_VOLT), 2.0*double(NOM_SYS_VOLT));
  static General2_Pole* VshuntSenseFiltObs = new General2_Pole(double(READ_DELAY)/1000., F_O_W, F_O_Z, -0.500, 0.500);
  static General2_Pole* VbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, 0.833*double(NOM_SYS_VOLT), 1.15*double(NOM_SYS_VOLT));
  static General2_Pole* TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -20.0, 150.);
  static General2_Pole* VshuntSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -0.500, 0.500);

  // 1-wire temp sensor battery temp
  static DS18* sensor_tbatt = new DS18(myPins->pin_1_wire);

  // Sensor conversions
  static Sensors *sen = new Sensors(NOMVBATT, NOMVBATT, NOMTBATT, NOMTBATT, NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT, 0, 0, bare_ads); // Manage sensor data    

  // Battery  models
  // Solved, driven by socu_s
  static Battery *myBatt_solved = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat);
  // Free, driven by socu_free
  static Battery *myBatt_free = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat);

  // Battery saturation
  static Debounce *saturated_obj = new Debounce(true, SAT_PERSISTENCE);       // Updates persistence

  unsigned long currentTime;                // Time result
  static unsigned long now = millis();      // Keep track of time
  static unsigned long past = millis();     // Keep track of time
  static unsigned long start = millis();    // Keep track of time
  unsigned long elapsed = 0;                // Keep track of time
  static int reset = 1;                     // Dynamic reset
  double T = 0;                             // Present update time, s

  // Synchronization
  bool publishP;                            // Particle publish, T/F
  static Sync *publishParticle = new Sync(PUBLISH_PARTICLE_DELAY);
  bool read;                                // Read, T/F
  static Sync *readSensors = new Sync(READ_DELAY);
  bool publishS;                            // Serial print, T/F
  static Sync *publishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  bool summarizing;                         // Summarize, T/F
  static Sync *summarize = new Sync(SUMMARIZE_DELAY);
  static double socu_solved = 1.0;
  static bool reset_free = false;

  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Start Blynk, only if connected since it is blocking
  if ( Particle.connected() && !myWifi->blynk_started )
  {
    if ( debug>2 ) Serial.printf("Starting Blynk at %ld...  ", millis());
    Blynk.begin(blynkAuth.c_str());   // blocking if no connection
    myWifi->blynk_started = true;
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

  // Keep track of time
  past = now;
  now = millis();
  T = (now - past)/1e3;

  // Read sensors and update filters
  read = readSensors->update(millis(), reset);               //  now || reset
  sen->T =  double(readSensors->updateTime())/1000.0;
  elapsed = readSensors->now() - start;
  if ( read )
  {
    if ( debug>2 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  ", sen->T, millis());

    // Load and filter
    load(reset_free, sen, sensor_tbatt, myPins, ads, readSensors->now());
    filter(reset, sen, VbattSenseFiltObs, VshuntSenseFiltObs, VbattSenseFilt, TbattSenseFilt, VshuntSenseFilt);
    boolean saturated_test = myBatt_solved->sat();
    boolean saturated = saturated_obj->calculate(saturated_test, reset);

    // Battery models
    double Tbatt_filt_C = (sen->Tbatt_filt-32.)*5./9.;
    myBatt_free->calculate(Tbatt_filt_C, socu_free, sen->Ishunt);

    // Solver
    double vbatt_f_o = sen->Vbatt_filt_obs + double(stepping*stepVal);
    int8_t count = 0;
    sen->Vbatt_solved = myBatt_solved->calculate(Tbatt_filt_C, socu_solved, sen->Ishunt_filt_obs);
    double err = vbatt_f_o - sen->Vbatt_solved;
    while( fabs(err)>SOLV_MAX_ERR && count++<SOLV_MAX_COUNTS )
    {
      socu_solved = max(min(socu_solved + max(min( err/myBatt_solved->dv_dsocu(), SOLV_MAX_STEP), -SOLV_MAX_STEP), mxepu_bb), mnepu_bb);
      sen->Vbatt_solved = myBatt_solved->calculate(Tbatt_filt_C, socu_solved, sen->Ishunt_filt_obs);
      err = vbatt_f_o - sen->Vbatt_solved;
      if ( debug == -5 ) Serial.printf("Tbatt_f,Ishunt_f_o,count,socu_s,vbatt_f_o,Vbatt_m_s,err,dv_dsocu, %7.3f,%7.3f,%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
          sen->Tbatt_filt, sen->Ishunt_filt_obs, count, socu_solved, vbatt_f_o, sen->Vbatt_solved, err, myBatt_solved->dv_dsocu());
    }
    // boolean solver_valid = count<SOLV_MAX_COUNTS; 
    
    // SOC Free Integrator - Coulomb Counting method
    static boolean vectoring_past = vectoring;
    if ( vectoring_past != vectoring )
    {
      reset_free = true;
      start = readSensors->now();
      elapsed = 0UL;
    }
    vectoring_past = vectoring;
    socu_free = max(min( socu_free + sen->Wshunt/NOM_SYS_VOLT*min(sen->T, F_MAX_T)/3600./NOM_BATT_CAP, 1.5), 0.);
    if ( saturated ) // Force initialization/reinitialization whenever saturated.   Keeps estimates close to reality
    {
      socu_free = mxepu_bb;
    }
    else if ( reset_free )
    {
      if ( vectoring && elapsed>INIT_WAIT )
      {
        reset_free = false;
        socu_free = socu_solved;
      }
    } 

    // Debug print statements
    if ( debug == -2 )
      Serial.printf("T,reset_free,vectoring,saturated,Tbatt,Ishunt,Vb_f_o,soc_s,soc_f,Vb_s,dvdsoc,T,count,tcharge,  %ld,%d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,\n",
      elapsed, reset_free, vectoring, saturated, sen->Tbatt, sen->Ishunt_filt_obs, sen->Vbatt_filt_obs+double(stepping*stepVal),
      socu_solved, socu_free, sen->Vbatt_solved, myBatt_solved->dv_dsocu(), sen->T, count, myBatt_free->tcharge());

    //if ( bare ) delay(41);  // Usual I2C time
    if ( debug>2 ) Serial.printf("completed load at %ld\n", millis());


    // Update display
    myDisplay(display);
  }

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  publishP = publishParticle->update(millis(), false);       //  now || false
  publishS = publishSerial->update(millis(), reset);         //  now || reset
  if ( publishP || publishS)
  {
    char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
    controlTime = decimalTime(&currentTime, tempStr);
    hmString = String(tempStr);
    assignPubList(&pubList, publishParticle->now(), unit, hmString, controlTime, sen, numTimeouts, myBatt_solved, myBatt_free);
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      // static bool led_on = false;
      // led_on = !led_on;
      // if ( led_on ) digitalWrite(myPins->status_led, HIGH);
      // else  digitalWrite(myPins->status_led, LOW);
      publish_particle(publishParticle->now(), myWifi, enable_wifi);
    }

    // Monitor for debug
    if ( debug>0 && publishS )
    {
      serial_print(publishSerial->now(), T);
    }

  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  int debug_saved = debug;
  talk(&stepping, &stepVal, &vectoring, &vec_num, myBatt_solved, myBatt_free);

  // Summary management
  if ( debug == -3 )
  {
    debug = debug_saved;
    print_all(mySum, isum, nsum);
  }
  summarizing = summarize->update(millis(), reset);               //  now || reset
  if ( summarizing )
  {
    if ( ++isum>nsum-1 ) isum = 0;
    mySum[isum].assign(currentTime, sen->Tbatt_filt, sen->Vbatt_filt_obs, sen->Ishunt_filt_obs,
      socu_solved, socu_free, myBatt_solved->dv_dsocu());
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = 0;

} // loop
