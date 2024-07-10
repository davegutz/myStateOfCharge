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

#include "application.h"
#include "command.h"
#include "Sensors.h"
#include "constants.h"
#include <math.h>
#include "debug.h"
#include "Summary.h"

extern CommandPars cp;  // Various parameters shared at system level
extern PrinterPars pr;  // Print buffer
extern PublishPars pp;  // For publishing
extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle

// Print bitmap utility
void bitMapPrint(char *buf, const int16_t fw, const uint8_t num)
{
  for ( int i=0; i<num; i++ )
  {
    if ( bitRead(fw, i) ) buf[num-i-1] = '1';
    else  buf[num-i-1] = '0';
  }
  buf[num] = '\0';
}


// class TempSensor
// constructors
TempSensor::TempSensor(const uint16_t pin, const bool parasitic, const uint16_t conversion_delay)
: DS18B20(pin, true, conversion_delay), tb_stale_flt_(true)
{
   SdTb = new SlidingDeadband(HDB_TBATT);
   Serial.printf("DS18 1-wire Tb started\n");
}
TempSensor::TempSensor(const uint16_t pin, const bool parasitic, const uint16_t conversion_delay, const uint16_t VTb_pin)
: DS18B20(pin, true, conversion_delay), tb_stale_flt_(true), VTb_pin_(VTb_pin)
{
   SdTb = new SlidingDeadband(HDB_TBATT);
   Serial.printf("DS18 1-wire Tb started\n");
}
TempSensor::~TempSensor() {}
// operators
// functions
float TempSensor::sample(Sensors *Sen)
{
  Log.info("top TempSensor::sample");
  // Read Sensor
  // MAXIM conversion 1-wire Tp plenum temperature
  static float Tb_hdwe = 0.;
  #ifdef HDWE_DS18B20_SWIRE
    uint8_t count = 0;
    float temp = 0.;
    // Read hardware and check
    while ( ++count<MAX_TEMP_READS && temp==0 && !sp.mod_tb_dscn() )
    {
        if ( crcCheck() ) temp = getTemperature() + (TBATT_TEMPCAL);
      delay(1);
    }

    // Check success
    if ( count<MAX_TEMP_READS && TEMP_RANGE_CHECK<temp && temp<TEMP_RANGE_CHECK_MAX && !ap.fail_tb )
    {
      Tb_hdwe = SdTb->update(temp);
      tb_stale_flt_ = false;
      if ( sp.debug()==16 ) Serial.printf("I:  t=%7.3f ct=%d, Tb_hdwe=%7.3f,\n", temp, count, Tb_hdwe);
    }
    else
    {
      Serial.printf("DS18 1-wire Tb, t=%8.1f, ct=%d, sending Tb_hdwe=%8.1f\n", temp, count, Tb_hdwe);
      tb_stale_flt_ = true;
      // Using last-good-value:  no assignment
    }
  #elif defined(HDWE_DS2482_1WIRE)
    // Check success

    if ( cp.tb_info.ready && TEMP_RANGE_CHECK<cp.tb_info.t_c && cp.tb_info.t_c<TEMP_RANGE_CHECK_MAX && !ap.fail_tb )
    {
      Tb_hdwe = SdTb->update(cp.tb_info.t_c);
      tb_stale_flt_ = false;
      if ( sp.debug()==16 ) Serial.printf("I:  t=%7.3f ready=%d, Tb_hdwe=%7.3f,\n", cp.tb_info.t_c, cp.tb_info.ready, Tb_hdwe);
    }
    else
    {
      if ( sp.debug()>0 )
        Serial.printf("DS18 1-wire Tb, t=%8.1f, ready=%d, sending Tb_hdwe=%8.1f\n", cp.tb_info.t_c, cp.tb_info.ready, Tb_hdwe);
      tb_stale_flt_ = true;
      // Using last-good-value:  no assignment
    }
  #elif defined(HDWE_2WIRE)
    float volt = float(analogRead(VTb_pin_))*VTB_CONV_GAIN;
    Tb_hdwe = float(HDWE_M_2WIRE) * log10( volt * float(HDWE_RS_2WIRE) / (V3V3 - volt) ) + float(HDWE_B_2WIRE);
    tb_stale_flt_ = false;
    if ( sp.debug()==16 ) Serial.printf("I 2wire:  volt=%7.3f Tb_hdwe=%7.3f,\n", volt, Tb_hdwe);
  #endif
  return ( Tb_hdwe );
}


// class Shunt
// constructors
Shunt::Shunt()
: Adafruit_ADS1015(), name_("None"), port_(0x00), bare_detected_(false){}
Shunt::Shunt(const String name, const uint8_t port, float *sp_ib_scale,  float *sp_Ib_bias, const float v2a_s,
  const uint8_t vc_pin, const uint8_t vo_pin, const uint8_t vh3v3_pin, const boolean using_opAmp)
: Adafruit_ADS1015(),
  name_(name), port_(port), bare_detected_(false), v2a_s_(v2a_s),
  vshunt_int_(0), vshunt_int_0_(0), vshunt_int_1_(0), vshunt_(0), Ishunt_cal_(0), Ishunt_cal_filt_(0),
  sp_ib_bias_(sp_Ib_bias), sp_ib_scale_(sp_ib_scale), sample_time_(0UL), sample_time_z_(0UL), dscn_cmd_(false),
  vc_pin_(vc_pin), vo_pin_(vo_pin), vr_pin_(vh3v3_pin), Vc_raw_(HALF_V3V3/VH3V3_CONV_GAIN), Vc_(HALF_V3V3),
  Vo_Vc_(0.), using_opamp_(using_opAmp)
{
  #ifdef HDWE_ADS1013_AMP_NOA
    if ( name_=="No Amp")
      setGain(GAIN_SIXTEEN, GAIN_SIXTEEN); // 16x gain differential and single-ended  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
    else if ( name_=="Amp")
      setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
    else
      setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
    if (!begin(port_)) {
      Serial.printf("FAILED init ADS SHUNT MON %s\n", name_.c_str());
      #ifndef HDWE_BARE
        bare_detected_ = true;
      #else
        bare_detected_ = false;
      #endif
    }
    else Serial.printf("SHUNT MON %s started\n", name_.c_str());
  #else
    if ( using_opamp_ ) Serial.printf("Ib %s sense ADC pin %d started using OpAmp and 3V3 pin %d\n", name_.c_str(), vo_pin_, vr_pin_);
    else Serial.printf("Ib %s sense ADC pins %d and %d started\n", name_.c_str(), vo_pin_, vc_pin_);
  #endif
  Filt_ = new General2_Pole(0.1, F_W_I, F_Z_I, -NOM_UNIT_CAP*sp.nP(), NOM_UNIT_CAP*sp.nP());  // actual update time provided run time
}
Shunt::~Shunt() {}
// operators
// functions

void Shunt::pretty_print()
{
#ifndef SOFT_DEPLOY_PHOTON
  Serial.printf(" *sp_Ib_bias%7.3f; A\n", *sp_ib_bias_);
  Serial.printf(" *sp_ib_scale%7.3f; A\n", *sp_ib_scale_);
  Serial.printf(" bare_det %d dscn_cmd %d\n", bare_detected_, dscn_cmd_);
  Serial.printf(" Ishunt_cal%7.3f; A\n", Ishunt_cal_);
  Serial.printf(" Ishunt_cal_filt%7.3f; A\n", Ishunt_cal_filt_);
  Serial.printf(" port 0x%X;\n", port_);
  Serial.printf(" v2a_s%7.2f; A/V\n", v2a_s_);
  Serial.printf(" Vc%10.6f; V\n", Vc_);
  Serial.printf(" Vc_raw %d;\n", Vc_raw_);
  Serial.printf(" Vo%10.6f; V\n", Vo_);
  Serial.printf(" Vo-Vc%10.6f; V\n", Vo_-Vc_);
  Serial.printf(" Vo_raw %d;\n", Vo_raw_);
  Serial.printf(" vshunt_int %d; count\n", vshunt_int_);
  Serial.printf("Shunt(%s)::\n", name_.c_str());
  // Serial.printf("Shunt(%s)::", name_.c_str()); Adafruit_ADS1015::pretty_print(name_);
#else
     Serial.printf("Shunt: silent DEPLOY\n");
#endif
}

