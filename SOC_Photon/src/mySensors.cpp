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
#include "command.h"
#include "mySensors.h"
#include "local_config.h"
#include <math.h>
#include "debug.h"

extern CommandPars cp;          // Various parameters shared at system level
extern PublishPars pp;            // For publishing
extern RetainedPars rp;         // Various parameters to be static at system level


// class TempSensor
// constructors
TempSensor::TempSensor(const uint16_t pin, const bool parasitic, const uint16_t conversion_delay)
: DS18(pin, parasitic, conversion_delay), tb_stale_flt_(true)
{
   SdTb = new SlidingDeadband(HDB_TBATT);
}
TempSensor::~TempSensor() {}
// operators
// functions
float TempSensor::load(Sensors *Sen)
{
  // Read Sensor
  // MAXIM conversion 1-wire Tp plenum temperature
  uint8_t count = 0;
  float temp = 0.;
  static float Tb_hdwe = 0.;
  // Read hardware and check
  while ( ++count<MAX_TEMP_READS && temp==0 && !rp.mod_tb() )
  {
    if ( read() ) temp = celsius() + (TBATT_TEMPCAL);
    delay(1);
  }

  // Check success
  if ( count<MAX_TEMP_READS && TEMP_RANGE_CHECK<temp && temp<TEMP_RANGE_CHECK_MAX && !Sen->Flt->fail_tb() )
  {
    Tb_hdwe = SdTb->update(temp);
    tb_stale_flt_ = false;
    if ( rp.debug==16 ) Serial.printf("I:  t=%7.3f ct=%d, Tb_hdwe=%7.3f,\n", temp, count, Tb_hdwe);
  }
  else
  {
    Serial.printf("E: DS18, t=%8.1f, ct=%d, using lgv=%8.1f\n", temp, count, Tb_hdwe);
    tb_stale_flt_ = true;
    // Using last-good-value:  no assignment
  }
  return ( Tb_hdwe );
}


// class Shunt
// constructors
Shunt::Shunt()
: Tweak(), Adafruit_ADS1015(), name_("None"), port_(0x00), bare_(false){}
Shunt::Shunt(const String name, const uint8_t port, float *rp_delta_q_cinf, float *rp_delta_q_dinf, float *rp_tweak_sclr,
  float *cp_ib_bias, float *cp_ib_scale, const float v2a_s, float *rp_shunt_gain_sclr)
: Tweak(name, TWEAK_MAX_CHANGE, TWEAK_MAX, TWEAK_WAIT, rp_delta_q_cinf, rp_delta_q_dinf, rp_tweak_sclr, COULOMBIC_EFF),
  Adafruit_ADS1015(),
  name_(name), port_(port), bare_(false), cp_ib_bias_(cp_ib_bias), cp_ib_scale_(cp_ib_scale), v2a_s_(v2a_s),
  vshunt_int_(0), vshunt_int_0_(0), vshunt_int_1_(0), vshunt_(0), ishunt_cal_(0), sclr_(1.), add_(0.),
  rp_shunt_gain_sclr_(rp_shunt_gain_sclr)
{
  if ( name_=="No Amp")
    setGain(GAIN_SIXTEEN, GAIN_SIXTEEN); // 16x gain differential and single-ended  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  else if ( name_=="Amp")
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  else
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  if (!begin(port_)) {
    Serial.printf("FAILED init ADS SHUNT MON %s\n", name_.c_str());
    bare_ = true;
  }
  else Serial.printf("SHUNT MON %s init\n", name_.c_str());
}
Shunt::~Shunt() {}
// operators
// functions

void Shunt::pretty_print()
{
  Serial.printf("Shunt(%s)::\n", name_.c_str());
  Serial.printf(" port=0x%X;\n", port_);
  Serial.printf(" bare=%d;\n", bare_);
  Serial.printf(" *cp_ib_bias=%7.3f; A\n", *cp_ib_bias_);
  Serial.printf(" *cp_ib_scale=%7.3f; A\n", *cp_ib_scale_);
  Serial.printf(" v2a_s=%7.2f; A/V\n", v2a_s_);
  Serial.printf(" vshunt_int=%d; count\n", vshunt_int_);
  Serial.printf(" ishunt_cal=%7.3f; A\n", ishunt_cal_);
  Serial.printf(" *rp_shunt_gain_sclr=%7.3f; A\n", *rp_shunt_gain_sclr_);
  Serial.printf("Shunt(%s)::", name_.c_str()); Tweak::pretty_print();
  Serial.printf("Shunt(%s)::", name_.c_str()); Adafruit_ADS1015::pretty_print(name_);
}

// load
void Shunt::load()
{
  if ( !bare_ && !rp.mod_ib() )
  {
    vshunt_int_ = readADC_Differential_0_1();
    
    // if ( rp.debug==-14 ) { vshunt_int_0_ = readADC_SingleEnded(0);  vshunt_int_1_ = readADC_SingleEnded(1); }
    //                 else { vshunt_int_0_ = 0;                       vshunt_int_1_ = 0; }
  }
  else
  {
    vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0;
  }
  vshunt_ = computeVolts(vshunt_int_);
  ishunt_cal_ = vshunt_*v2a_s_*float(!rp.mod_ib())*(*cp_ib_scale_)*(*rp_shunt_gain_sclr_) + *cp_ib_bias_;
}


