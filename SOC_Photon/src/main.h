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
  * 18-May-2022   Bunch of cleanup and reorganization
  * 20-Jul-2022   Add low-emission bluetooth (BLE).  Initialize to EKF when unsaturated.
  *               Correct time skews to align Vb and Ib.
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

#undef USE_BLYNK              // Change this to #define to use BLYNK

#include "constants.h"

// Dependent includes.   Easier to rp.debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"

#ifdef USE_BLYNK
  #define BLYNK_AUTH_TOKEN  "DU9igmWDh6RuwYh6QAI_fWsi-KPkb7Aa"
  #define BLYNK_USE_DIRECT_CONNECT
  #define BLYNK_PRINT Serial
  #include "Blynk/BlynkSimpleSerialBLE.h"
  char auth[] = BLYNK_AUTH_TOKEN;
#endif

#include "mySummary.h"
#include "myCloud.h"
#include "Tweak.h"
#include "debug.h"

#ifdef USE_BLYNK
  extern BlynkStream Blynk;       // Blynk object
  extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4; // Time Blynk events
  BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;        // Time Blynk events
#endif
extern CommandPars cp;            // Various parameters to be common at system level
extern RetainedPars rp;           // Various parameters to be static at system level
extern Sum_st mySum[NSUM];        // Summaries for saving charge history
extern PublishPars pp;            // For publishing

// Global locals
retained RetainedPars rp;             // Various control parameters static at system level
retained Sum_st mySum[NSUM];          // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level.   Forces reinit
PublishPars pp = PublishPars();       // Common for publishing
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm
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
  Serial.println("Hi!");

  // Bluetooth Serial1.  Use BT-AT project in this GitHub repository to change.  Directions
  // for HC-06 inside main.h of ../../BT-AT/src.   AT+BAUD8; to set 115200.
  Serial1.begin(115200);
  #ifdef USE_BLYNK
    if ( cp.blynking )
    {
      Serial.printf("Starting Blynk...");
      Blynk.begin(Serial1, auth);     // Blocking
      Serial.printf("done");
    }
  #endif
  // Peripherals
  myPins = new Pins(D6, D7, A1, D2);

  // Status
  pinMode(myPins->status_led, OUTPUT);
  digitalWrite(myPins->status_led, LOW);

  // I2C
  Wire.setSpeed(CLOCK_SPEED_100KHZ);
  Wire.begin();

  // AD
  // Shunts initialized in Sensors as static loop() instantiation

  // Display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  Serial.println("Init DISPLAY");
  if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) // Seems to return true even if depowered
  {
    Serial.println(F("SSD1306 DISP alloc FAIL"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("DISP allocated");
  display->display();   // Adafruit splash
  delay(2000); // Pause for 2 seconds
  display->clearDisplay();

  // Cloud
  Time.zone(GMT);
  unsigned long now = millis();
  myWifi = new Wifi(now-CHECK_INTERVAL+CONNECT_WAIT, now, false, false, Particle.connected());  // lastAttempt, last_disconnect, connected, blynk_started, Particle.connected
  Serial.printf("Init CLOUD...");
  Particle.disconnect();
  myWifi->last_disconnect = now;
  WiFi.off();
  myWifi->connected = false;
  if ( rp.debug >= 100 ) Serial.printf("wifi dscn...");
  #ifdef USE_BLYNK
    Serial.printf("Set up blynk...");
    blynk_timer_1.setInterval(PUBLISH_BLYNK_DELAY, publish1);
    blynk_timer_2.setTimeout(1*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_2.setInterval(PUBLISH_BLYNK_DELAY, publish2);});
    blynk_timer_3.setTimeout(2*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_3.setInterval(PUBLISH_BLYNK_DELAY, publish3);});
    blynk_timer_4.setTimeout(3*PUBLISH_BLYNK_DELAY/4, [](){blynk_timer_4.setInterval(PUBLISH_BLYNK_DELAY, publish4);});
  #endif
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
    Serial.printf("\n****MSG(setup): Corrupt SRAM- force nom *** %s\n", cp.buffer);
  }

  #ifdef PHOTON
    if ( rp.debug>101 ) { sprintf(cp.buffer, "Photon\n"); Serial.print(cp.buffer); }
  #else
    if ( rp.debug>101 ) { sprintf(cp.buffer, "Mega2560\n"); Serial.print(cp.buffer); }
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
  if ( rp.debug==4 || rp.debug==24 )
    print_all_summary(mySum, rp.isum, NSUM);

  // Header for rp.debug print
  if ( rp.debug>101 ) print_serial_header();

/*
  // prototype of PRBS-7 pseudo random binar sequence
  // uint8_t start = 0x02;
  uint8_t start = 0x03;  // seed
  uint8_t noise = start;
  int i;
  for (i=1;; i++)
  {
    int newbit = (((noise>>6) ^ (noise>>5)) & 1);
    noise = ((noise<<1) | newbit) & 0x7f;
    Serial.printf("%7.3f,%7.3f,\n", float(i)*0.1, float(noise));
    if ( noise==start )
    {
      Serial.printf("\nrep per is %d\n", i);
      break;
    }
  }
*/

  Serial.printf("End setup()\n");
} // setup