// Convert sampled shunt data to Ib engineering units
void Shunt::convert(const boolean disconnect, const boolean reset, Sensors *Sen)
{
  #ifdef HDWE_ADS1013_AMP_NOA
    if ( !bare_detected_ && !dscn_cmd_ )
    {
      #ifndef HDWE_BARE
        vshunt_int_ = readADC_Differential_0_1(name_);
      #else
        vshunt_int_ = 0;
      #endif
      sample_time_z_ = sample_time_;
      sample_time_ = System.millis();
    }
    else
    {
      vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0;
    }
    vshunt_ = computeVolts(vshunt_int_);
  #else
    #ifndef HDWE_BARE
      bare_detected_ = Vc_ < VC_BARE_DETECTED;
    #else
      bare_detected_ = false;
    #endif
    if ( !bare_detected_ && !dscn_cmd_ )
    {
      vshunt_ = Vo_Vc_;
      vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0;
    }
    else
    {
      vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0; vshunt_ = 0.;
      Vc_raw_ = 0; Vc_ = 0.; Vo_raw_ = 0; Vo_ = 0.;
      Ishunt_cal_ = 0.;
    }
  #endif
  if ( disconnect )
    Ishunt_cal_ = 0.;
  else
    Ishunt_cal_ = vshunt_*v2a_s_*(*sp_ib_scale_) + *sp_ib_bias_;

      // 2-pole filter
  Ishunt_cal_filt_ = Filt_->calculate(Ishunt_cal_, disconnect || reset, min(Sen->T, MAX_T_Q_FILT));
  if ( Ishunt_cal_filt_ < 0. ) Ishunt_cal_ *= sp.ib_disch_slr();

}

// Sample amplifier Vo-Vc
void Shunt::sample(const boolean reset_loc, const float T)
{
  sample_time_z_ = sample_time_;
  if ( using_opamp_ )
  {
    Vc_raw_ = analogRead(vr_pin_);
    Vc_ =  float(Vc_raw_)*VH3V3_CONV_GAIN + ap.vc_add;
  }
  else
  {
    Vc_raw_ = analogRead(vc_pin_);
    Vc_ =  float(Vc_raw_)*VC_CONV_GAIN + ap.vc_add;
  }
  sample_time_ = System.millis();
  Vo_raw_ = analogRead(vo_pin_);
  Vo_ =  float(Vo_raw_)*VO_CONV_GAIN;
  Vo_Vc_ = Vo_ - Vc_;
  #ifndef HDWE_PHOTON
    if  ( sp.debug()==14 )Serial.printf("ADCref %7.3f samp_t %lld vo_pin_%d V0_raw_%d Vo_%7.3f Vo_Vc_%7.3f Vc_%7.3f\n", (float)analogGetReference(), sample_time_, vo_pin_, Vo_raw_, Vo_, Vo_Vc_, Vc_);
  #endif
}


// Class Looparound
Looparound::Looparound(BatteryMonitor *Mon, Sensors *Sen):
  chem_(Mon->chem()), e_wrap_(0.), e_wrap_filt_(0.), e_wrap_trimmed_(0.), ib_(0.), Mon_(Mon), Sen_(Sen), voc_(0.)
{
  ChargeTransfer_ = new LagExp(EKF_NOM_DT, chem_->tau_ct, -NOM_UNIT_CAP, NOM_UNIT_CAP);     // actual update time provided run time
  Trim_ = new TustinIntegrator(EKF_NOM_DT, -MAX_WRAP_ERR_FILT, MAX_WRAP_ERR_FILT);          // actual update time provided run time
  WrapErrFilt_ = new LagTustin(2., WRAP_ERR_FILT, -MAX_WRAP_ERR_FILT, MAX_WRAP_ERR_FILT);   // actual update time provided run time
  WrapHi_ = new TFDelay(false, WRAP_HI_S, WRAP_HI_R, EKF_NOM_DT);  // Wrap test persistence.  Initializes false
  WrapLo_ = new TFDelay(false, WRAP_LO_S, WRAP_LO_R, EKF_NOM_DT);  // Wrap test persistence.  Initializes false
}

// Absorb states from leader
void Looparound::absorb_states(Looparound *Leader)
{
  ib_ = Leader->ib_;
  ChargeTransfer_->absorb(Leader->ChargeTransfer_);
}

// Update the loop
void Looparound::calculate(const boolean reset, const float ib, Looparound *Leader, const bool follow, const double loop_gain)
{
  following_ = follow;
  reset_ = reset | Sen_->Flt->reset_all_faults() | following_;
  if (following_)
    absorb_states(Leader);
  else
    ib_ = ib;
  voc_ = Mon_->vb() - (ChargeTransfer_->calculate(ib_, reset_, chem_->tau_ct, Sen_->T)*chem_->r_ct*ap.slr_res + ib_*chem_->r_0*ap.slr_res);
  e_wrap_ = Mon_->voc_soc() - voc_;
  e_wrap_trimmed_ = e_wrap_ - Trim_->calculate(e_wrap_filt_*float(AMP_WRAP_TRIM_GAIN), reset_, e_wrap_);
  e_wrap_filt_ = WrapErrFilt_->calculate(e_wrap_trimmed_, reset_, min(Sen_->T, F_MAX_T_WRAP));
  // sat logic screens out voc jumps when ib>0 when saturated
  // wrap_hi and wrap_lo don't latch because need them available to check next ib sensor selection for dual ib sensor
  // wrap_vb latches because vb is single sensor  faultAssign( (e_wrap_filt_ >= ewhi_thr_ && !Mon->sat()), WRAP_HI_FLT);

  hi_fault_ = e_wrap_filt_ >= Sen_->Flt->ewhi_thr();
  hi_fail_ = WrapHi_->calculate(hi_fault_, WRAP_HI_S, WRAP_HI_R, Sen_->T, reset_) && !Sen_->Flt->vb_fa();  // non-latching
  lo_fault_ = e_wrap_filt_ <= Sen_->Flt->ewlo_thr();
  lo_fail_ = WrapLo_->calculate(lo_fault_, WRAP_LO_S, WRAP_LO_R, Sen_->T, reset_) && !Sen_->Flt->vb_fa();  // non-latching
}

void Looparound::pretty_print()
{
  Serial.printf(" following %d\n", following_);
  Serial.printf(" reset %d\n", reset_);
  Serial.printf(" ib%7.3f A\n", ib_);
  Serial.printf(" voc%7.3f V\n", voc_);
  Serial.printf(" e_wrap%7.3f V\n", e_wrap_);
  Serial.printf(" e_wrap_f%7.3f V\n", e_wrap_filt_);
  Serial.printf(" hi_fault/fail %d/%d\n", hi_fault_, hi_fail_);
  Serial.printf(" lo_fault/fail %d/%d\n", lo_fault_, lo_fail_);
}


// Class Fault
Fault::Fault(const double T, uint8_t *preserving, BatteryMonitor *Mon, Sensors *Sen):
  cc_diff_(0.), cc_diff_empty_slr_(1), ewmin_slr_(1), ewsat_slr_(1), e_wrap_(0), e_wrap_filt_(0),
  ib_diff_(0), ib_diff_f_(0), ib_lo_active_(true), ib_quiet_(0), ib_rate_(0), latched_fail_(false), 
  latched_fail_fake_(false), tb_sel_stat_(1), vb_sel_stat_(1), ib_sel_stat_(1), reset_all_faults_(false),
  tb_sel_stat_last_(1), vb_sel_stat_last_(1), ib_sel_stat_last_(1), fltw_(0UL), falw_(0UL), sp_preserving_(preserving)
{
  IbErrFilt = new LagTustin(T, TAU_ERR_FILT, -MAX_ERR_FILT, MAX_ERR_FILT);  // actual update time provided run time
  IbdHiPer = new TFDelay(false, IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, T);
  IbdLoPer = new TFDelay(false, IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, T);
  IbAmpHardFail  = new TFDelay(false, IB_HARD_SET, IB_HARD_RESET, T);
  IbLoActive  = new TFDelay(true, IB_LO_ACTIVE_SET, IB_LO_ACTIVE_RESET, T);
  IbNoAmpHardFail  = new TFDelay(false, IB_HARD_SET, IB_HARD_RESET, T);
  TbHardFail  = new TFDelay(false, TB_HARD_SET, TB_HARD_RESET, T);
  TbStaleFail  = new TFDelay(false, TB_STALE_SET, TB_STALE_RESET, T);
  VbHardFail  = new TFDelay(false, VB_HARD_SET, VB_HARD_RESET, T);
  VcHardFail  = new TFDelay(false, VC_HARD_SET, VC_HARD_RESET, T);
  QuietPer  = new TFDelay(false, QUIET_S, QUIET_R, T);
  WrapErrFilt = new LagTustin(T, WRAP_ERR_FILT, -MAX_WRAP_ERR_FILT, MAX_WRAP_ERR_FILT);  // actual update time provided run time
  WrapHi = new TFDelay(false, WRAP_HI_S, WRAP_HI_R, EKF_NOM_DT);  // Wrap test persistence.  Initializes false
  WrapLo = new TFDelay(false, WRAP_LO_S, WRAP_LO_R, EKF_NOM_DT);  // Wrap test persistence.  Initializes false
  QuietFilt = new General2_Pole(T, WN_Q_FILT, ZETA_Q_FILT, MIN_Q_FILT, MAX_Q_FILT);  // actual update time provided run time
  QuietRate = new RateLagExp(T, TAU_Q_FILT, MIN_Q_FILT, MAX_Q_FILT);
  LoopIbAmp = new Looparound(Mon, Sen);
  LoopIbNoa = new Looparound(Mon, Sen);
}

