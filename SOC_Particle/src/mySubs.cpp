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

#include "mySubs.h"
#include "command.h"
#include "local_config.h"
#include <math.h>
#include "debug.h"
#include "mySummary.h"
#include "Variable.h"

extern CommandPars cp;  // Various parameters shared at system level
extern PublishPars pp;  // For publishing
extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle

// Print consolidation
void print_all_header(void)
{
  print_serial_header();
  if ( sp.Debug()==2  )
  {
    print_serial_sim_header();
    print_signal_sel_header();
  }
  if ( sp.Debug()==3  )
  {
    print_serial_sim_header();
    print_serial_ekf_header();
  }
  if ( sp.Debug()==4  )
  {
    print_serial_sim_header();
    print_signal_sel_header();
    print_serial_ekf_header();
  }
}
void print_rapid_data(const boolean reset, Sensors *Sen, BatteryMonitor *Mon)
{
  static uint8_t last_read_debug = 0;     // Remember first time with new debug to print headers
  if ( ( sp.Debug()==1 || sp.Debug()==2 || sp.Debug()==3 || sp.Debug()==4 ) )
  {
    if ( reset || (last_read_debug != sp.Debug()) )
    {
      cp.num_v_print = 0UL;
      print_all_header();
    }
    if ( sp.tweak_test() )
    {
      // no print, done by sub-functions
      cp.num_v_print++;
    }
    if ( cp.publishS )
    {
      rapid_print(Sen, Mon);
      cp.num_v_print++;
    }
  }
  last_read_debug = sp.Debug();
}
void print_serial_header(void)
{
  if ( ( sp.Debug()==1 || sp.Debug()==2 || sp.Debug()==3 || sp.Debug()==4 ) )
  {
    Serial.printf ("unit,               hm,                  cTime,       dt,       chm,qcrs,sat,sel,mod,bmso, Tb,  vb,  ib,   ib_charge, ioc, voc_soc,    vsat,dv_dyn,voc_stat,voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,soc_min, Tbl,\n");
    #ifndef CONFIG_PHOTON
      Serial1.printf("unit,               hm,                  cTime,       dt,       chm,qcrs,sat,sel,mod,bmso, Tb,  vb,  ib,   ib_charge, ioc, voc_soc,    vsat,dv_dyn,voc_stat,voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,soc_min, Tbl,\n");
    #endif
  }
}
void print_serial_sim_header(void)
{
  if ( sp.Debug()==2  || sp.Debug()==3 || sp.Debug()==4 ) // print_serial_sim_header
    Serial.printf("unit_m,  c_time,       chm_s, qcrs_s, bmso_s, Tb_s,Tbl_s,  vsat_s, voc_stat_s, dv_dyn_s, vb_s, ib_s, ib_in_s, ib_charge_s, ioc_s, sat_s, dq_s, soc_s, reset_s,\n");
}
void print_signal_sel_header(void)
{
  if ( sp.Debug()==2 || sp.Debug()==4 ) // print_signal_sel_header
    Serial.printf("unit_s,c_time,res,user_sel,   cc_dif,  ibmh,ibnh,ibmm,ibnm,ibm,   ib_diff, ib_diff_f,");
    Serial.printf("    voc_soc,e_w,e_w_f,  ib_sel_stat,ib_h,ib_s,mib,ib, vb_sel,vb_h,vb_s,mvb,vb,  Tb_h,Tb_s,mtb,Tb_f, ");
    Serial.printf("  fltw, falw, ib_rate, ib_quiet, tb_sel, ccd_thr, ewh_thr, ewl_thr, ibd_thr, ibq_thr, preserving,\n");
          // -----, cTime, reset, sp.ib_select,
          //                                     cc_diff_,
          //                                              ib_amp_hdwe, ib_noa_hdwe, ib_amp_model, ib_noa_model, ib_model,
          //                                                                           ib_diff_, ib_diff_f,
          //         voc_soc, e_wrap_, e_wrap_filt_, ib_sel_stat_, ib_hdwe, ib_hdwe_model, mod_ib(), ib,
          //                                                          vb_sel_stat, vb_hdwe, vb_model,mod_vb(), Vb,
          //                                                                                    Tb_hdwe, Tb, mod_tb(), Tb_filt,
          //         fltw_, falw_, ib_rate_, ib_quiet_, tb_sel_stat_, cc_diff_thr_, ewhi_thr_, ewlo_thr_, ib_diff_thr_, ib_quiet_thr_,
}
void print_serial_ekf_header(void)
{
  if ( sp.Debug()==3 || sp.Debug()==4 ) // print_serial_ekf_header
    Serial.printf("unit_e,c_time,dt,Fx_, Bu_, Q_, R_, P_, S_, K_, u_, x_, y_, z_, x_prior_, P_prior_, x_post_, P_post_, hx_, H_,\n");
}