// Class Fault
Fault::Fault(const double T):
  cc_diff_(0.), cc_diff_sclr_(1), disab_ib_fa_(false), disab_tb_fa_(false), disab_vb_fa_(false), 
  ewhi_sclr_(1), ewlo_sclr_(1), ewsat_sclr_(1), e_wrap_(0), e_wrap_filt_(0), fail_tb_(false),
  ib_diff_sclr_(1), ib_quiet_sclr_(1), ib_diff_(0), ib_diff_f_(0), ib_quiet_(0), ib_rate_(0), tb_sel_stat_(1),
  tb_stale_time_sclr_(1), vb_sel_stat_(1), ib_sel_stat_(1), reset_all_faults_(false),
  tb_sel_stat_last_(1), vb_sel_stat_last_(1), ib_sel_stat_last_(1), fltw_(0UL), falw_(0UL)
{
  IbErrFilt = new LagTustin(T, TAU_ERR_FILT, -MAX_ERR_FILT, MAX_ERR_FILT);  // actual update time provided run time
  IbdHiPer = new TFDelay(false, IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, T);
  IbdLoPer = new TFDelay(false, IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, T);
  IbAmpHardFail  = new TFDelay(false, IBATT_HARD_SET, IBATT_HARD_RESET, T);
  IbNoAmpHardFail  = new TFDelay(false, IBATT_HARD_SET, IBATT_HARD_RESET, T);
  TbStaleFail  = new TFDelay(false, TB_STALE_SET, TB_STALE_RESET, T);
  VbHardFail  = new TFDelay(false, VBATT_HARD_SET, VBATT_HARD_RESET, T);
  QuietPer  = new TFDelay(false, QUIET_S, QUIET_R, T);
  WrapErrFilt = new LagTustin(T, WRAP_ERR_FILT, -MAX_WRAP_ERR_FILT, MAX_WRAP_ERR_FILT);  // actual update time provided run time
  WrapHi = new TFDelay(false, WRAP_HI_S, WRAP_HI_R, EKF_NOM_DT);  // Wrap test persistence.  Initializes false
  WrapLo = new TFDelay(false, WRAP_LO_S, WRAP_LO_R, EKF_NOM_DT);  // Wrap test persistence.  Initializes false
  QuietFilt = new General2_Pole(T, WN_Q_FILT, ZETA_Q_FILT, MIN_Q_FILT, MAX_Q_FILT);  // actual update time provided run time
  QuietRate = new RateLagExp(T, TAU_Q_FILT, MIN_Q_FILT, MAX_Q_FILT);
}

// Print bitmap
void Fault::bitMapPrint(char *buf, const int16_t fw, const uint8_t num)
{
  for ( int i=0; i<num; i++ )
  {
    if ( bitRead(fw, i) ) buf[num-i-1] = '1';
    else  buf[num-i-1] = '0';
  }
  buf[num] = '\0';
}

// Detect no signal present based on detection of quiet signal.
// Research by sound industry found that 2-pole filtering is the sweet spot between seeing noise
// and actual motion without 'guilding the lily'
void Fault::ib_quiet(const boolean reset, Sensors *Sen)
{
  boolean reset_loc = reset | reset_all_faults_;

  // Rate (has some filtering)
  ib_rate_ = QuietRate->calculate(Sen->Ib_amp_hdwe + Sen->Ib_noa_hdwe, reset, min(Sen->T, MAX_T_Q_FILT));

  // 2-pole filter
  ib_quiet_ = QuietFilt->calculate(ib_rate_, reset_loc, min(Sen->T, MAX_T_Q_FILT));

  // Fault
  faultAssign( !rp.mod_ib() && abs(ib_quiet_)<=QUIET_A*ib_quiet_sclr_ && !reset_loc, IB_DSCN_FLT );   // initializes false
  failAssign( QuietPer->calculate(dscn_flt(), QUIET_S, QUIET_R, Sen->T, reset_loc), IB_DSCN_FA);
  debug_m13(Sen);
}

// Voltage wraparound logic for current selection
// Avoid using hysteresis data for this test and accept more generous thresholds
void Fault::ib_wrap(const boolean reset, Sensors *Sen, BatteryMonitor *Mon)
{
  boolean reset_loc = reset | reset_all_faults_;
  e_wrap_ = Mon->voc_soc() - Mon->voc();
  if ( Mon->soc()>WRAP_HI_SOC_OFF ) ewsat_sclr_ = WRAP_HI_SOC_SCLR;
  else if ( Mon->voc_soc()>(Mon->vsat()-WRAP_HI_SAT_MARG) ) ewsat_sclr_ = WRAP_HI_SAT_SCLR;
  else ewsat_sclr_ = 1.;
  e_wrap_filt_ = WrapErrFilt->calculate(e_wrap_, reset_loc, min(Sen->T, F_MAX_T_WRAP));
  // sat logic screens out voc jumps when ib>0 when saturated
  // wrap_hi and wrap_lo don't latch because need them available to check new ib sensor selection for dual ib sensor
  // wrap_vb latches because vb is simplex sensor
  faultAssign( (e_wrap_filt_ >= Mon->r_ss()*WRAP_HI_A*ewhi_sclr_*ewsat_sclr_ && !Mon->sat()), WRAP_HI_FLT);
  faultAssign( (e_wrap_filt_ <= Mon->r_ss()*WRAP_LO_A*ewlo_sclr_*ewsat_sclr_), WRAP_LO_FLT);
  failAssign( (WrapHi->calculate(wrap_hi_flt(), WRAP_HI_S, WRAP_HI_R, Sen->T, reset_loc) && !vb_fa()), WRAP_HI_FA );
  failAssign( (WrapLo->calculate(wrap_lo_flt(), WRAP_LO_S, WRAP_LO_R, Sen->T, reset_loc) && !vb_fa()), WRAP_LO_FA );
  failAssign( (wrap_vb_fa() && !reset_loc) || (!ib_dif_fa() && wrap_fa()), WRAP_VB_FA);
}