// Coulomb Counter difference test - failure conditions track poorly
void Fault::cc_diff(Sensors *Sen, BatteryMonitor *Mon)
{
  cc_diff_ = Mon->soc_ekf() - Mon->soc(); // These are filtered in their construction (EKF is a dynamic filter and 
                                          // Coulomb counter is wrapa big integrator)
  if ( Mon->soc() <= max(Mon->soc_min()+WRAP_SOC_LO_OFF_REL, WRAP_SOC_LO_OFF_ABS) )
  {
    cc_diff_empty_slr_ = CC_DIFF_LO_SOC_SLR;
  }
  else
  {
    cc_diff_empty_slr_ = 1.;
  }
  // ewsat_slr_ used here because voc_soc map inaccurate on cold days
  cc_diff_thr_ = CC_DIFF_SOC_DIS_THRESH*ap.cc_diff_slr*cc_diff_empty_slr_*ewsat_slr_;
  failAssign( abs(cc_diff_)>=cc_diff_thr_ , CC_DIFF_FA );  // Not latched
}

// Compare current sensors - failure conditions large difference
void Fault::ib_diff(const boolean reset, Sensors *Sen, BatteryMonitor *Mon)
{
  boolean reset_loc = reset || reset_all_faults_;

  if ( !ib_lo_active_ && !reset_loc ) return;  // Logic freezes when ib_lo_active_ is false

  // Difference error, filter, check, persist, doesn't latch
  if ( sp.mod_ib() )
  {
    ib_diff_ = Sen->ib_amp_model() - Sen->ib_noa_model();
  }
  else
  {
    ib_diff_ = Sen->ib_amp_hdwe() - Sen->ib_noa_hdwe();
  }
  ib_diff_f_ = IbErrFilt->calculate(ib_diff_, reset_loc, min(Sen->T, MAX_ERR_T));
  ib_diff_thr_ = IBATT_DISAGREE_THRESH*ap.ib_diff_slr;
  faultAssign( ib_diff_f_>=ib_diff_thr_, IB_DIFF_HI_FLT );
  faultAssign( ib_diff_f_<=-ib_diff_thr_, IB_DIFF_LO_FLT );
  failAssign( IbdHiPer->calculate(ib_diff_hi_flt(), IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, Sen->T, reset_loc), IB_DIFF_HI_FA ); // IB_DIFF_FA not latched
  failAssign( IbdLoPer->calculate(ib_diff_lo_flt(), IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, Sen->T, reset_loc), IB_DIFF_LO_FA ); // IB_DIFF_FA not latched
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
  ib_quiet_thr_ = QUIET_A * ap.ib_quiet_slr;
  faultAssign( !sp.mod_ib() && abs(ib_quiet_)<=ib_quiet_thr_ && !reset_loc, IB_DSCN_FLT );   // initializes false
  failAssign( QuietPer->calculate(dscn_flt(), QUIET_S, QUIET_R, Sen->T, reset_loc), IB_DSCN_FA);
  #ifndef HDWE_PHOTON
    if ( sp.debug()==-13 ) debug_m13(Sen);
    if ( sp.debug()==-23 ) debug_m23(Sen);
    if ( sp.debug()==-24 ) debug_m24(Sen);
  #endif
}

// Voltage wraparound logic for current selection
// Avoid using hysteresis data for this test and accept more generous thresholds
void Fault::ib_wrap(const boolean reset, Sensors *Sen, BatteryMonitor *Mon)
{
  boolean reset_loc = reset | reset_all_faults_;
  e_wrap_ = Mon->voc_soc() - Mon->voc_stat();
  // e_wrap_ = Mon->y_ekf();
  e_wrap_filt_ = WrapErrFilt->calculate(e_wrap_, reset_loc, min(Sen->T, F_MAX_T_WRAP));
  // sat logic screens out voc jumps when ib>0 when saturated
  // wrap_hi and wrap_lo don't latch because need them available to check next ib sensor selection for dual ib sensor
  // wrap_vb latches because vb is single sensor
  // Thresholds calculated by wrap_scalars()
  faultAssign( (e_wrap_filt_ >= ewhi_thr_ && !Mon->sat()), WRAP_HI_FLT);
  faultAssign( (e_wrap_filt_ <= ewlo_thr_), WRAP_LO_FLT);
  failAssign( (WrapHi->calculate(wrap_hi_flt(), WRAP_HI_S, WRAP_HI_R, Sen->T, reset_loc) && !vb_fa()), WRAP_HI_FA );  // non-latching
  failAssign( (WrapLo->calculate(wrap_lo_flt(), WRAP_LO_S, WRAP_LO_R, Sen->T, reset_loc) && !vb_fa()), WRAP_LO_FA );  // non-latching
  failAssign( (wrap_vb_fa() && !reset_loc) || (!ib_diff_fa() && wrap_fa()), WRAP_VB_FA);    // latches
}

