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
#if (PLATFORM_ID==6)
#define PHOTON
//#define BOOT_CLEAN        // Use this to clear 'lockup' problems introduced during testing using Talk
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

// Dependent includes.   Easier to rp.debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"
#include "Blynk/blynk.h"              // Only place this can appear is top level main.h
#include "mySummary.h"
#include "myCloud.h"

extern BlynkParticle Blynk;       // Blynk object
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4; // Time Blynk events
BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;        // Time Blynk events
extern CommandPars cp;            // Various parameters to be common at system level
extern RetainedPars rp;           // Various parameters to be static at system level
extern Sum_st mySum[NSUM];        // Summaries for saving charge history

// Global locals
retained RetainedPars rp;             // Various control parameters static at system level
retained CommandPars cp = CommandPars(); // Various control parameters commanding at system level
retained Sum_st mySum[NSUM];          // Summaries
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm
double control_time = 0.0;      // Decimal time, seconds since 1/1/2021
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_ADS1015 *ads_amp;      // Use this for the 12-bit version; 1115 for 16-bit; amplified; different address
Adafruit_ADS1015 *ads_noamp;    // Use this for the 12-bit version; 1115 for 16-bit; non-amplified
Adafruit_SSD1306 *display;
boolean bare_ads_noamp = false; // If ADS to be ignored
boolean bare_ads_amp = false;   // If ADS to be ignored
Wifi *myWifi;                   // Manage Wifi

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
  // Amped
  Serial.println("Initializing SHUNT MONITORS");
  ads_amp = new Adafruit_ADS1015;
  ads_amp->setGain(GAIN_EIGHT, GAIN_TWO);    // First argument is differential, second is single-ended.
  // 8 was used by Texas Instruments in their example implementation.   16 was used by another
  // Particle user in their non-amplified implementation.
  // TODO:  why 8 scaled by R/R gives same result as 16 for ads_noamp?
  if (!ads_amp->begin((0x49))) {
    Serial.println("FAILED to initialize ADS AMPLIFIED SHUNT MONITOR.");
    bare_ads_amp = true;
  }
  // Non-amped
  ads_noamp = new Adafruit_ADS1015;
  ads_noamp->setGain(GAIN_SIXTEEN, GAIN_SIXTEEN); // 16x gain differential and single-ended  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  if (!ads_noamp->begin()) {
    Serial.println("FAILED to initialize ADS SHUNT MONITOR.");
    bare_ads_noamp = true;
  }
  Serial.println("SHUNT MONITORS initialized");
  
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

#ifdef BOOT_CLEAN
  rp.nominal();
  rp.print_part_1(cp.buffer);
  Serial.printf("Force nominal rp %s", cp.buffer);
  rp.print_part_2(cp.buffer);
  Serial.printf("%s", cp.buffer);
#endif

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
  if ( rp.debug>103 ) { Serial.print(F("End setup rp.debug message=")); Serial.println(F(", ")); };

} // setup