// Print strings
void create_rapid_string(Publish *pubList, Sensors *Sen, BatteryMonitor *Mon)
{
  double cTime;
  if ( sp.tweak_test() ) cTime = double(Sen->now)/1000.;
  else cTime = Sen->control_time;

  sprintf(cp.buffer, "%s, %s, %13.3f,%6.3f,   %d, %7.0f, %d,  %d,  %d,  %d, %6.3f,%6.3f,%10.3f,%10.3f,%10.3f,%7.5f,    %7.5f,%7.5f,%7.5f,%7.5f,  %9.6f, %7.5f,%7.5f,%7.5f,%7.5f,%6.3f,", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), cTime, Sen->T,
    sp.mon_chm(), Mon->q_cap_rated_scaled(), pubList->sat, sp.Ib_select(), sp.Modeling(), Mon->bms_off(),
    Mon->Tb(), Mon->vb(), Mon->ib(), Mon->ib_charge(), Mon->ioc(), Mon->voc_soc(), 
    Mon->vsat(), Mon->dv_dyn(), Mon->voc_stat(), Mon->hx(),
    Mon->y_ekf(),
    Sen->Sim->soc(), Mon->soc_ekf(), Mon->soc(), Mon->soc_min(), sp.t_last());
}

// Convert time to decimal for easy lookup
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip)
{
  *current_time = Time.now();  // Seconds since start of epoch
  uint32_t year = Time.year(*current_time);
  uint8_t month = Time.month(*current_time);
  uint8_t day = Time.day(*current_time);
  uint8_t hours = Time.hour(*current_time);

  // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
  if ( USE_DST )
  {
    uint8_t dayOfWeek = Time.weekday(*current_time);     // 1-7
    if (  month>2   && month<12 &&
      !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
      !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
      {
        Time.zone(GMT+1);
        *current_time = Time.now();  // Seconds since start of epoch
        day = Time.day(*current_time);
        hours = Time.hour(*current_time);
      }
  }
  // uint8_t dayOfWeek = Time.weekday(*current_time)-1;  // 0-6
  uint8_t minutes   = Time.minute(*current_time);
  uint8_t seconds   = Time.second(*current_time);

  // Convert the string
  time_long_2_str(*current_time, tempStr);

  // Convert the decimal
  static double cTimeInit = ((( (double(year-2021)*12 + double(month))*30.4375 + double(day))*24.0 + double(hours))*60.0 + double(minutes))*60.0 + \
                      double(seconds) + double(now-millis_flip)/1000.;
  // Ignore Time.now if corrupt
  if ( year<2020 ) cTimeInit = 0.;
  // Serial.printf("y %ld m %d d %d h %d m %d s %d now %ld millis_flip %ld\n", year, month, day, hours, minutes, seconds, now, millis_flip);
  double cTime = cTimeInit + double(now-millis_flip)/1000.;
  // Serial.printf("%ld - %ld %18.12g cTimeInit%18.12g cTime%18.12g\n", now, millis_flip, double(now-millis_flip)/1000., cTimeInit, cTime);
  return ( cTime );
}

// Non-blocking delay
void delay_no_block(const unsigned long int interval)
{
  unsigned long int previousMillis = millis();
  unsigned long currentMillis = previousMillis;
  while( currentMillis - previousMillis < interval )
  {
    currentMillis = millis();
  }
}

// Harvest charge caused temperature change.   More charge becomes available as battery warms
void harvest_temp_change(const float temp_c, BatteryMonitor *Mon, BatterySim *Sim)
{
  sp.put_Delta_q(sp.Delta_q() - Mon->dqdt() * Mon->q_capacity() * (temp_c - sp.t_last()));
  sp.put_t_last(temp_c);
  sp.put_delta_q_model(sp.delta_q_model() - Sim->dqdt() * Sim->q_capacity() * (temp_c - sp.t_last_model()));
  sp.put_t_last_model(temp_c);
}

// Complete initialization of all parameters in Mon and Sim including EKF
// Force current to be zero because initial condition undefined otherwise with charge integration
void initialize_all(BatteryMonitor *Mon, Sensors *Sen, const float soc_in, const boolean use_soc_in)
{
  // Sample debug statements
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 )
    {
      Serial.printf("\n\n");
      sp.pretty_print(true);
      Serial.printf("falw %d tb_fa %d\n", Sen->Flt->falw(), Sen->Flt->tb_fa());
      // Sen->Flt->disab_tb_fa(1);
      // Serial.printf("S/M.a_d_q_t:"); debug_m1(Mon, Sen);
      // Serial.printf("af cal: Tb_f=%5.2f Vb=%7.3f Ib=%7.3f :", Sen->Tb_filt, Sen->Vb, Sen->Ib); Serial.printf("Top:"); debug_m1(Mon, Sen);
    }
  #endif

  // Gather and apply inputs
  if ( sp.mod_ib() )
    Sen->Ib_model_in = sp.inj_bias() + sp.Ib_bias_all();
  else
    Sen->Ib_model_in = Sen->Ib_hdwe;
  Sen->temp_load_and_filter(Sen, true);
  if ( sp.mod_tb() )
  {
    Sen->Tb = Sen->Tb_model;
    Sen->Tb_filt = Sen->Tb_model_filt;
  }
  else
  {
    Sen->Tb = Sen->Tb_hdwe;
    Sen->Tb_filt = Sen->Tb_hdwe_filt;
  }
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 )
    { 
      Serial.printf("before harvest_temp, falw %d tb_fa %d:", Sen->Flt->falw(), Sen->Flt->tb_fa()); debug_m1(Mon, Sen);
    }
  #endif
  if ( !Sen->Flt->tb_fa() ) harvest_temp_change(Sen->Tb_filt, Mon, Sen->Sim);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("after harvest_temp:"); debug_m1(Mon, Sen);}
  #endif
  if ( use_soc_in )
    Mon->apply_soc(soc_in, Sen->Tb_filt);  // saves sp.delta_q and sp.t_last
  Sen->Sim->apply_delta_q_t(Mon->delta_q(), Mon->t_last());  // applies sp.delta_q and sp.t_last
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("S.a_d_q_t:"); debug_m1(Mon, Sen);}
  #endif

  // Make Sim accurate even if not used
  Sen->Sim->init_battery_sim(true, Sen);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("S.i_b:"); debug_m1(Mon, Sen);}
  #endif
  if ( !sp.mod_vb() )
  {
    Sen->Sim->apply_soc(Sen->Sim->soc(), Sen->Tb_filt);
  }
  // Call calculate twice because sat_ is a used-before-calculated (UBC)
  // Simple 'call twice' method because sat_ is discrete no analog which would require iteration
  Sen->Vb_model = Sen->Sim->calculate(Sen, cp.dc_dc_on, true) * sp.nS();
  Sen->Vb_model = Sen->Sim->calculate(Sen, cp.dc_dc_on, true) * sp.nS();  // Call again because sat is a UBC
  Sen->Ib_model = Sen->Sim->ib_fut() * sp.nP();

  // Call to count_coulombs not strictly needed for init.  Calculates some things not otherwise calculated for 'all'
  Sen->Sim->count_coulombs(Sen, true, Mon, true);

  // Signal preparations
  if ( sp.mod_vb() )
  {
    Sen->Vb = Sen->Vb_model;
  }
  else
  {
    Sen->Vb = Sen->Vb_hdwe;
  }
  if ( sp.mod_ib() )
  {
    Sen->Ib = Sen->Ib_model;
  }
  else
  {
    Sen->Ib = Sen->Ib_hdwe;
  }
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("SENIB:"); debug_m1(Mon, Sen);}
  #endif
  if ( sp.mod_vb() )
  {
    Mon->apply_soc(Sen->Sim->soc(), Sen->Tb_filt);
  }
  Mon->init_battery_mon(true, Sen);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ) { Serial.printf("M.i_b:"); debug_m1(Mon, Sen);}
  #endif

  // Call calculate/count_coulombs twice because sat_ is a used-before-calculated (UBC)
  // Simple 'call twice' method because sat_ is discrete not analog which would require iteration
  Mon->calculate(Sen, true);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("M.calc1:"); debug_m1(Mon, Sen);}
  #endif
  Mon->count_coulombs(0., true, Mon->t_last(), 0., Mon->is_sat(true), 0.);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("M.c_c1:"); debug_m1(Mon, Sen);}
  #endif
  Mon->calculate(Sen, true);  // Call again because sat is a UBC
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("M.calc2:"); debug_m1(Mon, Sen);}
  #endif
  Mon->count_coulombs(0., true, Mon->t_last(), 0., Mon->is_sat(true), 0.);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("M.c_c2:"); debug_m1(Mon, Sen);}
  #endif
  
  // Solve EKF
  Mon->solve_ekf(true, true, Sen);
  #ifdef DEBUG_INIT
    if ( sp.Debug()==-1 ){ Serial.printf("end:"); debug_m1(Mon, Sen);}
  #endif
}