void Fault::pretty_print(Sensors *Sen, BatteryMonitor *Mon)
{
  Serial.printf("\nLooparound Amp:\n");
  LoopIbAmp->pretty_print();
  Serial.printf("\nLooparound Noa:\n");
  LoopIbNoa->pretty_print();

  Serial.printf("\nFault:\n");
  Serial.printf(" cc_diff  %7.3f  thr=%7.3f Fc^\n", cc_diff_, cc_diff_thr_);
  Serial.printf(" ib_diff  %7.3f  thr=%7.3f Fd^\n", ib_diff_f_, ib_diff_thr_);
  Serial.printf(" e_wrap   %7.3f  thr=%7.3f Fo^%7.3f Fi^\n", e_wrap_filt_, ewlo_thr_, ewhi_thr_);
  Serial.printf(" ib_quiet %7.3f  thr=%7.3f Fq v\n\n", ib_quiet_, ib_quiet_thr_);

  Serial.printf(" soc  %7.3f  soc_inf %7.3f voc %7.3f  voc_soc %7.3f\n", Mon->soc(), Mon->soc_inf(), Mon->voc(), Mon->voc_soc());
  Serial.printf(" dis_tb_fa %d  dis_vb_fa %d  dis_ib_fa %d\n", ap.disab_tb_fa, ap.disab_vb_fa, ap.disab_ib_fa);
  Serial.printf(" bms_off   %d\n\n", Mon->bms_off());

  Serial.printf(" Tbh=%7.3f  Tbm=%7.3f sel %7.3f\n", Sen->Tb_hdwe, Sen->Tb_model, Sen->Tb);
  Serial.printf(" Vbh %7.3f  Vbm %7.3f sel %7.3f\n", Sen->Vb_hdwe, Sen->Vb_model, Sen->Vb);
  Serial.printf(" V3v3%7.3f \n", Sen->ShuntAmp->Vc()*2.);
  Serial.printf(" Imh %7.3f  Imm %7.3f Ib %7.3f\n", Sen->Ib_amp_hdwe, Sen->Ib_amp_model, Sen->Ib);
  Serial.printf(" Inh %7.3f  Inm %7.3f Ib %7.3f\n", Sen->Ib_noa_hdwe, Sen->Ib_noa_model, Sen->Ib);
  Serial.printf(" Ibh %7.3f  Ibh %7.3f Ib %7.3f\n\n", Sen->Ib_hdwe, Sen->Ib_hdwe_model, Sen->Ib);

  Serial.printf(" mod_tb %d mod_vb %d mod_ib  %d\n", sp.mod_tb(), sp.mod_vb(), sp.mod_ib());
  Serial.printf(" mod_tb_dscn %d mod_vb_dscn %d mod_ib_amp_dscn %d mod_ib_noa_dscn %d\n", sp.mod_tb_dscn(), sp.mod_vb_dscn(), sp.mod_ib_amp_dscn(), sp.mod_ib_noa_dscn());
  Serial.printf(" tb_s_st %d  vb_s_st %d  ib_s_st %d\n", tb_sel_stat_, vb_sel_stat_, ib_sel_stat_);
  Serial.printf(" fake_faults %d latched_fail %d latched_fail_fake %d preserving %d\n\n", ap.fake_faults, latched_fail_, latched_fail_fake_, *sp_preserving_);

  Serial.printf(" wml     %d  %d 'Fo ^'\n", wrap_lo_m_flt(), wrap_lo_m_fa());
  Serial.printf(" wmh     %d  %d 'Fi ^'\n", wrap_hi_m_flt(), wrap_hi_m_fa());
  Serial.printf(" wnl     %d  %d 'Fo ^'\n", wrap_lo_n_flt(), wrap_lo_n_fa());
  Serial.printf(" wnh     %d  %d 'Fi ^'\n", wrap_hi_n_flt(), wrap_hi_n_fa());
  Serial.printf(" vc      %d  %d 'FI 1'\n", vc_flt(), vc_fa());
  Serial.printf(" bare n  %d  x \n", ib_noa_bare());
  Serial.printf(" bare m  %d  x \n", ib_amp_bare());
  Serial.printf(" ib_dsc  %d  %d 'Fq v'\n", ib_dscn_flt(), ib_dscn_fa());
  Serial.printf(" ibd_lo  %d  %d 'Fd ^  *SA/*SB'\n", ib_diff_lo_flt(), ib_diff_lo_fa());
  Serial.printf(" ibd_hi  %d  %d 'Fd ^  *SA/*SB'\n", ib_diff_hi_flt(), ib_diff_hi_fa());
  Serial.printf(" red wv  %d  %d   'Fd, Fi/Fo ^'\n",  red_loss(), wrap_vb_fa());
  Serial.printf(" wl      %d  %d 'Fo ^'\n", wrap_lo_flt(), wrap_lo_fa());
  Serial.printf(" wh      %d  %d 'Fi ^'\n", wrap_hi_flt(), wrap_hi_fa());
  Serial.printf(" vc | cc_dif %d  %d 'x Fc ^'\n", vc_fa(), cc_diff_fa());
  Serial.printf(" ib n    %d  %d 'FI 1'\n", ib_noa_flt(), ib_noa_fa());
  Serial.printf(" ib m    %d  %d 'FI 1'\n", ib_amp_flt(), ib_amp_fa());
  Serial.printf(" vb      %d  %d 'Fv 1  *SV, *Dc/*Dv'\n", vb_flt(), vb_fa());
  Serial.printf(" tb      %d  %d 'Ft 1'\n  ", tb_flt(), tb_fa());
  bitMapPrint(pr.buff, fltw_, NUM_FLT);
  Serial.print(pr.buff);
  Serial.printf("   ");
  bitMapPrint(pr.buff, falw_, NUM_FA);
  Serial.printf("%s\n", pr.buff);
  Serial.printf("  10FEDCBA9876543210   10FExxBA9876543210\n");
  Serial.printf("  fltw=%ld     falw=%ld\n", fltw_, falw_);
  if ( ap.fake_faults )
    Serial.printf("fake_faults=>redl\n");
}
void Fault::pretty_print1(Sensors *Sen, BatteryMonitor *Mon)
{
  Serial1.printf("Fault:\n");
  Serial1.printf(" cc_diff  %7.3f  thr=%7.3f Fc^\n", cc_diff_, cc_diff_thr_);
  Serial1.printf(" ib_diff  %7.3f  thr=%7.3f Fd^\n", ib_diff_f_, ib_diff_thr_);
  Serial1.printf(" e_wrap   %7.3f  thr=%7.3f Fo^%7.3f Fi^\n", e_wrap_filt_, ewlo_thr_, ewhi_thr_);
  Serial1.printf(" ib_quiet %7.3f  thr=%7.3f Fq v\n\n", ib_quiet_, ib_quiet_thr_);

  Serial1.printf(" soc  %7.3f  soc_inf %7.3f voc %7.3f  voc_soc %7.3f\n", Mon->soc(), Mon->soc_inf(), Mon->voc(), Mon->voc_soc());
  Serial1.printf(" dis_tb_fa %d  dis_vb_fa %d  dis_ib_fa %d\n", ap.disab_tb_fa, ap.disab_vb_fa, ap.disab_ib_fa);
  Serial1.printf(" bms_off   %d\n\n", Mon->bms_off());

  Serial1.printf(" Tbh=%7.3f  Tbm=%7.3f\n", Sen->Tb_hdwe, Sen->Tb_model);
  Serial1.printf(" Vbh %7.3f  Vbm %7.3f\n", Sen->Vb_hdwe, Sen->Vb_model);
  Serial1.printf(" V3v3 %7.3f \n", Sen->ShuntAmp->Vc()*2.);
  Serial1.printf(" Imh %7.3f  Imm %7.3f\n", Sen->Ib_amp_hdwe, Sen->Ib_amp_model);
  Serial1.printf(" Inh %7.3f  Inm %7.3f\n", Sen->Ib_noa_hdwe, Sen->Ib_noa_model);
  Serial1.printf(" Ibh %7.3f  Ibm %7.3f Ib %7.3f\n\n", Sen->Ib_hdwe, Sen->Ib_hdwe_model, Sen->Ib);

  Serial1.printf(" mod_tb  %d  mod_vb  %d  mod_ib  %d\n", sp.mod_tb(), sp.mod_vb(), sp.mod_ib());
  Serial1.printf(" tb_s_st %d  vb_s_st %d  ib_s_st %d\n", tb_sel_stat_, vb_sel_stat_, ib_sel_stat_);
  Serial1.printf(" fake_faults %d latched_fail %d latched_fail_fake %d preserving %d\n\n", ap.fake_faults, latched_fail_, latched_fail_fake_, *sp_preserving_);

  Serial1.printf(" wml     %d  %d 'Fo ^'\n", wrap_lo_m_flt(), wrap_lo_m_fa());
  Serial1.printf(" wmh     %d  %d 'Fi ^'\n", wrap_hi_m_flt(), wrap_hi_m_fa());
  Serial1.printf(" wnl     %d  %d 'Fo ^'\n", wrap_lo_n_flt(), wrap_lo_n_fa());
  Serial1.printf(" wnh     %d  %d 'Fi ^'\n", wrap_hi_n_flt(), wrap_hi_n_fa());
  Serial1.printf(" vc      %d  %d 'FI 1'\n", vc_flt(), vc_fa());
  Serial1.printf(" bare n  %d  x \n", Sen->ShuntNoAmp->bare_detected());
  Serial1.printf(" bare m  %d  x \n", Sen->ShuntAmp->bare_detected());
  Serial1.printf(" ib_dsc  %d  %d 'Fq v'\n", ib_dscn_flt(), ib_dscn_fa());
  Serial1.printf(" ibd_lo  %d  %d 'Fd ^  *SA/*SB'\n", ib_diff_lo_flt(), ib_diff_lo_fa());
  Serial1.printf(" ibd_hi  %d  %d 'Fd ^  *SA/*SB'\n", ib_diff_hi_flt(), ib_diff_hi_fa());
  Serial1.printf(" red wv  %d  %d   'Fd  Fi/Fo ^'\n",  red_loss(), wrap_vb_fa());
  Serial1.printf(" wl      %d  %d 'Fo ^'\n", wrap_lo_flt(), wrap_lo_fa());
  Serial1.printf(" wh      %d  %d 'Fi ^'\n", wrap_hi_flt(), wrap_hi_fa());
  Serial1.printf(" cc_dif      %d 'Fc ^'\n", cc_diff_fa());
  Serial1.printf(" ib n    %d  %d 'FI 1'\n", ib_noa_flt(), ib_noa_fa());
  Serial1.printf(" ib m    %d  %d 'FI 1'\n", ib_amp_flt(), ib_amp_fa());
  Serial1.printf(" vb      %d  %d 'Fv 1, *SV, *Dc/*Dv'\n", vb_flt(), vb_fa());
  Serial1.printf(" tb      %d  %d 'Ft 1'\n  ", tb_flt(), tb_fa());
  bitMapPrint(pr.buff, fltw_, NUM_FLT);
  Serial1.print(pr.buff);
  Serial1.printf("   ");
  bitMapPrint(pr.buff, falw_, NUM_FA);
  Serial1.printf("%s\n", pr.buff);
  Serial1.printf("  10FEDCBA9876543210   10FExxBA9876543210\n");
  Serial1.printf("  fltw=%ld     falw=%ld\n", fltw_, falw_);
  if ( ap.fake_faults )
    Serial1.printf("fake_faults=>redl\n");
  Serial1.printf("vv0; to return\n");
}

// Redundancy loss.   Here in cpp because sp circular reference in .h files due to sp.ib_select()
boolean Fault::red_loss_calc() { return (ib_sel_stat_!=1 || (sp.ib_select()!=0 && !ap.fake_faults)
  || ib_diff_fa() || vb_fail()); };

// Wrap scalars
void Fault::wrap_scalars(BatteryMonitor *Mon)
{
  if ( Mon->soc()>=WRAP_SOC_HI_OFF )
  {
    ewsat_slr_ = WRAP_SOC_HI_SLR;
    ewmin_slr_ = 1.;
  }
  else if ( Mon->soc() <= max(Mon->soc_min()+WRAP_SOC_LO_OFF_REL, WRAP_SOC_LO_OFF_ABS)  )
  {
    ewsat_slr_ = 1.;
    ewmin_slr_ = WRAP_SOC_LO_SLR;
  }
  else if ( Mon->voc_soc()>(Mon->vsat()-WRAP_HI_SAT_MARG) ||
          ( Mon->voc_stat()>(Mon->vsat()-WRAP_HI_SAT_MARG) && Mon->C_rate()>WRAP_MOD_C_RATE && Mon->soc()>WRAP_SOC_MOD_OFF) ) // Use voc_stat to get some anticipation
  {
    ewsat_slr_ = WRAP_HI_SAT_SLR;
    ewmin_slr_ = 1.;
  }
  else
  {
    ewsat_slr_ = 1.;
    ewmin_slr_ = 1.;
  }
  ewhi_thr_ = Mon->r_ss() * WRAP_HI_A * ap.ewhi_slr * ewsat_slr_ * ewmin_slr_;
  ewlo_thr_ = Mon->r_ss() * WRAP_LO_A * ap.ewlo_slr * ewsat_slr_ * ewmin_slr_;
}

