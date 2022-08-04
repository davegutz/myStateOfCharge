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

// Text headers
void print_serial_header(void)
{
  if ( rp.debug==4 || rp.debug==24 || rp.debug==26 )
  {
    Serial.printf("unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,dV_dyn,Voc_stat,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,,\n");
    if ( !cp.blynking )
      Serial1.printf("unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,dV_dyn,Voc_stat,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,\n");
  }
}
void print_serial_sim_header(void)
{
  if ( rp.debug==24 || rp.debug==26) // print_serial_sim_header
    Serial.printf("unit_m,  c_time,       Tb_m,Tbl_m,  vsat_m, voc_stat_m, dv_dyn_m, vb_m, ib_m, ib_in_m, sat_m, ddq_m, dq_m, q_m, qcap_m, soc_m, reset_m,\n");
}
void print_signal_sel_header(void)
{
  if ( rp.debug==26 ) // print_signal_sel_header
    Serial.printf("unit_s,c_time,res,user_sel,   m_bare,n_bare,  cc_dif,cc_flt,  ibmh,ibnh,ibmm,ibnm,ibm,                     ib_dif,ib_dif_flt,ib_dif_fa,  ib_sel,Ib_h,Ib_m,mib,Ib_s,          Vb_h,Vb_m,mvb,Vb_s,                Tb_h,Tb_s,mtb,Tb_f,\n");
          // -----, cTime, reset, rp.ibatt_select,
          //                                    ShuntAmp->bare(), ShuntNoAmp->bare(),
          //                                                        cc_diff_, cc_flt_,
          //                                                                       Ibatt_amp_hdwe, Ibatt_noamp_hdwe, Ibatt_amp_model, Ibatt_noamp_model, Ibatt_model,
          //                                                                                                                   ib_diff_, ib_diff_flt_, ib_diff_fa_,
          //                                                                                                                                                   ib_sel_stat_, Ibatt_hdwe, Ibatt_hdwe_model, mod_ib(), Ibatt,
          //                                                                                                                                                                                      Vbatt_hdwe, Vbatt_model,mod_vb(), Vbatt,
          //                                                                                                                                                                                                                        Tbatt_hdwe, Tbatt, mod_tb(), Tbatt_filt,
}