// Load high fidelity signals; filtered in hardware the same bandwidth, sampled the same
// Outputs:   Sen->Ib_model_in, Sen->Ib_hdwe, 
void load_ib_vb(const boolean reset, const boolean reset_temp, Sensors *Sen, Pins *myPins, BatteryMonitor *Mon)
{
  // Load shunts Ib
  // Outputs:  Sen->Ib_model_in, Sen->Ib_hdwe, Sen->Vb, Sen->Wb
  Sen->ShuntAmp->convert( sp.mod_ib_amp_dscn() );
  Sen->ShuntNoAmp->convert( sp.mod_ib_noa_dscn() );
  Sen->Flt->shunt_check(Sen, Mon, reset);
  Sen->shunt_select_initial(reset);
  if ( sp.Debug()==14 ) Sen->shunt_print();

  // Load voltage Vb
  // Outputs:  Sen->Vb
  Sen->vb_load(myPins->Vb_pin, reset);
  if ( !sp.mod_vb_dscn() )  Sen->Flt->vb_check(Sen, Mon, VB_MIN, VB_MAX, reset);
  else                      Sen->Flt->vb_check(Sen, Mon, -1.0, 1.0, reset);
  if ( sp.Debug()==15 ) Sen->vb_print();

  // Power calculation
  Sen->Wb = Sen->Vb*Sen->Ib;
}