// Calculate selection for choice
// Use model instead of sensors when running tests as user
// Equivalent to using voc(soc) as voter between two hardware currrents
// Over-ride sensed Ib, Vb and Tb with model when running tests
// Inputs:  Sen->Ib_model, Sen->Ib_hdwe,
//          Sen->Vb_model, Sen->Vb_hdwe,
//          ----------------, Sen->Tb_hdwe, Sen->Tb_hdwe_filt
// Outputs: Ib,
//          Vb,

//          Tb, Tb_filt
//          latched_fail_
void Fault::select_all(Sensors *Sen, BatteryMonitor *Mon, const boolean reset)
{
  // Reset
  if ( reset_all_faults_ )
  {
    if ( sp.ib_select() >= 0 )
    {
      ib_sel_stat_ = 1;
    }
    else
    {
      ib_sel_stat_ = -1;
    }
    ib_sel_stat_last_ =  ib_sel_stat_;

    Serial.printf("reset ib flt\n");
  }

  // Ib truth table
  if ( ap.fake_faults )
  {
    #ifdef HDWE_IB_HI_LO  
      ib_sel_stat_ = 0;
    #else
      ib_sel_stat_ = 1;
    #endif
    latched_fail_ = false;
  }
  else if ( Sen->ShuntAmp->bare_detected() && Sen->ShuntNoAmp->bare_detected() )  // these separate inputs don't latch
  {
    ib_sel_stat_ = 0;    // takes two non-latching inputs to set and latch
    latched_fail_ = true;
  }
  else if ( sp.ib_select()>0 && !Sen->ShuntAmp->bare_detected() )
  {
    ib_sel_stat_ = 1;
    latched_fail_ = true;
  }
  else if ( ib_sel_stat_last_==-1 && !Sen->ShuntNoAmp->bare_detected() )  // latches - use reset
  {
    ib_sel_stat_ = -1;
    latched_fail_ = true;
  }
  else if ( sp.ib_select()<0 && !Sen->ShuntNoAmp->bare_detected() )  // latches - use reset
  {
    ib_sel_stat_ = -1;
    latched_fail_ = true;
  }
  else if ( sp.ib_select()==0 )  // auto
  {
    if ( Sen->ShuntAmp->bare_detected() && !Sen->ShuntNoAmp->bare_detected() )  // these inputs don't latch
    {
      ib_sel_stat_ = -1;
      latched_fail_ = true;
    }
    else if ( ib_diff_fa() )  // this input doesn't latch
    {
      if ( vb_sel_stat_ && wrap_fa() )    // wrap_fa is non-latching
      {
        ib_sel_stat_ = -1;      // two non-latching fails
        latched_fail_ = true;
      }
      else if ( cc_diff_fa() )  // this input doesn't latch but result of 'and' with ib_diff_fa is latched
      {
        ib_sel_stat_ = -1;      // takes two non-latching inputs to isolate ib failure and to set and latch ib_sel
        latched_fail_ = true;
      }
    }
  }
  else if ( ( (sp.ib_select() <  0) && ib_sel_stat_last_>-1 ) ||
            ( (sp.ib_select() >= 0) && ib_sel_stat_last_< 1 )   )  // Latches.  Must reset to move out of no amp selection
  {
    latched_fail_ = true;
  }
  else
  {
    latched_fail_ = false;
  }

  // Fake faults.  Objective is to provide same recording behavior as normal operation so can see debug faults without shutdown of anything
  if ( ap.fake_faults )
  {
    if ( Sen->ShuntAmp->bare_detected() && Sen->ShuntNoAmp->bare_detected() )  // these separate inputs don't latch
    {
      latched_fail_fake_ = true;
    }
    else if ( ib_sel_stat_last_==-1 && !Sen->ShuntNoAmp->bare_detected() )  // latches
    {
      latched_fail_fake_ = true;
    }
    else if ( sp.ib_select()<0 && !Sen->ShuntNoAmp->bare_detected() )  // latches
    {
      latched_fail_fake_ = true;
    }
    else if ( Sen->ShuntAmp->bare_detected() && !Sen->ShuntNoAmp->bare_detected() )  // these inputs don't latch
    {
      latched_fail_fake_ = true;
    }
    else if ( ib_diff_fa() )  // this input doesn't latch
    {
      if ( vb_sel_stat_ && wrap_fa() )    // wrap_fa is non-latching
      {
        latched_fail_fake_ = true;
      }
      else if ( cc_diff_fa() )  // this input doesn't latch but result is latched
      {
        latched_fail_fake_ = true;
      }
    }
    else
      latched_fail_fake_ = false;
  }

  // Redundancy loss
  faultAssign(red_loss_calc(), RED_LOSS); // redundancy loss anytime ib_sel_stat<0
  if ( ap.fake_faults )
  {
    ib_sel_stat_ = sp.ib_select();  // Can manually select ib amp or noa using talk when ap.fake_faults is set
  }

  // vb failure from wrap result
  if ( reset_all_faults_ )
  {
    vb_sel_stat_last_ = 1;
    vb_sel_stat_ = 1;
    Serial.printf("reset vb flts\n");
  }
  if ( !ap.fake_faults )
  {
    if ( !vb_sel_stat_last_ )
    {
      vb_sel_stat_ = 0;   // Latches
      latched_fail_ = true;
    }
    if (  wrap_vb_fa() || vb_fa() )
    {
      vb_sel_stat_ = 0; // Latches
      latched_fail_ = true;
    }
  }
  else  // fake_faults
  {
    if ( !vb_sel_stat_last_ )
    {
      latched_fail_fake_ = true;
    }
    if (  wrap_vb_fa() || vb_fa() )
    {
      latched_fail_fake_ = true;
    }
  }

  // tb failure from inactivity. Does not latch because can heal and failure not critical
  if ( reset_all_faults_ )
  {
    tb_sel_stat_last_ = 1;
    tb_sel_stat_ = 1;
    Serial.printf("reset tb flts\n");
  }
  if ( tb_fa() )  // Latches
  {
    tb_sel_stat_ = 0;
    latched_fail_ = true;
  }
  else
    tb_sel_stat_ = 1;

  // Print
  if ( ib_sel_stat_ != ib_sel_stat_last_ || vb_sel_stat_ != vb_sel_stat_last_ || tb_sel_stat_ != tb_sel_stat_last_ )
  {
    Serial.printf("Sel chg:  Amp->bare %d NoAmp->bare %d ib_diff_fa %d wh_fa %d wl_fa %d wv_fa %d cc_diff_fa_ %d\n sp.ib_select() %d ib_sel_stat %d vb_sel_stat %d tb_sel_stat %d vb_fail %d Tb_fail %d\n",
      Sen->ShuntAmp->bare_detected(), Sen->ShuntNoAmp->bare_detected(), ib_diff_fa(), wrap_hi_fa(), wrap_lo_fa(), wrap_vb_fa(), cc_diff_fa_, sp.ib_select(), ib_sel_stat_, vb_sel_stat_, tb_sel_stat_, vb_fa(), tb_fa());
    Serial.printf("  fake %d ibss %d ibssl %d vbss %d vbssl %d tbss %d  tbssl %d latched_fail %d latched_fail_fake %d\n",
      ap.fake_faults, ib_sel_stat_, ib_sel_stat_last_, vb_sel_stat_, vb_sel_stat_last_, tb_sel_stat_, tb_sel_stat_last_, latched_fail_, latched_fail_fake_);
    Serial.printf("  preserving %d\n", *sp_preserving_);
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
      latched_fail_ = false;
      latched_fail_fake_ = false;
      preserving(false);
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
  faultAssign( Sen->ShuntAmp->bare_detected(), IB_AMP_BARE);
  faultAssign( Sen->ShuntNoAmp->bare_detected(), IB_NOA_BARE);
  #ifndef HDWE_BARE
    faultAssign( ( ib_amp_bare() || abs(Sen->ShuntAmp->Ishunt_cal()) >= IB_ABS_MAX_AMP ) && !ap.disab_ib_fa, IB_AMP_FLT );
    faultAssign( ( ib_noa_bare() || abs(Sen->ShuntNoAmp->Ishunt_cal()) >= IB_ABS_MAX_NOA ) && !ap.disab_ib_fa, IB_NOA_FLT );
  #else
    float current_max = NOM_UNIT_CAP * sp.nP();
    faultAssign( abs(Sen->ShuntAmp->Ishunt_cal()) >= current_max && !ap.disab_ib_fa, IB_AMP_FLT );
    faultAssign( abs(Sen->ShuntNoAmp->Ishunt_cal()) >= current_max && !ap.disab_ib_fa, IB_NOA_FLT );
  #endif
  if ( ap.disab_ib_fa )
  {
    failAssign( false, IB_AMP_FA );
    failAssign( false, IB_NOA_FA);
  }
  else
  {
    failAssign( vc_fa() || ib_amp_fa() || IbAmpHardFail->calculate(ib_amp_flt(), IB_HARD_SET, IB_HARD_RESET, Sen->T, reset_loc), IB_AMP_FA );
    failAssign( vc_fa() || ib_noa_fa() || IbNoAmpHardFail->calculate(ib_noa_flt(), IB_HARD_SET, IB_HARD_RESET, Sen->T, reset_loc), IB_NOA_FA);
  }
  #ifdef HDWE_HI_LO_AMP
    ib_lo_active_ = IbLoActive->calculate(HDWE_IB_HI_LO_AMP_LO < Sen->Ib_noa_hdwe && Sen->Ib_noa_hdwe < HDWE_IB_HI_LO_AMP_HI,
                                         IB_LO_ACTIVE_SET, IB_LO_ACTIVE_RESET, Sen->T , reset_loc);
  #endif
}

// Check Tb 2-wire analog voltage.  Latches
void Fault::tb_check(Sensors *Sen, const float _tb_min, const float _tb_max, const boolean reset)
{
  boolean reset_loc = reset | reset_all_faults_;
  if ( reset_loc )
  {
    failAssign(false, VB_FA);
  }
  if ( ap.disab_tb_fa || sp.mod_tb() )
  {
    faultAssign(false, TB_FLT);
    failAssign( false, TB_FA);
  }
  else
  {
    faultAssign( (Sen->Tb_hdwe<=_tb_min) || (Sen->Tb_hdwe>=_tb_max), TB_FLT);
    failAssign( tb_fa() || TbHardFail->calculate(tb_flt(), TB_HARD_SET, TB_HARD_RESET, Sen->T, reset_loc), TB_FA);
  }
}

// Temp stale check
void Fault::tb_stale(const boolean reset, Sensors *Sen)
{
  boolean reset_loc = reset | reset_all_faults_;

  if ( ap.disab_tb_fa || reset_loc || (sp.mod_tb() && !ap.fail_tb) )
  {
    faultAssign( false, TB_FLT );
    failAssign( false, TB_FA );
  }
  else
  {
    faultAssign( Sen->SensorTb->tb_stale_flt(), TB_FLT );
    failAssign( TbStaleFail->calculate(tb_flt(), TB_STALE_SET*ap.tb_stale_time_slr, TB_STALE_RESET*ap.tb_stale_time_slr,
      Sen->T_temp, reset_loc), TB_FA );
  }
}

// Check analog voltage.  Latches
void Fault::vb_check(Sensors *Sen, BatteryMonitor *Mon, const float _vb_min, const float _vb_max, const boolean reset)
{
  boolean reset_loc = reset | reset_all_faults_;
  if ( reset_loc )
  {
    failAssign(false, VB_FA);
  }
  if ( ap.disab_vb_fa || sp.mod_vb() )
  {
    faultAssign(false, VB_FLT);
    failAssign( false, VB_FA);
  }
  else
  {
    faultAssign( (Sen->vb_hdwe()<=_vb_min && Sen->ib_hdwe()*sp.nP()>IB_MIN_UP) || (Sen->vb_hdwe()>=_vb_max), VB_FLT);
    failAssign( vb_fa() || VbHardFail->calculate(vb_flt(), VB_HARD_SET, VB_HARD_RESET, Sen->T, reset_loc), VB_FA);
  }
}
void Fault::vc_check(Sensors *Sen, BatteryMonitor *Mon, const float _vc_min, const float _vc_max, const boolean reset)
{
  boolean reset_loc = reset | reset_all_faults_;
  if ( reset_loc )
  {
    failAssign(false, VC_FA);
  }
  if ( sp.mod_ib() || ap.disab_ib_fa )
  {
    faultAssign(false, VC_FLT);
    failAssign( false, VC_FA);
  }
  else
  {
    faultAssign( ( ((Sen->Vc<=_vc_min) || (Sen->Vc>=_vc_max)) && !reset_loc ), VC_FLT);
    failAssign( vc_fa() || VcHardFail->calculate(vc_flt(), VC_HARD_SET, VC_HARD_RESET, Sen->T, reset_loc), VC_FA);
  }
}

// Class Sensors
Sensors::Sensors(double T, double T_temp, Pins *pins, Sync *ReadSensors, Sync *Talk, Sync *Summarize, unsigned long long time_now,
  unsigned long long millis, BatteryMonitor *Mon):   reset_temp_(false), sample_time_ib_(0UL), sample_time_vb_(0UL),
  sample_time_ib_hdwe_(0UL), sample_time_vb_hdwe_(0UL), inst_time_(time_now), inst_millis_(millis)
{
  this->T = T;
  this->T_filt = T;
  this->T_temp = T_temp;
  #if defined(HDWE_IB_DUAL) || defined(HDWE_IB_HI_LO) || defined(HDWE_BARE)
    this->ShuntAmp = new Shunt("Amp", 0x49, &sp.ib_scale_amp_z, &sp.ib_bias_amp_z, SHUNT_AMP_GAIN, pins->Vcm_pin, pins->Vom_pin, pins->Vh3v3_pin, true);
    this->ShuntNoAmp = new Shunt("No Amp", 0x48, &sp.ib_scale_noa_z, &sp.ib_bias_noa_z, SHUNT_NOA_GAIN, pins->Vcn_pin, pins->Von_pin, pins->Vh3v3_pin, true);
  #else
    this->ShuntAmp = new Shunt("Amp", 0x49, &sp.ib_scale_amp_z, &sp.ib_bias_amp_z, SHUNT_AMP_GAIN, pins->Vcm_pin, pins->Vom_pin, pins->Vh3v3_pin, false);
    this->ShuntNoAmp = new Shunt("No Amp", 0x48, &sp.ib_scale_noa_z, &sp.ib_bias_noa_z, SHUNT_NOA_GAIN, pins->Vcn_pin, pins->Von_pin, pins->Vh3v3_pin, false);
  #endif
  #if !defined(HDWE_2WIRE) & !defined(HDWE_BARE)
    this->SensorTb = new TempSensor(pins->pin_1_wire, TEMP_PARASITIC, TEMP_DELAY);
  #elif !defined(HDWE_BARE)
    this->SensorTb = new TempSensor(pins->pin_1_wire, TEMP_PARASITIC, TEMP_DELAY, pins->VTb_pin);
  #endif
  this->TbSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);
  this->Sim = new BatterySim();
  this->elapsed_inj = 0ULL;
  this->start_inj = 0ULL;
  this->stop_inj = 0ULL;
  this->end_inj = 0ULL;
  this->ReadSensors = ReadSensors;
  this->Summarize = Summarize;
  this->Talk = Talk;
  this->display = true;
  this->Ib_hdwe_model = 0.;
  Prbn_Tb_ = new PRBS_7(TB_NOISE_SEED);
  Prbn_Vb_ = new PRBS_7(VB_NOISE_SEED);
  Prbn_Ib_amp_ = new PRBS_7(IB_AMP_NOISE_SEED);
  Prbn_Ib_noa_ = new PRBS_7(IB_NOA_NOISE_SEED);
  Flt = new Fault(T, &sp.preserving_z, Mon, this);
  Serial.printf("Vb sense ADC pin started\n");
  AmpFilt = new LagExp(T, AMP_FILT_TAU, -NOM_UNIT_CAP, NOM_UNIT_CAP);
  NoaFilt = new LagExp(T, AMP_FILT_TAU, -NOM_UNIT_CAP*sp.nS()*sp.nP(), NOM_UNIT_CAP*sp.nS()*sp.nP());
  SelFilt = new LagExp(T, AMP_FILT_TAU, -NOM_UNIT_CAP*sp.nS()*sp.nP(), NOM_UNIT_CAP*sp.nS()*sp.nP());
  VbFilt = new LagExp(T, AMP_FILT_TAU, 0., NOMINAL_VB*2.5);
  #ifdef HDWE_IB_HI_LO
    sel_brk_hdwe_ = new ScaleBrk(HDWE_IB_HI_LO_NOA_LO, HDWE_IB_HI_LO_AMP_LO, HDWE_IB_HI_LO_AMP_HI, HDWE_IB_HI_LO_NOA_HI);
  #else
    sel_brk_hdwe_ = new ScaleBrk(0., 0., 0., 0.);
  #endif
}

