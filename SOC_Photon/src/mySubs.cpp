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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "mySubs.h"
#include "command.h"
#include "local_config.h"
#include <math.h>
#include "debug.h"

extern CommandPars cp;          // Various parameters shared at system level
extern PublishPars pp;            // For publishing
extern RetainedPars rp;         // Various parameters to be static at system level


// class Shunt
// constructors
Shunt::Shunt()
: Tweak(), Adafruit_ADS1015(), name_("None"), port_(0x00), bare_(false){}
Shunt::Shunt(const String name, const uint8_t port, float *rp_delta_q_inf, float *rp_tweak_bias, float *cp_curr_bias,
  const double v2a_s)
: Tweak(name, TWEAK_GAIN, TWEAK_MAX_CHANGE, TWEAK_MAX, TWEAK_WAIT, rp_delta_q_inf, rp_tweak_bias),
  Adafruit_ADS1015(),
  name_(name), port_(port), bare_(false), cp_curr_bias_(cp_curr_bias), v2a_s_(v2a_s),
  vshunt_int_(0), vshunt_int_0_(0), vshunt_int_1_(0), vshunt_(0), ishunt_cal_(0)
{
  if ( name_=="No Amp")
    setGain(GAIN_SIXTEEN, GAIN_SIXTEEN); // 16x gain differential and single-ended  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  else if ( name_=="Amp")
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  else
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  if (!begin(port_)) {
    Serial.printf("FAILED to initialize ADS SHUNT MONITOR %s\n", name_.c_str());
    bare_ = true;
  }
  else Serial.printf("SHUNT MONITOR %s initialized\n", name_.c_str());
}
Shunt::~Shunt() {}
// operators
// functions

void Shunt::pretty_print()
{
  Serial.printf("Shunt(%s)::\n", name_.c_str());
  Serial.printf("  port_ =                0x%X; // I2C port used by Acafruit_ADS1015\n", port_);
  Serial.printf("  bare_ =                   %d; // If ADS to be ignored\n", bare_);
  Serial.printf("  *cp_curr_bias_ =    %7.3f; // Global bias, A\n", *cp_curr_bias_);
  Serial.printf("  v2a_s_ =            %7.2f; // Selected shunt conversion gain, A/V\n", v2a_s_);
  Serial.printf("  vshunt_int_ =           %d; // Sensed shunt voltage, count\n", vshunt_int_);
  Serial.printf("  ishunt_cal_ =       %7.3f; // Sensed, calibrated ADC, A\n", ishunt_cal_);
  Serial.printf("Shunt(%s)::", name_.c_str()); Tweak::pretty_print();
  Serial.printf("Shunt(%s)::", name_.c_str()); Adafruit_ADS1015::pretty_print(name_);
}

// load
void Shunt::load()
{
  if (!bare_)
  {
    if ( rp.debug>102 ) Serial.printf("begin %s->readADC_Differential_0_1 at %ld...", name_.c_str(), millis());

    vshunt_int_ = readADC_Differential_0_1();
    
    if ( rp.debug>102 ) Serial.printf("done at %ld\n", millis());
    if ( rp.debug==-14 ) { vshunt_int_0_ = readADC_SingleEnded(0);  vshunt_int_1_ = readADC_SingleEnded(1); }
                    else { vshunt_int_0_ = 0;                       vshunt_int_1_ = 0; }
  }
  else
  {
    vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0;
  }
  vshunt_ = computeVolts(vshunt_int_);
  ishunt_cal_ = vshunt_*v2a_s_*float(!rp.modeling) + *cp_curr_bias_;
}