// Calculate Ah remaining for display to user
// Inputs:  sp.mon_chm, Sen->Ib, Sen->Vb, Sen->Tb_filt
// States:  Mon.soc, Mon.soc_ekf
// Outputs: tcharge_wt, tcharge_ekf, Voc, Voc_filt
void  monitor(const boolean reset, const boolean reset_temp, const unsigned long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen)
{
  // EKF - calculates temp_c_, voc_stat_, voc_ as functions of sensed parameters vb & ib (not soc)
  Mon->calculate(Sen, reset_temp);

  // Debounce saturation calculation done in ekf using voc model
  boolean sat = Mon->is_sat(reset);
  Sen->saturated = Is_sat_delay->calculate(sat, T_SAT*cp.s_t_sat, T_DESAT*cp.s_t_sat, min(Sen->T, T_SAT/2.), reset);

  // Memory store
  // Initialize to ekf when not saturated
  Mon->count_coulombs(Sen->T, reset_temp, Sen->Tb_filt, Mon->ib_charge(), Sen->saturated, Mon->delta_q_ekf());

  // Charge charge time for display
  Mon->calc_charge_time(Mon->q(), Mon->q_capacity(), Sen->ib(), Mon->soc());
}

/* OLED display drive
 e.g.:
   35  13.71 -4.2    Tb,C  VOC,V  Ib,A 
   45  -10.0  46     EKF,Ah  chg,hrs  CC, Ah
*/
void oled_display(Adafruit_SSD1306 *display, Sensors *Sen, BatteryMonitor *Mon)
{
  static uint8_t blink = 0;
  String disp_0, disp_1, disp_2;

  #ifndef CONFIG_BARE
    display->clearDisplay();
  #endif
  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  #ifdef CONFIG_DISP_SKIP
    display->setCursor(0, CONFIG_DISP_SKIP);              // Start at top-left corner
  #else
    display->setCursor(0, 0);              // Start at top-left corner
  #endif

  // ---------- Top Line of Display -------------------------------------------
  // Tb
  sprintf(cp.buffer, "%3.0f", pp.pubList.Tb);
  disp_0 = cp.buffer;
  if ( Sen->Flt->tb_fa() && (blink==0 || blink==1) )
    disp_0 = "***";

  // Voc
  sprintf(cp.buffer, "%5.2f", pp.pubList.Voc);
  disp_1 = cp.buffer;
  if ( Sen->Flt->vb_sel_stat()==0 && (blink==1 || blink==2) )
    disp_1 = "*fail";
  else if ( Sen->bms_off )
    disp_1 = " off ";

  // Ib
  sprintf(cp.buffer, "%6.1f", pp.pubList.Ib);
  disp_2 = cp.buffer;
  if ( blink==2 )
  {
    if ( Sen->ShuntAmp->bare_detected() && Sen->ShuntNoAmp->bare_detected() && !sp.mod_ib() )
      disp_2 = "*fail";
    else if ( Sen->Flt->dscn_fa() && !sp.mod_ib() )
      disp_2 = " conn ";
    else if ( Sen->Flt->ib_diff_fa() )
      disp_2 = " diff ";
    else if ( Sen->Flt->red_loss() )
      disp_2 = " redl ";
  }
  else if ( blink==3 )
  {
    if ( Sen->ShuntAmp->bare_detected() && Sen->ShuntNoAmp->bare_detected() && !sp.mod_ib() )
      disp_2 = "*fail";
    else if ( Sen->Flt->dscn_fa() && !sp.mod_ib() )
      disp_2 = " conn ";
  }
  String disp_Tbop = disp_0.substring(0, 4) + " " + disp_1.substring(0, 6) + " " + disp_2.substring(0, 7);
  display->println(disp_Tbop.c_str());
  display->println(F(""));
  display->setTextColor(SSD1306_WHITE);

  // --------------------- Bottom line of Display ------------------------------
  // Hrs EHK
  sprintf(cp.buffer, "%3.0f", pp.pubList.Amp_hrs_remaining_ekf);
  disp_0 = cp.buffer;
  if ( blink==0 || blink==1 || blink==2 )
  {
    if ( Sen->Flt->cc_diff_fa() )
      disp_0 = "---";
  }
  display->print(disp_0.c_str());

  // t charge
  if ( abs(pp.pubList.tcharge) < 24. )
  {
    sprintf(cp.buffer, "%5.1f", pp.pubList.tcharge);
  }
  else
  {
    sprintf(cp.buffer, " --- ");
  }  
  disp_1 = cp.buffer;
  display->print(disp_1.c_str());

  // Hrs large
  display->setTextSize(2);             // Draw 2X-scale text
  if ( blink==1 || blink==3 || !Sen->saturated )
  {
    sprintf(cp.buffer, "%3.0f", min(pp.pubList.Amp_hrs_remaining_soc, 999.));
    disp_2 = cp.buffer;
  }
  else if (Sen->saturated)
    disp_2 = "SAT";
  display->print(disp_2.c_str());
  String dispBot = disp_0 + disp_1 + " " + disp_2;

  // Display
  #ifndef CONFIG_BARE
    display->display();
  #endif

  // Text basic Bluetooth (use serial bluetooth app)
  if ( sp.Debug()==99 ) // Calibration mode
    debug_99(Mon, Sen);
  else if ( sp.Debug()!=4 && sp.Debug()!=-2 )  // Normal display
    Serial1.printf("%s   Tb,C  VOC,V  Ib,A \n%s   EKF,Ah  chg,hrs  CC, Ah\nPf; for fails.  prints=%ld\n\n",
      disp_Tbop.c_str(), dispBot.c_str(), cp.num_v_print);

  if ( sp.Debug()==5 ) debug_5(Mon, Sen);  // Charge time display on UART

  blink += 1;
  if (blink>3) blink = 0;
}