// Loop
void loop()
{
  // Sensor noise filters.   There are AAF in hardware for Vbatt and VshuntAmp and VshuntNoAmp
  // static General2_Pole* VbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -50., 50.);
  // static General2_Pole* IshuntSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -500., 500.);
  static General2_Pole* TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);

  // 1-wire temp sensor battery temp
  static DS18* SensorTbatt = new DS18(myPins->pin_1_wire, temp_parasitic, temp_delay);

  // Sensor conversions
  static Sensors *Sen = new Sensors(NOMVBATT, NOMTBATT, NOMTBATT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        0, 0, 0, bare_ads_noamp, bare_ads_amp); // Manage sensor data
  static SlidingDeadband *SdTbatt = new SlidingDeadband(HDB_TBATT);
  static double t_bias_last;  // Memory for rate limiter in filter_temp call, deg C

  // Battery  models
  // Free, driven by soc
  // Instantiate the table
  static BatteryMonitor *Mon = new BatteryMonitor(batt_num_cells, batt_r1, batt_r2, batt_r2c2, batt_vsat,
    dvoc_dt, q_cap_rated, RATED_TEMP, t_rlim, -1.);
  // Sim, driven by soc, used to get Vbatt.   Use Talk 'x' to toggle model on/off. 
  static BatteryModel *Sim = new BatteryModel(batt_num_cells, batt_r1, batt_r2, batt_r2c2, batt_vsat,
    dvoc_dt, q_cap_rated, RATED_TEMP, t_rlim, 1.);

  // Battery saturation
  static Debounce *SatDebounce = new Debounce(true, SAT_PERSISTENCE);       // Updates persistence

  unsigned long current_time;               // Time result
  static unsigned long now = millis();      // Keep track of time
  time32_t time_now;                        // Keep track of time
  static unsigned long start = millis();    // Keep track of time
  unsigned long elapsed = 0;                // Keep track of time
  static boolean reset = true;              // Dynamic reset
  static boolean reset_temp = true;         // Dynamic reset
  static boolean reset_publish = true;      // Dynamic reset
  
  // Synchronization
  boolean publishP;                            // Particle publish, T/F
  static Sync *PublishParticle = new Sync(PUBLISH_PARTICLE_DELAY);
  boolean publishB;                            // Particle publish, T/F
  static Sync *PublishBlynk = new Sync(PUBLISH_BLYNK_DELAY);
  boolean read;                                // Read, T/F
  static Sync *ReadSensors = new Sync(READ_DELAY);
  boolean filt;                                // Filter, T/F
  static Sync *FilterSync = new Sync(FILTER_DELAY);
  boolean read_temp;                           // Read temp, T/F
  static Sync *ReadTemp = new Sync(READ_TEMP_DELAY);
  boolean publishS;                            // Serial print, T/F
  static Sync *PublishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  boolean display_to_user;                     // User display, T/F
  static Sync *DisplayUserSync = new Sync(DISPLAY_USER_DELAY);
  boolean summarizing;                         // Summarize, T/F
  static boolean summarizing_waiting = true;  // waiting for a while before summarizing
  static Sync *Summarize = new Sync(SUMMARIZE_DELAY);
  boolean control;                         // Summarize, T/F
  static Sync *ControlSync = new Sync(CONTROL_DELAY);
  static uint8_t last_publishS_debug = 0;    // Remember first time with new debug to print headers

  
  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Start Blynk, only if connected since it is blocking
  if ( Particle.connected() && !myWifi->blynk_started )
  {
    if ( rp.debug>102 ) Serial.printf("Starting Blynk at %ld...  ", millis());
    Blynk.begin(blynkAuth.c_str());   // blocking if no connection
    myWifi->blynk_started = true;
    if ( rp.debug>102 ) Serial.printf("completed at %ld\n", millis());
  }
  if ( myWifi->blynk_started && myWifi->connected )
  {
    Blynk.run(); blynk_timer_1.run(); blynk_timer_2.run(); blynk_timer_3.run(); blynk_timer_4.run(); 
  }

  // Keep time
  now = millis();
  time_now = Time.now();
  sync_time(now, &last_sync, &millis_flip);      // Refresh time synchronization

  // Input temperature only
  read_temp = ReadTemp->update(millis(), reset);              //  now || reset
  if ( read_temp )
  {
    Sen->T_temp =  ReadTemp->updateTime();
    if ( rp.debug>102 ) Serial.printf("Read temp update=%7.3f and performing load_temp() at %ld...  \n", Sen->T_temp, millis());

    // Load and filter temperature only
    load_temp(Sen, SensorTbatt, SdTbatt);
    if ( rp.debug>102 ) Serial.printf("Read temp update=%7.3f and done       load_temp() at %ld...  \n", Sen->T_temp, millis());
    filter_temp(reset_temp, t_rlim, Sen, TbattSenseFilt, rp.t_bias, &t_bias_last);
  }

  // Input all other sensors and do high rate calculations
  read = ReadSensors->update(millis(), reset);               //  now || reset
  elapsed = ReadSensors->now() - start;
  if ( read )
  {
    Sen->T =  ReadSensors->updateTime();
    if ( rp.debug>102 || rp.debug==-13 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  \n", Sen->T, millis());

    // Load and filter
    load(reset, Sen, myPins, ads_amp, ads_noamp, ReadSensors->now());
    
    // Arduino plots
    if ( rp.debug==-7 ) Serial.printf("%7.3f,%7.3f,%7.3f,   %7.3f, %7.3f, %7.3f,\n",
        Mon->soc(), Sen->Ishunt_amp_cal, Sen->Ishunt_noamp_cal,
        Sen->Vbatt, Sim->voc_stat(), Sim->voc());

    //
    // Sim used for built-in testing (rp.modeling = true and jumper wire).   Needed here in this location
    // to have availabe a value for Sen->Tbatt_filt when called
    //  Inputs:
    //    Sen->Ishunt     A
    //    Sen->Vbatt      V
    //    Sen->Tbatt_filt deg C
    //    Cc    Coulomb charge counter memory structure
    //  Outputs:
    //    tb              deg C
    //    ib              A
    //    vb              V
    //    rp.duty         (0-255) for D2 hardware injection when rp.modeling

    // Initialize as needed
    if ( reset )
    {
      Sim->load(rp.delta_q_model, rp.t_last_model, rp.s_cap_model);
      Sim->apply_delta_q_t(rp.delta_q_model, rp.t_last_model);
      Sim->init_battery();  // for cp.soft_reset
    }

    // Sim calculation
    Sen->Vbatt_model = Sim->calculate(Sen->Tbatt_filt, Sim->soc(), Sen->Ishunt, min(Sen->T, F_MAX_T),
        Sim->q_capacity(), Sim->q_cap_rated(), cp.dc_dc_on);
    cp.model_cutback = Sim->cutback();
    cp.model_saturated = Sim->saturated();

    // Use model instead of sensors when running tests as user
    // Over-ride Ishunt, Vbatt and Tbatt with model when running tests.  rp.modeling should never be set in use
    if ( rp.modeling )
    {
      Sen->Ishunt = Sim->ib();
      Sen->Vbatt = Sen->Vbatt_model;
      Sen->Tbatt_filt = Sim->temp_c();
    }

    // Charge calculation and memory store
    Sim->count_coulombs(Sen->T, reset, Sen->Tbatt_filt, Sen->Ishunt, rp.t_last_model);
    Sim->update(&rp.delta_q_model, &rp.t_last_model);

    // D2 signal injection to hardware current sensors
    rp.duty = Sim->calc_inj_duty(now, rp.type, rp.amp, rp.freq);
    ////////////////////////////////////////////////////////////////////////////

    //
    // Main Battery
    //  Inputs:
    //    Sen->Ishunt     A
    //    Sen->Vbatt      V
    //    Sen->Tbatt_filt deg C
    //    Cc    Coulomb charge counter memory structure

    // Initialize Cc structure if needed.   Needed here in this location to have a value for Sen->Tbatt_filt
    if ( reset )
    {
      Mon->load(rp.delta_q, rp.t_last, rp.delta_q_inf);
      Mon->apply_delta_q_t(rp.delta_q, rp.t_last, rp.delta_q_inf);
      Mon->init_battery();  // for cp.soft_reset
      if ( rp.modeling )
        Mon->init_soc_ekf(Sim->soc());  // When modeling, ekf wants to equal model
      else
        Mon->init_soc_ekf(Mon->soc());
      Mon->init_hys(0.0);
    }
    
    // EKF - calculates temp_c_, voc_, voc_dyn_ as functions of sensed parameters vb & ib (not soc)
    Mon->calculate_ekf(Sen->Tbatt_filt, Sen->Vbatt, Sen->Ishunt,  min(Sen->T, F_MAX_T));
    
    // Debounce saturation calculation done in ekf using voc model
    Sen->saturated = SatDebounce->calculate(is_sat(Sen->Tbatt_filt, Mon->voc_stat(), Mon->soc()), reset);

    // Memory store
    Mon->count_coulombs(Sen->T, reset, Sen->Tbatt_filt, Sen->Ishunt, Sen->saturated, rp.t_last);
    Mon->update(&rp.delta_q, &rp.t_last, &rp.delta_q_inf);

    // Charge time for display
    Mon->calculate_charge_time(Mon->q(), Mon->q_capacity(), Sen->Ishunt,Mon->soc());
    //////////////////////////////////////////////////////////////

    //
    // Useful for Arduino plotting
    if ( rp.debug==-1 )
      Serial.printf("%7.3f,     %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
        Sim->SOC()-90,
        Sen->Ishunt_amp_cal, Sen->Ishunt_noamp_cal,
        Sen->Vbatt*10-110, Sim->voc()*10-110, Sim->vdyn()*10, Sim->vb()*10-110, Mon->vdyn()*10-110);
    if ( rp.debug==12 )
      Serial.printf("ib,ib_mod,   vb,vb_mod,  voc_dyn,voc_stat_mod,voc_mod,   K, y,    SOC_mod, SOC_ekf, SOC,   %7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
        Mon->ib(), Sim->ib(),
        Mon->vb(), Sim->vb(),
        Mon->voc_dyn(), Sim->voc_stat(), Sim->voc(),
        Mon->K_ekf(), Mon->y_ekf(),
        Sim->soc(), Mon->soc_ekf(), Mon->soc());
    if ( rp.debug==-12 )
      Serial.printf("ib,ib_mod,   vb*10-110,vb_mod*10-110,  voc_dyn*10-110,voc_stat_mod*10-110,voc_mod*10-110,   K, y,    SOC_mod-90, SOC_ekf-90, SOC-90,\n%7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
        Mon->ib(), Sim->ib(),
        Mon->vb()*10-110, Sim->vb()*10-110,
        Mon->voc_dyn()*10-110, Sim->voc_stat()*10-110, Sim->voc()*10-110,
        Mon->K_ekf(), Mon->y_ekf(),
        Sim->soc()*100-90, Mon->soc_ekf()*100-90, Sim->soc()*100-90);
    if ( rp.debug==-3 )
      Serial.printf("fast,et,reset,Wshunt,q_f,q,soc,T, %12.3f,%7.3f, %d, %7.3f,    %7.3f,     %7.3f,\n",
      control_time, double(elapsed)/1000., reset, Sen->Wshunt, Sim->soc(), Sen->T_filt);

  }  // end read (high speed frame)

  // Run filters on other signals
  filt = FilterSync->update(millis(), reset);               //  now || reset
  if ( filt )
  {
    Sen->T_filt =  FilterSync->updateTime();
    if ( rp.debug>102 ) Serial.printf("Filter update=%7.3f and performing load() at %ld...  ", Sen->T_filt, millis());
    if ( rp.modeling && reset && Sim->q()<=0. ) Sen->Ishunt = 0.;

    // Filter
    // filter(reset, Sen, VbattSenseFilt, IshuntSenseFilt);

    // rp.debug print statements
    // Useful for vector testing and serial data capture
    if ( rp.debug==-35 ) Serial.printf("soc_mod,soc_ekf,voc_ekf= %7.3f, %7.3f, %7.3f\n",
        Sim->soc(), Mon->x_ekf(), Mon->z_ekf());

    //if ( bare ) delay(41);  // Usual I2C time
    if ( rp.debug>102 ) Serial.printf("completed load at %ld\n", millis());

  }

  // Control
  control = ControlSync->update(millis(), reset);               //  now || reset
  if ( control )
  {
    pwm_write(rp.duty, myPins);
    if ( rp.debug>102 ) Serial.printf("completed control at %ld.  rp.dutyy=%ld\n", millis(), rp.duty);
  }

  // Display driver
  display_to_user = DisplayUserSync->update(millis(), reset);               //  now || reset
  if ( display_to_user )
  {
    myDisplay(display, Sen);
  }

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  publishP = PublishParticle->update(millis(), false);        //  now || false
  publishB = PublishBlynk->update(millis(), false);           //  now || false
  publishS = PublishSerial->update(millis(), reset_publish);          //  now || reset
  if ( publishP || publishS)
  {
    char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
    control_time = decimalTime(&current_time, tempStr, now, millis_flip);
    hm_string = String(tempStr);
    assign_publist(&cp.pubList, PublishParticle->now(), unit, hm_string, control_time, Sen, num_timeouts, Sim, Mon);
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      // static boolean led_on = false;
      // led_on = !led_on;
      // if ( led_on ) digitalWrite(myPins->status_led, HIGH);
      // else  digitalWrite(myPins->status_led, LOW);
      publish_particle(PublishParticle->now(), myWifi, cp.enable_wifi);
    }
    if ( reset_publish ) digitalWrite(myPins->status_led, HIGH);
    else  digitalWrite(myPins->status_led, LOW);

    // Mon for rp.debug
    if ( publishS )
    {
      if ( rp.debug==2 )
      {
        if ( reset_publish || (last_publishS_debug != rp.debug) ) print_serial_header();
        serial_print(PublishSerial->now(), Sen->T);
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

  // Summary management
  boolean initial_summarize = summarizing_waiting && ( elapsed >= SUMMARIZE_WAIT );
  if ( elapsed >= SUMMARIZE_WAIT ) summarizing_waiting = false;
  summarizing = Summarize->update(millis(), initial_summarize, !rp.modeling) || (rp.debug==-11 && publishB);               //  now || initial_summarize && !rp.modeling
  if ( !summarizing_waiting && (summarizing || cp.write_summary) )
  {
    if ( ++rp.isum>NSUM-1 ) rp.isum = 0;
    mySum[rp.isum].assign(time_now, Sen->Tbatt_filt, Sen->Vbatt, Sen->Ishunt,
                          Mon->soc_ekf(), Mon->soc(), Mon->voc_dyn(), Mon->voc(), Mon->delta_q_inf());
    if ( rp.debug==0 ) Serial.printf("Summarized.....................\n");
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = false;
  if ( read_temp ) reset_temp = false;
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
