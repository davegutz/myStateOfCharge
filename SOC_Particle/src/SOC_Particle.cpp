/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/daveg/Documents/GitHub/myStateOfCharge/SOC_Particle/src/SOC_Particle.ino"
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
  * 21-Sep-2022   Alpha release v20220917.  Branch GitHub repository.  Added signal redundancy checks and fault handling.
  * 26-Nov-2022   First Beta release v20221028.   Branch GitHub repository.  Various debugging fixes hysteresis.
  * 12-Dec-2022   RetainedPars-->SavedPars to support Argon with 47L16 EERAM device
  * 22-Dec-2022   Dual amplifier replaces dual ADS.  Beta release v20221220.  ADS still used on Photon.
  * 01-Dec-2023   g20231111 Photon 2, DS2482
  * 
//
// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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
// This works when I'm using three platforms:   PHOTON = 6 and ARGON = 12 and PHOTON2 = (>=20)
void setup();
void loop();
#line 55 "c:/Users/daveg/Documents/GitHub/myStateOfCharge/SOC_Particle/src/SOC_Particle.ino"
#ifndef PLATFORM_ID
  #define PLATFORM_ID 6
#endif
#ifndef PLATFORM_PHOTON
  #define PLATFORM_PHOTON 6
#endif

#include "constants.h"
// Prevent mixing up local_config files (still could sneak soc0p through as pro0p)
#if defined(CONFIG_PHOTON)
  #undef ARDUINO
  #if (PLATFORM_ID != PLATFORM_PHOTON)
    #error "copy local_config.xxxx.h to local_config.h"
  #endif
#elif defined(CONFIG_ARGON)
  #undef ARDUINO
  #if (PLATFORM_ID != PLATFORM_ARGON)
    #error "copy local_config.xxxx.h to local_config.h"
  #endif
#elif defined(CONFIG_PHOTON2)
  #undef ARDUINO
  #if (PLATFORM_ID != PLATFORM_P2)
    #error "copy local_config.xxxx.h to local_config.h"
  #endif
#endif

// Dependent includes.   Easier to sp.debug code if remove unused include files
#include "Sync.h"
#include "subs.h"
#include "Summary.h"
#include "Cloud.h"
#include "debug.h"
#include "parameters.h"
#include "serial.h"

//#define BOOT_CLEAN      // Use this to clear 'lockup' problems introduced during testing using Talk
SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status

// Turn on Log
#ifdef LOGHANDLE
  SerialLogHandler logHandler;
#endif

#ifdef CONFIG_DS2482_1WIRE
  #include "myDS2482.h"
  MyDs2482_Class Ds2482(0);
  DS2482 ds(Wire, 0);
  DS2482DeviceListStatic<10> deviceList;
#endif

#ifdef CONFIG_47L16_EERAM
  #include "hardware/SerialRAM.h"
  SerialRAM ram;
#endif

// Globals
extern SavedPars sp;              // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap;           // Various adjustment parameters shared at system level
extern CommandPars cp;            // Various parameters shared at system level
extern PrinterPars pr;            // Print buffer structure
extern PublishPars pp;            // For publishing
extern Flt_st mySum[NSUM];        // Summaries for saving charge history

#ifdef CONFIG_47L16_EERAM
  retained SavedPars sp = SavedPars(&ram);  // Various parameters to be common at system level
#else
  retained Flt_st saved_hist[NHIS];    // For displaying faults
  retained Flt_st saved_faults[NFLT];  // For displaying faults
  retained SavedPars sp = SavedPars(saved_hist, uint16_t(NHIS), saved_faults, uint16_t(NFLT));  // Various parameters to be common at system level
#endif

Flt_st mySum[NSUM];                   // Summaries
PrinterPars pr = PrinterPars();       // Print buffer
VolatilePars ap = VolatilePars();     // Various adjustment parameters commanding at system level.  Initialized on start up.  Not retained.
CommandPars cp = CommandPars();       // Various control parameters commanding at system level.  Initialized on start up.  Not retained.
PublishPars pp = PublishPars();       // Common parameters for publishing.  Future-proof cloud monitoring
unsigned long long millis_flip = millis(); // Timekeeping
unsigned long long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm
Pins *myPins;                   // Photon hardware pin mapping used
#ifdef CONFIG_SSD1306_OLED
  Adafruit_SSD1306 *display;      // Main OLED display
#endif