// Read sensors, model signals, select between them.
// Sim used for any missing signals (Tb, Vb, Ib)
//    Needed here in this location to have available a value for
//    Sen->Tb_filt when called.   Recalculates Sen->Ib accounting for
//    saturation.  Sen->Ib is a feedback (used-before-calculated).
// Inputs:  sp.config, sp.sim_chm, Sen->Tb, Sen->Ib_model_in
// States:  Sim.soc
// Outputs: Sim.temp_c_, Sen->Tb_filt, Sen->Ib, Sen->Ib_model,
//   Sen->Vb_model, Sen->Tb_filt, sp.inj_bias
void sense_synth_select(const boolean reset, const boolean reset_temp, const unsigned long now, const unsigned long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen)
{
  static unsigned long int last_snap = now;
  boolean storing_fault_data = ( now - last_snap )>SNAP_WAIT;
  if ( storing_fault_data || reset ) last_snap = now;

  // Load Ib and Vb
  // Outputs: Sen->Ib_model_in, Sen->Ib, Sen->Vb
  load_ib_vb(reset, reset_temp, Sen, myPins, Mon);
  Sen->Flt->ib_wrap(reset, Sen, Mon);
  Sen->Flt->ib_quiet(reset, Sen);
  Sen->Flt->cc_diff(Sen, Mon);
  Sen->Flt->ib_diff(reset, Sen, Mon);


  // Sim initialize as needed from memory
  if ( reset_temp )
  {
    Sen->Tb_model = Sen->Tb_model_filt = RATED_TEMP + cp.Tb_bias_model;
    initialize_all(Mon, Sen, 0., false);
  }
  Sen->Sim->apply_delta_q_t(reset);
  Sen->Sim->init_battery_sim(reset, Sen);

  // Sim calculation
  //  Inputs:  Sen->Tb_filt(past), Sen->Ib_model_in
  //  States: Sim->soc(past)
  //  Outputs:  Tb_hdwe, Ib_model, Vb_model, sp.inj_bias, Sim.model_saturated
  Sen->Tb_model = Sen->Tb_model_filt = Sen->Sim->temp_c();
  Sen->Vb_model = Sen->Sim->calculate(Sen, cp.dc_dc_on, reset) + Sen->Vb_add();
  Sen->Ib_model = Sen->Sim->ib_fut() * sp.nP();
  cp.model_cutback = Sen->Sim->cutback();
  cp.model_saturated = Sen->Sim->saturated();

  // Inputs:  Sim->Ib
  Sen->Ib_amp_model = Sen->Ib_model*Sen->ib_amp_sclr() + Sen->Ib_amp_add() + Sen->Ib_amp_noise();  // Sm/Dm
  Sen->Ib_noa_model = Sen->Ib_model*Sen->ib_noa_sclr() + Sen->Ib_noa_add() + Sen->Ib_noa_noise();  // Sn/Dn

  // Select
  //  Inputs:                                       --->   Outputs:
  //  Ib_model, Ib_hdwe,                            --->   Ib
  //  Vb_model, Vb_hdwe,                            --->   Vb
  //  constant,         Tb_hdwe, Tb_hdwe_filt       --->   Tb, Tb_filt
  Sen->Flt->select_all(Sen, Mon, reset);
  Sen->final_assignments(Mon);

  // Fault snap buffer management
  static uint8_t fails_repeated = 0;
  if ( Sen->Flt->reset_all_faults() )
  {
    fails_repeated = 0;
    Sen->Flt->preserving(false);
  }
  static boolean record_past = Sen->Flt->record();
  boolean instant_of_failure = record_past && !Sen->Flt->record();
  if ( storing_fault_data || instant_of_failure )
  {
    if ( Sen->Flt->record() ) fails_repeated = 0;
    else fails_repeated = min(fails_repeated + 1, 99);
    if ( fails_repeated < 3 )
    {
      sp.put_iflt(sp.iflt()+1);
      if ( sp.iflt()>sp.nflt()-1 ) sp.put_iflt(0);  // wrap buffer
      Flt_st fault_snap;
      fault_snap.assign(Time.now(), Mon, Sen);
      sp.put_fault(fault_snap, sp.iflt());
    }
    else if ( fails_repeated < 4 )
    {
      Serial.printf("preserving fault buffer\n");
      Sen->Flt->preserving(true);
    }
    if ( instant_of_failure ) last_snap = now;
  }
  record_past = Sen->Flt->record();

  // Charge calculation and memory store
  // Inputs: Sim.model_saturated, Sen->Tb, Sen->Ib
  // States: Sim.soc
  Sen->Sim->count_coulombs(Sen, reset_temp, Mon, false);

  // Injection tweak test
  if ( (Sen->start_inj <= Sen->now) && (Sen->now <= Sen->end_inj) && (Sen->now > 0UL) ) // in range, test in progress
  {
    // Shift times because sampling is asynchronous: improve repeatibility
    if ( Sen->elapsed_inj==0UL )
    {
      Sen->end_inj += Sen->now - Sen->start_inj;
      Sen->stop_inj += Sen->now - Sen->start_inj;
      Sen->start_inj = Sen->now;
    }

    Sen->elapsed_inj = Sen->now - Sen->start_inj + 1UL; // Shift by 1 because using ==0 as reset button

    // Put a stop to this
    if (Sen->now > Sen->stop_inj) sp.put_Amp(0.);
  }
  else if ( Sen->elapsed_inj && sp.tweak_test() )  // Done.  elapsed_inj set to 0 is the reset button
  {
    Serial.printf("STOP echo\n");
    Sen->elapsed_inj = 0UL;
    chit("v0;", ASAP);      // Turn off echo
    chit("Xm247;", QUEUE);  // Turn off tweak_test
    chit("Pa;", QUEUE);     // Print all for record.  Last so Pf last and visible
    chit("Xp0;", QUEUE);    // Reset
  }
  Sen->Sim->calc_inj(Sen->elapsed_inj, sp.type(), sp.Amp(), sp.Freq());
}

