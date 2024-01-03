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

#include "subs.h"
#include "command.h"
#include "local_config.h"
#include <math.h>
#include "debug.h"
#include "Summary.h"
#include "talk/chitchat.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern PrinterPars pr;  // Print buffer
extern PublishPars pp;  // For publishing

// Harvest charge caused temperature change.   More charge becomes available as battery warms
void harvest_temp_change(const float temp_c, BatteryMonitor *Mon, BatterySim *Sim)
{
#ifdef DEBUG_INIT
if ( sp.debug()==-1 ) Serial.printf("entry harvest_temp_change:  Delta_q %10.1f temp_c %5.1f t_last %5.1f delta_q_model %10.1f temp_c_s %5.1f t_last_s %5.1f\n",
  sp.delta_q(), temp_c, sp.T_state(), sp.delta_q_model(), temp_c, sp.T_state_model());
#endif
  sp.put_Delta_q(sp.delta_q() - Mon->dqdt() * Mon->q_capacity() * (temp_c - sp.T_state()));
  sp.put_T_state(temp_c);
  sp.put_delta_q_model(sp.delta_q_model() - Sim->dqdt() * Sim->q_capacity() * (temp_c - sp.T_state_model()));
  sp.put_T_state_model(temp_c);
#ifdef DEBUG_INIT
if ( sp.debug()==-1 ) Serial.printf("exit harvest_temp_change:  Delta_q %10.1f temp_c %5.1f t_last %5.1f delta_q_model %10.1f temp_c_s %5.1f t_last_s %5.1f\n",
  sp.delta_q(), temp_c, sp.T_state(), sp.delta_q_model(), temp_c, sp.T_state_model());
#endif
}

// Complete initialization of all parameters in Mon and Sim including EKF
// Force current to be zero because initial condition undefined otherwise with charge integration
void initialize_all(BatteryMonitor *Mon, Sensors *Sen, const float soc_in, const boolean use_soc_in)
{
  // Sample debug statements
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 )
    {
      Serial.printf("\n\n");
      sp.pretty_print(true);
      Serial.printf("falw %d tb_fa %d\n", Sen->Flt->falw(), Sen->Flt->tb_fa());
    }
  #endif

  // Gather and apply inputs
  if ( sp.mod_ib() )
    Sen->Ib_model_in = sp.inj_bias() + sp.ib_bias_all();
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
  if ( use_soc_in )
    Mon->apply_soc(soc_in, Sen->Tb_filt);  // saves sp.delta_q and sp.T_state
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 )
    { 
      Serial.printf("before harvest_temp, falw %d tb_fa %d:", Sen->Flt->falw(), Sen->Flt->tb_fa()); debug_m1(Mon, Sen);
    }
  #endif
  if ( !Sen->Flt->tb_fa() ) harvest_temp_change(Sen->Tb_filt, Mon, Sen->Sim);
  
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("after harvest_temp:"); debug_m1(Mon, Sen);}
  #endif

  if ( cp.soft_sim_hold )  
    Sen->Sim->apply_delta_q_t(Sen->Sim->delta_q(), Sen->Sim->t_last());  // applies sp.delta_q and sp.T_state
  else
    Sen->Sim->apply_delta_q_t(Mon->delta_q(), Mon->t_last());  // applies sp.delta_q and sp.T_state

  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("S.a_d_q_t:"); debug_m1(Mon, Sen);}
  #endif

  // Make Sim accurate even if not used
  Sen->Sim->init_battery_sim(true, Sen);
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("S.i_b:"); debug_m1(Mon, Sen);}
  #endif
  if ( !sp.mod_vb() )
  {
    Sen->Sim->apply_soc(Sen->Sim->soc(), Sen->Tb_filt);
  }
  // Call calculate twice because sat_ is a used-before-calculated (UBC)
  // Simple 'call twice' method because sat_ is discrete no analog which would require iteration
  Sen->Vb_model = Sen->Sim->calculate(Sen, ap.dc_dc_on, true) * sp.nS();
  Sen->Vb_model = Sen->Sim->calculate(Sen, ap.dc_dc_on, true) * sp.nS();  // Call again because sat is a UBC
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
    if ( sp.debug()==-1 ){ Serial.printf("SENIB:"); debug_m1(Mon, Sen);}
  #endif
  if ( sp.mod_vb() && !cp.soft_sim_hold )
  {
    Mon->apply_soc(Sen->Sim->soc(), Sen->Tb_filt);
  }
  Mon->init_battery_mon(true, Sen);
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ) { Serial.printf("M.i_b:"); debug_m1(Mon, Sen);}
  #endif

  // Call calculate/count_coulombs twice because sat_ is a used-before-calculated (UBC)
  // Simple 'call twice' method because sat_ is discrete not analog which would require iteration
  Mon->calculate(Sen, true);
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("M.calc1:"); debug_m1(Mon, Sen);}
  #endif
  Mon->count_coulombs(0., true, Mon->t_last(), 0., Mon->is_sat(true), 0.);
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("M.c_c1:"); debug_m1(Mon, Sen);}
  #endif
  Mon->calculate(Sen, true);  // Call again because sat is a UBC
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("M.calc2:"); debug_m1(Mon, Sen);}
  #endif
  Mon->count_coulombs(0., true, Mon->t_last(), 0., Mon->is_sat(true), 0.);
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("M.c_c2:"); debug_m1(Mon, Sen);}
  #endif
  
  // Solve EKF
  Mon->solve_ekf(true, true, Sen);
  #ifdef DEBUG_INIT
    if ( sp.debug()==-1 ){ Serial.printf("end:"); debug_m1(Mon, Sen);}
  #endif

  // Finally....clear all faults
  Sen->Flt->reset_all_faults();
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
  if ( sp.debug()==14 ) Sen->shunt_print();

  // Load voltage Vb
  // Outputs:  Sen->Vb
  Sen->vb_load(myPins->Vb_pin, reset);
  if ( !sp.mod_vb_dscn() )  Sen->Flt->vb_check(Sen, Mon, VB_MIN, VB_MAX, reset);
  else                      Sen->Flt->vb_check(Sen, Mon, -1.0, 1.0, reset);
  if ( sp.debug()==15 ) Sen->vb_print();

  // Power calculation
  Sen->Wb = Sen->Vb*Sen->Ib;
}

