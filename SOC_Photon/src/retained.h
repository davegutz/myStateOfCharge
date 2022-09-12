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

#ifndef _RETAINED_H
#define _RETAINED_H
#define t_float float

#include "local_config.h"
#include "Battery.h"
#include "math.h"

// Definition of structure to be saved in SRAM.  Many are needed to calibrate.  Others are
// needed to allow testing with resets.  Others allow application to remember dynamic
// tweaks.  Default values below are important:  they prevent junk
// behavior on initial build. Don't put anything in here that you can't live with normal running
// because could get set by testing and forgotten.  Not reset by hard reset
// ********CAUTION:  any special includes or logic in here breaks retained function
struct RetainedPars
{
  int8_t debug = 0;             // Level of debug printing
  double delta_q = 0.;          // Charge change since saturated, C
  float t_last = RATED_TEMP;    // Updated value of battery temperature injection when rp.modeling and proper wire connections made, deg C
  double delta_q_model = 0.;    // Coulomb Counter state for model, C
  float t_last_model = RATED_TEMP;        // Battery temperature past value for rate limit memory, deg C
  float shunt_gain_sclr = 1.;             // Shunt gain scalar
  float ib_scale_amp = CURR_SCALE_AMP;    // Calibration scalar of amplified shunt sensor, A
  float ib_bias_amp = CURR_BIAS_AMP;      // Calibration adder of amplified shunt sensor, A
  float ib_scale_noa = CURR_SCALE_NOA;    // Calibration scalar of non-amplified shunt sensor, A
  float ib_bias_noa = CURR_BIAS_NOA;      // Calibration adder of non-amplified shunt sensor, A
  float ib_bias_all = CURR_BIAS_ALL;      // Bias on all shunt sensors, A
  int8_t ib_select = 0;         // Force current sensor (-1=non-amp, 0=auto, 1=amp)
  float vb_bias = VOLT_BIAS;    // Calibrate Vb, V
  uint8_t modeling = 0;         // Driving saturation calculation with model.  Bits specify which signals use model
  float amp = 0.;               // Injected amplitude, A pk (0-18.3)
  float freq = 0.;              // Injected frequency, Hz (0-2)
  uint8_t type = 0;             // Injected waveform type.   0=sine, 1=square, 2=triangle
  float inj_bias = 0.;          // Constant bias, A
  float tb_bias_hdwe = TEMP_BIAS;    // Bias on Tb sensor, deg C
  float s_cap_model = 1.;       // Scalar on battery model size
  float cutback_gain_scalar = 1.;        // Scalar on battery model saturation cutback function
          // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
  int isum = -1;                // Summary location.   Begins at -1 because first action is to increment isum
  float delta_q_cinf_amp = -RATED_BATT_CAP*3600.;   // Dyn tweak.  Charge delta_q since last reset.  Simple integration of amplified current
  float delta_q_cinf_noa = -RATED_BATT_CAP*3600.;   // Dyn tweak.  Charge delta_q since last reset.  Simple integration of non-amplified current
  float delta_q_dinf_amp = RATED_BATT_CAP*3600.;    // Dyn tweak.  Discharge delta_q since last reset.  Simple integration of amplified current
  float delta_q_dinf_noa = RATED_BATT_CAP*3600.;    // Dyn tweak.  Discharge delta_q since last reset.  Simple integration of non-amplified current
  float hys_scale = 1.;         // Hysteresis scalar
  float tweak_sclr_amp = 1.;    // Dyn tweak.  Tweak calibration for amplified current sensor
  float tweak_sclr_noa = 1.;    // Dyn tweak.  Tweak calibration for non-amplified current sensor
  float nP = NP;                // Number of parallel batteries in bank, e.g. '2P1S'
  float nS = NS;                // Number of series batteries in bank, e.g. '2P1S'
  uint8_t mon_mod = MON_CHEM;   // Monitor battery chemistry type
  uint8_t sim_mod = SIM_CHEM;   // Simulation battery chemistry type

  // Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
  // battery.  Small compilation changes can change where in this memory the program points, too.
  boolean is_corrupt()
  {
    return ( this->nP==0 || this->nS==0 || this->mon_mod>10 || isnan(this->amp) || this->freq>2. ||
     abs(this->ib_bias_amp)>500. || abs(this->cutback_gain_scalar)>1000. || abs(this->ib_bias_noa)>500. ||
     this->t_last_model<-10. || this->t_last_model>70. );
  }

  // Nominalize
  void nominal()
  {
    this->debug = 0;
    this->delta_q = 0.;
    this->t_last = RATED_TEMP;
    this->delta_q_model = 0.;
    this->t_last_model = RATED_TEMP;
    this->shunt_gain_sclr = 1.;
    this->ib_scale_amp = CURR_SCALE_AMP;
    this->ib_bias_amp = CURR_BIAS_AMP;
    this->ib_scale_noa = CURR_SCALE_NOA;
    this->ib_bias_noa = CURR_BIAS_NOA;
    this->ib_bias_all = CURR_BIAS_ALL;
    this->ib_select = false;
    this->vb_bias = VOLT_BIAS;
    this->modeling = 0;
    this->amp = 0.;
    this->freq = 0.;
    this->type = 0;
    this->inj_bias = 0.;
    this->tb_bias_hdwe = TEMP_BIAS;
    this->s_cap_model = 1.;
    this->cutback_gain_scalar = 1.;
    this->isum = -1;
    this->delta_q_cinf_amp = -RATED_BATT_CAP*3600.;
    this->delta_q_cinf_noa = -RATED_BATT_CAP*3600.;
    this->delta_q_dinf_amp = RATED_BATT_CAP*3600.;
    this->delta_q_dinf_noa = RATED_BATT_CAP*3600.;
    this->hys_scale = 1.;
    this->tweak_sclr_amp = 1.;
    this->tweak_sclr_noa = 1.;
    this->nP = NP;
    this->nS = NS;
    this->mon_mod = MON_CHEM;
    this->sim_mod = SIM_CHEM;
  }
  void large_reset()
  {
    nominal();
  }