void Fault::pretty_print(Sensors *Sen, BatteryMonitor *Mon)
{
  Serial.printf("Fault:\n");
  Serial.printf(" cc_dif=%7.3f;\n", cc_diff_);
  Serial.printf(" voc_soc=%7.3f;\n", Mon->voc_soc());
  Serial.printf(" soc=%7.3f;\n", Mon->soc());
  Serial.printf(" voc=%7.3f;\n", Mon->voc());
  Serial.printf(" e_w_f=%7.3f;\n", e_wrap_filt_);
  Serial.printf(" disab_ib_fa=%d;\n", disab_ib_fa_);
  Serial.printf(" disab_tb_fa=%d;\n", disab_tb_fa_);
  Serial.printf(" disab_vb_fa=%d;\n", disab_vb_fa_);
  Serial.printf(" imh=%7.3f;\n", Sen->Ib_amp_hdwe);
  Serial.printf(" inh=%7.3f;\n", Sen->Ib_noa_hdwe);
  Serial.printf(" imm=%7.3f;\n", Sen->Ib_amp_model);
  Serial.printf(" inm=%7.3f;\n", Sen->Ib_noa_model);
  Serial.printf(" ibd_f=%7.3f;\n", ib_diff_f_);
  Serial.printf(" tb_s_st=%d;\n", tb_sel_stat_);
  Serial.printf(" vb_s_st=%d;\n", vb_sel_stat_);
  Serial.printf(" ib_s_st=%d;\n", ib_sel_stat_);
  Serial.printf(" nbar=%d;\n", Sen->ShuntNoAmp->bare());
  Serial.printf(" mbar=%d;\n", Sen->ShuntAmp->bare());
  Serial.printf(" mod_tb=%d, mod_vb=%d, mod_ib=%d\n", rp.mod_tb(), rp.mod_vb(), rp.mod_ib());
  Serial.printf(" ib_dscn_ft=%d;  'Sq'\n", ib_dscn_flt());
  Serial.printf(" ibd_lo_ft=%d;   'Sd, *SA/*SB'\n", ib_dif_lo_flt());
  Serial.printf(" ibd_hi_ft=%d;   'Sd, *SA/*SB'\n", ib_dif_hi_flt());
  Serial.printf(" red_loss=%d;\n", ib_red_loss());
  Serial.printf(" wl_ft=%d;   'Sb'\n", wrap_lo_flt());
  Serial.printf(" wh_ft=%d;   'Sa'\n    4\n", wrap_hi_flt());
  Serial.printf(" ibn_ft=%d;  'Fi'\n", ib_noa_flt());   // TODO: add __
  Serial.printf(" ibm_ft=%d;  'Fi'\n", ib_amp_flt());   // TODO: add __
  Serial.printf(" vb_ft=%d;   'Fv'\n", vb_flt());   // TODO: add __
  Serial.printf(" tb_ft=%d;   'Ft'\n  ", tb_flt());
  bitMapPrint(cp.buffer, fltw_, NUM_FLT);
  Serial.print(cp.buffer);
  Serial.printf(";\n");
  Serial.printf(" CBA98765x3210\n");
  Serial.printf(" fltw=%d;\n", fltw_);
  Serial.printf(" ib_dscn_fa=%d;   'Sq'\n", ib_dscn_fa());
  Serial.printf(" ibd_lo_fa=%d;   'Sd, *SA/*SB'\n", ib_dif_lo_fa());
  Serial.printf(" ibd_hi_fa=%d;   'Sd, *SA/*SB'\n", ib_dif_hi_fa());
  Serial.printf(" wv_fa=%d;   'Sd, Sa/Sb'\n", wrap_vb_fa());
  Serial.printf(" wl_fa=%d;   'Sb'\n", wrap_lo_fa());
  Serial.printf(" wh_fa=%d;   'Sa'\n", wrap_hi_fa());
  Serial.printf(" cc_diff_fa=%d;   'Sf'\n", cc_diff_fa());
  Serial.printf(" ibn_fa=%d;  'Fi'\n", ib_noa_fa());    // TODO: add __
  Serial.printf(" ibm_fa=%d;  'Fi'\n", ib_amp_fa());    // TODO: add __
  Serial.printf(" vb_fa=%d;   'Fv'\n", vb_fa());    // TODO: add __
  Serial.printf(" tb_fa=%d;   'Ft'\n  ", tb_fa());
  bitMapPrint(cp.buffer, falw_, NUM_FA);
  Serial.print(cp.buffer);
  Serial.printf(";\n");
  Serial.printf(" A9876543210\n");
  Serial.printf(" falw=%d;\n", falw_);
}
void Fault::pretty_print1(Sensors *Sen, BatteryMonitor *Mon)
{
  Serial1.printf("Fault:\n");
  Serial1.printf(" mbar=%d;\n", Sen->ShuntAmp->bare());
  Serial1.printf(" nbar=%d;\n", Sen->ShuntNoAmp->bare());
  Serial1.printf(" cc_dif=%7.3f;\n", cc_diff_);
  Serial1.printf(" voc_soc=%7.3f;\n", Mon->voc_soc());
  Serial1.printf(" soc=%7.3f;\n", Mon->soc());
  Serial1.printf(" voc=%7.3f;\n", Mon->voc());
  Serial1.printf(" e_w_f=%7.3f;\n", e_wrap_filt_);
  Serial1.printf(" red_loss=%d;\n", ib_red_loss());
  Serial1.printf(" imh=%7.3f;\n", Sen->Ib_amp_hdwe);
  Serial1.printf(" inh=%7.3f;\n", Sen->Ib_noa_hdwe);
  Serial1.printf(" ibd_f=%7.3f;\n", ib_diff_f_);
  Serial1.printf(" mod_tb=%d, mod_vb=%d, mod_ib=%d\n", rp.mod_tb(), rp.mod_vb(), rp.mod_ib());
  Serial1.printf(" tb_s_st=%d;\n", tb_sel_stat_);
  Serial1.printf(" vb_s_st=%d;\n", vb_sel_stat_);
  Serial1.printf(" ib_s_st=%d;\n", ib_sel_stat_);
  Serial1.printf(" nbar=%d;\n", Sen->ShuntNoAmp->bare());
  Serial1.printf(" mbar=%d;\n", Sen->ShuntAmp->bare());
  Serial1.printf(" fltw=%d;  ", fltw_);
  Serial1.printf(";\n");
  Serial1.printf(" ib_dscn_fa=%d;   'Sq'\n", ib_dscn_fa());
  Serial1.printf(" ibd_lo_fa=%d;   'Sd, *SA/*SB'\n", ib_dif_lo_fa());
  Serial1.printf(" ibd_hi_fa=%d;   'Sd, *SA/*SB'\n", ib_dif_hi_fa());
  Serial1.printf(" wv_fa=%d;   'Sd', Sa/Sb'\\n", wrap_vb_fa());
  Serial1.printf(" wl_fa=%d;   'Sb'\n", wrap_lo_fa());
  Serial1.printf(" wh_fa=%d;   'Sa'\n", wrap_hi_fa());
  Serial1.printf(" cc_diff_fa=%d;   'Sf'\n", cc_diff_fa());
  Serial1.printf(" ibn_fa=%d;  'Fi'\n", ib_noa_fa());    // TODO: add __
  Serial1.printf(" ibm_fa=%d;  'Fi'\n", ib_amp_fa());    // TODO: add __
  Serial1.printf(" vb_fa=%d;   'Fv'\n", vb_fa());    // TODO: add __
  Serial1.printf(" tb_fa=%d;   'Fv'\n  ", tb_fa());
  bitMapPrint(cp.buffer, falw_, NUM_FA);
  Serial1.print(cp.buffer);
  Serial1.printf(";\n");
  Serial1.printf(" A9876543210\n");
  Serial1.printf(" falw=%d;\n", falw_);
  Serial1.printf("v0; to return\n");
}