// Calculate Ah remaining for display to user
// Inputs:  sp.mon_chm, Sen->Ib, Sen->Vb, Sen->Tb_filt
// States:  Mon.soc, Mon.soc_ekf
// Outputs: tcharge_wt, tcharge_ekf, Voc, Voc_filt
void  monitor(const boolean reset, const boolean reset_temp, const unsigned long long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen)
{
  // EKF - calculates temp_c_, voc_stat_, voc_ as functions of sensed parameters vb & ib (not soc)
  Mon->calculate(Sen, reset_temp);

  // Debounce saturation calculation done in ekf using voc model
  boolean sat = Mon->is_sat(reset);
  Sen->saturated = Is_sat_delay->calculate(sat, T_SAT*ap.s_t_sat, T_DESAT*ap.s_t_sat, min(Sen->T, T_SAT/2.), reset);

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
  sprintf(pr.buff, "%3.0f", pp.pubList.Tb);
  disp_0 = pr.buff;
  if ( Sen->Flt->tb_fa() && (blink==0 || blink==1) )
    disp_0 = "***";

  // Voc
  sprintf(pr.buff, "%5.2f", pp.pubList.Voc);
  disp_1 = pr.buff;
  if ( Sen->Flt->vb_sel_stat()==0 && (blink==1 || blink==2) )
    disp_1 = "*fail";
  else if ( Sen->bms_off )
    disp_1 = " off ";

  // Ib
  sprintf(pr.buff, "%6.1f", pp.pubList.Ib);
  disp_2 = pr.buff;
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
  sprintf(pr.buff, "%3.0f", pp.pubList.Amp_hrs_remaining_ekf);
  disp_0 = pr.buff;
  if ( blink==0 || blink==1 || blink==2 )
  {
    if ( Sen->Flt->cc_diff_fa() )
      disp_0 = "---";
  }
  display->print(disp_0.c_str());

  // t charge
  if ( abs(pp.pubList.tcharge) < 24. )
  {
    sprintf(pr.buff, "%5.1f", pp.pubList.tcharge);
  }
  else
  {
    sprintf(pr.buff, " --- ");
  }  
  disp_1 = pr.buff;
  display->print(disp_1.c_str());

  // Hrs large
  display->setTextSize(2);             // Draw 2X-scale text
  if ( blink==1 || blink==3 || !Sen->saturated )
  {
    sprintf(pr.buff, "%3.0f", min(pp.pubList.Amp_hrs_remaining_soc, 999.));
    disp_2 = pr.buff;
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
  if ( sp.debug()==99 ) // Calibration mode
    debug_99(Mon, Sen);
  else if ( sp.debug()!=4 && sp.debug()!=-2 )  // Normal display
    Serial1.printf("%s   Tb,C  VOC,V  Ib,A \n%s   EKF,Ah  chg,hrs  CC, Ah\nPf; for fails.  prints=%ld\n\n",
      disp_Tbop.c_str(), dispBot.c_str(), cp.num_v_print);

  if ( sp.debug()==5 ) debug_5(Mon, Sen);  // Charge time display on UART

  blink += 1;
  if (blink>3) blink = 0;
}
void oled_display(Sensors *Sen, BatteryMonitor *Mon)
{
  static uint8_t blink = 0;
  String disp_0, disp_1, disp_2;

  // ---------- Top Line of Display -------------------------------------------
  // Tb
  sprintf(pr.buff, "%3.0f", pp.pubList.Tb);
  disp_0 = pr.buff;
  if ( Sen->Flt->tb_fa() && (blink==0 || blink==1) )
    disp_0 = "***";

  // Voc
  sprintf(pr.buff, "%5.2f", pp.pubList.Voc);
  disp_1 = pr.buff;
  if ( Sen->Flt->vb_sel_stat()==0 && (blink==1 || blink==2) )
    disp_1 = "*fail";
  else if ( Sen->bms_off )
    disp_1 = " off ";

  // Ib
  sprintf(pr.buff, "%6.1f", pp.pubList.Ib);
  disp_2 = pr.buff;
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

  // --------------------- Bottom line of Display ------------------------------
  // Hrs EHK
  sprintf(pr.buff, "%3.0f", pp.pubList.Amp_hrs_remaining_ekf);
  disp_0 = pr.buff;
  if ( blink==0 || blink==1 || blink==2 )
  {
    if ( Sen->Flt->cc_diff_fa() )
      disp_0 = "---";
  }

  // t charge
  if ( abs(pp.pubList.tcharge) < 24. )
  {
    sprintf(pr.buff, "%5.1f", pp.pubList.tcharge);
  }
  else
  {
    sprintf(pr.buff, " --- ");
  }  
  disp_1 = pr.buff;

  // Hrs large
  if ( blink==1 || blink==3 || !Sen->saturated )
  {
    sprintf(pr.buff, "%3.0f", min(pp.pubList.Amp_hrs_remaining_soc, 999.));
    disp_2 = pr.buff;
  }
  else if (Sen->saturated)
    disp_2 = "SAT";
  String dispBot = disp_0 + disp_1 + " " + disp_2;

  // Text basic Bluetooth (use serial bluetooth app)
  if ( sp.debug()==99 ) // Calibration mode
    debug_99(Mon, Sen);
  else if ( sp.debug()!=4 && sp.debug()!=-2 )  // Normal display
    Serial1.printf("%s   Tb,C  VOC,V  Ib,A \n%s   EKF,Ah  chg,hrs  CC, Ah\nPf; for fails.  prints=%ld\n\n",
      disp_Tbop.c_str(), dispBot.c_str(), cp.num_v_print);

  if ( sp.debug()==5 ) debug_5(Mon, Sen);  // Charge time display on UART

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
void sense_synth_select(const boolean reset, const boolean reset_temp, const unsigned long long now, const unsigned long long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen)
{
  static unsigned long long int last_snap = now;
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
    Sen->Tb_model = Sen->Tb_model_filt = RATED_TEMP + ap.Tb_bias_model;
    initialize_all(Mon, Sen, 0., false);
  }
  Sen->Sim->apply_delta_q_t(reset);
  Sen->Sim->init_battery_sim(reset, Sen);

  // Sim calculation
  //  Inputs:  Sen->Tb_filt(past), Sen->Ib_model_in
  //  States: Sim->soc(past)
  //  Outputs:  Tb_hdwe, Ib_model, Vb_model, sp.inj_bias, Sim.model_saturated
  Sen->Tb_model = Sen->Tb_model_filt = Sen->Sim->temp_c();
  Sen->Vb_model = Sen->Sim->calculate(Sen, ap.dc_dc_on, reset) * sp.nS() + Sen->Vb_add();
  Sen->Ib_model = Sen->Sim->ib_fut() * sp.nP();
  cp.model_cutback = Sen->Sim->cutback();
  cp.model_saturated = Sen->Sim->saturated();

  // Inputs:  Sim->Ib
  Sen->Ib_amp_model = Sen->Ib_model + Sen->Ib_amp_add() + Sen->Ib_amp_noise();  // Sm/Dm
  Sen->Ib_noa_model = Sen->Ib_model + Sen->Ib_noa_add() + Sen->Ib_noa_noise();  // Sn/Dn

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
      sp.put_Iflt(sp.Iflt() + 1);
      if ( sp.Iflt()>sp.nflt() - 1 ) sp.put_Iflt(0);  // wrap buffer
      Flt_st fault_snap;
      fault_snap.assign(Time.now(), Mon, Sen);
      sp.put_fault(fault_snap, sp.Iflt());
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

  // Injection test
  if ( (Sen->start_inj <= Sen->now) && (Sen->now <= Sen->end_inj) && (Sen->now > 0ULL) ) // in range, test in progress
  {
    // Shift times because sampling is asynchronous: improve repeatibility
    if ( Sen->elapsed_inj==0ULL )
    {
      Sen->end_inj += Sen->now - Sen->start_inj;
      Sen->stop_inj += Sen->now - Sen->start_inj;
      Sen->start_inj = Sen->now;
    }

    Sen->elapsed_inj = Sen->now - Sen->start_inj + 1UL; // Shift by 1 because using ==0 as reset button

    // Put a stop to this but retain sp.amp_z to scale fault and history printouts properly
    if (Sen->now > Sen->stop_inj)
    {
      sp.put_Inj_bias(0);
      sp.put_Type(0);
    }
  }

  else if ( Sen->elapsed_inj && sp.tweak_test() )  // Done.  elapsed_inj set to 0 is the reset button
  {
    Serial.printf("STOP echo\n");
    Sen->elapsed_inj = 0ULL;
    chit("vv0;", ASAP);    // Turn off echo
    chit("Xp0;", SOON);    // Reset
  }
  Sen->Sim->calc_inj(Sen->elapsed_inj, sp.type(), sp.Amp(), sp.freq());

  // Quiet logic.   Reset to ready state at soc=0.5; do not change Modeling.  Passes at least once before running chit.
  static unsigned long long millis_past = System.millis();
  static unsigned long int until_q_past = ap.until_q;
  if ( ap.until_q>0UL && until_q_past==0UL ) until_q_past = ap.until_q;
  ap.until_q = (unsigned long) max(0, (long) ap.until_q  - (long)(System.millis() - millis_past));
  if ( ap.until_q==0UL && until_q_past>0UL )
  {
    chit("BZ;", SOON);
    cp.freeze = false;  // unfreeze the queues
  }
  until_q_past = ap.until_q;
  millis_past = System.millis();

}


// Time synchro for web information
void sync_time(unsigned long long now, unsigned long long *last_sync, unsigned long long *millis_flip)
{
  *last_sync = System.millis();

  // Request time synchronization from the Particle Cloud
  if ( Particle.connected() ) Particle.syncTime();

  // Refresh millis() at turn of Time.now
  int count = 0;
  long time_begin = Time.now();  // Seconds since start of epoch
  while ( Time.now()==time_begin && ++count<1100 )  // Time.now() truncates to seconds
  {
    delay(1);
    *millis_flip = System.millis()%1000;
  }
}


// For summary prints
String time_long_2_str(const time_t time, char *tempStr)
{
    // Serial.printf("Time.year:  time_t %d ul %d as-is %d\n", 
    //   Time.year((time_t) 1703267248), Time.year((unsigned long )1703267248), Time.year(time));
    uint32_t year = Time.year(time);
    uint8_t month = Time.month(time);
    uint8_t day = Time.day(time);
    uint8_t hours = Time.hour(time);
    uint8_t minutes   = Time.minute(time);
    uint8_t seconds   = Time.second(time);
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    // Serial.printf("time_long_2_str: %lld %ld %d %d %d %d %d\n", time, year, month, day, hours, minutes, seconds);
    // sprintf(tempStr, "time_long_2_str");
    return ( String(tempStr) );
}
