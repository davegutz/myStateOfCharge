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
extern String input_string;        // a string to hold incoming data
extern boolean string_complete;    // whether the string is complete
extern boolean stepping;          // active step adder
extern double step_val;           // Step size
extern boolean vectoring;         // Active battery test vector
extern int8_t vec_num;            // Active vector number
extern unsigned long vec_start;   // Start of active vector
extern boolean enable_wifi;       // Enable wifi
extern RetainedPars rp;          // Various parameters to be static at system level

// Global locals
const int nsum = 154;           // Number of summary strings, 17 Bytes per isum
retained int isum = -1;         // Summary location.   Begins at -1 because first action is to increment isum
retained Sum_st mySum[nsum];    // Summaries
retained RetainedPars rp;       // Various control parameters static at system level
retained int8_t debug = 2;
String input_string = "";
boolean string_complete = false;
boolean stepping = false;
double step_val = -2;
boolean vectoring = false;
int8_t vec_num = 1;
unsigned long vec_start = 0UL;
boolean enable_wifi = false;
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

char buffer[256];               // Serial print buffer
int num_timeouts = 0;            // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";      // time, hh:mm
double control_time = 0.0;      // Decimal time, seconds since 1/1/2021
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_ADS1015 *ads;          // Use this for the 12-bit version; 1115 for 16-bit
Adafruit_ADS1015 *ads_amp;      // Use this for the 12-bit version; 1115 for 16-bit; amplified; different address
Adafruit_SSD1306 *display;
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
  Serial.println("Initializing SHUNT MONITORS");
  ads = new Adafruit_ADS1015;
  ads->setGain(GAIN_SIXTEEN, GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  if (!ads->begin()) {
    Serial.println("FAILED to initialize ADS SHUNT MONITOR.");
    bare_ads = true;
  }
  ads_amp = new Adafruit_ADS1015;
  ads_amp->setGain(GAIN_EIGHT, GAIN_TWO);    // Differential 8x gain
  // if (!ads->begin()) {
  //   Serial.println("FAILED to initialize ADS AMPLIFIED SHUNT MONITOR.");
  //   bare_ads_amp = true;
  // }
  if (!ads_amp->begin((0x49))) {
    Serial.println("FAILED to initialize ADS AMPLIFIED SHUNT MONITOR.");
    bare_ads_amp = true;
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

  // Determine millis() at turn of Time.now
  long time_begin = Time.now();
  while ( Time.now()==time_begin )
  {
    delay(1);
    millis_flip = millis()%1000;
  }

  // Summary
  System.enableFeature(FEATURE_RETAINED_MEMORY);
  print_all(mySum, isum, nsum);

  // Header for debug print
  if ( debug>1 ) print_serial_header();
  if ( debug>3 ) { Serial.print(F("End setup debug message=")); Serial.println(F(", ")); };

} // setup


// Loop
void loop()
{
  // Sensor noise filters.   Obs filters 0.5 --> 0.1 to eliminate digital instability.   Also rate limited the observer belts+suspenders
  static General2_Pole* VbattSenseFiltObs = new General2_Pole(double(READ_DELAY)/1000., F_O_W, F_O_Z, 0.4*double(NOM_SYS_VOLT), 2.0*double(NOM_SYS_VOLT));
  static General2_Pole* VshuntSenseFiltObs = new General2_Pole(double(READ_DELAY)/1000., F_O_W, F_O_Z, -0.500, 0.500);
  static General2_Pole* VshuntAmpSenseFiltObs = new General2_Pole(double(READ_DELAY)/1000., F_O_W, F_O_Z, -0.500, 0.500);
  static General2_Pole* VbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, 0.833*double(NOM_SYS_VOLT), 1.15*double(NOM_SYS_VOLT));
  static General2_Pole* TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -20.0, 150.);
  static General2_Pole* VshuntSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -0.500, 0.500);
  static General2_Pole* VshuntAmpSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W, F_Z, -0.500, 0.500);

  // 1-wire temp sensor battery temp
  static DS18* SensorTbatt = new DS18(myPins->pin_1_wire);

  // Sensor conversions
  static Sensors *Sen = new Sensors(NOMVBATT, NOMVBATT, NOMTBATT, NOMTBATT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        NOMVSHUNTI, NOMVSHUNT, NOMVSHUNT,
        0, 0, 0, bare_ads, bare_ads_amp); // Manage sensor data
  static SlidingDeadband *SdIshunt = new SlidingDeadband(HDB_ISHUNT);
  static SlidingDeadband *SdVbatt = new SlidingDeadband(HDB_VBATT);
  static SlidingDeadband *SdTbatt = new SlidingDeadband(HDB_TBATT);
  static SlidingDeadband *SdIshunt_amp = new SlidingDeadband(HDB_ISHUNT_AMP);

  // Battery  models
  // Solved, driven by socu_s
  static Battery *MyBattSolved = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat, dvoc_dt);
  // Free, driven by socu_free
  static Battery *MyBattFree = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat, dvoc_dt);
  // Model, driven by socu_model, used to get Vbatt.   Use Talk 'x' to toggle model on/off.   (n set socu_model different than)
  static Battery *MyBattModel = new Battery(t_bb, b_bb, a_bb, c_bb, m_bb, n_bb, d_bb, nz_bb, batt_num_cells,
    batt_r1, batt_r2, batt_r2c2, batt_vsat, dvoc_dt);

  // Battery saturation
  static Debounce *SaturatedObj = new Debounce(true, SAT_PERSISTENCE);       // Updates persistence

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

  static double socu_solved = 1.0;
  static bool reset_free = false;
  static bool reset_free_ekf = true;
  static boolean saturated = false;
  
  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Start Blynk, only if connected since it is blocking
  if ( Particle.connected() && !myWifi->blynk_started )
  {
    if ( debug>2 ) Serial.printf("Starting Blynk at %ld...  ", millis());
    Blynk.begin(blynkAuth.c_str());   // blocking if no connection
    myWifi->blynk_started = true;
    if ( debug>2 ) Serial.printf("completed at %ld\n", millis());
  }
  if ( myWifi->blynk_started && myWifi->connected && !vectoring )
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
    if ( debug>2 ) Serial.printf("Read temp update=%7.3f and performing load_temp() at %ld...  ", Sen->T_temp, millis());

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
    if ( debug>2 || debug==-13 ) Serial.printf("Read update=%7.3f and performing load() at %ld...  ", Sen->T, millis());

    // Load and filter
    load(reset_free, Sen, myPins, ads, ads_amp, ReadSensors->now(), SdIshunt, SdIshunt_amp, SdVbatt);
    Tbatt_filt_C = (Sen->Tbatt_filt-32.)*5./9.;

    // Arduino plots
    if ( debug==-7 ) Serial.printf("%7.3f,%7.3f,%7.3f,   %7.3f, %7.3f,\n", socu_solved, Sen->Ishunt_amp, Sen->Ishunt,
        Sen->Vbatt, MyBattSolved->voc());

    // Initialize SOC Free Integrator - Coulomb Counting method
    // Runs unfiltered and fast to capture most data
    static boolean vectoring_past = vectoring;
    static double socu_free_saved = rp.socu_free;
    if ( vectoring_past != vectoring )
    {
      reset_free = true;
      start = ReadSensors->now();
      elapsed = 0UL;
      if ( vectoring ) socu_free_saved = rp.socu_free;
      else rp.socu_free = socu_free_saved;
      rp.socu_model = rp.socu_free;
    }
    vectoring_past = vectoring;
    if ( reset_free )
    {
      if ( vectoring ) rp.socu_free = socu_solved;
      else rp.socu_free = socu_free_saved;  // Only way to reach this line is resetting from vector test
      rp.socu_model = rp.socu_free;
      MyBattFree->init_soc_ekf(rp.socu_free);
      if ( elapsed>INIT_WAIT ) reset_free = false;
    }
    if ( reset_free_ekf )
    {
      MyBattFree->init_soc_ekf(rp.socu_free);
      if ( elapsed>INIT_WAIT_EKF ) reset_free_ekf = false;
    }

    // Model driven by itself and highly filtered shunt current to keep Vbatt_model quiet
    if ( reset_free )
    {
      rp.socu_model = rp.socu_free;
    }

    Sen->Vbatt_model = MyBattModel->calculate_model(Tbatt_filt_C, rp.socu_model, Sen->Ishunt_filt_obs, min(Sen->T, 0.5));

    // EKF
    MyBattFree->calculate_ekf(Tbatt_filt_C, Sen->Vbatt, Sen->Ishunt,  min(Sen->T, 0.5), saturated);  // TODO:  hardcoded time of 0.5 into constants
    
    // Coulomb Count integrator
    rp.socu_free = max(min( rp.socu_free + Sen->Wshunt/NOM_SYS_VOLT*Sen->T/3600./NOM_BATT_CAP, 1.5), 0.);
    rp.socu_model = max(min( rp.socu_model + Sen->Wshunt/NOM_SYS_VOLT*Sen->T/3600./NOM_BATT_CAP, 1.5), 0.);
    // Force initialization/reinitialization whenever saturated.   Keeps estimates close to reality
    if ( saturated )
    {
      rp.socu_free = mxepu_bb;
      // rp.socu_model = rp.socu_free;   // Let saturation subside
    }
    // Useful for Arduino plotting
    if ( debug==-1 ) Serial.printf("%7.3f,%7.3f,   %7.3f, %7.3f,%7.3f,%7.3f,%7.3f,\n", socu_solved, Sen->Ishunt, Sen->Ishunt_amp,
        Sen->Vbatt_filt_obs, MyBattSolved->voc(), MyBattSolved->vdyn(), MyBattSolved->v());
    if ( debug==-3 )
      Serial.printf("fast,et,reset_free,Wshunt,soc_f,T, %12.3f,%7.3f, %d, %7.3f,%7.3f,%7.3f,\n",
      control_time, double(elapsed)/1000., reset_free, Sen->Wshunt, rp.socu_free, Sen->T_filt);

  }

  // Run filters on other signals
  filt = FilterSync->update(millis(), reset);               //  now || reset
  if ( filt )
  {
    Sen->T_filt =  FilterSync->updateTime();
    if ( debug>2 ) Serial.printf("Filter update=%7.3f and performing load() at %ld...  ", Sen->T_filt, millis());

    // Filter
    filter(reset, Sen, VbattSenseFiltObs, VshuntSenseFiltObs,  VshuntAmpSenseFiltObs,
       VbattSenseFilt, VshuntSenseFilt, VshuntAmpSenseFilt);
    saturated = SaturatedObj->calculate(MyBattSolved->sat(), reset);

    // Battery models
    MyBattFree->calculate(Tbatt_filt_C, rp.socu_free, Sen->Ishunt,  min(Sen->T_filt, F_MAX_T));

    // Solver
    double vbatt_f_o = Sen->Vbatt_filt_obs + double(stepping*step_val);
    int8_t count = 0;
    Sen->Vbatt_solved = MyBattSolved->calculate(Tbatt_filt_C, socu_solved, Sen->Ishunt_filt_obs, Sen->T_filt);
    double err = vbatt_f_o - Sen->Vbatt_solved;
    while( fabs(err)>SOLVE_MAX_ERR && count++<SOLVE_MAX_COUNTS )
    {
      socu_solved = max(min(socu_solved + max(min( err/MyBattSolved->dv_dsocu(), SOLVE_MAX_STEP), -SOLVE_MAX_STEP), mxepu_bb), mnepu_bb);
      Sen->Vbatt_solved = MyBattSolved->calculate(Tbatt_filt_C, socu_solved, Sen->Ishunt_filt_obs, Sen->T_filt);
      err = vbatt_f_o - Sen->Vbatt_solved;
      if ( debug==-5 ) Serial.printf("Tbatt_f,Ishunt_f_o,count,socu_s,vbatt_f_o,Vbatt_m_s,err,dv_dsocu, %7.3f,%7.3f,%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
          Sen->Tbatt_filt, Sen->Ishunt_filt_obs, count, socu_solved, vbatt_f_o, Sen->Vbatt_solved, err, MyBattSolved->dv_dsocu());
    }
    // boolean solver_valid = count<SOLVE_MAX_COUNTS; 
    if ( debug==-35 ) Serial.printf("soc_avail,socu_solved,Vbatt_solved, soc_ekf,voc_ekf= %7.3f, %7.3f, %7.3f, %7.3f, %7.3f\n",
        MyBattFree->soc_avail(), socu_solved, Sen->Vbatt_solved, MyBattFree->x_ekf(), MyBattFree->z_ekf());

    // Debug print statements
    // Useful for vector testing and serial data capture
    if ( debug==-2 )
      Serial.printf("slow,et,reset_f,vect,sat,Tbatt,Ishunt,Vb_f_o,soc_s,soc_f,Vb_s,voc,dvdsoc,T,count,tcharge,  %12.3f, %7.3f, %d,%d,%d, %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,\n",
      control_time, double(elapsed)/1000., reset_free, vectoring, saturated, Sen->Tbatt, Sen->Ishunt_filt_obs, Sen->Vbatt_filt_obs+double(stepping*step_val),
      socu_solved, rp.socu_free, Sen->Vbatt_solved, MyBattSolved->voc(), MyBattSolved->dv_dsocu(), Sen->T, count, MyBattFree->tcharge());

    //if ( bare ) delay(41);  // Usual I2C time
    if ( debug>2 ) Serial.printf("completed load at %ld\n", millis());

  }

  // Control
  control = ControlSync->update(millis(), reset);               //  now || reset
  if ( control )
  {
    pwm_write(rp.duty, myPins);
    if ( debug>2 ) Serial.printf("completed control at %ld.  rp.dutyy=%ld\n", millis(), rp.duty);
  }

  // Display driver
  display_to_user = DisplayUserSync->update(millis(), reset);               //  now || reset
  if ( display_to_user )
  {
    myDisplay(display);
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
    assign_PubList(&pubList, PublishParticle->now(), unit, hm_string, control_time, Sen, num_timeouts, MyBattSolved, MyBattFree);
 
    // Publish to Particle cloud - how data is reduced by SciLab in ../dataReduction
    if ( publishP )
    {
      // static bool led_on = false;
      // led_on = !led_on;
      // if ( led_on ) digitalWrite(myPins->status_led, HIGH);
      // else  digitalWrite(myPins->status_led, LOW);
      publish_particle(PublishParticle->now(), myWifi, enable_wifi);
    }
    if ( reset_free || reset ) digitalWrite(myPins->status_led, HIGH);
    else  digitalWrite(myPins->status_led, LOW);

    // Monitor for debug
    if ( debug>0 && publishS )
    {
      serial_print(PublishSerial->now(), Sen->T);
    }

  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  int debug_saved = debug;
  talk(&stepping, &step_val, &vectoring, &vec_num, MyBattSolved, MyBattFree, MyBattModel);

  // Summary management
  if ( debug==-4 )
  {
    debug = debug_saved;
    print_all(mySum, isum, nsum);
  }
  summarizing = Summarize->update(millis(), reset, !vectoring) || (debug==-11 && publishB);               //  now || reset && !vectoring
  if ( summarizing )
  {
    if ( ++isum>nsum-1 ) isum = 0;
    mySum[isum].assign(current_time, Sen->Tbatt_filt, Sen->Vbatt_filt_obs, Sen->Ishunt_filt_obs,
      socu_solved, rp.socu_free, MyBattSolved->dv_dsocu());
    if ( debug==-11 )
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