// Calculate selection for choice
// Use model instead of sensors when running tests as user
// Equivalent to using voc(soc) as voter between two hardware currrents
// Over-ride sensed Ib, Vb and Tb with model when running tests
// TODO:  fix this nomenclature
// Inputs:  Sen->Ib_model, Sen->Ib_hdwe,
//          Sen->Vb_model, Sen->Vb_hdwe,
//          ----------------, Sen->Tb_hdwe, Sen->Tb_hdwe_filt
// Outputs: Ib,
//          Vb,
//          Tb, Tb_filt
void Fault::select_all(Sensors *Sen, BatteryMonitor *Mon, const boolean reset)
{
  boolean reset_loc = reset || reset_all_faults_;

  // Temperature persistence
  if ( disab_tb_fa_ )
  {
    faultAssign( false, TB_FLT );
    failAssign( false, TB_FA );
  }
  else
  {
    faultAssign( Sen->SensorTb->tb_stale_flt(), TB_FLT );
    failAssign( TbStaleFail->calculate(tb_flt(), TB_STALE_SET*tb_stale_time_sclr_, TB_STALE_RESET*tb_stale_time_sclr_,
      Sen->T, reset_loc), TB_FA );
  }

  // EKF error test - failure conditions track poorly
  cc_diff_ = Mon->soc_ekf() - Mon->soc();  // These are filtered in their construction (EKF is a dynamic filter and 
                                                  // Coulomb counter is wrapa big integrator)
  failAssign( abs(cc_diff_) >= SOC_DISAGREE_THRESH*cc_diff_sclr_, CCD_FA );

  // Compare current sensors - failure conditions large difference
  // Difference error, filter, check, persist
  if ( rp.mod_ib() ) ib_diff_ = (Sen->Ib_amp_model - Sen->Ib_noa_model) / Mon->nP();
  else ib_diff_ = (Sen->Ib_amp_hdwe - Sen->Ib_noa_hdwe) / Mon->nP();
  ib_diff_f_ = IbErrFilt->calculate(ib_diff_, reset_loc, min(Sen->T, MAX_ERR_T));
  faultAssign( ib_diff_f_>=IBATT_DISAGREE_THRESH*ib_diff_sclr_, IB_DIF_HI_FLT );
  faultAssign( ib_diff_f_<=-IBATT_DISAGREE_THRESH*ib_diff_sclr_, IB_DIF_LO_FLT );
  failAssign( IbdHiPer->calculate(ib_dif_hi_flt(), IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, Sen->T, reset_loc), IB_DIF_HI_FA ); // IB_DIF_FA
  failAssign( IbdLoPer->calculate(ib_dif_lo_flt(), IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, Sen->T, reset_loc), IB_DIF_LO_FA ); // IB_DIF_FA

  // Truth tables

  // ib
  // Serial.printf("\nTopTruth: rpibs,rloc,raf,ibss,ibssl,vbss,vbssl,ampb,noab,ibdf,whf,wlf,ccdf, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n",
  //  rp.ib_select, reset_loc, reset_all_faults_, ib_sel_stat_, ib_sel_stat_last_, vb_sel_stat_, vb_sel_stat_last_, Sen->ShuntAmp->bare(), Sen->ShuntNoAmp->bare(), ib_diff_fa_, wrap_hi_fa(), wrap_lo_fa(), cc_diff_fa_);
  if ( reset_all_faults_ )
  {
    ib_sel_stat_last_ = 1;
    Serial.printf("reset ib flt\n");
  }
    if ( Sen->ShuntAmp->bare() && Sen->ShuntNoAmp->bare() )
  {
    ib_sel_stat_ = 0;
  }
  else if ( rp.ib_select>0 && !Sen->ShuntAmp->bare() )
  {
    ib_sel_stat_ = 1;
  }
  else if ( ib_sel_stat_last_==-1 && !Sen->ShuntNoAmp->bare() )  // latch - use reset
  {
    ib_sel_stat_ = -1;
  }
  else if ( rp.ib_select<0 && !Sen->ShuntNoAmp->bare() )
  {
    ib_sel_stat_ = -1;
  }
  else if ( rp.ib_select==0 )  // auto
  {
    if ( Sen->ShuntAmp->bare() && !Sen->ShuntNoAmp->bare() )
    {
      ib_sel_stat_ = -1;
    }
    else if ( ib_dif_fa() )
    {
      if ( vb_sel_stat_ && wrap_fa() )
      {
        ib_sel_stat_ = -1;
      }
      else if ( cc_diff_fa() )
      {
        ib_sel_stat_ = -1;
      }
    }
    else if ( ib_sel_stat_last_ >= 0 )  // Must reset to move out of no amp selection
    {
      ib_sel_stat_ = 1;
    }
  }
  faultAssign(ib_red_loss_calc(), IB_RED_LOSS); // ib_sel_stat<0

  // vb failure from wrap result.  Latches
  if ( reset_all_faults_ )
  {
    vb_sel_stat_last_ = 1;
    vb_sel_stat_ = 1;
    Serial.printf("reset vb flts\n");
  }
  if ( !vb_sel_stat_last_ )
  {
    vb_sel_stat_ = 0;   // Latch
  }
  if (  wrap_vb_fa() )
  {
    vb_sel_stat_ = 0;
  }

  // tb failure from inactivity. Does not latch because can heal and failure not critical
  if ( reset_all_faults_ )
  {
    tb_sel_stat_last_ = 1;
    tb_sel_stat_ = 1;
    Serial.printf("reset tb flts\n");
  }
  if ( tb_fa() )
  {
    tb_sel_stat_ = 0;
  }
  else
    tb_sel_stat_ = 1;

  // Print
  if ( ib_sel_stat_ != ib_sel_stat_last_ || vb_sel_stat_ != vb_sel_stat_last_ || tb_sel_stat_ != tb_sel_stat_last_ )
  {
    Serial.printf("Sel chg:  Amp->bare=%d, NoAmp->bare=%d, ib_dif_fa=%d, wh_fa=%d, wl_fa=%d, wv_fa=%d, cc_diff_fa_=%d,\n rp.ib_select=%d, ib_sel_stat=%d, vb_sel_stat=%d, tb_sel_stat=%d, vb_fail=%d, Tb_fail=%d,\n",
      Sen->ShuntAmp->bare(), Sen->ShuntNoAmp->bare(), ib_dif_fa(), wrap_hi_fa(), wrap_lo_fa(), wrap_vb_fa(), cc_diff_fa_, rp.ib_select, ib_sel_stat_, vb_sel_stat_, tb_sel_stat_, vb_fa(), tb_fa());
    Serial.printf("ibss=%d, ibssl=%d, vbss=%d, vbssl=%d, tbss=%d, tbssl=%d\n",
      ib_sel_stat_, ib_sel_stat_last_, vb_sel_stat_, vb_sel_stat_last_, tb_sel_stat_, tb_sel_stat_last_);
  }
  if ( ib_sel_stat_ != ib_sel_stat_last_ )
  {
    Serial.printf("Small reset\n");
    cp.cmd_reset();   
  }
  ib_sel_stat_last_ = ib_sel_stat_;
  vb_sel_stat_last_ = vb_sel_stat_;
  tb_sel_stat_last_ = tb_sel_stat_;

  // Make sure asynk Rf command gets executed at least once all fault logic
  static uint8_t count = 0;
  if ( reset_all_faults_ )
  {
    if ( ( falw_==0 && fltw_==0 ) || count>1 )
    {
      reset_all_faults_ = false;
      count = 0;
    }
    else
    {
      count++;
      Serial.printf("Rf%d\n", count);
    }
  }
}

