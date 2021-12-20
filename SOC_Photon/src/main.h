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
extern CommandPars cp;            // Various parameters to be common at system level (reset on PLC reset)
extern RetainedPars rp;           // Various parameters to be static at system level (don't reset on PLC reset)

// Global locals
const int nsum = 100;           // Number of summary strings, 17 Bytes per isum
retained RetainedPars rp;       // Various control parameters static at system level
retained CommandPars cp = CommandPars(); // Various control parameters commanding at system level
retained int isum = -1;         // Summary location.   Begins at -1 because first action is to increment isum
retained Sum_st mySum[nsum];    // Summaries
CoulombCounter Cc;              // Calculate and remember state of charge (retained doesn't work)
CoulombCounter CcModel;         // Calculate and remember state of charge for model (retained doesn't work)
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

// char buffer[256];               // Serial print buffer
int num_timeouts = 0;            // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";      // time, hh:mm
double control_time = 0.0;      // Decimal time, seconds since 1/1/2021
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_ADS1015 *ads_amp;      // Use this for the 12-bit version; 1115 for 16-bit; amplified; different address
Adafruit_ADS1015 *ads_noamp;    // Use this for the 12-bit version; 1115 for 16-bit; non-amplified
Adafruit_SSD1306 *display;
bool bare_ads_noamp = false;          // If ADS to be ignored
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
    print_all(mySum, isum, nsum);

  // Header for rp.debug print
  if ( rp.debug>101 ) print_serial_header();
  if ( rp.debug>103 ) { Serial.print(F("End setup rp.debug message=")); Serial.println(F(", ")); };

  // Reload memorized states from retained
  CcModel.load(rp.delta_q_model, rp.t_sat_model, rp.q_sat_model);
  Cc.load(rp.delta_q, rp.t_sat, rp.q_sat);

} // setup