// If false token, get new string from source
void get_string(String *source)
{
  while ( !cp.token && source->length() )
  {
    // get the new byte, add to input and check for completion
    char inChar = source->charAt(0);
    source->remove(0, 1);
    cp.input_str += inChar;
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      cp.input_str = ">" + cp.input_str;
      break;  // enable reading multiple inputs
    }
  }
}

// Cleanup string for final processing by talk
void finish_request(void)
{
  // Remove whitespace
  cp.input_str.trim();
  cp.input_str.replace("\n","");
  cp.input_str.replace("\0","");
  cp.input_str.replace(";","");
  cp.input_str.replace(",","");
  cp.input_str.replace(" ","");
  cp.input_str.replace("=","");
  cp.token = true;  // token:  temporarily inhibits while loop until talk() call resets token
}

/*
  Special handler for UART usb that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.

  Particle documentation says not to use something like
  the cp.token in the while loop statement.
  They suggest handling all the data in one call.   But 
  this works, so far.
 */
void serialEvent()
{
  while ( !cp.token && Serial.available() )
  {
    // get the new byte:
    char inChar = (char)Serial.read();

    // add it to the cp.input_str:
    cp.input_str += inChar;
    
    // if (inChar == '\n') Serial.printf("<CR>");
    // else if (inChar == '\0') Serial.printf("<EOL>");
    // else Serial.printf("%c", inChar);
    if ( inChar=='\r' ) Serial.printf("\n");

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      break;  // enable reading multiple inputs
    }
  }
  // if ( cp.token ) Serial.printf("serialEvent:  %s\n", cp.input_str.c_str());
}