// Text headers
void print_serial_header(void)
{
  if ( rp.debug==2 )
    Serial.println(F("unit,         hm,                  cTime,        T,       Tb_f, Tb_f_m,  Vb, voc, vsat,    sat,sel,mod, Ib,    tcharge, soc_m,soc_ekf,soc,soc_wt,"));
  else if ( rp.debug==4 )
    Serial.printf("unit,               hm,                  cTime,        T,       sat,sel,mod,  Tb,  Vb,  Ib,        vsat,vdyn,voc,voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,\n");
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  if ( rp.debug==2 )
  sprintf(buffer, "%s, %s, %12.3f,%6.3f,   %4.1f,%4.1f,   %5.2f,%5.2f,%5.2f,  %d,  %d,  %d,   %7.3f,  %5.1f,  %5.3f,%5.3f,%5.3f,%5.3f,  %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time, pubList->T,
    pubList->Tbatt, rp.t_last_model,
    pubList->Vbatt, pubList->voc, pubList->vsat,
    pubList->sat, rp.curr_sel_noamp, rp.modeling,
    pubList->Ishunt,
    pubList->tcharge,
    pubList->soc_model, pubList->soc_ekf, pubList->soc, pubList->soc_wt,
    '\0');
  else if ( rp.debug==4 )
  sprintf(buffer, "%s, %s, %12.3f,%6.3f,   %d,  %d,  %d,  %4.1f,%5.2f,%7.3f,    %5.2f,%5.2f,%5.2f,%5.2f,  %9.6f, %5.3f,%5.3f,%5.3f,%5.3f,%c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time, pubList->T,
    pubList->sat, rp.curr_sel_noamp, rp.modeling,
    pubList->Tbatt, pubList->Vbatt, pubList->Ishunt,
    pubList->vsat, pubList->vdyn, pubList->voc, pubList->voc_ekf,
    pubList->y_ekf,
    pubList->soc_model, pubList->soc_ekf, pubList->soc, pubList->soc_wt,
    '\0');
}

// Convert time to decimal for easy lookup
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip)
{
    *current_time = Time.now();
    uint32_t year = Time.year(*current_time);
    uint8_t month = Time.month(*current_time);
    uint8_t day = Time.day(*current_time);
    uint8_t hours = Time.hour(*current_time);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(*current_time);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          *current_time = Time.now();
          day = Time.day(*current_time);
          hours = Time.hour(*current_time);
        }
    }
    uint8_t dayOfWeek = Time.weekday(*current_time)-1;  // 0-6
    uint8_t minutes   = Time.minute(*current_time);
    uint8_t seconds   = Time.second(*current_time);

    // Convert the string
    time_long_2_str(*current_time, tempStr);

    // Convert the decimal
    if ( rp.debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    static double cTimeInit = ((( (double(year-2021)*12 + double(month))*30.4375 + double(day))*24.0 + double(hours))*60.0 + double(minutes))*60.0 + \
                        double(seconds) + double(now-millis_flip)/1000.;
    double cTime = cTimeInit + double(now-millis_flip)/1000.;
    return ( cTime );
}

// Filter temperature only
void filter_temp(const int reset_loc, const double t_rlim, Sensors *Sen, const double t_bias, double *t_bias_last)
{
  // Rate limit the temperature bias
  if ( reset_loc ) *t_bias_last = t_bias;
  double t_bias_loc = max(min(t_bias, *t_bias_last + t_rlim*Sen->T_temp), *t_bias_last - t_rlim*Sen->T_temp);
  *t_bias_last = t_bias_loc;

  // Filter and add rate limited bias
  if ( reset_loc && Sen->Tbatt>40. )
  {
    Sen->Tbatt = RATED_TEMP + t_bias_loc; // Cold startup T=85.5 C
    Sen->Tbatt_filt = Sen->TbattSenseFilt->calculate(RATED_TEMP, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP)) + t_bias_loc;
  }
  else
  {
    Sen->Tbatt_filt = Sen->TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP)) + t_bias_loc;
    Sen->Tbatt += t_bias_loc;
  }
}

// Load all others
void load(const boolean reset_free, const unsigned long now, Sensors *Sen, Pins *myPins)
{
  static unsigned long int past = now;
  double T = (now - past)/1e3;
  past = now;

  // Current bias.  Feeds into signal conversion, not to duty injection
  cp.curr_bias_noamp =  rp.curr_bias_noamp  + rp.curr_bias_all + rp.inj_soft_bias + rp.tweak_bias_noamp;
  cp.curr_bias_amp =    rp.curr_bias_amp    + rp.curr_bias_all + rp.inj_soft_bias + rp.tweak_bias_amp;

  // Read Sensors
  // ADS1015 conversion
  Sen->ShuntAmp->load();
  Sen->ShuntNoAmp->load();

  // Current signal selection, based on if there or not.
  // Over-ride 'permanent' with Talk(rp.curr_sel_noamp) = Talk('s')
  if ( !rp.curr_sel_noamp && !Sen->ShuntAmp->bare())
  {
    Sen->Vshunt = Sen->ShuntAmp->vshunt();
    Sen->Ishunt = Sen->ShuntAmp->ishunt_cal();
    Sen->shunt_v2a_s = Sen->ShuntAmp->v2a_s();
  }
  else if ( !Sen->ShuntNoAmp->bare() )
  {
    Sen->Vshunt = Sen->ShuntNoAmp->vshunt();
    Sen->Ishunt = Sen->ShuntNoAmp->ishunt_cal();
    Sen->shunt_v2a_s = Sen->ShuntNoAmp->v2a_s();
  }
  else
  {
    Sen->Vshunt = 0.;
    Sen->Ishunt = 0.;
    Sen->shunt_v2a_s = Sen->ShuntNoAmp->v2a_s(); // noamp preferred, default to that
  }

  // Print results
  if ( rp.debug==14 ) Serial.printf("reset_free,select,duty,vs_int_a,vshunt_a,ishunt_cal_a,vs_int_na,vshunt_na,ishunt_cal_na,Ishunt,T=,    %d,%d,%ld,    %d,%7.3f,%7.3f,    %d,%7.3f,%7.3f,    %7.3f,%7.3f,\n",
    reset_free, rp.curr_sel_noamp, rp.duty,
    Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), Sen->ShuntAmp->ishunt_cal(),
    Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), Sen->ShuntNoAmp->ishunt_cal(),
    Sen->Ishunt, T);

  // Vbatt
  if ( rp.debug>102 ) Serial.printf("begin analogRead at %ld...", millis());
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  if ( rp.debug>102 ) Serial.printf("done at %ld\n", millis());
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A) + rp.vbatt_bias;
  if ( rp.modeling ) Sen->Vbatt = Sen->Vbatt_model;
  else Sen->Vbatt = vbatt_free;

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
}