// Loop
void loop()
{
  // Sensor noise filters.   There are AAF in hardware for Vbatt and VshuntAmp and VshuntNoAmp
  static General2_Pole* VbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -50., 50.);
  static General2_Pole* IshuntSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -500., 500.);
  static General2_Pole* TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);

  // 1-wire temp sensor battery temp
  static DS18* SensorTbatt = new DS18(myPins->pin_1_wire);

  // Sensor conversions
  static Sensors *Sen = new Sensors(NOMVBATT, NOMVBATT, NOMTBATT, NOMTBATT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        0, 0, 0, bare_ads_noamp, bare_ads_amp); // Manage sensor data
  static SlidingDeadband *SdVbatt = new SlidingDeadband(HDB_VBATT);
  static SlidingDeadband *SdTbatt = new SlidingDeadband(HDB_TBATT);

  // Battery  models
  // Free, driven by soc
  static Battery *MyBatt = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat, dvoc_dt);
  // Model, driven by soc, used to get Vbatt.   Use Talk 'x' to toggle model on/off. 
  static BatteryModel *MyBattModel = new BatteryModel(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat, dvoc_dt);

  // Battery saturation
  static Debounce *SatDebounce = new Debounce(true, SAT_PERSISTENCE);       // Updates persistence

  unsigned long current_time;                // Time result
  static unsigned long now = millis();      // Keep track of time
  static unsigned long start = millis();    // Keep track of time
  unsigned long elapsed = 0;                // Keep track of time
  static int reset = 1;                     // Dynamic reset
  static int reset_temp = 1;                // Dynamic reset
  double Tbatt_filt_C = 0;

  // Synchronization
  bool publishP;                            // Particle publish, T/F
  static Sync *PublishParticle = new Sync(PUBLISH_PARTICLE_DELAY);
  bool publishB;                            // Particle publish, T/F
  static Sync *PublishBlynk = new Sync(PUBLISH_BLYNK_DELAY);
  bool read;                                // Read, T/F
  static Sync *ReadSensors = new Sync(READ_DELAY);
  bool filt;                                // Filter, T/F
  static Sync *FilterSync = new Sync(FILTER_DELAY);
  bool read_temp;                           // Read temp, T/F
  static Sync *ReadTemp = new Sync(READ_TEMP_DELAY);
  bool publishS;                            // Serial print, T/F
  static Sync *PublishSerial = new Sync(PUBLISH_SERIAL_DELAY);
  bool display_to_user;                     // User display, T/F
  static Sync *DisplayUserSync = new Sync(DISPLAY_USER_DELAY);
  bool summarizing;                         // Summarize, T/F
  static Sync *Summarize = new Sync(SUMMARIZE_DELAY);
  bool control;                         // Summarize, T/F
  static Sync *ControlSync = new Sync(CONTROL_DELAY);

  static bool reset_free = false;
  static bool reset_free_ekf = true;
  
  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Start Blynk, only if connected since it is blocking
  if ( Particle.connected() && !myWifi->blynk_started )
  {
    if ( rp.debug>102 ) Serial.printf("Starting Blynk at %ld...  ", millis());
    Blynk.begin(blynkAuth.c_str());   // blocking if no connection
    myWifi->blynk_started = true;
    if ( rp.debug>102 ) Serial.printf("completed at %ld\n", millis());
  }
  if ( myWifi->blynk_started && myWifi->connected && !cp.vectoring )
  {
    Blynk.run(); blynk_timer_1.run(); blynk_timer_2.run(); blynk_timer_3.run(); blynk_timer_4.run(); 
  }

  // Keep time
  now = millis();
  sync_time(now, &last_sync, &millis_flip);      // Refresh time synchronization

  // Input temperature only
  read_temp = ReadTemp->update(millis(), reset);              //  now || reset
  if ( read_temp )
  {
    Sen->T_temp =  ReadTemp->updateTime();
    if ( rp.debug>102 ) Serial.printf("Read temp update=%7.3f and performing load_temp() at %ld...  ", Sen->T_temp, millis());

    // Load and filter temperature only
    load_temp(Sen, SensorTbatt, SdTbatt);
    filter_temp(reset_temp, Sen, TbattSenseFilt);
  }

  // Input all other sensors
  read = ReadSensors->update(millis(), reset);               //  now || reset
  elapsed = ReadSensors->now() - start;
  if ( read )
  {
    Sen->T =  ReadSensors->updateTime();
    if ( rp.debug>102 || rp.debug==-13 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  ", Sen->T, millis());

    // Load and filter
    load(reset_free, Sen, myPins, ads_amp, ads_noamp, ReadSensors->now(), SdVbatt);
    Tbatt_filt_C = (Sen->Tbatt_filt-32.)*5./9.;

    // Arduino plots
    if ( rp.debug==-7 ) Serial.printf("%7.3f,%7.3f,%7.3f,   %7.3f, %7.3f,\n",
        Cc.soc, Sen->Ishunt_amp_cal, Sen->Ishunt_noamp_cal,
        Sen->Vbatt, MyBattModel->voc());

    // Initialize SOC Free Integrator - Coulomb Counting method
    // Runs unfiltered and fast to capture most data
    static boolean vectoring_past = cp.vectoring;
    static double soc_saved = Cc.soc;
    if ( vectoring_past != cp.vectoring )
    {
      reset_free = true;
      start = ReadSensors->now();
      elapsed = 0UL;
      if ( cp.vectoring ) soc_saved = Cc.soc;
      else Cc.soc = soc_saved;
    }
    vectoring_past = cp.vectoring;
    if ( reset_free )
    {
      if ( cp.vectoring ) Cc.soc = Cc.soc;
      else Cc.soc = soc_saved;  // Only way to reach this line is resetting from vector test
      MyBatt->init_soc_ekf(Cc.soc);
      if ( elapsed>INIT_WAIT ) reset_free = false;
    }
    if ( reset_free_ekf )
    {
      MyBatt->init_soc_ekf(Cc.soc);
      if ( elapsed>INIT_WAIT_EKF ) reset_free_ekf = false;
    }

    // Model used for built-in testing (rp.modeling = true and jumper wire).   Needed here in this location
    // to have a value for Tbatt_filt_C
    if ( CcModel.nom_q_cap == 0 )
      CcModel.prime(nom_q_cap, RATED_TEMP, rp.q_sat, Tbatt_filt_C, rp.s_cap);
    // Model call
    Sen->Vbatt_model = MyBattModel->calculate(Tbatt_filt_C, CcModel.soc, Sen->Ishunt, min(Sen->T, 0.5));
    boolean sat_model = is_sat(Tbatt_filt_C, MyBattModel->voc());
    CcModel.soc = CcModel.count_coulombs(Sen->T, Tbatt_filt_C, Sen->Ishunt, sat_model);
    CcModel.update(&rp.delta_q_model, &rp.t_sat_model, &rp.q_sat_model);
    Sen->Voc = MyBattModel->voc();
    rp.duty = MyBattModel->calc_inj_duty(now, rp.type, rp.amp, rp.freq);
    // Over-ride Ishunt, Vbatt and Tbatt with model
    if ( rp.modeling )
    {
      Sen->Ishunt = MyBattModel->ib();
      Sen->Vbatt = MyBattModel->vb();
      Tbatt_filt_C = MyBattModel->temp_c();
    }
    
    // Main Battery
    //  Inputs:
    //    Sen->Ishunt     A
    //    Sen->Vbatt      V
    //    Tbatt_filt_C    deg C
    //    Cc    Coulomb charge counter memory structure
    // Initialize Cc structure if needed.   Needed here in this location to have a value for Tbatt_filt_C
    if ( Cc.nom_q_cap == 0 )
      Cc.prime(nom_q_cap, RATED_TEMP, rp.q_sat, Tbatt_filt_C, 1.);
    // EKF - calculates temp_c_, voc_, voc_dyn_
    cp.soc_ekf = MyBatt->calculate_ekf(Tbatt_filt_C, Sen->Vbatt, Sen->Ishunt,  min(Sen->T, 0.5), Sen->saturated);  // TODO:  hardcoded time of 0.5 into constants
    cp.SOC_ekf = cp.soc_ekf*100.*Cc.q_capacity/Cc.q_cap;
    Sen->saturated = SatDebounce->calculate(is_sat(Tbatt_filt_C, MyBatt->voc()), reset);
    Cc.count_coulombs(Sen->T, Tbatt_filt_C, Sen->Ishunt, Sen->saturated);
    Cc.update(&rp.delta_q, &rp.t_sat, &rp.q_sat);
    MyBatt->calculate_charge_time(Tbatt_filt_C, Sen->Ishunt, rp.delta_q, rp.t_sat, rp.q_sat, Cc.soc);

    
    // Useful for Arduino plotting
    if ( rp.debug==-1 )
      Serial.printf("%7.3f,     %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
        CcModel.SOC-90,
        Sen->Ishunt_amp_cal, Sen->Ishunt_noamp_cal,
        Sen->Vbatt_filt*10-110, MyBattModel->voc()*10-110, MyBattModel->vdyn()*10, MyBattModel->vb()*10-110, MyBatt->vdyn()*10-110);
    if ( rp.debug==12 )
      Serial.printf("ib,ib_mod,   vb,vb_mod,  voc_dyn,voc_mod,   K, y,    SOC_mod, SOC_ekf, SOC,   %7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
        MyBatt->ib(), MyBattModel->ib(),
        MyBatt->vb(), MyBattModel->vb(),
        MyBatt->voc_dyn(), MyBattModel->voc(),
        MyBatt->K_ekf(), MyBatt->y_ekf(),
        CcModel.soc, MyBatt->soc_ekf(), Cc.soc);
    if ( rp.debug==-12 )
      Serial.printf("ib,ib_mod,   vb*10-110,vb_mod*10-110,  voc_dyn*10-110,voc_mod*10-110,   K, y,    SOC_mod-90, SOC_ekf-90, SOC-90,\n%7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
        MyBatt->ib(), MyBattModel->ib(),
        MyBatt->vb()*10-110, MyBattModel->vb()*10-110,
        MyBatt->voc_dyn()*10-110, MyBattModel->voc()*10-110,
        MyBatt->K_ekf(), MyBatt->y_ekf(),
        CcModel.soc*100-90, MyBatt->soc_ekf()*100-90, Cc.soc*100-90);
    if ( rp.debug==-3 )
      Serial.printf("fast,et,reset_free,Wshunt,q_f,q,soc,T, %12.3f,%7.3f, %d, %7.3f,    %7.3f,     %7.3f,\n",
      control_time, double(elapsed)/1000., reset_free, Sen->Wshunt, Cc.soc, Sen->T_filt);

  }

  // Run filters on other signals
  filt = FilterSync->update(millis(), reset);               //  now || reset
  if ( filt )
  {
    Sen->T_filt =  FilterSync->updateTime();
    if ( rp.debug>102 ) Serial.printf("Filter update=%7.3f and performing load() at %ld...  ", Sen->T_filt, millis());

    // Filter
    filter(reset, Sen, VbattSenseFilt, IshuntSenseFilt);

    // rp.debug print statements
    // Useful for vector testing and serial data capture
    if ( rp.debug==-35 ) Serial.printf("soc_mod,soc_ekf,voc_ekf= %7.3f, %7.3f, %7.3f\n",
        CcModel.soc, MyBatt->x_ekf(), MyBatt->z_ekf());

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
  publishS = PublishSerial->update(millis(), reset);          //  now || reset
  if ( publishP || publishS)
  {
    char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
    control_time = decimalTime(&current_time, tempStr, now, millis_flip);
    hm_string = String(tempStr);
    assign_publist(&cp.pubList, PublishParticle->now(), unit, hm_string, control_time, Sen, num_timeouts, MyBattModel, MyBatt);
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      // static bool led_on = false;
      // led_on = !led_on;
      // if ( led_on ) digitalWrite(myPins->status_led, HIGH);
      // else  digitalWrite(myPins->status_led, LOW);
      publish_particle(PublishParticle->now(), myWifi, cp.enable_wifi);
    }
    if ( reset_free || reset ) digitalWrite(myPins->status_led, HIGH);
    else  digitalWrite(myPins->status_led, LOW);

    // Monitor for rp.debug
    if ( rp.debug==2 && publishS )
    {
      serial_print(PublishSerial->now(), Sen->T);
    }

  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  int debug_saved = rp.debug;
  talk(&cp.stepping, &cp.step_val, &cp.vectoring, &cp.vec_num, MyBatt, MyBattModel);

  // Summary management
  if ( rp.debug==-4 )
  {
    rp.debug = debug_saved;
    print_all(mySum, isum, nsum);
  }
  summarizing = Summarize->update(millis(), reset, !cp.vectoring) || (rp.debug==-11 && publishB);               //  now || reset && !cp.vectoring
  if ( summarizing )
  {
    if ( ++isum>nsum-1 ) isum = 0;
    mySum[isum].assign(current_time, Sen->Tbatt_filt, Sen->Vbatt_filt, Sen->Ishunt_filt,
      Cc.soc, Cc.soc, MyBattModel->dv_dsoc());
    if ( rp.debug==-11 )
    {
      Serial.printf("Summm***********************\n");
      print_all(mySum, isum, nsum);
      Serial.printf("*********************** %d \n", isum);
    }
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = 0;
  if ( read_temp ) reset_temp = 0;

} // loop
