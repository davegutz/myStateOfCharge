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
  * 26-Nov-2022   Beta release v20221028.   Branch GitHub repository.  Various debugging fixes hysteresis.
  * 12-Dec-2022   RetainedPars-->SavedPars to support Argon with 47L16 EERAM device
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
#include "constants.h"


// Dependent includes.   Easier to sp.debug code if remove unused include files
#include "mySync.h"
#include "mySubs.h"
#include "mySummary.h"
#include "myCloud.h"
#include "debug.h"
#include "parameters.h"

// This works when I'm using two platforms:   PHOTON = 6 and ARGON = 12
#ifndef PLATFORM_ID
  #define PLATFORM_ID 12
#endif
#define PLATFORM_ARGON 12

//#define BOOT_CLEAN      // Use this to clear 'lockup' problems introduced during testing using Talk
SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status

#if ( PLATFORM_ID == PLATFORM_ARGON )
  #include "hardware/SerialRAM.h"
  SerialRAM ram;
  #ifdef USE_BLE
    #include <BleSerialPeripheralRK.h>
    SerialLogHandler logHandler;
  #endif
#endif

// Globals
extern SavedPars sp;              // Various parameters to be common at system level
extern CommandPars cp;            // Various parameters to be common at system level
extern Flt_st mySum[NSUM];        // Summaries for saving charge history
extern PublishPars pp;            // For publishing

#if ( PLATFORM_ID == PLATFORM_ARGON )
  SavedPars sp = SavedPars(&ram);     // Various parameters to be common at system level
#else
  retained Flt_st saved_hist[NHIS];    // For displaying faults
  retained Flt_st saved_faults[NFLT];  // For displaying faults
  retained SavedPars sp = SavedPars(saved_hist, NHIS, saved_faults, NFLT);  // Various parameters to be common at system level
#endif
Flt_st mySum[NSUM];                   // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level
PublishPars pp = PublishPars();       // Common parameters for publishing.  Future-proof cloud monitoring
unsigned long millis_flip = millis(); // Timekeeping
unsigned long last_sync = millis();   // Timekeeping

int num_timeouts = 0;           // Number of Particle.connect() needed to unfreeze
String hm_string = "00:00";     // time, hh:mm
Pins *myPins;                   // Photon hardware pin mapping used
Adafruit_SSD1306 *display;      // Main OLED display

  #if ( PLATFORM_ID == PLATFORM_ARGON ) & defined( USE_BLE )
  // First parameter is the transmit buffer size, second parameter is the receive buffer size
  BleSerialPeripheralStatic<32, 256> bleSerial;
  const unsigned long TRANSMIT_PERIOD_MS = 2000;
  unsigned long lastTransmit = 0;
  int counter = 0;
#endif

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
  // Serial1.blockOnOverrun(false); doesn't work.  Mess; partial lines galore
  Serial1.begin(115200);
  Serial1.flush();
  #if ( PLATFORM_ID == PLATFORM_ARGON )
    ram.begin(0, 0);
    ram.setAutoStore(true);
    delay(1000);
    sp.load_all();
    #ifdef USE_BLE
      bleSerial.setup();
      bleSerial.advertise();
      Serial.printf("BLE mac=>%s\n", BLE.address().toString().c_str());
    #endif
  #endif

  // Peripherals
  myPins = new Pins(D6, D7, A1, A3, A4, A5);

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
  // the SRAM is not explicitly initialized.   This is by design, as SRAM must be remembered between boots
#ifdef BOOT_CLEAN
  sp.nominal();
  Serial.printf("Force nominal rp %s\n", cp.buffer);
  sp.pretty_print();