// Checks analog current.  Latches
void Fault::shunt_check(Sensors *Sen, BatteryMonitor *Mon, const boolean reset)
{
    boolean reset_loc = reset | reset_all_faults_;
    if ( reset_loc )
    {
      failAssign(false, IB_AMP_FA);
      failAssign(false, IB_NOA_FA);
    }
    float current_max = RATED_BATT_CAP * Mon->nP();
    faultAssign( abs(Sen->ShuntAmp->ishunt_cal()) >= current_max && !disab_ib_fa_, IB_AMP_FLT );
    faultAssign( abs(Sen->ShuntNoAmp->ishunt_cal()) >= current_max && !disab_ib_fa_, IB_NOA_FLT );
    if ( disab_ib_fa_ )
    {
      failAssign( false, IB_AMP_FA );
      failAssign( false, IB_NOA_FA);
    }
    else
    {
      failAssign( ib_amp_fa() || IbAmpHardFail->calculate(ib_amp_flt(), IBATT_HARD_SET, IBATT_HARD_RESET, Sen->T, reset_loc), IB_AMP_FA );
      failAssign( ib_noa_fa() || IbNoAmpHardFail->calculate(ib_noa_flt(), IBATT_HARD_SET, IBATT_HARD_RESET, Sen->T, reset_loc), IB_NOA_FA);
    }
}

