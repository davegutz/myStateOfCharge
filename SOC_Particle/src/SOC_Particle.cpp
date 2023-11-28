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
// This works when I'm using three platforms:   PHOTON = 6 and ARGON = 12 and PHOTON2 = (>=20)
void setup();
void loop();
#line 54 "c:/Users/daveg/Documents/GitHub/myStateOfCharge/SOC_Particle/src/SOC_Particle.ino"
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
#include "mySync.h"
#include "mySubs.h"
#include "mySummary.h"
#include "myCloud.h"
#include "debug.h"
#include "parameters.h"
#include "Variable.h"

//#define BOOT_CLEAN      // Use this to clear 'lockup' problems introduced during testing using Talk
SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status

#ifdef CONFIG_47L16
  #include "hardware/SerialRAM.h"
  SerialRAM ram;
#endif

#ifdef CONFIG_USE_BLE
  #include <BleSerialPeripheralRK.h>
  SerialLogHandler logHandler;
#endif

// Globals
extern SavedPars sp;              // Various parameters to be static at system level and saved through power cycle
extern CommandPars cp;            // Various parameters to be common at system level
extern Flt_st mySum[NSUM];        // Summaries for saving charge history
extern PublishPars pp;            // For publishing

#ifdef CONFIG_47L16
  SavedPars sp = SavedPars(&ram);     // Various parameters to be common at system level
  Variable <float> Freq = Variable<float>(sp.freq_ptr(), &ram, sp.freq_eeram(), "Inj freq", "Hz");
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

#ifdef CONFIG_USE_BLE
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
  Serial.begin(230400);
  Serial.flush();
  delay(1000);          // Ensures a clean display on Serial startup on CoolTerm
  Serial.println("Hi!");

  // EERAM and Bluetooth Serial1.  Use BT-AT project in this GitHub repository to change.
  // TX of HC-06 
  // Compile and flash onto the SOC_Photon target temporarily to set baud rate.  Directions
  // for HC-06 inside SOC_Photon.ino of ../../BT-AT/src.   AT+BAUD8; to set 115200.
  // Serial1.blockOnOverrun(false); doesn't work:  it's a mess; partial lines galore
  Serial1.begin(S1BAUD);
  Serial1.flush();

  // EERAM
  #ifdef CONFIG_47L16
    ram.begin(0, 0);
    ram.setAutoStore(true);
    delay(1000);
    sp.init_ram();
    sp.load_all();
  #else
    sp.init();
  #endif

  // Argon built-in BLE does not have friendly UART terminal app available.  Using HC-06
  #ifdef CONFIG_USE_BLE
    bleSerial.setup();
    bleSerial.advertise();
    Serial.printf("BLE mac=>%s\n", BLE.address().toString().c_str());
  #endif

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
  // A2->A0->D11 - Primary Ib amp (called by old ADS name Amplified, amp)
  // A1->D12 - Vb
  // A3->A2->D13 - Backup Ib amp (called by old ADS name Non Amplified, noa)
  // A4 - not available
  // A5-->D14 - spare
  #ifdef CONFIG_PHOTON2
    myPins = new Pins(D3, D7, D12, D11, D13);
    // pinMode(D3, INPUT_PULLUP);
  #else
    myPins = new Pins(D6, D7, A1, A2, A3, A4, A5);
  #endif
  pinMode(myPins->status_led, OUTPUT);
  digitalWrite(myPins->status_led, LOW);

  // I2C for OLED, ADS, backup EERAM
  #ifndef CONFIG_BARE
    Wire.setSpeed(CLOCK_SPEED_100KHZ);
    Wire.begin();
  #endif

  // Display
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  Serial.println("Init DISPLAY");
  if(!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) // Seems to return true even if depowered
  {
    Serial.println(F("DISP FAIL"));
    // Can Use Bluetooth as workaround
  }
  else
    Serial.println("DISP allocated");
  #ifndef CONFIG_BARE
    display->clearDisplay();
  #endif

  // Synchronize clock
  // Device needs to be configured for wifi (hold setup 3 sec run Particle app) and in range of wifi
  // Phone hotspot is very convenient
  WiFi.disconnect();
  delay(2000);
  WiFi.off();
  delay(1000);
  Serial.printf("Done WiFi\n");
  Serial.printf("done CLOUD\n");

  // Clean boot logic.  This occurs only when doing a structural rebuild clean make on initial flash, because
  // the SRAM is not explicitly initialized.   This is by design, as SRAM must be remembered between boots
  Serial.printf("Check corruption......");
  if ( sp.is_corrupt() ) 
  {
    sp.reset_pars();
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
  Serial.printf("millis_flip%ld count%d\n", millis_flip, count);

  // Enable and print stored history
  #if defined(CONFIG_PHOTON) || defined(CONFIG_PHOTON2)  // TODO: test that ARGON still works with the #if in place
    System.enableFeature(FEATURE_RETAINED_MEMORY);
  #endif
  if ( sp.debug()==1 || sp.debug()==2 || sp.debug()==3 || sp.debug()==4 )
  {
    sp.print_history_array();
    sp.print_fault_header();
  }
  // Ask to renominalize
  if ( ASK_DURING_BOOT )
  {
    if ( sp.num_diffs() )
    {
      wait_on_user_input(display);
    }
    else
    {
      // sp.pretty_print( true );
      Serial.printf(" No diffs in retained...\n\n"); Serial1.printf(" No diffs in retained...\n\n");
    }
  }

  Serial.printf("End setup()\n\n");
} // setup