// Print strings
void create_print_string(Publish *pubList)
{
  if ( rp.debug==4 || rp.debug==24 || rp.debug==26 )
    sprintf(cp.buffer, "%s, %s, %13.3f,%6.3f,   %d,  %d,  %d,  %5.2f,%7.5f,%7.5f,    %7.5f,%7.5f,%7.5f,%7.5f,  %9.6f, %7.5f,%7.5f,%7.5f,%c", \
      pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time, pubList->T,
      pubList->sat, rp.ibatt_select, rp.modeling,
      pubList->Tbatt, pubList->Vbatt, pubList->Ibatt,
      pubList->Vsat, pubList->dV_dyn, pubList->Voc_stat, pubList->Voc_ekf,
      pubList->y_ekf,
      pubList->soc_model, pubList->soc_ekf, pubList->soc,
      '\0');
}
void create_tweak_string(Publish *pubList, Sensors *Sen, BatteryMonitor *Mon)
{
  if ( rp.debug==4 || rp.debug || rp.debug==26 )
  {
    sprintf(cp.buffer, "%s, %s, %13.3f,%6.3f,   %d,  %d,  %d,  %4.1f,%6.3f,%10.3f,    %7.5f,%7.5f,%7.5f,%7.5f,  %9.6f, %7.5f,%7.5f,%7.5f,%c", \
      pubList->unit.c_str(), pubList->hm_string.c_str(), double(Sen->now)/1000., Sen->T,
      pubList->sat, rp.ibatt_select, rp.modeling,
      Mon->Tb(), Mon->Vb(), Mon->Ib(),
      Mon->Vsat(), Mon->dV_dyn(), Mon->Voc_stat(), Mon->Hx(),
      Mon->y_ekf(),
      Sen->Sim->soc(), Mon->soc_ekf(), Mon->soc(),
      '\0');
  }
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

// Load all others
// Outputs:   Sen->Ibatt_model_in, Sen->Ibatt_hdwe, 
void load_ibatt_vbatt(const boolean reset, const unsigned long now, Sensors *Sen, Pins *myPins, BatteryMonitor *Mon)
{
  // Load shunts
  // Outputs:  Sen->Ibatt_model_in, Sen->Ibatt_hdwe, Sen->Vbatt, Sen->Wbatt
  Sen->now = now;
  Sen->shunt_bias();
  Sen->shunt_load();
  Sen->shunt_check(Mon);
  Sen->shunt_select_initial();
  if ( rp.debug==14 ) Sen->shunt_print();

  // Vbatt
  // Outputs:  Sen->Vbatt
  Sen->vbatt_load(myPins->Vbatt_pin);
  Sen->vbatt_check(Mon, VBATT_MIN, VBATT_MAX);
  if ( rp.debug==15 ) Sen->vbatt_print();

  // Power calculation
  Sen->Wbatt = Sen->Vbatt*Sen->Ibatt;
}

// Manage wifi
void manage_wifi(unsigned long now, Wifi *wifi)
{
  if ( rp.debug >= 100 )
  {
    Serial.printf("P.cn=%i, dscn chk: %ld >=? %ld, on chk: %ld >=? %ld, conf chk: %ld >=? %ld, cn=%i, bly_strt=%i,\n",
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
    if ( rp.debug >= 100 ) Serial.printf("wifi off\n");
  }
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL && cp.enable_wifi )
  {
    wifi->last_disconnect = now;   // Give it a chance
    wifi->lastAttempt = now;
    WiFi.on();
    Particle.connect();
    if ( rp.debug >= 100 ) Serial.printf("wifi retry\n");
  }
  if ( now-wifi->lastAttempt>=CONFIRMATION_DELAY )
  {
    wifi->connected = Particle.connected();
    if ( rp.debug >= 100 ) Serial.printf("wifi dsc chk\n");
  }
  wifi->particle_connected_last = wifi->particle_connected_now;
}

// Calculate Ah remaining
// Inputs:  rp.mon_mod, Sen->Ibatt, Sen->Vbatt, Sen->Tbatt_filt
// States:  Mon.soc, Mon.soc_ekf
// Outputs: tcharge_wt, tcharge_ekf, Voc, Voc_filt
void  monitor(const boolean reset, const boolean reset_temp, const unsigned long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen)
{

  // Initialize charge state if temperature initial condition changed
  // Needed here in this location to have a value for Sen->Tbatt_filt
  Mon->apply_delta_q_t(reset_temp);  // From memory
  Mon->init_battery(reset_temp, Sen);
  Mon->solve_ekf(reset_temp, Sen);

  // EKF - calculates temp_c_, voc_stat_, voc_ as functions of sensed parameters vb & ib (not soc)
  Mon->calculate(Sen, reset_temp);

  // Debounce saturation calculation done in ekf using voc model
  boolean sat = Mon->is_sat();
  Sen->saturated = Is_sat_delay->calculate(sat, T_SAT, T_DESAT, min(Sen->T, T_SAT/2.), reset);

  // Memory store // TODO:  simplify arg list here.  Unpack Sen inside count_coulombs
  // Initialize to ekf when not saturated
  Mon->count_coulombs(Sen->T, reset_temp, Sen->Tbatt_filt, Sen->Ibatt, Sen->saturated, Sen->sclr_coul_eff,
    Mon->delta_q_ekf());

  // Charge time for display
  Mon->calc_charge_time(Mon->q(), Mon->q_capacity(), Sen->Ibatt, Mon->soc());
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
    sprintf(dispString, "%3.0f %5.2f      ", pp.pubList.Tbatt, pp.pubList.Voc);
  else
  {
    if (no_currents)
      sprintf(dispString, "%3.0f %5.2f fail", pp.pubList.Tbatt, pp.pubList.Voc);
    else
      sprintf(dispString, "%3.0f %5.2f %5.1f", pp.pubList.Tbatt, pp.pubList.Voc, pp.pubList.Ibatt);
  }
  display->println(dispString);

  display->println(F(""));
  display->setTextColor(SSD1306_WHITE);

  char dispStringT[9];
  if ( abs(pp.pubList.tcharge) < 24. )
    sprintf(dispStringT, "%3.0f%5.1f", pp.pubList.Amp_hrs_remaining_ekf, pp.pubList.tcharge);
  else
    sprintf(dispStringT, "%3.0f --- ", pp.pubList.Amp_hrs_remaining_ekf);
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text

  char dispStringS[4];
  if ( pass || !Sen->saturated )
    sprintf(dispStringS, "%3.0f", min(pp.pubList.Amp_hrs_remaining_soc, 999.));
  else if (Sen->saturated)
    sprintf(dispStringS, "SAT");
  display->print(dispStringS);

  display->display();
  pass = !pass;

  // Text basic Bluetooth (uses serial bluetooth app)
  if ( rp.debug!=4 && !cp.blynking )
    Serial1.printf("%s   Tb,C  VOC,V  Ib,A \n%s    %s EKF,Ah  chg,hrs  CC, Ah\n\n\n", dispString, dispStringT, dispStringS);

  if ( rp.debug==5 ) debug_5();
  if ( rp.debug==-5 ) debug_m5();  // Arduino plot
}

// Read sensors, model signals, select between them.
// Sim used for built-in testing (rp.modeling = 7 and jumper wire).
//    Needed here in this location to have available a value for
//    Sen->Tbatt_filt when called.   Recalculates Sen->Ibatt accounting for
//    saturation.  Sen->Ibatt is a feedback (used-before-calculated).
// Inputs:  rp.config, rp.sim_mod, Sen->Tbatt, Sen->Ibatt_model_in
// States:  Sim.soc
// Outputs: Sim.temp_c_, Sen->Tbatt_filt, Sen->Ibatt, Sen->Ibatt_model,
//   Sen->Vbatt_model, Sen->Tbatt_filt, rp.inj_bias
void sense_synth_select(const boolean reset, const boolean reset_temp, const unsigned long now, const unsigned long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen)
{
  // Load Ib and Vb
  // Outputs: Sen->Ibatt_model_in, Sen->Ibatt, Sen->Vbatt 
  load_ibatt_vbatt(reset, now, Sen, myPins, Mon);

  // Arduino plots
  if ( rp.debug==-7 ) debug_m7(Mon, Sen);

  // Sim initialize as needed from memory
  Sen->Sim->apply_delta_q_t(reset);
  Sen->Sim->init_battery(reset, Sen);

  // Sim calculation
  //  Inputs:  Sen->Tbatt_filt(past), Sen->Ibatt_model_in
  //  States: Sim->soc(past)
  //  Outputs:  Sim->temp_c(), Sim->Ib(), Sim->Vb(), rp.inj_bias, Sim.model_saturated
  Sen->Tbatt_model = Sen->Tbatt_model_filt = Sen->Sim->temp_c() + Sen->Tbatt_noise();
  Sen->Vbatt_model = Sen->Sim->calculate(Sen, cp.dc_dc_on, reset) + Sen->Vbatt_noise();
  Sen->Ibatt_model = Sen->Sim->Ib() + Sen->Ibatt_noise();
  cp.model_cutback = Sen->Sim->cutback();
  cp.model_saturated = Sen->Sim->saturated();

  // Inputs:  Sim->Ib
  Sen->bias_all_model();   // Bias model outputs for sensor fault injection

  // Use model instead of sensors when running tests as user
  //  Inputs:                                             --->   Outputs:
  // TODO:  control parameter list here...Vbatt_fail, soc, soc_ekf,
  //  Ibatt_model, Ibatt_hdwe,                            --->   Ibatt
  //  Vbatt_model, Vbatt_hdwe,                            --->   Vbatt
  //  constant,         Tbatt_hdwe, Tbatt_hdwe_filt       --->   Tbatt, Tbatt_filt
  Sen->select_all(Mon, reset);

  // Charge calculation and memory store
  // Inputs: Sim.model_saturated, Sen->Tbatt, Sen->Ibatt, and Sim.soc
  // Outputs: Sim.soc
  Sen->Sim->count_coulombs(Sen, reset_temp, Mon);

  // Injection tweak test
  if ( (Sen->start_inj <= Sen->now) && (Sen->now <= Sen->end_inj) ) // in range, test in progress
  {
    // Shift times because sampling is asynchronous: improve repeatibility
    if ( Sen->elapsed_inj==0 )
    {
      Sen->end_inj += Sen->now - Sen->start_inj;
      Sen->stop_inj += Sen->now - Sen->start_inj;
      Sen->start_inj = Sen->now;
    }

    Sen->elapsed_inj = Sen->now - Sen->start_inj + 1UL; // Shift by 1 because using ==0 to turn off

    // Turn off amplitude of injection and wait for end_inj
    if (Sen->now > Sen->stop_inj) rp.amp = 0;
  }
  else if ( Sen->elapsed_inj && rp.tweak_test() )  // Done.  Turn things off by setting 0
  {
    Sen->elapsed_inj = 0;
    chit("v0;", ASAP);  // Turn off echo
    chit("Pa;", QUEUE);  // Print all for record
    chit("Xm7;", QUEUE); // Turn off tweak_test
  }
  // Perform the calculation of injection signals 
  rp.inj_bias = Sen->Sim->calc_inj(Sen->elapsed_inj, rp.type, rp.amp, rp.freq);
}

/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.

  Particle documentation says not to use something like
  the cp.string_complete in the while loop statement.
  They suggest handling all the data in one call.   But 
  this works, so far.
 */
void serialEvent()
{
  while ( !cp.string_complete && Serial.available() )
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace("\0","");
      cp.input_string.replace(";","");
      cp.input_string.replace(",","");
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      cp.string_complete = true;  // Temporarily inhibits while loop until talk() call resets string_complete
      break;  // enable reading multiple inputs
    }
  }
}

/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent1()
{
  if ( cp.blynking ) return;
  while (!cp.string_complete && Serial1.available())
  {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
     // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace("\0","");
      cp.input_string.replace(";","");
      cp.input_string.replace(",","");
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      cp.string_complete = true;
      break;  // enable reading multiple inputs
    }
  }
}

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(&pp.pubList);
  if ( rp.debug >= 100 ) Serial.printf("serial_print:");
  Serial.println(cp.buffer);
  if ( !cp.blynking )
    Serial1.println(cp.buffer);
}
void tweak_print(Sensors *Sen, BatteryMonitor *Mon)
{
  create_tweak_string(&pp.pubList, Sen, Mon);
  if ( rp.debug >= 100 ) Serial.printf("tweak_print:");
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