// Check analog voltage.  Latches
void Fault::vb_check(Sensors *Sen, BatteryMonitor *Mon, const float _Vb_min, const float _Vb_max, const boolean reset)
{
  boolean reset_loc = reset | reset_all_faults_;
  if ( reset_loc )
  {
    failAssign(false, VB_FA);
  }
  if ( disab_vb_fa_ )
  {
    faultAssign(false, VB_FLT);
    failAssign( false, VB_FA);
  }
  else
  {
    faultAssign( (Sen->Vb_hdwe<=_Vb_min*Mon->nS()) || (Sen->Vb_hdwe>=_Vb_max*Mon->nS()), VB_FLT);
    failAssign( vb_fa() || VbHardFail->calculate(vb_flt(), VBATT_HARD_SET, VBATT_HARD_RESET, Sen->T, reset_loc), VB_FA);
  }
}


// Class Sensors
Sensors::Sensors(double T, double T_temp, byte pin_1_wire, Sync *ReadSensors):
  rp_tb_bias_(&rp.tb_bias), tb_bias_last_(0.), Tb_noise_amp_(TB_NOISE), Vb_noise_amp_(VB_NOISE),
  Ib_amp_noise_amp_(IB_AMP_NOISE), Ib_noa_noise_amp_(IB_NOA_NOISE)
{
  this->T = T;
  this->T_filt = T;
  this->T_temp = T_temp;
  this->ShuntAmp = new Shunt("Amp", 0x49, &rp.delta_q_cinf_amp, &rp.delta_q_dinf_amp, &rp.tweak_sclr_amp,
    &cp.ib_tot_bias_amp, &rp.ib_scale_amp, SHUNT_AMP_GAIN, &rp.shunt_gain_sclr);
  this->ShuntNoAmp = new Shunt("No Amp", 0x48, &rp.delta_q_cinf_noa, &rp.delta_q_dinf_noa, &rp.tweak_sclr_noa,
    &cp.ib_tot_bias_noa, &rp.ib_scale_noa, SHUNT_NOA_GAIN, &rp.shunt_gain_sclr);
  this->SensorTb = new TempSensor(pin_1_wire, TEMP_PARASITIC, TEMP_DELAY);
  this->TbSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);
  this->Sim = new BatteryModel(&rp.delta_q_model, &rp.t_last_model, &rp.s_cap_model, &rp.nP, &rp.nS, &rp.sim_mod);
  this->elapsed_inj = 0UL;
  this->start_inj = 0UL;
  this->stop_inj = 0UL;
  this->end_inj = 0UL;
  this->cycles_inj = 0.;
  this->ReadSensors = ReadSensors;
  this->display = true;
  this->sclr_coul_eff = 1.;
  this->Ib_hdwe_model = 0.;
  Prbn_Tb_ = new PRBS_7(TB_NOISE_SEED);
  Prbn_Vb_ = new PRBS_7(VB_NOISE_SEED);
  Prbn_Ib_amp_ = new PRBS_7(IB_AMP_NOISE_SEED);
  Prbn_Ib_noa_ = new PRBS_7(IB_NOA_NOISE_SEED);
  Flt = new Fault(T);
}

// Bias model outputs for sensor fault injection
void Sensors::bias_all_model()
{
  Ib_amp_model = ShuntAmp->bias_any( Ib_model ) + Ib_amp_noise();
  Ib_noa_model = ShuntNoAmp->bias_any( Ib_model ) + Ib_noa_noise();
}


// Deliberate choice based on results and inputs
// Inputs:  ib_sel_stat_, Ib_amp_hdwe, Ib_noa_hdwe, Ib_amp_model, Ib_noa_model
// Outputs:  Ib_hdwe_model, Ib_hdwe, sclr_coul_eff, Vshunt
void Sensors::choose_()
{
  if ( Flt->ib_sel_stat()>0 )
  {
      Vshunt = ShuntAmp->vshunt();
      Ib_hdwe = Ib_amp_hdwe;
      Ib_hdwe_model = Ib_amp_model;
      sclr_coul_eff = rp.tweak_sclr_amp;
  }
  else if ( Flt->ib_sel_stat()<0 )
  {
      Vshunt = ShuntNoAmp->vshunt();
      Ib_hdwe = Ib_noa_hdwe;
      Ib_hdwe_model = Ib_noa_model;
      sclr_coul_eff = rp.tweak_sclr_noa;
  }
  else
  {
      Vshunt = 0.;
      Ib_hdwe = 0.;
      Ib_hdwe_model = 0.;
      sclr_coul_eff = 1.;
  }
}

