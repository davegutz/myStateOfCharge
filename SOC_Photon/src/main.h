/*
 * Project SOC_Photon
  * Description:
  * Monitor battery State of Charge (SOC) using Coulomb Counting (CC).  An experimental
  * Extended Kalman Filter (EKF) method is developed alongside though not used to
  * improve the CC yet.
  * By:  Dave Gutz September 2021
  * 09-Aug-2021   Initial Git commit.   Unamplified ASD1013 12-bit shunt voltage sensor
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
  * 21-Sep-2022   Alpha release v20220917.   Branch GitHub repository.  Added signal redundancy checks and fault handling.
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
  // SYSTEM_MODE(MANUAL);      // Turn off wifi
  // SYSTEM_MODE(SEMI_AUTOMATIC);      // Turn off wifi
  #include <Arduino.h>      // Used instead of Print.h - breaks Serial
#else
  #undef PHOTON
  using namespace std;
  #undef max
  #undef min
#endif

#include "constants.h"

// Dependent includes.   Easier to rp.debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"

#include "mySummary.h"
#include "myCloud.h"
#include "debug.h"

// Globals
extern CommandPars cp;            // Various parameters to be common at system level
extern RetainedPars rp;           // Various parameters to be static at system level
extern Sum_st mySum[NSUM];        // Summaries for saving charge history
extern PublishPars pp;            // For publishing
extern Flt_st myFlt[NFLT];        // Summaries for saving fault history

retained RetainedPars rp;             // Various control parameters static at system level
retained Sum_st mySum[NSUM];          // Summaries
retained Flt_st myFlt[NFLT];          // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level
PublishPars pp = PublishPars();       // Common parameters for publishing.  Future-proof cloud monitoring
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_SSD1306 *display;      // Main OLED display

// Setup
void setup()
{
  // Serial
  // Serial.blockOnOverrun(false);  doesn't work
  Serial.begin(115200);
  Serial.flush();
  delay(1000);          // Ensures a clean display on Serial startup on CoolTerm
  Serial.println("Hi!");

  // Bluetooth Serial1.  Use BT-AT project in this GitHub repository to change.  Directions
  // for HC-06 inside main.h of ../../BT-AT/src.   AT+BAUD8; to set 115200.
  // Serial1.blockOnOverrun(false); doesn't work
  Serial1.begin(115200);
  Serial1.flush();

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
    Serial.println(F("DISP FAIL"));
    // for(;;); // Don't proceed, loop forever // Use Bluetooth if needed
  }
  else
    Serial.println("DISP allocated");
  // display->display();   // Adafruit splash
  // delay(2000); // Pause for 2 seconds
  display->clearDisplay();

  // Cloud, to synchronize clock.   Device needs to be configured for wifi (hold setup 3 sec run Particle app) and in range of wifi
  WiFi.disconnect();
  delay(2000);
  WiFi.off();
  delay(1000);
  Serial.printf("Done WiFi\n");
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

  // Determine millis() at turn of Time.now
  long time_begin = Time.now();
  while ( Time.now()==time_begin )
  {
    delay(1);
    millis_flip = millis()%1000;
  }

  // Summary
  System.enableFeature(FEATURE_RETAINED_MEMORY);
  if ( rp.debug==1 || rp.debug==2 || rp.debug==3 || rp.debug==4 )
    print_all_summary(mySum, rp.isum, NSUM);

  // Ask to renominalize
  if ( ASK_DURING_BOOT )
  {
    if ( rp.num_diffs() )
    {
      Serial.printf("#off-nominal = %d", rp.num_diffs());
      rp.pretty_print( false );
      display->clearDisplay();
      display->setTextSize(1);              // Normal 1:1 pixel scale
      display->setTextColor(SSD1306_WHITE); // Draw white text
      display->setCursor(0,0);              // Start at top-left corner    rp.print_versus_local_config();
      display->println("Waiting for user talk\n\nignores after 60s");
      display->display();
      Serial.printf("Do you wish to reset to defaults? [Y/n]:"); Serial1.printf("Do you wish to reset to defaults? [Y/n]:");
      uint8_t count = 0;
      while ( !Serial.available() && !Serial1.available() && ++count<60 ) delay(1000);
      uint16_t answer = 'n';
      if ( Serial.available() ) answer=Serial.read();
      else if ( Serial1.available() ) answer=Serial1.read();
      if ( answer=='Y' )
      {
        Serial.printf(" Y\n"); Serial1.printf(" Y\n");
        rp.nominal();
        rp.pretty_print( true );
      }
      else
      {
        Serial.printf(" N.  moving on...\n\n"); Serial1.printf(" N.  moving on...\n\n");
      }
    }
    else
    {
      rp.pretty_print( true );
      Serial.printf(" No diffs in retained...\n\n"); Serial1.printf(" No diffs in retained...\n\n");
    }
  }

  Serial.printf("End setup()\n\n");
} // setup


// Loop
void loop()
{

  // Synchronization
  boolean read;                               // Read, T/F
  static Sync *ReadSensors = new Sync(READ_DELAY);

  boolean read_temp;                          // Read temp, T/F
  static Sync *ReadTemp = new Sync(READ_TEMP_DELAY);

  boolean display_to_user;                    // User display, T/F
  static Sync *DisplayUserSync = new Sync(DISPLAY_USER_DELAY);

  boolean summarizing;                        // Summarize, T/F
  static boolean boot_wait = true;  // waiting for a while before summarizing
  static Sync *Summarize = new Sync(SUMMARIZE_DELAY);

  boolean control;                            // Summarize, T/F
  static Sync *ControlSync = new Sync(CONTROL_DELAY);

  unsigned long current_time;                 // Time result
  static unsigned long now = millis();        // Keep track of time
  time32_t time_now;                          // Keep track of time
  static unsigned long start = millis();      // Keep track of time
  unsigned long elapsed = 0;                  // Keep track of time
  static boolean reset = true;                // Dynamic reset
  static boolean reset_temp = true;           // Dynamic reset
  static boolean reset_publish = true;        // Dynamic reset

  // Sensor conversions
  static Sensors *Sen = new Sensors(EKF_NOM_DT, 0, myPins->pin_1_wire, ReadSensors); // Manage sensor data.  Sim is in here.

   // Mon, used to count Coulombs and run EKF
  static BatteryMonitor *Mon = new BatteryMonitor(&rp.delta_q, &rp.t_last, &rp.nP, &rp.nS, &rp.mon_chm, &rp.hys_scale);

  // Battery saturation debounce
  static TFDelay *Is_sat_delay = new TFDelay(false, T_SAT, T_DESAT, EKF_NOM_DT);   // Time persistence

  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Synchronize
  now = millis();
  time_now = Time.now();
  sync_time(now, &last_sync, &millis_flip);      // Refresh time synchronization
  char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
  Sen->control_time = decimalTime(&current_time, tempStr, Sen->now, millis_flip);
  hm_string = String(tempStr);
  read_temp = ReadTemp->update(millis(), reset);              //  now || reset
  read = ReadSensors->update(millis(), reset);                //  now || reset
  elapsed = ReadSensors->now() - start;
  control = ControlSync->update(millis(), reset);             //  now || reset
  display_to_user = DisplayUserSync->update(millis(), reset); //  now || reset
  boolean boot_summ = boot_wait && ( elapsed >= SUMMARIZE_WAIT ) && !rp.modeling;
  if ( elapsed >= SUMMARIZE_WAIT ) boot_wait = false;
  summarizing = Summarize->update(millis(), false); // now || boot_summ && !rp.modeling
  summarizing = summarizing || boot_summ;

  // Load temperature
  // Outputs:   Sen->Tb,  Sen->Tb_filt
  if ( read_temp )
  {
    Sen->T_temp =  ReadTemp->updateTime();
    Sen->temp_load_and_filter(Sen, reset_temp);
  }

  // Input all other sensors and do high rate calculations
  if ( read )
  {
    Sen->reset = reset;

    // Set print frame
    static uint8_t print_count = 0;
    if ( print_count>=cp.print_mult-1 || print_count==UINT8_MAX )  // > avoids lockup on change by user
    {
      print_count = 0;
      cp.publishS = true;
    }
    else
    {
      print_count++;
      cp.publishS = false;
    }

    // Read sensors, model signals, select between them, synthesize injection signals on current
    // Inputs:  rp.config, rp.sim_chm
    // Outputs: Sen->Ib, Sen->Vb, Sen->Tb_filt, rp.inj_bias
    sense_synth_select(reset, reset_temp, ReadSensors->now(), elapsed, myPins, Mon, Sen);
    Sen->T =  double(Sen->dt_ib())/1000.;

    // Calculate Ah remaining`
    // Inputs:  rp.mon_chm, Sen->Ib, Sen->Vb, Sen->Tb_filt
    // States:  Mon.soc
    // Outputs: tcharge_wt, tcharge_ekf
    monitor(reset, reset_temp, now, Is_sat_delay, Mon, Sen);

    // Re-init Coulomb Counter to EKF if it is different than EKF or if never saturated
    Mon->regauge(Sen->Tb_filt);

    // Empty battery
    if ( rp.modeling && reset && Sen->Sim->q()<=0. ) Sen->Ib = 0.;

    // Debug for read
    if ( rp.debug==12 ) debug_12(Mon, Sen);  // EKF
    if ( rp.debug==-4 ) debug_m4(Mon, Sen);

    // Publish for variable print rate
    if ( cp.publishS )
    {
      assign_publist(&pp.pubList, ReadSensors->now(), unit, hm_string, Sen, num_timeouts, Mon);
      static boolean wrote_last_time = false;
      if ( wrote_last_time )
        digitalWrite(myPins->status_led, LOW);
      else
        digitalWrite(myPins->status_led, HIGH);
      wrote_last_time = !wrote_last_time;
    }

    // Print
    print_rapid_data(reset, Sen, Mon);

  }  // end read (high speed frame)

  // OLED and Bluetooth display drivers
  if ( display_to_user )
  {
    oled_display(display, Sen, Mon);
  }

  // Discuss things with the user
  // When open interactive serial monitor such as CoolTerm
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  // Control
  if ( control ){} // placeholder
  // Chit-chat requires 'read' timing so 'DP' and 'Dr' can manage sequencing
  asap();
  if ( read )
  {
    chat();         // Work on internal chit-chat
  }
  talk(Mon, Sen);   // Collect user inputs

  // Summary management.   Every boot after a wait an initial summary is saved in rotating buffer
  // Then every half-hour unless modeling.   Can also request manually via cp.write_summary (Talk)
  if ( (!boot_wait && summarizing) || cp.write_summary )
  {
    if ( ++rp.isum>NSUM-1 ) rp.isum = 0;
    mySum[rp.isum].assign(time_now, Mon, Sen);
    Serial.printf("Summ...\n");
    cp.write_summary = false;
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = false;
  if ( read_temp && elapsed>TEMP_INIT_DELAY ) reset_temp = false;
  if ( cp.publishS ) reset_publish = false;

  // Soft reset
  if ( cp.soft_reset )
  {
    reset = reset_temp = reset_publish = true;
    Serial.printf("soft reset...\n");
  }
  cp.soft_reset = false;
} // loop