// Setup
void setup()
{
  Log.info("begin setup");
  // Serial
  // Serial.blockOnOverrun(false);  doesn't work
  Serial.begin(CONFIG_SBAUD);
  Serial.flush();
  delay(1000);          // Ensures a clean display
  Serial.printf("Hi!\n");

  // EERAM and Bluetooth Serial1.  Use BT-AT project in this GitHub repository to change.
  // TX of HC-06 
  // Compile and flash onto the SOC_Photon target temporarily to set baud rate.  Directions
  // for HC-06 inside SOC_Photon.ino of ../../BT-AT/src.   AT+BAUD8; to set 115200.
  // Serial1.blockOnOverrun(false); doesn't work:  it's a mess; partial lines galore
  Serial1.begin(CONFIG_S1BAUD);
  Serial1.flush();

  // EERAM chip card for I2C
  #ifdef CONFIG_47L16_EERAM
    Log.info("setup EERAM");
    ram.begin(0, 0);
    ram.setAutoStore(true);
    delay(1000);
    sp.load_all();
  #endif
  sp.put_Time_now(max(sp.Time_now_z, (unsigned long)Time.now()));  // Synch with web when possible
  Time.setTime(sp.Time_now_z);

  // Peripherals (non-Photon2)
  // D6 - one-wire temp sensor
  // D7 - status led heartbeat
  // A1 - Vb
  // A2 - Primary Ib amp (called by old ADS name Amplified, amp)
  // A3 - Backup Ib amp (called by old ADS name Non Amplified, noa)
  // A4 - Ib amp common

  // Peripherals (Photon2)
  // D3 - one-wire temp sensor ******** to be replaced by I2C device
  // D7 - status led heartbeat
  // A0 (pin 'D11') - Primary Ib amp (called by old ADS name Amplified, amp)
  // A1 (pin 'D12') - Vb
  // A2 (pin 'D13') - Backup Ib amp (called by old ADS name Non Amplified, noa)
  // A4 - not available
  // A5-->D14 - spare
  
  Log.info("setup Pins");
  #ifdef CONFIG_PHOTON2
    myPins = new Pins(D3, D7, D12, D11, D13);
  #else
    myPins = new Pins(D6, D7, A1, A2, A3, A4, A5);
  #endif
  pinMode(myPins->status_led, OUTPUT);
  digitalWrite(myPins->status_led, LOW);

  // I2C for OLED, ADS, backup EERAM, DS2482
  // Photon2 only accepts 100 and 400 khz
  #ifndef CONFIG_BARE
    Log.info("setup I2C Wire");
    #ifdef CONFIG_ADS1013_OPAMP
      Wire.setSpeed(CLOCK_SPEED_100KHZ);
      Serial.printf("Nominal Wire setup for ADS1013\n");
    #else
      Wire.setSpeed(CLOCK_SPEED_100KHZ);
      Serial.printf("Wire started\n");
    #endif
    Wire.begin();
  #endif

  // Display (after start Wire)
  #ifdef CONFIG_SSD1306_OLED
    Log.info("setup display");
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    Serial.printf("Init DISP\n");
    if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) // Seems to return true even if depowered
    {
      Serial.printf("FAIL. Use BT\n");
      // Can Use Bluetooth as workaround
    }
    else
      Serial.printf("DISP ok\n");
    #ifndef CONFIG_BARE
      display->clearDisplay();
    #endif
  #endif

  // 1-Wire chip card for I2C (after start Wire)
  #ifdef CONFIG_DS2482_1WIRE
    Log.info("setup DS2482 special 1-wire");
    ds.setup();
    Ds2482.setup();
    Serial.printf("DS2482 multi-drop setup complete\n");
  #endif

  // Synchronize clock
  // Device needs to be configured for wifi (hold setup 3 sec run Particle app) and in range of wifi
  // Phone hotspot is very convenient
  Log.info("setup WiFi or lack of");
  WiFi.disconnect();
  delay(2000);
  WiFi.off();
  delay(1000);
  Serial.printf("Done WiFi\n");
  Serial.printf("done CLOUD\n");

  // Clean boot logic.  This occurs only when doing a structural rebuild clean make on initial flash, because
  // the SRAM is not explicitly initialized.   This is by design, as SRAM must be remembered between boots
  // Time is never changed by this operation.  It could be corrupt.  Change using "UT" talk feature.
  Serial.printf("Check corruption......");
  if ( sp.is_corrupt() ) 
  {
    Serial.printf("\n\n");
    sp.pretty_print( false );
    Serial.printf("\n\n");
    sp.set_nominal();
    Serial.printf("Fixed corruption\n");
    sp.pretty_print(true);
  }
  else Serial.printf("clean\n");

  // Determine millis() at turn of Time.now   Used to improve accuracy of timing.
  long time_begin = Time.now();
  uint16_t count = 0;
  while ( Time.now()==time_begin && count++<1000 )
  {
    delay(1);
    millis_flip = millis()%1000;
  }

  // Enable and print stored history
  #if defined(CONFIG_PHOTON) || defined(CONFIG_PHOTON2)  // TODO: test that ARGON still works with the #if in place
    System.enableFeature(FEATURE_RETAINED_MEMORY);
  #endif
  if ( sp.debug_z==1 || sp.debug_z==2 || sp.debug_z==3 || sp.debug_z==4 )
  {
    sp.print_history_array();
    sp.print_fault_header();
  }
  sp.nsum(NSUM);  // Store

  // Ask to renominalize
  if ( ASK_DURING_BOOT )
  {
    Log.info("setup renominalize");
    if ( sp.num_diffs() )
    {
      #ifdef CONFIG_SSD1306_OLED
        wait_on_user_input(display);
      #else
        wait_on_user_input();
      #endif
    }
  }

  Log.info("setup end");
  Serial.printf("End setup()\n\n");
} // setup