// Deliberate choice based on results and inputs
// Inputs:  ib_sel_stat_, Ib_amp_hdwe, Ib_noa_hdwe, Ib_amp_model, Ib_noa_model
// Outputs:  Ib_hdwe_model, Ib_hdwe
void Sensors::choose_()
{
  if ( Flt->ib_sel_stat()==2 )
  {
    Ib_hdwe = scale_select(Ib_noa_hdwe, sel_brk_hdwe_, Ib_amp_hdwe, Ib_noa_hdwe);
    // if ( sp.debug()==99 ) Serial.printf("choose: Ib_noa_hdwe=%7.3f, Ib_amp_hdwe=%7.3f, Ib_hdwe=%7.3f\n", Ib_noa_hdwe, Ib_amp_hdwe, Ib_hdwe);
    Ib_hdwe_model = Ib_hdwe;
    sample_time_ib_hdwe_ = ShuntNoAmp->sample_time();
    dt_ib_hdwe_ = ShuntNoAmp->dt();
  }
  else if ( Flt->ib_sel_stat()==1 )
  {
    Ib_hdwe = Ib_amp_hdwe;
    Ib_hdwe_model = Ib_amp_model;
    sample_time_ib_hdwe_ = ShuntAmp->sample_time();
    dt_ib_hdwe_ = ShuntAmp->dt();
  }
  else if ( Flt->ib_sel_stat()==-1 )
  {
    Ib_hdwe = Ib_noa_hdwe;
    Ib_hdwe_model = Ib_noa_model;
    sample_time_ib_hdwe_ = ShuntNoAmp->sample_time();
    dt_ib_hdwe_ = ShuntNoAmp->dt();
  }
  else
  {
    Ib_hdwe = 0.;
    Ib_hdwe_model = 0.;
    sample_time_ib_hdwe_ = 0ULL;
    dt_ib_hdwe_ = 0ULL;
  }
}