// Loop
void loop()
{
  // Synchronization
  boolean publishP;                           // Particle publish, T/F
  static Sync *PublishParticle = new Sync(PUBLISH_PARTICLE_DELAY);

  boolean publishB;                           // Particle publish, T/F
  static Sync *PublishBlynk = new Sync(PUBLISH_BLYNK_DELAY);

  boolean read;                               // Read, T/F
  static Sync *ReadSensors = new Sync(READ_DELAY);

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

  static uint8_t last_read_debug = 0;         // Remember first time with new debug to print headers
  static uint8_t last_publishS_debug = 0;     // Remember first time with new debug to print headers

  unsigned long current_time;                 // Time result
  static unsigned long now = millis();        // Keep track of time
  time32_t time_now;                          // Keep track of time
  static unsigned long start = millis();      // Keep track of time
  unsigned long elapsed = 0;                  // Keep track of time
  static boolean reset = true;                // Dynamic reset
  static boolean reset_temp = true;           // Dynamic reset
  static boolean reset_publish = true;        // Dynamic reset
 
  // Sensor conversions
  static Sensors *Sen = new Sensors(0, 0, myPins->pin_1_wire, PublishSerial, ReadSensors); // Manage sensor data
  static float tbatt_bias_last;  // Memory for rate limiter in temp_filter call, deg C

   // Mon, used to count Coulombs and run EKF
  static BatteryMonitor *Mon = new BatteryMonitor(&rp.delta_q, &rp.t_last, &rp.nP, &rp.nS, &rp.mon_mod);

  // Battery saturation debounce
  static TFDelay *Is_sat_delay = new TFDelay();   // Time persistence

  
  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Run Blynk
  #ifdef USE_BLYNK
    static boolean blynk_began = cp.blynking;
    if ( cp.blynking )
    {
      if ( !blynk_began )
      {
        Serial.printf("Starting Blynk...\n");
        Blynk.begin(Serial1, auth);     // Blocking
        Serial.printf("Starting Blynk done\n");
        blynk_began = true;
      }
      Blynk.run();
      blynk_timer_1.run();
      blynk_timer_2.run();
      blynk_timer_3.run();
      blynk_timer_4.run(); 
    }
  #endif

  // Synchronize
  now = millis();
  time_now = Time.now();
  sync_time(now, &last_sync, &millis_flip);      // Refresh time synchronization
  char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
  Sen->control_time = decimalTime(&current_time, tempStr, now, millis_flip);
  hm_string = String(tempStr);
  read_temp = ReadTemp->update(millis(), reset);              //  now || reset
  read = ReadSensors->update(millis(), reset);                //  now || reset
  elapsed = ReadSensors->now() - start;
  control = ControlSync->update(millis(), reset);             //  now || reset
  display_to_user = DisplayUserSync->update(millis(), reset); //  now || reset
  publishP = PublishParticle->update(millis(), false);        //  now || false
  publishB = PublishBlynk->update(millis(), false);           //  now || false
  publishS = PublishSerial->update(millis(), reset_publish);  //  now || reset_publish
  boolean boot_summ = boot_wait && ( elapsed >= SUMMARIZE_WAIT );
  if ( elapsed >= SUMMARIZE_WAIT ) boot_wait = false;
  summarizing = Summarize->update(millis(), boot_summ, !rp.modeling); // now || boot_summ && !rp.modeling
  summarizing = summarizing || (rp.debug==-11 && publishB);

  // Load temperature
  // Outputs:   Sen->Tbatt,  Sen->Tbatt_filt
  if ( read_temp )
  {
    Sen->T_temp =  ReadTemp->updateTime();
    // TODO:   rp.tbatt_bias and tbatt_bias_last into Sen class
    Sen->temp_load_and_filter(Sen, reset_temp, T_RLIM, rp.tbatt_bias, &tbatt_bias_last);
  }

  // Input all other sensors and do high rate calculations
  if ( read )
  {
    Sen->T =  ReadSensors->updateTime();
    Sen->reset = reset;
    if ( rp.debug>102 || rp.debug==-13 ) Serial.printf("Read dt=%7.3f; load at %ld...  \n", Sen->T, millis());

    // Read sensors, model signals, select between them, synthesize injection signals on current
    // Inputs:  rp.config, rp.sim_mod
    // Outputs: Sen->Ibatt, Sen->Vbatt, Sen->Tbatt_filt, rp.inj_bias
    sense_synth_select(reset, reset_temp, ReadSensors->now(), elapsed, myPins, Mon, Sen);

    // Calculate Ah remaining
    // Inputs:  rp.mon_mod, Sen->Ibatt, Sen->Vbatt, Sen->Tbatt_filt
    // States:  Mon.soc
    // Outputs: tcharge_wt, tcharge_ekf
    monitor(reset, reset_temp, now, Is_sat_delay, Mon, Sen);

    // Adjust current sensors
    tweak_on_new_desat(Sen, now);

    // Re-init Coulomb Counter to EKF if it is different than EKF or if never saturated
    Mon->regauge(Sen->Tbatt_filt);

    // Empty battery
    if ( rp.modeling && reset && Sen->Sim->q()<=0. ) Sen->Ibatt = 0.;

    // Debug for read
    // TODO:  debug_main() into debug.cpp.  Move create_print_string, tweak_print, print_serial_header and print_serial_sim_header to debug.cpp 
    if ( rp.debug==-1 ) debug_m1(Mon, Sen); // General purpose Arduino
    if ( rp.debug==12 ) debug_12(Mon, Sen);  // EKF
    if ( rp.debug==-12 ) debug_m12(Mon, Sen);  // EKF Arduino
    if ( rp.debug==-3 ) debug_m3(Mon, Sen, elapsed, reset);  // Power Arduino
    if ( rp.debug==-35 ) debug_m35(Mon, Sen); // EKF Arduino
    if ( rp.tweak_test() )
    {
      if ( rp.debug==4 || rp.debug==24 )
      {
        if ( reset || (last_read_debug != rp.debug) )
        {
          print_serial_header();
          if ( rp.debug==24 ) print_serial_sim_header();
        }
        tweak_print(Sen, Mon);
      }

      if ( rp.debug==-4 )
      {
        debug_m4(Mon, Sen);
      }
      last_read_debug = rp.debug;
    }


  }  // end read (high speed frame)

  // Control
  if ( control )
  {
    // continue;
  }

  // OLED and Bluetooth display drivers
  if ( display_to_user )
  {
    oled_display(display, Sen);
  }

  // Publish to Particle cloud if desired (different than Blynk)
  // Visit https://console.particle.io/events.   Click on "view events on a terminal"
  // to get a curl command to run
  // TODO:  publish_main(publishP, reset_publish, publishS) in myCloud
  if ( publishP || publishS)
  {
    assign_publist(&pp.pubList, PublishParticle->now(), unit, hm_string, Sen, num_timeouts, Mon);
 
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
    if ( publishS && !rp.tweak_test() )
    {
      if ( rp.debug==4 || rp.debug==24 )
      {
        if ( reset_publish || (last_publishS_debug != rp.debug) )
        {
          print_serial_header();
          if ( rp.debug==24 ) print_serial_sim_header();
        }
        serial_print(PublishSerial->now(), Sen->T);
      }

      if ( rp.debug==-4 )
      {
        debug_m4(Mon, Sen);
      }
      last_publishS_debug = rp.debug;
    }

  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  talk(Mon, Sen);

  // Summary management.   Every boot after a wait an initial summary is saved in rotating buffer
  // Then every half-hour unless modeling.   Can also request manually via cp.write_summary (Talk)
  if ( !boot_wait && (summarizing || cp.write_summary) )
  {
    if ( ++rp.isum>NSUM-1 ) rp.isum = 0;
    mySum[rp.isum].assign(time_now, Sen->Tbatt_filt, Sen->Vbatt, Sen->Ibatt,
                          Mon->soc_ekf(), Mon->soc(), Mon->Voc(), Mon->Voc(),
                          Sen->ShuntAmp->tweak_sclr(), Sen->ShuntNoAmp->tweak_sclr());
    if ( rp.debug==0 ) Serial.printf("Summ...\n");
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
    Serial.printf("soft reset...\n");
  }
  cp.soft_reset = false;
  cp.write_summary = false;

} // loop


// TODO:  move to myCloud.cpp
#ifdef USE_BLYNK
  // Publish1 Blynk
  void publish1(void)
  {
    if (rp.debug==25) Serial.printf("Blynk write1\n");
    Blynk.virtualWrite(V2,  pp.pubList.Vbatt);
    Blynk.virtualWrite(V3,  pp.pubList.Voc);
    Blynk.virtualWrite(V4,  pp.pubList.Vbatt);
  }


  // Publish2 Blynk
  void publish2(void)
  {
    if (rp.debug==25) Serial.printf("Blynk write2\n");
    Blynk.virtualWrite(V6,  pp.pubList.soc);
    Blynk.virtualWrite(V8,  pp.pubList.T);
    Blynk.virtualWrite(V10, pp.pubList.Tbatt);
  }


  // Publish3 Blynk
  void publish3(void)
  {
    if (rp.debug==25) Serial.printf("Blynk write3\n");
    Blynk.virtualWrite(V15, pp.pubList.hm_string);
    Blynk.virtualWrite(V16, pp.pubList.tcharge);
  }


  // Publish4 Blynk
  void publish4(void)
  {
    if (rp.debug==25) Serial.printf("Blynk write4\n");
    Blynk.virtualWrite(V18, pp.pubList.Ibatt);
    Blynk.virtualWrite(V20, pp.pubList.Wbatt);
    Blynk.virtualWrite(V21, pp.pubList.soc_ekf);
  }
#endif