#endif
  Serial.printf("Check corruption......");
  if ( sp.is_corrupt() ) 
  {
    sp.reset_pars();
    Serial.printf("Fixed corruption\n");
    sp.pretty_print(true);
  }
  else Serial.printf("clean\n");

  // Determine millis() at turn of Time.now
  long time_begin = Time.now();
  while ( Time.now()==time_begin )
  {
    delay(1);
    millis_flip = millis()%1000;
  }

  // Summary
  System.enableFeature(FEATURE_RETAINED_MEMORY);
  if ( sp.debug==1 || sp.debug==2 || sp.debug==3 || sp.debug==4 )
  {
    sp.print_history_array();
    sp.print_fault_header();
  }

  // Ask to renominalize
  if ( ASK_DURING_BOOT )
  {
    if ( sp.num_diffs() )
    {
      Serial.printf("#off-nominal = %d", sp.num_diffs());
      sp.pretty_print( false );
      display->clearDisplay();
      display->setTextSize(1);              // Normal 1:1 pixel scale
      display->setTextColor(SSD1306_WHITE); // Draw white text
      display->setCursor(0,0);              // Start at top-left corner    sp.print_versus_local_config();
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
        sp.reset_pars();
        sp.pretty_print( true );
      }
      else
      {
        Serial.printf(" N.  moving on...\n\n"); Serial1.printf(" N.  moving on...\n\n");
      }
    }
    else
    {
      sp.pretty_print( true );
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

  #ifndef USE_ADS
    boolean samp;                               // Samp, T/F
    static Sync *SampIb = new Sync(SAMP_DELAY);
  #endif

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
  static Sensors *Sen = new Sensors(EKF_NOM_DT, 0, myPins, ReadSensors, &sp.nP, &sp.nS); // Manage sensor data.  Sim is in here.

   // Mon, used to count Coulombs and run EKF
  static BatteryMonitor *Mon = new BatteryMonitor(&sp.delta_q, &sp.t_last, &sp.mon_chm, &sp.hys_scale);

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
  #ifndef USE_ADS
    samp = SampIb->update(millis(), reset);                     //  now || reset
  #endif
  control = ControlSync->update(millis(), reset);             //  now || reset
  display_to_user = DisplayUserSync->update(millis(), reset); //  now || reset
  boolean boot_summ = boot_wait && ( elapsed >= SUMMARIZE_WAIT ) && !sp.modeling();
  if ( elapsed >= SUMMARIZE_WAIT ) boot_wait = false;
  summarizing = Summarize->update(millis(), false); // now || boot_summ && !sp.modeling()
  summarizing = summarizing || boot_summ;

  #if ( PLATFORM_ID == PLATFORM_ARGON ) & defined( USE_BLE )
    // This must be called from loop() on every call to loop.
    bleSerial.loop();
    // Print out anything we receive
    if(bleSerial.available()) {
        String s = bleSerial.readString();
        Log.info("received: %s", s.c_str());
    }
    if (millis() - lastTransmit >= TRANSMIT_PERIOD_MS) {
        lastTransmit = millis();
        // Every two seconds, send something to the other side
        bleSerial.printlnf("testing %d", ++counter);
        // Log.info("counter=%d", counter);
        // Serial.printf("passing argon bleSerial\n");
    }
  #endif

  // Load temperature
  // Outputs:   Sen->Tb,  Sen->Tb_filt
  if ( read_temp )
  {
    Sen->T_temp =  ReadTemp->updateTime();
    Sen->temp_load_and_filter(Sen, reset_temp);
  }

  // Sample Ib
  #ifndef USE_ADS
    if ( samp )
    {
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
    if ( sp.modeling() && reset && Sen->Sim->q()<=0. ) Sen->Ib = 0.;

    // Debug for read
    if ( sp.debug==12 ) debug_12(Mon, Sen);  // EKF
    if ( sp.debug==-4 ) debug_m4(Mon, Sen);

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
    sp.put_ihis(sp.ihis+1);
    if ( sp.ihis>sp.nhis()-1 ) sp.put_ihis(0);  // wrap buffer
    Flt_st hist_snap, hist_bounced;
    hist_snap.assign(time_now, Mon, Sen);
    hist_bounced = sp.put_history(hist_snap, sp.ihis);

    if ( ++sp.isum>NSUM-1 ) sp.isum = 0;
    mySum[sp.isum].copy_from(hist_bounced);

    Serial.printf("Summ...\n");
    cp.write_summary = false;
  }

  // Initialize complete once sensors and models started and summary written
  if ( read ) reset = false;
  #ifdef DEBUG_INIT
    if ( sp.debug==-1 )
    {
      if ( read ) Serial.printf("before read read_temp, elapsed, reset_temp %d %d %ld %d\n", read, read_temp, elapsed, reset_temp);
    }
  #endif
  if ( read_temp && elapsed>TEMP_INIT_DELAY ) reset_temp = false;
  #ifdef DEBUG_INIT
    if ( sp.debug==-1 )
    {
      if ( read ) Serial.printf("after read read_temp, elapsed, reset_temp %d %d %ld %d\n", read, read_temp, elapsed, reset_temp);
    }
  #endif
  if ( cp.publishS ) reset_publish = false;

  // Soft reset
  if ( cp.soft_reset )
  {
    reset = reset_temp = reset_publish = true;
    Serial.printf("soft reset...\n");
  }
  cp.soft_reset = false;
} // loop