// Make final assignemnts
void Sensors::final_assignments(BatteryMonitor *Mon)
{
  // Reselect since may be changed
  // Inputs:  ib_sel_stat_, Ib_amp_hdwe, Ib_noa_hdwe, Ib_hdwe_model
  // Outputs:  Tb, Tb_filt, Vb, Ib, sample_time_ib
  choose_();

  // Final assignments
  // tb
  if ( sp.mod_tb() )
  {
    if ( Flt->tb_fa() )
    {
      Tb = NOMINAL_TB;
      Tb_filt = NOMINAL_TB;

    }
    else
    {
      Tb = RATED_TEMP + Tb_noise() + ap.Tb_bias_model;
      Tb_filt = RATED_TEMP + ap.Tb_bias_model;  // Simplifying assumption that Tb_filt perfectly quiet - so don't have to make model of filter
    }
    #ifndef HDWE_PHOTON
      if ( sp.debug()==16) Serial.printf("Tb_noise %7.3f Tb%7.3f Tb_filt%7.3f tb_fa %d\n", Tb_noise(), Tb, Tb_filt, Flt->tb_fa());
    #endif
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
  if ( sp.mod_vb() )
  {
    if ( (Flt->wrap_vb_fa() || Flt->vb_fa()) && !ap.fake_faults )
    {
      Vb = Mon->vb_model_rev() * sp.nS();
      sample_time_vb_ = Sim->sample_time();
    }
    else
    {
      Vb = Vb_model + Vb_noise();
      sample_time_vb_ = Sim->sample_time();
    }
  }
  else
  {
    if ( (Flt->wrap_vb_fa() || Flt->vb_fa()) && !ap.fake_faults )
    {
      Vb = Mon->vb_model_rev() * sp.nS();
      sample_time_vb_ = Sim->sample_time();
    }
    else
    {
      Vb = Vb_hdwe;
      sample_time_vb_ = sample_time_vb_hdwe_;
    }
  }
  
  // ib
  if ( sp.mod_ib() )
  {
    Ib = Ib_hdwe_model;
    Ib_amp = Ib_amp_model;
    Ib_noa = Ib_noa_model;
    Vc = HALF_V3V3;
    sample_time_ib_ = Sim->sample_time();
    dt_ib_ = Sim->dt();
  }
  else
  {
    Ib = Ib_hdwe;
    Ib_amp = Ib_amp_hdwe;
    Ib_noa = Ib_noa_hdwe;
    Vc = Vc_hdwe;
    sample_time_ib_ = sample_time_ib_hdwe_;
    dt_ib_ = dt_ib_hdwe_;
  }
  now = sample_time_ib_ - inst_millis_ + inst_time_*1000;

  // print_signal_select for data collection
  if ( (sp.debug()==2 || sp.debug()==4)  && cp.publishS )
  {
      double cTime = double(now)/1000.;

      sprintf(pr.buff, "unit_sel,%13.3f, %d, %d,  %10.7f, %8.5f,%8.5f,%8.5f,%8.5f,%8.5f, %8.5f,%8.5f, ",
          cTime, reset, sp.ib_select(),
          Flt->cc_diff(),
          ib_amp_hdwe(), ib_noa_hdwe(), ib_amp_model(), ib_noa_model(), ib_model(), 
          Flt->ib_diff(), Flt->ib_diff_f());
      Serial.printf("%s", pr.buff);

      sprintf(pr.buff, "  %7.5f,%8.5f,%8.5f,%8.5f,%8.5f,%8.5f,%8.5f,  ",
          Mon->voc_soc(), Flt->e_wrap(), Flt->e_wrap_filt(), Flt->e_wrap_m(), Flt->e_wrap_m_filt(), Flt->e_wrap_n(), Flt->e_wrap_n_filt());
      Serial.printf("%s", pr.buff);

      sprintf(pr.buff, "  %d,%8.5f,%8.5f,%8.5f, %d,%8.5f,  %d,%8.5f,%8.5f, %d,%8.5f,  %5.2f,%5.2f, %d, %5.2f, ",
          Flt->ib_sel_stat(), vc_hdwe(), ib_hdwe(), ib_hdwe_model(), sp.mod_ib(), ib(),
          Flt->vb_sel_stat(), vb_hdwe(), vb_model(), sp.mod_vb(), vb(),
          Tb_hdwe, Tb, sp.mod_tb(), Tb_filt);
      Serial.printf("%s", pr.buff);

      sprintf(pr.buff, "%ld, %ld, %7.3f, %7.3f, %d, %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,%d,%7.3f,",
          Flt->fltw(), Flt->falw(), Flt->ib_rate(), Flt->ib_quiet(), Flt->tb_sel_status(),
          Flt->cc_diff_thr(), Flt->ewhi_thr(), Flt->ewlo_thr(), Flt->ib_diff_thr(), Flt->ib_quiet_thr(), Flt->preserving(), ap.fake_faults,
          Mon->y_ekf_filt());
      Serial.printf("%s\n", pr.buff);
  }
}

// Tb noise
float Sensors::Tb_noise()
{
  if ( ap.Tb_noise_amp==0. ) return ( 0. );
  uint8_t raw = Prbn_Tb_->calculate();
  float noise = (float(raw)/127. - 0.5) * ap.Tb_noise_amp;
  return ( noise );
}

// Conversion.   Here to avoid circular reference to sp in headers.
float Sensors::Ib_amp_add() { return ( ap.ib_amp_add * sp.nP() ); };
float Sensors::Ib_amp_max() { return ( ap.ib_amp_max * sp.nP() ); };
float Sensors::Ib_amp_min() { return ( ap.ib_amp_min * sp.nP() ); };
float Sensors::Ib_noa_add() { return ( ap.ib_noa_add * sp.nP() ); };
float Sensors::Ib_noa_max() { return ( ap.ib_noa_max * sp.nP() ); };
float Sensors::Ib_noa_min() { return ( ap.ib_noa_min * sp.nP() ); };
float Sensors::Vb_add() { return ( ap.vb_add * sp.nS() ); };

// Vb noise
float Sensors::Vb_noise()
{
  if ( ap.Vb_noise_amp==0. ) return ( 0. );
  uint8_t raw = Prbn_Vb_->calculate();
  float noise = (float(raw)/127. - 0.5) * ap.Vb_noise_amp;
  return ( noise );
}

// Ib noise
float Sensors::Ib_amp_noise()
{
  if ( ap.Ib_amp_noise_amp==0. ) return ( 0. );
  uint8_t raw = Prbn_Ib_amp_->calculate();
  float noise = (float(raw)/125. - 0.5) * ap.Ib_amp_noise_amp;
  return ( noise );
}
float Sensors::Ib_noa_noise()
{
  if ( ap.Ib_noa_noise_amp==0. ) return ( 0. );
  uint8_t raw = Prbn_Ib_noa_->calculate();
  float noise = (float(raw)/125. - 0.5) * ap.Ib_noa_noise_amp;
  return ( noise );
}

// Print Shunt selection data
void Sensors::shunt_print()
{
    Serial.printf("reset,T,select,inj_bias,  vim,Vsm,Vcm,Vom,Ibhm,  vin,Vsn,Vcn,Von,Ibhn,  vi3,vh3, Ib_hdwe,T,Ib_amp_fault,Ib_amp_fail,Ib_noa_fault,Ib_noa_fail,=,    %d,%7.3f,%d,%7.3f,    %d,%7.3f,%7.3f,%7.3f,%7.3f,    %d,%7.3f,%7.3f,%7.3f,%7.3f,    %7.3f,%7.3f, %d,%d,  %d,%d,\n",
        reset, T, sp.ib_select(), sp.inj_bias(),
        ShuntAmp->vshunt_int(), ShuntAmp->vshunt(), ShuntAmp->Vc(), ShuntAmp->Vo(), ShuntAmp->Ishunt_cal(),
        ShuntNoAmp->vshunt_int(), ShuntNoAmp->vshunt(), ShuntNoAmp->Vc(), ShuntNoAmp->Vo(), ShuntNoAmp->Ishunt_cal(),
        Ib_hdwe, T,
        Flt->ib_amp_flt(), Flt->ib_amp_fa(), Flt->ib_noa_flt(), Flt->ib_noa_fa());
}

// Shunt selection.  Use Coulomb counter and EKF to sort three signals:  amp current, non-amp current, voltage
// Initial selection to charge the Sim for modeling currents on BMS cutback
// Inputs: sp.ib_select (user override), Mon (EKF status)
// States:  Ib_fail_noa_
// Outputs:  Ib_hdwe, Ib_model_in, Vb_sel_status_
void Sensors::shunt_select_initial(const boolean reset)
{
    // Current signal selection, based on if there or not.
    // Over-ride 'permanent' with Talk(sp.ib_select) = Talk('s')

    // Hardware and model current assignments
    float hdwe_add, mod_add;
    if ( !sp.mod_ib() )
    {
      mod_add = 0.;
      hdwe_add = sp.ib_bias_all() + sp.inj_bias();
    }
    else
    {
      mod_add = sp.inj_bias() + sp.ib_bias_all();
      if ( sp.tweak_test() )
        hdwe_add = sp.inj_bias();
      else
        hdwe_add = 0.;
    }
    Ib_amp_model = max(min(Ib_model + Ib_amp_add(), Ib_amp_max()), Ib_amp_min()); // uses past Ib.  Synthesized signal to use as substitute for sensor, Dm/Mm/Nm
    if(sp.debug()==71) Serial.printf("Ib_model Ib_amp_add Ib_amp_max Ib_amp_min = Ib_amp_model %7.3f %7.3f %7.3f %7.3f ->%7.3f\n", Ib_model, Ib_amp_add(), Ib_amp_max(), Ib_amp_min(), Ib_amp_model);
    Ib_noa_model = max(min(Ib_model + Ib_noa_add(), Ib_noa_max()), Ib_noa_min()); // uses past Ib.  Synthesized signal to use as substitute for sensor, Dn/Mn/Nn
    Ib_amp_hdwe = ShuntAmp->Ishunt_cal() + hdwe_add;    // Sense fault injection feeds logic, not model
    Ib_amp_hdwe_f = AmpFilt->calculate(Ib_amp_hdwe, reset, AMP_FILT_TAU, T);
    Vc_hdwe = max(ShuntAmp->Vc(), ShuntNoAmp->Vc());
    Ib_noa_hdwe = ShuntNoAmp->Ishunt_cal() + hdwe_add;  // Sense fault injection feeds logic, not model
    Ib_noa_hdwe_f = NoaFilt->calculate(Ib_noa_hdwe, reset, AMP_FILT_TAU, T);
    Ib_hdwe_f = SelFilt->calculate(Ib_hdwe, reset, AMP_FILT_TAU, T);

    // Initial choice
    // Inputs:  ib_sel_stat_, Ib_amp_hdwe, Ib_noa_hdwe, Ib_amp_model(past), Ib_noa_model(past)
    // Outputs:  Ib_hdwe_model, Ib_hdwe
    choose_();

    // When running normally the model tracks hdwe to synthesize reference information
    if ( !sp.mod_ib() )
    {
      Ib_model_in = Ib_hdwe;
    }
    // Otherwise it generates signals for feedback into monitor
    else
    {
      Ib_model_in = mod_add;
      // if ( sp.debug()==-24 ) Serial.printf("ib_bias_all%7.3f mod_add%7.3f Ib_model_in%7.3f\n", sp.ib_bias_all(), mod_add, Ib_model_in);
    }
}

// Load and filter Tb
void Sensors::temp_load_and_filter(Sensors *Sen, const boolean reset_temp)
{
  Log.info("top temp_load_and_filter");
  reset_temp_ = reset_temp;
  #ifndef HDWE_BARE
    Tb_hdwe = SensorTb->sample(Sen);
  #else
    Tb_hdwe = RATED_TEMP;
  #endif

  // Filter and add rate limited bias
  if ( reset_temp_ && Tb_hdwe>TEMP_RANGE_CHECK_MAX )  // Bootup T=85.5 C
  {
      Tb_hdwe = RATED_TEMP;
      Tb_hdwe_filt = TbSenseFilt->calculate(RATED_TEMP, reset_temp_, min(T_temp, F_MAX_T_TEMP));
  }
  else
  {
      Tb_hdwe_filt = TbSenseFilt->calculate(Tb_hdwe, reset_temp_, min(T_temp, F_MAX_T_TEMP));
  }
  Tb_hdwe += sp.Tb_bias_hdwe();
  Tb_hdwe_filt += sp.Tb_bias_hdwe();

  if ( sp.debug()==16 || (sp.debug()==-1 && reset_temp_) ) Serial.printf("reset_temp_,Tb_bias_hdwe_loc, RATED_TEMP, Tb_hdwe, Tb_hdwe_filt, ready %d %7.3f %7.3f %7.3f %7.3f %d\n",
    reset_temp_, sp.Tb_bias_hdwe(), RATED_TEMP, Tb_hdwe, Tb_hdwe_filt, cp.tb_info.ready);

  #ifdef HDWE_2WIRE
    Flt->tb_check(Sen, TB_MIN, TB_MAX,  reset_temp_);
  #else
    Flt->tb_stale(reset_temp_, Sen);
  #endif
}

// Load analog voltage
void Sensors::vb_load(const uint16_t vb_pin, const boolean reset)
{
  if ( !sp.mod_vb_dscn() )
  {
    #if !defined(HDWE_BARE)
      Vb_raw = analogRead(vb_pin);
      Vb_hdwe =  float(Vb_raw)*VB_CONV_GAIN*sp.Vb_scale() + float(VB_A) + sp.Vb_bias_hdwe();
    #endif
    Vb_hdwe_f = VbFilt->calculate(Vb_hdwe, reset, AMP_FILT_TAU, T);
  }
  else
  {
    Vb_raw = 0;
    Vb_hdwe = 0.;
  }
  sample_time_vb_hdwe_ = System.millis();
}

// Print analog voltage
void Sensors::vb_print()
{
  Serial.printf("reset, T, vb_dscn, Vb_raw, sp.Vb_bias_hdwe(), Vb_hdwe, vb_flt(), vb_fa(), wv_fa=, %d, %7.3f, %d, %d, %7.3f,  %7.3f, %d, %d, %d,\n",
    reset, T, sp.mod_vb_dscn(), Vb_raw, sp.Vb_bias_hdwe(), Vb_hdwe, Flt->vb_flt(), Flt->vb_fa(), Flt->wrap_vb_fa());
}

// Scale select between a high and low set of inputs.  Low might be a precise, amplified sensor and high might be the high range equivalent
float scale_select(const float in, const ScaleBrk *brk, const float lo, const float hi)
{
  
  if ( brk->n_hi <= in && in <= brk->p_lo )
  {
    // if ( sp.debug()==99 ) Serial.printf("scale_select lo: in=%7.3f, brk->n_hi=%7.3f, brk->p_lo=%7.3f, lo=%7.3f, hi=%7.3f, sel=%7.3f\n", in, brk->n_hi, brk->p_lo, lo, hi, lo);
    return ( lo );
  }

  else if ( in <= brk->n_lo || in >= brk->p_hi )
  {
    // if ( sp.debug()==99 ) Serial.printf("scale_select hi: in=%7.3f, brk->n_lo=%7.3f, brk->p_hi=%7.3f, lo=%7.3f, hi=%7.3f, sel=%7.3f\n", in, brk->n_lo, brk->p_hi, lo, hi, hi);
    return ( hi );
  }

  else if ( in < brk->n_hi )
  {
    // if ( sp.debug()==99 ) Serial.printf("scale_select n t: in=%7.3f, brk->n_hi=%7.3f, brk->p_lo=%7.3f, lo=%7.3f, hi=%7.3f\n", in, brk->n_hi, brk->p_lo, lo, hi);
    return ( (in - brk->n_lo) / brk->n_d * (hi - lo) + hi );
  }

  else
  {
    // if ( sp.debug()==99 ) Serial.printf("scale_select p t: in=%7.3f, brk->n_hi=%7.3f, brk->p_lo=%7.3f, lo=%7.3f, hi=%7.3f\n", in, brk->n_hi, brk->p_lo, lo, hi);
    return ( (in - brk->p_lo) / brk->p_d * (hi - lo) + hi );
  }

}