// Loop
void loop()
{
  // Synchronization
  static unsigned long long now = (unsigned long long) millis();
  unsigned long long time_now = (unsigned long long) Time.now();
  now = (unsigned long long) millis();
  boolean chitchat = false;
  static Sync *Talk = new Sync(TALK_DELAY);
  boolean read = false;
  static Sync *ReadSensors = new Sync(READ_DELAY);
  boolean read_temp = false;
  static Sync *ReadTemp = new Sync(READ_TEMP_DELAY);
  boolean display_and_remember;
  static Sync *DisplayUserSync = new Sync(DISPLAY_USER_DELAY);
  boolean summarizing;
  static boolean boot_wait = true;  // waiting for a while before summarizing
  static Sync *Summarize = new Sync(SUMMARY_DELAY);
  boolean control;
  static Sync *ControlSync = new Sync(CONTROL_DELAY);
  static unsigned long long start = millis();
  unsigned long long elapsed = 0;
  static boolean reset = true;
  static boolean reset_temp = true;
  static boolean reset_publish = true;

  // Sensor conversions.  The embedded model 'Sim' is contained in Sensors
  static Sensors *Sen = new Sensors(EKF_NOM_DT, 0, myPins, ReadSensors, Talk, Summarize, time_now);

   // Monitor to count Coulombs and run EKF
  static BatteryMonitor *Mon = new BatteryMonitor();

  // Battery saturation debounce
  static TFDelay *Is_sat_delay = new TFDelay(false, T_SAT, T_DESAT, EKF_NOM_DT);

  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Synchronize
  #ifdef CONFIG_DS2482_1WIRE
    Ds2482.loop();
  #endif
  if ( now - last_sync > ONE_DAY_MILLIS || reset )  sync_time(now, &last_sync, &millis_flip); 
  Sen->control_time = double(Sen->now/1000);
  char buffer[32];
  time_long_2_str(time_now, buffer);
  hm_string = String(buffer);
  read_temp = ReadTemp->update(millis(), reset);
  read = ReadSensors->update(millis(), reset);
  chitchat = Talk->update(millis(), reset);
  elapsed = ReadSensors->now() - start;
  control = ControlSync->update(millis(), reset);
  display_and_remember = DisplayUserSync->update(millis(), reset);
  boolean boot_summ = boot_wait && ( elapsed >= SUMMARY_WAIT / (SUMMARY_DELAY / ap.his_delay) ) && !sp.modeling_z;
  if ( elapsed >= SUMMARY_WAIT / (SUMMARY_DELAY / ap.his_delay) ) boot_wait = false;
  summarizing = Summarize->update(millis(), false) || boot_summ;

  // Sample temperature
  // Outputs:   Sen->Tb,  Sen->Tb_filt
  if ( read_temp )
  {
    Log.info("read_temp");
    #ifdef CONFIG_DS2482_1WIRE
        Ds2482.check();
        cp.tb_info.t_c = Ds2482.tempC(0);
        cp.tb_info.ready = Ds2482.ready();
    #endif
    Sen->T_temp = ReadTemp->updateTime();
    Sen->temp_load_and_filter(Sen, reset_temp);
  }

  // Sample Ib
  #ifndef CONFIG_ADS1013_OPAMP
    if ( read )
    {
      Log.info("Read shunt");
      static unsigned int t_us_last = micros();
      unsigned int t_us_now = micros();
      float T = float(t_us_now - t_us_last) / 1e6;
      t_us_last = t_us_now;
      Sen->ShuntAmp->sample(reset, T);
      Sen->ShuntNoAmp->sample(reset, T);
    }
  #endif
  
  // Input all other sensors and do high rate calculations
  if ( read )
  {
    Log.info("read");
    Sen->reset = reset;
    
    // Check for really slow data capture and run EKF each read frame
    ap.eframe_mult = max(int(float(READ_DELAY)*float(EKF_EFRAME_MULT)/float(ReadSensors->delay())+0.9999), 1);

    // Set print frame
    static uint8_t print_count = 0;
    if ( print_count>=ap.print_mult-1 || print_count==UINT8_MAX )  // > avoids lockup on change by user
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
    // Inputs:  sp.config, sp.sim_chm
    // Outputs: Sen->Ib, Sen->Vb, Sen->Tb_filt, sp.inj_bias
    sense_synth_select(reset, reset_temp, ReadSensors->now(), elapsed, myPins, Mon, Sen);
    Sen->T =  double(Sen->dt_ib())/1000.;

    // Calculate Ah remaining`
    // Inputs:  sp.mon_chm, Sen->Ib, Sen->Vb, Sen->Tb_filt
    // States:  Mon.soc
    // Outputs: tcharge_wt, tcharge_ekf
    monitor(reset, reset_temp, now, Is_sat_delay, Mon, Sen);

    // Re-init Coulomb Counter to EKF if it is different than EKF or if never saturated
    Mon->regauge(Sen->Tb_filt);

    // Empty battery
    if ( sp.modeling_z && reset && Sen->Sim->q()<=0. ) Sen->Ib = 0.;

    // Debug for read
    #ifndef CONFIG_PHOTON
      if ( sp.debug_z==12 ) debug_12(Mon, Sen);
    #endif
    
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

    Log.info("end read");
  }  // end read (high speed frame)

  // OLED and Bluetooth display drivers.   Also convenient update time for saving parameters (remember)
  if ( display_and_remember )
  {
    Log.info("display and remember");
    #ifdef CONFIG_SSD1306_OLED
      oled_display(display, Sen, Mon);
    #else
      oled_display(Sen, Mon);
    #endif

    #ifdef CONFIG_47L16_EERAM
      // Save EERAM dynamic parameters.  Saves critical few state parameters
      sp.put_all_dynamic();
    #else
      sp.put_Time_now(max( sp.Time_now_z, (unsigned long)Time.now()));  // If happen to connect to wifi (assume updated automatically), save new time
    #endif
  }

  // Discuss things with the user
  // When open interactive serial monitor such as puTTY
  // then can enter commands by sending strings.   End the strings with a real carriage return
  // right in the "Send String" box then press "Send."
  // String definitions are below.
  // Control
  if ( control ){} // placeholder

  // Chit-chat requires 'read' timing so 'DP' and 'Dr' can manage sequencing
  // Running chitter unframed allows queues of different priorities to be built from long
  // runs of Serial inputs
  chitter(chitchat);  // Parse inputs to queues
  chatter();  // Prioritize commands to describe.  ctl_str and asap_str queues always run.  Others only with chitchat
  describe(Mon, Sen);  // Run the commands

  // Summary management.   Every boot after a wait an initial summary is saved in rotating buffer
  // Then every half-hour unless modeling.   Can also request manually via cp.write_summary (Talk)
  if ( (!boot_wait && summarizing) || cp.write_summary )
  {
    sp.put_Ihis(sp.ihis_z + 1);
    if ( sp.ihis_z > (sp.nhis() - 1) ) sp.put_Ihis(0);  // wrap buffer
    Flt_st hist_snap, hist_bounced;
    hist_snap.assign(Time.now(), Mon, Sen);
    hist_bounced = sp.put_history(hist_snap, sp.ihis_z);

    sp.put_Isum(sp.isum_z + 1);
    if ( sp.isum_z > (uint16_t)(sp.nsum()-1) ) sp.put_Isum(0);  // wrap buffer
    mySum[sp.isum_z].copy_to_Flt_ram_from(hist_bounced);
    Serial.printf("Summ...\n");
    cp.write_summary = false;
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = false;
  if ( read_temp && elapsed>TEMP_INIT_DELAY && reset_temp )
  {
    Serial.printf("\ntemp init complete\n");
    reset_temp = false;
  }
  if ( cp.publishS ) reset_publish = false;

  // Soft reset
  if ( read ) cp.soft_sim_hold = false;
  if ( cp.soft_reset || cp.soft_reset_sim )
  {
    reset = reset_temp = reset_publish = true;
    if ( cp.soft_reset_sim ) cp.cmd_soft_sim_hold();
  }
  cp.soft_reset = false;
  cp.soft_reset_sim = false;

} // loop