  // Configuration functions
  boolean tweak_test() { return ( 0x8 & modeling ); } // Driving signal injection completely using software inj_bias 
  boolean mod_ib() { return ( 0x4 & modeling ); }     // Using Sim as source of ib
  boolean mod_vb() { return ( 0x2 & modeling ); }     // Using Sim as source of vb
  boolean mod_tb() { return ( 0x1 & modeling ); }     // Using Sim as source of tb
  boolean mod_none() { return ( 0==modeling ); }      // Using nothing
  boolean mod_any() { return ( 0<modeling ); }        // Using any
  
  // Print
  void pretty_print(void)
  {
    Serial.printf("retained parameters (rp):\n");
    Serial.printf("  debug=%d;*v<>\n", this->debug);
    Serial.printf("  delta_q=%7.3f;*Ca<>\n", this->delta_q);
    Serial.printf("  t_last=%7.3f; deg C\n", this->t_last);
    Serial.printf("  delta_q_model=%10.1f;*Ca<>,*Cm<>\n", this->delta_q_model);
    Serial.printf("  t_last_model=%7.3f; deg C\n", this->t_last_model);
    Serial.printf("  shunt_gain_sclr= %7.3f; A\n", this->shunt_gain_sclr);
    Serial.printf("  ib_scale_amp= %7.3f;*SA<> A\n", this->ib_scale_amp);
    Serial.printf("  ib_bias_amp= %7.3f;*DA<> A\n", this->ib_bias_amp);
    Serial.printf("  ib_scale_noa= %7.3f;*SB<> A\n", this->ib_scale_noa);
    Serial.printf("  ib_bias_noa=%7.3f;*DB<> A\n", this->ib_bias_noa);
    Serial.printf("  ib_bias_all=%7.3f;*Di<> A \n", this->ib_bias_all);
    Serial.printf("  ib_select=%d;*s<> -1=noa, 0=auto, 1=amp\n", this->ib_select);
    Serial.printf("  vb_bias=%7.3f;*Dv,*Dc<> V\n", this->vb_bias);
    Serial.printf("  modeling=%d;*Xm<>\n", this->modeling);
    Serial.printf("  amp=%7.3f;*Xa<> A pk\n", this->amp);
    Serial.printf("  freq=%7.3f;*Xf<> r/s\n", this->freq);
    Serial.printf("  type=%d;*Xt<> 0=sin, 1=sq, 2=tri\n", this->type);
    Serial.printf("  inj_bias=%7.3f; A\n", this->inj_bias);
    Serial.printf("  tb_bias_hdwe=%7.3f;*Dt<> deg C\n", this->tb_bias_hdwe);
    Serial.printf("  s_cap_model=%7.3f;*Sc<>\n", this->s_cap_model);
    Serial.printf("  cutback_gain_scalar=%7.3f;*Sk<>\n", this->cutback_gain_scalar);
    Serial.printf("  isum=%d;\n", this->isum);
    Serial.printf("  delta_q_cinf_amp= %10.1f; C\n", this->delta_q_cinf_amp);
    Serial.printf("  delta_q_dinf_amp= %10.1f; C\n", this->delta_q_dinf_amp);
    Serial.printf("  delta_q_cinf_noa=%10.1f; C\n", this->delta_q_cinf_noa);
    Serial.printf("  delta_q_dinf_noa=%10.1f; C\n", this->delta_q_dinf_noa);
    Serial.printf("  hys_scale=%7.3f;*Sh\n", this->hys_scale);
    Serial.printf("  tweak_sclr_amp=%7.3f;*Mk<>\n", this->tweak_sclr_amp);
    Serial.printf("  tweak_sclr_noa=%7.3f;*Nk<>\n", this->tweak_sclr_noa);
    Serial.printf("  nP=%5.2f;*BP<> e.g. '2P1S'\n", this->nP);
    Serial.printf("  nS=%5.2f;*BS<> e.g. '2P1S'\n", this->nS);
    Serial.printf("  mon_mod=%d;*Bm<> 0=Battle, 1=LION\n", this->mon_mod);
    Serial.printf("  sim_mod=%d;*Bs<> 0=Battle, 1=LION\n", this->sim_mod);
  }

  // Compare memory to local_config.h
  void print_versus_local_config()
  {
    Serial.printf("          local    memory\n");
    Serial.printf("bias amp %7.3f   %7.3f\n", CURR_BIAS_AMP, ib_bias_amp);
    Serial.printf("bias noa %7.3f   %7.3f\n", CURR_BIAS_NOA, ib_bias_noa);
    Serial.printf("sclr amp %7.3f   %7.3f\n", CURR_SCALE_AMP, ib_scale_amp);
    Serial.printf("sclr noa %7.3f   %7.3f\n", CURR_SCALE_NOA, ib_scale_noa);
    Serial.printf("mon chem %d   %d\n", MON_CHEM, mon_mod);
    Serial.printf("sim chem %d   %d\n", SIM_CHEM, sim_mod);
  }

  // Renominalize as requested in setup()
  void renominalize_to_local_config()
  {
    ib_bias_amp = CURR_BIAS_AMP;
    ib_bias_noa = CURR_BIAS_NOA;
    ib_scale_amp = CURR_SCALE_AMP;
    ib_scale_noa = CURR_SCALE_NOA;
    mon_mod = MON_CHEM;
    sim_mod = SIM_CHEM;
    print_versus_local_config();
  }
};            

#endif