// Load temperature only
void load_temp(Sensors *Sen)
{
  // Read Sensor
  // MAXIM conversion 1-wire Tp plenum temperature
  uint8_t count = 0;
  double temp = 0.;
  if ( !rp.modeling )
  {
    // Read hardware and check
    while ( ++count<MAX_TEMP_READS && temp==0)
    {
      if ( Sen->SensorTbatt->read() ) temp = Sen->SensorTbatt->celsius() + (TBATT_TEMPCAL);
      delay(1);
    }

    // Check success
    if ( count<MAX_TEMP_READS )
    {
      Sen->Tbatt = Sen->SdTbatt->update(temp);
      if ( rp.debug==-103 ) Serial.printf("Temperature %7.3f read on count=%d\n", temp, count);
    }
    else
    {
      Serial.printf("Did not read DS18 1-wire temperature sensor, using last-good-value.   Sometimes a hard reset will stop these\n");
      // Using last-good-value:  no assignment
    }
  }

  // Use model instead
  else
  {
    Sen->Tbatt = RATED_TEMP;
  }
}

// Manage wifi
void manage_wifi(unsigned long now, Wifi *wifi)
{
  if ( rp.debug >= 100 )
  {
    Serial.printf("P.connected=%i, disconnect check: %ld >=? %ld, turn on check: %ld >=? %ld, confirmation check: %ld >=? %ld, connected=%i, blynk_started=%i,\n",
      Particle.connected(), now-wifi->last_disconnect, DISCONNECT_DELAY, now-wifi->lastAttempt,  CHECK_INTERVAL, now-wifi->lastAttempt, CONFIRMATION_DELAY, wifi->connected, wifi->blynk_started);
  }
  wifi->particle_connected_now = Particle.connected();
  if ( wifi->particle_connected_last && !wifi->particle_connected_now )  // reset timer
  {
    wifi->last_disconnect = now;
  }
  if ( !wifi->particle_connected_now && now-wifi->last_disconnect>=DISCONNECT_DELAY )
  {
    wifi->last_disconnect = now;
    WiFi.off();
    wifi->connected = false;
    if ( rp.debug >= 100 ) Serial.printf("wifi turned off\n");
  }
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL && cp.enable_wifi )
  {
    wifi->last_disconnect = now;   // Give it a chance
    wifi->lastAttempt = now;
    WiFi.on();
    Particle.connect();
    if ( rp.debug >= 100 ) Serial.printf("wifi reattempted\n");
  }
  if ( now-wifi->lastAttempt>=CONFIRMATION_DELAY )
  {
    wifi->connected = Particle.connected();
    if ( rp.debug >= 100 ) Serial.printf("wifi disconnect check\n");
  }
  wifi->particle_connected_last = wifi->particle_connected_now;
}