// Make final assignemnts
void Sensors::final_assignments(BatteryMonitor *Mon)
{
  // Reselect since may be changed
  // Inputs:  ib_sel_stat_, Ib_amp_hdwe, Ib_noa_hdwe, Ib_amp_model, Ib_noa_model
  // Outputs:  Ib_hdwe_model, Ib_hdwe, sclr_coul_eff, Vshunt
  choose_();

  // Final assignments
  // tb
  if ( rp.mod_tb() )
  {
    if ( Flt->tb_fa() )
    {
      Tb = NOMINAL_TB;
      Tb_filt = NOMINAL_TB;

    }
    else
    {
      Tb = RATED_TEMP + Tb_noise();
      Tb_filt = RATED_TEMP;
    }
  }
  else
  {
    if ( Flt->tb_fa() )
    {
      Tb = NOMINAL_TB;
      Tb_filt = NOMINAL_TB;
    }
    else
    {
      Tb = Tb_hdwe;
      Tb_filt = Tb_hdwe_filt;
    }
  }

  // vb
  if ( rp.mod_vb() )
  {
    Vb = Vb_model + Vb_noise();
  }
  else
  {
    if ( Flt->wrap_vb_fa() || Flt->vb_fa() )
    {
      Vb = Vb_model;
    }
    else
    {
      Vb = Vb_hdwe;
    }
  }
  
  // ib
  if ( rp.mod_ib() )
  {
    Ib = Ib_hdwe_model;
  }
  else
  {
    Ib = Ib_hdwe;
  }

  // print_signal_select
  if ( rp.debug==26 && cp.publishS )
  {
      double cTime;
      if ( rp.tweak_test() ) cTime = double(now)/1000.;
      else cTime = control_time;
      sprintf(cp.buffer, "unit_sel,%13.3f, %d, %d,  %d, %d,  %10.7f,  %7.5f,%7.5f,%7.5f,%7.5f,%7.5f,  %7.5f,%7.5f, ",
          cTime, reset, rp.ib_select,
          ShuntAmp->bare(), ShuntNoAmp->bare(),
          Flt->cc_diff(),
          Ib_amp_hdwe, Ib_noa_hdwe, Ib_amp_model, Ib_noa_model, Ib_model, 
          Flt->ib_diff(), Flt->ib_diff_f());
      Serial.print(cp.buffer);
      sprintf(cp.buffer, "  %7.5f,%7.5f,%7.5f,  %d, %7.5f,%7.5f, %d, %7.5f,  %d, %7.5f,%7.5f, %d, %7.5f,  %5.2f,%5.2f, %d, %5.2f, ",
          Mon->voc_soc(), Flt->e_wrap(), Flt->e_wrap_filt(),
          Flt->ib_sel_stat(), Ib_hdwe, Ib_hdwe_model, rp.mod_ib(), Ib,
          Flt->vb_sel_stat(), Vb_hdwe, Vb_model, rp.mod_vb(), Vb,
          Tb_hdwe, Tb, rp.mod_tb(), Tb_filt);
      Serial.print(cp.buffer);
      sprintf(cp.buffer, "%d, %d, %7.3f, %7.3f, %d,",
          Flt->fltw(), Flt->falw(), Flt->ib_rate(), Flt->ib_quiet(), Flt->tb_sel_status());
      Serial.print(cp.buffer);
      Serial.printlnf("%c,", '\0');
  }
}

// Tb noise
float Sensors::Tb_noise()
{
  if ( Tb_noise_amp_==0. ) return ( 0. );
  uint8_t raw = Prbn_Tb_->calculate();
  float noise = (float(raw)/127. - 0.5) * Tb_noise_amp_;
  return ( noise );
}

// Vb noise
float Sensors::Vb_noise()
{
  if ( Vb_noise_amp_==0. ) return ( 0. );
  uint8_t raw = Prbn_Vb_->calculate();
  float noise = (float(raw)/127. - 0.5) * Vb_noise_amp_;
  return ( noise );
}

// Ib noise
float Sensors::Ib_amp_noise()
{
  if ( Ib_amp_noise_amp_==0. ) return ( 0. );
  uint8_t raw = Prbn_Ib_amp_->calculate();
  float noise = (float(raw)/125. - 0.5) * Ib_amp_noise_amp_;
  return ( noise );
}
float Sensors::Ib_noa_noise()
{
  if ( Ib_noa_noise_amp_==0. ) return ( 0. );
  uint8_t raw = Prbn_Ib_noa_->calculate();
  float noise = (float(raw)/125. - 0.5) * Ib_noa_noise_amp_;
  return ( noise );
}

// Current bias.  Feeds into signal conversion
void Sensors::shunt_bias(void)
{
  if ( !rp.mod_ib() )
  {
    ShuntAmp->bias( rp.ib_bias_amp + rp.ib_bias_all + rp.inj_bias );
    ShuntNoAmp->bias( rp.ib_bias_noa + rp.ib_bias_all + rp.inj_bias );
  }
}

// Read and convert shunt Sensors
void Sensors::shunt_load(void)
{
    ShuntAmp->load();
    ShuntNoAmp->load();
}