/*
  Special handler for Bluetooth UART that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent1()
{
  while (!cp.token && Serial1.available())
  {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the cp.input_str:
    cp.input_str += inChar;

    if ( inChar=='\r' ) Serial.printf("\n");

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      break;  // enable reading multiple inputs
    }
  }
}

// Inputs serial print
void rapid_print(Sensors *Sen, BatteryMonitor *Mon)
{
  create_rapid_string(&pp.pubList, Sen, Mon);
  Serial.printf("%s\n", cp.buffer);
  #ifndef CONFIG_PHOTON
    Serial1.printf("%s\n", cp.buffer);
  #endif
}

// Time synchro for web information
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip)
{
  *last_sync = millis();

  // Request time synchronization from the Particle Cloud
  if ( Particle.connected() ) Particle.syncTime();

  // Refresh millis() at turn of Time.now
  int count = 0;
  long time_begin = Time.now();  // Seconds since start of epoch
  while ( Time.now()==time_begin && ++count<1100 )  // Time.now() truncates to seconds
  {
    delay(1);
    *millis_flip = millis()%1000;
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
        // uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
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

// Wait on user input to reset EERAM values
void wait_on_user_input(Adafruit_SSD1306 *display)
{
  display->clearDisplay();
  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner    sp.print_versus_local_config();
  display->println("Waiting for USB/BT talk\n\nignores after 120s");
  display->display();
  uint8_t count = 0;
  uint16_t answer = '\r';
  // Get user input but timeout at 120 seconds if no response
  while ( count<30 && answer!='Y' && answer!='n' && answer!='N' )
  {
    if ( answer=='\r')
    {
      count++;
      if ( count>1 ) delay(4000);
    }
    else delay(100);

    if ( Serial.available() )
      answer=Serial.read();
    else if ( Serial1.available() )
      answer=Serial1.read();

    if ( answer=='\r')
    {
      Serial.printf("\n\n");
      sp.pretty_print( false );
      Serial.printf("Reset to defaults? [Y/n]:"); Serial1.printf("Reset to defaults? [Y/n]:");
    }
    else  // User is typing.  Ignore him until they answer 'Y', 'N', or 'n'.  But timeout at 30 seconds if they don't
    {
      while ( answer!='Y' && answer!='N' && answer!='n' && count<30 )
      {
        if ( Serial.available() )
          answer=Serial.read();

        else if ( Serial1.available() )
          answer=Serial1.read();

        else
        {
          Serial.printf("?");
          count++;
          delay(1000);
        }
      }
    }

  }

  // Wrap it up
  if ( answer=='Y' )
  {
    Serial.printf("  Y\n\n"); Serial1.printf("  Y\n\n");
    sp.reset_pars();
    sp.pretty_print( true );
    #ifdef CONFIG_PHOTON2
      System.backupRamSync();
    #endif
  }
  else if ( answer=='n' || answer=='N' || count==30 )
  {
    Serial.printf(" N.  moving on...\n\n"); Serial1.printf(" N.  moving on...\n\n");
  }

}