// Calculate Ah remaining
// Inputs:  rp.mon_mod, Sen->Ishunt, Sen->Vbatt, Sen->Tbatt_filt
// States:  Mon.soc
// Outputs: tcharge_wt, tcharge_ekf
void  monitor(const int reset, const boolean reset_temp, const unsigned long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen)
{
  /* Main Battery Monitor
      Inputs:
        rp.mon_mod      Monitor battery chemistry type
        Sen->Ishunt     Battery terminal current, A
        Sen->Vbatt      Battery terminal voltage, V
        Sen->Tbatt_filt Tb filtered for noise, past value, deg C
        Mon.soc         State of charge
      Outputs:
        voc             Static model open circuit voltage, V
        voc_filt        Filtered open circuit voltage for saturation detect, V
        Mon.soc         State of charge
        Mon.soc_ekf     EKF state of charge
        Mon.soc_wt      Weighted selection of soc
        tcharge    	    Counted charging time to full, hr
        tcharge_ekf     Solved charging time to full from ekf, hr
        tcharge_wf  	    Counted charging time to full, hr
  */

  // Initialize charge state if temperature initial condition changed
  // Needed here in this location to have a value for Sen->Tbatt_filt
  if ( reset_temp )
  {
    Mon->apply_delta_q_t(rp.delta_q, rp.t_last);  // From memory
    Mon->init_battery(Sen);
    Mon->solve_ekf(Sen);
  }

  // EKF - calculates temp_c_, voc_, voc_dyn_ as functions of sensed parameters vb & ib (not soc)
  Mon->calculate(Sen);

  // Debounce saturation calculation done in ekf using voc model
  boolean sat = Mon->is_sat();
  Sen->saturated = Is_sat_delay->calculate(sat, T_SAT, T_DESAT, min(Sen->T, T_SAT/2.), reset);

  // Memory store
  Mon->count_coulombs(Sen->T, reset_temp, Sen->Tbatt_filt, Sen->Ishunt, Sen->saturated, rp.t_last);

  // Charge time for display
  Mon->calc_charge_time(Mon->q(), Mon->q_capacity(), Sen->Ishunt, Mon->soc());

  // Select between Coulomb Counter and EKF
  Mon->select();
}

// OLED display drive
void oled_display(Adafruit_SSD1306 *display, Sensors *Sen)
{
  static boolean pass = false;
  display->clearDisplay();

  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner

  boolean no_currents = Sen->ShuntAmp->bare() && Sen->ShuntNoAmp->bare();
  char dispString[21];
  if ( !pass && cp.model_cutback && rp.modeling )
    sprintf(dispString, "%3.0f %5.2f      ", pp.pubList.Tbatt, pp.pubList.voc);
  else
  {
    if (no_currents)
      sprintf(dispString, "%3.0f %5.2f fail", pp.pubList.Tbatt, pp.pubList.voc);
    else
      sprintf(dispString, "%3.0f %5.2f %5.1f", pp.pubList.Tbatt, pp.pubList.voc, pp.pubList.Ishunt);
  }
  display->println(dispString);

  display->println(F(""));

  display->setTextColor(SSD1306_WHITE);
  char dispStringT[9];
  if ( abs(pp.pubList.tcharge) < 24. )
    sprintf(dispStringT, "%3.0f%5.1f", pp.pubList.amp_hrs_remaining_ekf, pp.pubList.tcharge);
  else
    sprintf(dispStringT, "%3.0f --- ", pp.pubList.amp_hrs_remaining_ekf);
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text
  char dispStringS[4];
  if ( pass || !Sen->saturated )
    sprintf(dispStringS, "%3.0f", min(pp.pubList.amp_hrs_remaining_wt, 999.));
  else if (Sen->saturated)
    sprintf(dispStringS, "SAT");
  display->print(dispStringS);

  display->display();
  pass = !pass;

  if ( rp.debug==5 ) debug_5();
  if ( rp.debug==-5 ) debug_m5();  // Arduino plot
}

// Write to the D/A converter
uint32_t pwm_write(uint32_t duty, Pins *myPins)
{
    analogWrite(myPins->pwm_pin, duty, pwm_frequency);
    return duty;
}