// Print Shunt selection data
void Sensors::shunt_print()
{
    Serial.printf("reset,T,select,inj_bias,vs_int_a,Vshunt_a,Ib_hdwe_a,vs_int_na,Vshunt_na,Ib_hdwe_na,Ib_hdwe,T,sclr_coul_eff,Ib_amp_fault,Ib_amp_fail,Ib_noa_fault,Ib_noa_fail,=,    %d,%7.3f,%d,%7.3f,    %d,%7.3f,%7.3f,    %d,%7.3f,%7.3f,    %7.3f,%7.3f,  %7.3f, %d,%d,  %d,%d,\n",
        reset, T, rp.ib_select, rp.inj_bias, ShuntAmp->vshunt_int(), ShuntAmp->vshunt(), ShuntAmp->ishunt_cal(),
        ShuntNoAmp->vshunt_int(), ShuntNoAmp->vshunt(), ShuntNoAmp->ishunt_cal(),
        Ib_hdwe, T, sclr_coul_eff,
        Flt->ib_amp_flt(), Flt->ib_amp_fa(), Flt->ib_noa_flt(), Flt->ib_noa_fa());
}

// Current scale.  Feeds into signal conversion
void Sensors::shunt_scale(void)
{
  if ( !rp.mod_ib() )
  {
    ShuntAmp->scale( rp.ib_scale_amp );
    ShuntNoAmp->scale( rp.ib_scale_noa );
  }
}

// Shunt selection.  Use Coulomb counter and EKF to sort three signals:  amp current, non-amp current, voltage
// Initial selection to charge the Sim for modeling currents on BMS cutback
// Inputs: rp.ib_select (user override), Mon (EKF status)
// States:  Ib_fail_noa_
// Outputs:  Ib_hdwe, Ib_model_in, Vb_sel_status_
void Sensors::shunt_select_initial()
{
    // Current signal selection, based on if there or not.
    // Over-ride 'permanent' with Talk(rp.ib_select) = Talk('s')

    // Retrieve values.   Return values include scalar/adder for fault
    Ib_amp_model = ShuntAmp->bias_any( Ib_model );      // uses past Ib
    Ib_noa_model = ShuntNoAmp->bias_any( Ib_model );  // uses pst Ib
    Ib_amp_hdwe = ShuntAmp->ishunt_cal();
    Ib_noa_hdwe = ShuntNoAmp->ishunt_cal();

    // Initial choice
    // Inputs:  ib_sel_stat_, Ib_amp_hdwe, Ib_noa_hdwe, Ib_amp_model(past), Ib_noa_model(past)
    // Outputs:  Ib_hdwe_model, Ib_hdwe, sclr_coul_eff, Vshunt
    choose_();

    // Check for modeling
    if ( rp.mod_ib() )
        Ib_model_in = rp.inj_bias + rp.ib_bias_all;
    else
        Ib_model_in = Ib_hdwe;
}

// Filter temp
void Sensors::temp_filter(const boolean reset_loc, const float t_rlim)
{
    // Rate limit the temperature bias, 2x so not to interact with rate limits in logic that also use t_rlim
    if ( reset_loc ) tb_bias_last_ = *rp_tb_bias_;
    float t_bias_loc = max(min(*rp_tb_bias_,  tb_bias_last_ + t_rlim*2.*T_temp),
                                            tb_bias_last_ - t_rlim*2.*T_temp);
    tb_bias_last_ = t_bias_loc;

    // Filter and add rate limited bias
    if ( reset_loc && Tb>40. )  // Bootup T=85.5 C
    {
        Tb_hdwe = RATED_TEMP + t_bias_loc;
        Tb_hdwe_filt = TbSenseFilt->calculate(RATED_TEMP, reset_loc, min(T_temp, F_MAX_T_TEMP)) + t_bias_loc;
    }
    else
    {
        Tb_hdwe_filt = TbSenseFilt->calculate(Tb_hdwe, reset_loc, min(T_temp, F_MAX_T_TEMP)) + t_bias_loc;
        Tb_hdwe += t_bias_loc;
    }
    if ( rp.debug==16 ) Serial.printf("reset_loc,t_bias_loc, RATED_TEMP, Tb_hdwe, Tb_hdwe_filt, %d, %7.3f, %7.3f, %7.3f, %7.3f,\n",
      reset_loc,t_bias_loc, RATED_TEMP, Tb_hdwe, Tb_hdwe_filt );
}

// Filter temp
void Sensors::temp_load_and_filter(Sensors *Sen, const boolean reset_loc, const float t_rlim)
{
    Tb_hdwe = SensorTb->load(Sen);
    temp_filter(reset_loc, T_RLIM);
}

// Load analog voltage
void Sensors::vb_load(const byte vb_pin)
{
    Vb_raw = analogRead(vb_pin);
    Vb_hdwe =  float(Vb_raw)*VB_CONV_GAIN + float(VBATT_A) + rp.vb_bias;
}

// Print analog voltage
void Sensors::vb_print()
{
  Serial.printf("reset, T, Vb_raw, rp.vb_bias, Vb_hdwe, vb_flt(), vb_fa(), wv_fa=, %d, %7.3f, %d, %7.3f,  %7.3f, %d, %d, %d,\n",
    reset, T, Vb_raw, rp.vb_bias, Vb_hdwe, Flt->vb_flt(), Flt->vb_fa(), Flt->wrap_vb_fa());
}