// Loop
void loop()
{
  // Synchronization
  boolean read;
  static Sync *ReadSensors = new Sync(READ_DELAY);
  boolean read_temp;
  static Sync *ReadTemp = new Sync(READ_TEMP_DELAY);
  boolean display_and_remember;
  static Sync *DisplayUserSync = new Sync(DISPLAY_USER_DELAY);
  boolean summarizing;
  static boolean boot_wait = true;  // waiting for a while before summarizing
  static Sync *Summarize = new Sync(SUMMARIZE_DELAY);
  boolean control;
  static Sync *ControlSync = new Sync(CONTROL_DELAY);
  unsigned long current_time;
  static unsigned long now = millis();
  time32_t time_now;
  static unsigned long start = millis();
  unsigned long elapsed = 0;
  static boolean reset = true;
  static boolean reset_temp = true;
  static boolean reset_publish = true;

  // Sensor conversions.  The embedded model 'Sim' is contained in Sensors
  static Sensors *Sen = new Sensors(EKF_NOM_DT, 0, myPins, ReadSensors);

   // Monitor to count Coulombs and run EKF
  static BatteryMonitor *Mon = new BatteryMonitor();

  // Battery saturation debounce
  static TFDelay *Is_sat_delay = new TFDelay(false, T_SAT, T_DESAT, EKF_NOM_DT);

  // Variables storage
  #ifdef CONFIG_47L16
    static Vars *V = new Vars(&sp, &ram);
  #else
    static Vars *V = new Vars(&sp);
  #endif

  ///////////////////////////////////////////////////////////// Top of loop////////////////////////////////////////

  // Synchronize
  now = millis();
  time_now = Time.now();
  if ( now - last_sync > ONE_DAY_MILLIS || reset )  sync_time(now, &last_sync, &millis_flip); 
  char  tempStr[23];  // time, year-mo-dyThh:mm:ss iso format, no time zone
  Sen->control_time = decimalTime(&current_time, tempStr, Sen->now, millis_flip);
  hm_string = String(tempStr);
  read_temp = ReadTemp->update(millis(), reset);
  read = ReadSensors->update(millis(), reset);
  elapsed = ReadSensors->now() - start;
  control = ControlSync->update(millis(), reset);
  display_and_remember = DisplayUserSync->update(millis(), reset);
  boolean boot_summ = boot_wait && ( elapsed >= SUMMARIZE_WAIT ) && !sp.Modeling();
  if ( elapsed >= SUMMARIZE_WAIT ) boot_wait = false;
  summarizing = Summarize->update(millis(), false);
  summarizing = summarizing || boot_summ;

  #ifdef CONFIG_USE_BLE
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

  // Sample temperature
  // Outputs:   Sen->Tb,  Sen->Tb_filt
  if ( read_temp )
  {
    Sen->T_temp = ReadTemp->updateTime();
    Sen->temp_load_and_filter(Sen, reset_temp);
  }

  // Sample Ib
  #ifndef CONFIG_ADS1013
    if ( read )
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
    
    // Check for really slow data capture and run EKF each read frame
    cp.assign_eframe_mult(max(int(float(READ_DELAY)*float(EKF_EFRAME_MULT)/float(ReadSensors->delay())+0.9999), 1));

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
    if ( sp.Modeling() && reset && Sen->Sim->q()<=0. ) Sen->Ib = 0.;

    // Debug for read
    #ifndef CONFIG_PHOTON
      if ( sp.debug()==12 ) debug_12(Mon, Sen);
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

  }  // end read (high speed frame)

  // OLED and Bluetooth display drivers.   Also convenient update time for saving parameters (remember)
  if ( display_and_remember )
  {
    oled_display(display, Sen, Mon);

    #ifdef CONFIG_47L16
      // Save EERAM dynamic parameters.  Saves critical few state parameters
      sp.put_all_dynamic();
    #endif
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
  talk(Mon, Sen, V);   // Collect user inputs

  // Summary management.   Every boot after a wait an initial summary is saved in rotating buffer
  // Then every half-hour unless modeling.   Can also request manually via cp.write_summary (Talk)
  if ( (!boot_wait && summarizing) || cp.write_summary )
  {
    sp.put_ihis(sp.ihis()+1);
    if ( sp.ihis()>sp.nhis()-1 ) sp.put_ihis(0);  // wrap buffer
    Flt_st hist_snap, hist_bounced;
    hist_snap.assign(time_now, Mon, Sen);
    hist_bounced = sp.put_history(hist_snap, sp.ihis());

    sp.put_isum(sp.isum()+1);
    if ( sp.isum()>NSUM-1 ) sp.put_isum(0);
    mySum[sp.isum()].copy_to_Flt_ram_from(hist_bounced);

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