// Read sensors, model signals, select between them
// Inputs:  rp.config, rp.sim_mod
// Outputs: Sen->Ishunt, Sen->Vbatt, Sen->Tbatt_filt, rp.duty
void sense_synth_select(const int reset, const boolean reset_temp, const unsigned long now, const unsigned long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen)
{
  // Load Ib and Vb
  // Outputs: Sen->Ishunt, Sen->Vbatt 
  load(reset, now, Sen, myPins);

  // Arduino plots
  if ( rp.debug==-7 ) debug_m7(Mon, Sen);

  /* Sim used for built-in testing (rp.modeling = true and jumper wire).   Needed here in this location
  to have available a value for Sen->Tbatt_filt when called.   Recalculates Sen->Ishunt accounting for
  saturation.  Sen->Ishunt is a feedback.
      Inputs:
        rp.sim_mod        Simulation battery chemistry type
        Sim.soc           Model state of charge, Coulombs
        Sen->Ishunt       Battery terminal current, A
        Sen->Tbatt        Battery temperature, deg C
        Sen->Tbatt_filt   Tb filtered for noise, past value, deg C
      Outputs:
        Sim.soc           Model state of charge, Coulombs
        Sim.temp_c_       Simulated Tb, deg C
        Sen->Tbatt_filt   = Sim.temp_c override
        Sim.ib_           Simulated over-ridden by saturation, A
        Sen->Ishunt       = Sim.ib_ override
        Sen->Vbatt_model  = Sim.vb_.  Battery terminal voltage, V
        Sen->Vbatt        = Sen->Vbatt_model override
        rp.duty           (0-255) for DF2 hardware injection when rp.modeling and proper wire connections made
  */

  // Sim initialize as needed from memory
  if ( reset )
  {
    Sen->Sim->apply_delta_q_t(rp.delta_q_model, rp.t_last_model);
    Sen->Sim->init_battery(Sen);
  }

  // Sim calculation
  Sen->Vbatt_model = Sen->Sim->calculate(Sen, cp.dc_dc_on);
  cp.model_cutback = Sen->Sim->cutback();
  cp.model_saturated = Sen->Sim->saturated();

  // Use model instead of sensors when running tests as user
  // Over-ride sensed Ib, Vb and Tb with model when running tests
  if ( rp.modeling )    // Should never be set in real use
  {
    Sen->Ishunt = Sen->Sim->Ib();
    Sen->Vbatt = Sen->Vbatt_model;
    Sen->Tbatt_filt = Sen->Sim->temp_c();
  }

  // Charge calculation and memory store
  // Inputs: Sen->Tbatt, Sen->Ishunt, and Sim.soc
  // Outputs: Sim.soc
  Sen->Sim->count_coulombs(Sen, reset_temp, rp.t_last_model);

  // D2 signal injection to hardware current sensors (also has rp.inj_soft_bias path for rp.tweak_test)
  rp.duty = Sen->Sim->calc_inj_duty(elapsed, rp.type, rp.amp, rp.freq);
}


/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      cp.string_complete = true;
     // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace("\0","");
      cp.input_string.replace(";","");
      cp.input_string.replace(",","");
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      break;  // enable reading multiple inputs
    }
  }
}

// Copy for bluetooth connected to TX/RX
/*
void serialEvent1()
{
  while (Serial1.available())
  {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      cp.string_complete = true;
     // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      Serial1.println(cp.input_string);
    }
  }
}
*/

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(cp.buffer, &pp.pubList);
  if ( rp.debug >= 100 ) Serial.printf("serial_print:  ");
  Serial.println(cp.buffer);
}

// Time synchro for web information
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip)
{
  if (now - *last_sync > ONE_DAY_MILLIS) 
  {
    *last_sync = millis();

    // Request time synchronization from the Particle Cloud
    if ( Particle.connected() ) Particle.syncTime();

    // Refresh millis() at turn of Time.now
    long time_begin = Time.now();
    while ( Time.now()==time_begin )
    {
      delay(1);
      *millis_flip = millis()%1000;
    }
  }
}

// For summary prints
String time_long_2_str(const unsigned long current_time, char *tempStr)
{
    uint32_t year = Time.year(current_time);
    uint8_t month = Time.month(current_time);
    uint8_t day = Time.day(current_time);
    uint8_t hours = Time.hour(current_time);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(current_time);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          day = Time.day(current_time);
          hours = Time.hour(current_time);
        }
    }
        uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
        if ( rp.debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    return ( String(tempStr) );
}

// Returns any text found between a start and end string inside 'str'
// example: startfooend  -> returns foo
String tryExtractString(String str, const char* start, const char* end)
{
  if (str=="")
  {
    return "";
  }
  int idx = str.indexOf(start);
  if (idx < 0)
  {
    return "";
  }
  int endIdx = str.indexOf(end);
  if (endIdx < 0)
  {
    return "";
  }
  return str.substring(idx + strlen(start), endIdx);
}

// Tweak
void tweak_on_new_desat(Sensors *Sen, unsigned long int now)
{

  if ( Sen->ShuntAmp->new_desat(Sen->ShuntAmp->ishunt_cal(), Sen->T, Sen->saturated, now) )
    Sen->ShuntAmp->adjust(now);

  if ( Sen->ShuntNoAmp->new_desat(Sen->ShuntNoAmp->ishunt_cal(), Sen->T, Sen->saturated, now) )
    Sen->ShuntNoAmp->adjust(now);

}

