/*
 * Project SOC_Photon
  * Description:
  * Monitor battery State of Charge (SOC) using Coulomb Counting (CC).  An experimental
  * Extended Kalman Filter (EKF) method is developed alongside though not used to
  * improve the CC yet.
  * By:  Dave Gutz September 2021
  * 09-Aug-2021   Initial Git committ.   Unamplified ASD1013 12-bit shunt voltage sensor
  * ??-Sep-2021   Added 1 Hz anti-alias filters (AAF) in hardware to cleanup the 60 Hz
  * inverter noise on Vb and Ib.
  * 27-Oct-2021   Add amplified (OPA333) current sensor ASD1013 with Texas Instruments (TI)
  * amplifier design in hardware
  * 27-Aug-2021   First working prototype with iterative solver SOC-->Vb from polynomial
  * that have coefficients in tables
  * 22-Dec-2021   Mark last good working version before class code.  EKF functional
  * 26-Dec-2021   Put in class code for Monitor and Model
  * ??-Jan-2021   Vb model in tables.  Add battery heater in hardware
  * 03-Mar-2022   Manually tune for current sensor errors.   Vb model in tables
  * 21-Apr-2022   Add Tweak methods to dynamically determine current sensor erros
  * 
//
// MIT License
//
// Copyright (C) 2022 - Dave Gutz
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

// Dependent includes.   Easier to rp.debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"
#include "Blynk/blynk.h"              // Only place this can appear is top level main.h
#include "mySummary.h"
#include "myCloud.h"
#include "Tweak.h"
#include "debug.h"

extern BlynkParticle Blynk;       // Blynk object
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4; // Time Blynk events
BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;        // Time Blynk events
extern CommandPars cp;            // Various parameters to be common at system level
extern RetainedPars rp;           // Various parameters to be static at system level
extern Sum_st mySum[NSUM];        // Summaries for saving charge history
extern PublishPars pp;            // For publishing

// Global locals
retained RetainedPars rp;             // Various control parameters static at system level
retained CommandPars cp = CommandPars(); // Various control parameters commanding at system level
retained Sum_st mySum[NSUM];          // Summaries
PublishPars pp = PublishPars();       // Common for publishing
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm
double control_time = 0.0;      // Decimal time, seconds since 1/1/2021
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_SSD1306 *display;      // Main OLED display
Wifi *myWifi;                   // Manage Wifi

// Setup
void setup()
{
  // Serial
  Serial.begin(115200);
  Serial.flush();
  delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  Serial.println("Hello!");
  // Bluetooth (hardware didn't work)
  // Serial1.begin(9600);
  // Serial1.flush();
  // delay(1000);          // Ensures a clean display on Arduino Serial startup on CoolTerm
  // Serial1.println("Hello!");

  // Peripherals
  myPins = new Pins(D6, D7, A1, D2);

  // Status
  pinMode(myPins->status_led, OUTPUT);
  digitalWrite(myPins->status_led, LOW);

  // Initialize output (Used in Test mode only)
  pinMode(myPins->pwm_pin, OUTPUT);
  pwm_write(0, myPins);

  // I2C
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();

  // AD
  // Shunts initialized in Sensors as static loop() instantiation

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
  Time.zone(GMT);
  unsigned long now = millis();
  myWifi = new Wifi(now-CHECK_INTERVAL+CONNECT_WAIT, now, false, false, Particle.connected());  // lastAttempt, last_disconnect, connected, blynk_started, Particle.connected
  Serial.printf("Initializing CLOUD...");
  Particle.disconnect();
  myWifi->last_disconnect = now;
  WiFi.off();
  myWifi->connected = false;
  if ( rp.debug >= 100 ) Serial.printf("wifi disconnect...");
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

  // Clean boot logic.  This occurs only when doing a structural rebuild clean make on initial flash, because
  // the SRAM is not explicitly initialized.   This is by design, as SRAM must be remembered between boots.
#ifdef BOOT_CLEAN
  rp.nominal();
  Serial.printf("Force nominal rp %s\n", cp.buffer);
  rp.pretty_print();
#endif
  if ( rp.is_corrupt() ) 
  {
    rp.nominal();
    Serial.printf("\n************************WARNING(setup):  Forced nominal SRAM ****************************** %s\n", cp.buffer);
  }

  #ifdef PHOTON
    if ( rp.debug>101 ) { sprintf(cp.buffer, "Particle Photon\n"); Serial.print(cp.buffer); } //Serial1.print(buffer); }
  #else
    if ( rp.debug>101 ) { sprintf(cp.buffer, "Arduino Mega2560\n"); Serial.print(cp.buffer); } //Serial1.print(buffer); }
  #endif

  // Determine millis() at turn of Time.now
  long time_begin = Time.now();
  while ( Time.now()==time_begin )
  {
    delay(1);
    millis_flip = millis()%1000;
  }

  // Summary
  System.enableFeature(FEATURE_RETAINED_MEMORY);
  if ( rp.debug==2 )
    print_all_summary(mySum, rp.isum, NSUM);

  // Header for rp.debug print
  if ( rp.debug>101 ) print_serial_header();
  if ( rp.debug>103 ) { Serial.print(F("End setup")); Serial.println(F(", ")); };

} // setup


// Loop
void loop()
{
  // Sensor conversions
  static Sensors *Sen = new Sensors(0, 0, myPins->pin_1_wire); // Manage sensor data
  static double t_bias_last;  // Memory for rate limiter in filter_temp call, deg C

   // Mon, used to count Coulombs and run EKF
  static BatteryMonitor *Mon = new BatteryMonitor(&rp.delta_q, &rp.t_last, &rp.nP, &rp.nS, &rp.mon_mod);

  // Sim, used to model Vb and Ib.   Use Talk 'Xp?' to toggle model on/off. 
  static BatteryModel *Sim = new BatteryModel(&rp.delta_q_model, &rp.t_last_model, &rp.s_cap_model, &rp.nP, &rp.nS, &rp.sim_mod);

  // Battery saturation debounce
  static TFDelay *Is_sat_delay = new TFDelay();   // Time persistence

  unsigned long current_time;               // Time result
  static unsigned long now = millis();      // Keep track of time
  time32_t time_now;                        // Keep track of time
  static unsigned long start = millis();    // Keep track of time
  unsigned long elapsed = 0;                // Keep track of time
  static boolean reset = true;              // Dynamic reset
  static boolean reset_temp = true;         // Dynamic reset
  static boolean reset_publish = true;      // Dynamic reset
  
  // Synchronization
  boolean publishP;                           // Particle publish, T/F
  static Sync *PublishParticle = new Sync(PUBLISH_PARTICLE_DELAY);
  boolean publishB;                           // Particle publish, T/F
  static Sync *PublishBlynk = new Sync(PUBLISH_BLYNK_DELAY);
  boolean read;                               // Read, T/F
  static Sync *ReadSensors = new Sync(READ_DELAY);
  boolean filt;                               // Filter, T/F
  static Sync *FilterSync = new Sync(FILTER_DELAY);
  boolean read_temp;                          // Read temp, T/F
  static Sync *ReadTemp = new Sync(READ_TEMP_DELAY);
  boolean publishS;                           // Serial print, T/F
  static Sync *PublishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  boolean display_to_user;                    // User display, T/F
  static Sync *DisplayUserSync = new Sync(DISPLAY_USER_DELAY);
  boolean summarizing;                        // Summarize, T/F
  static boolean boot_wait = true;  // waiting for a while before summarizing
  static Sync *Summarize = new Sync(SUMMARIZE_DELAY);
  boolean control;                            // Summarize, T/F
  static Sync *ControlSync = new Sync(CONTROL_DELAY);
  static uint8_t last_publishS_debug = 0;     // Remember first time with new debug to print headers
 
  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Start Blynk, only if connected since it is blocking
  if ( Particle.connected() && !myWifi->blynk_started )
  {
    if ( rp.debug>102 ) Serial.printf("Starting Blynk at %ld...  ", millis());

    Blynk.begin(blynkAuth.c_str());   // warning:  blocks if no connection
    myWifi->blynk_started = true;

    if ( rp.debug>102 ) Serial.printf("completed at %ld\n", millis());
  }
  if ( myWifi->blynk_started && myWifi->connected )
  {
    Blynk.run();
    blynk_timer_1.run();
    blynk_timer_2.run();
    blynk_timer_3.run();
    blynk_timer_4.run(); 
  }

  // Keep time
  now = millis();
  time_now = Time.now();
  sync_time(now, &last_sync, &millis_flip);      // Refresh time synchronization

  // Load temperature only
  // Outputs:   Sen->Tbatt,  Sen->Tbatt_filt
  read_temp = ReadTemp->update(millis(), reset);              //  now || reset
  if ( read_temp )
  {
    Sen->T_temp =  ReadTemp->updateTime();
    load_temp(Sen);
    filter_temp(reset_temp, T_RLIM, Sen, rp.t_bias, &t_bias_last);
  }

  // Input all other sensors and do high rate calculations
  read = ReadSensors->update(millis(), reset);               //  now || reset
  elapsed = ReadSensors->now() - start;
  if ( read )
  {
    Sen->T =  ReadSensors->updateTime();
    if ( rp.debug>102 || rp.debug==-13 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  \n", Sen->T, millis());

    // Read sensors, model signals, select between them
    // Inputs:  rp.config, rp.sim_mod
    // Outputs: Sen->Ishunt, Sen->Vbatt, Sen->Tbatt_filt, rp.duty
    sense_synth_select(reset, reset_temp, ReadSensors->now(), elapsed, myPins, Mon, Sim, Sen);

    // Calculate Ah remaining
    // Inputs:  rp.mon_mod, Sen->Ishunt, Sen->Vbatt, Sen->Tbatt_filt
    // States:  Mon.soc
    // Outputs: tcharge_wt, tcharge_ekf
    monitor(reset, reset_temp, now, Is_sat_delay, Mon, Sim, Sen);

    // Adjust current sensors
    tweak_on_new_desat(Sen, now);

    // Re-init Coulomb Counter to EKF if it is different than EKF or if never saturated
    Mon->regauge(Sen->Tbatt_filt);

    //////////////////////////////////////////////////////////////

    //
    // Useful for Arduino plotting
    if ( rp.debug==-1 ) debug_m1(Mon, Sim, Sen); // General purpose Arduino
    if ( rp.debug==12 ) debug_12(Mon, Sim, Sen);  // EKF
    if ( rp.debug==-12 ) debug_m12(Mon, Sim, Sen);  // EKF Arduino
    if ( rp.debug==-3 ) debug_m3(Mon, Sim, Sen, control_time, elapsed, reset);  // Power Arduino

  }  // end read (high speed frame)

  // Run filters on other signals
  filt = FilterSync->update(millis(), reset);               //  now || reset
  if ( filt )
  {

    // Empty battery
    if ( rp.modeling && reset && Sim->q()<=0. ) Sen->Ishunt = 0.;

    // Arduino plot
    if ( rp.debug==-35 ) debug_m35(Mon, Sim, Sen); // EKF Arduino

  }

  // Control
  control = ControlSync->update(millis(), reset);               //  now || reset
  if ( control )
  {
    pwm_write(rp.duty, myPins);
    if ( rp.debug>102 ) Serial.printf("completed control at %ld.  rp.duty=%ld\n", millis(), rp.duty);
  }

  // OLED display driver
  display_to_user = DisplayUserSync->update(millis(), reset);  //  now || reset
  if ( display_to_user )
  {
    oled_display(display, Sen);
  }

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  publishP = PublishParticle->update(millis(), false);        //  now || false
  publishB = PublishBlynk->update(millis(), false);           //  now || false
  publishS = PublishSerial->update(millis(), reset_publish);  //  now || reset_publish
  if ( publishP || publishS)
  {
    char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
    control_time = decimalTime(&current_time, tempStr, now, millis_flip);
    hm_string = String(tempStr);
    assign_publist(&pp.pubList, PublishParticle->now(), unit, hm_string, control_time, Sen, num_timeouts, Sim, Mon);
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      publish_particle(PublishParticle->now(), myWifi, cp.enable_wifi);
    }
    if ( reset_publish )
      digitalWrite(myPins->status_led, HIGH);
    else
      digitalWrite(myPins->status_led, LOW);

// Mon for rp.debug
    if ( publishS )
    {
      if ( rp.debug==2 || rp.debug==4 )
      {
        if ( reset_publish || (last_publishS_debug != rp.debug) ) print_serial_header();
        serial_print(PublishSerial->now(), Sen->T);
      }

      if ( rp.debug==-4 )
      {
        debug_m4(Mon, Sim, Sen);
      }
      last_publishS_debug = rp.debug;
    }

  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  talk(Mon, Sim, Sen);

  // Summary management.   Every boot after a wait an initial summary is saved in rotating buffer
  // Then every half-hour unless modeling.   Can also request manually via cp.write_summary (Talk)
  boolean boot_summ = boot_wait && ( elapsed >= SUMMARIZE_WAIT );
  if ( elapsed >= SUMMARIZE_WAIT ) boot_wait = false;
  summarizing = Summarize->update(millis(), boot_summ, !rp.modeling); // now || boot_summ && !rp.modeling
  summarizing = summarizing || (rp.debug==-11 && publishB);
  if ( !boot_wait && (summarizing || cp.write_summary) )
  {
    if ( ++rp.isum>NSUM-1 ) rp.isum = 0;
    mySum[rp.isum].assign(time_now, Sen->Tbatt_filt, Sen->Vbatt, Sen->Ishunt,
                          Mon->soc_ekf(), Mon->soc(), Mon->voc_dyn(), Mon->voc(),
                          Sen->ShuntAmp->tweak_bias(), Sen->ShuntNoAmp->tweak_bias());
    if ( rp.debug==0 ) Serial.printf("Summarized.....................\n");
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = false;
  if ( read_temp && elapsed>TEMP_INIT_DELAY ) reset_temp = false;
  if ( publishP || publishS ) reset_publish = false;

  // Soft reset
  if ( cp.soft_reset )
  {
    reset = true;
    reset_temp = true;
    reset_publish = true;
    Serial.printf("soft reset initiated...\n");
  }
  cp.soft_reset = false;
  cp.write_summary = false;

} // loop
