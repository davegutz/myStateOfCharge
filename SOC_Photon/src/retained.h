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
  float Ib_scale_amp = CURR_SCALE_AMP;    // Calibration scalar of amplified shunt sensor, A
  float ib_bias_amp = CURR_BIAS_AMP;      // Calibration adder of amplified shunt sensor, A
  float Ib_scale_noa = CURR_SCALE_NOA;    // Calibration scalar of non-amplified shunt sensor, A
  float ib_bias_noa = CURR_BIAS_NOA;      // Calibration adder of non-amplified shunt sensor, A
  float ib_bias_all = CURR_BIAS_ALL;      // Bias on all shunt sensors, A
  int8_t ib_select = FAKE_FAULTS;         // Force current sensor (-1=non-amp, 0=auto, 1=amp)
  float Vb_bias = VOLT_BIAS;    // Calibrate Vb, V
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
  float hys_scale = HYS_SCALE;  // Hysteresis scalar
  float tweak_sclr_amp = 1.;    // Dyn tweak.  Tweak calibration for amplified current sensor
  float tweak_sclr_noa = 1.;    // Dyn tweak.  Tweak calibration for non-amplified current sensor
  float nP = NP;                // Number of parallel batteries in bank, e.g. '2P1S'
  float nS = NS;                // Number of series batteries in bank, e.g. '2P1S'
  uint8_t mon_mod = MON_CHEM;   // Monitor battery chemistry type
  uint8_t sim_mod = SIM_CHEM;   // Simulation battery chemistry type
  float Vb_scale = 1.;          // Calibration scalar for Vb. V/count

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
    this->Ib_scale_amp = CURR_SCALE_AMP;
    this->ib_bias_amp = CURR_BIAS_AMP;
    this->Ib_scale_noa = CURR_SCALE_NOA;
    this->ib_bias_noa = CURR_BIAS_NOA;
    this->ib_bias_all = CURR_BIAS_ALL;
    this->ib_select = FAKE_FAULTS;
    this->Vb_bias = VOLT_BIAS;
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
    this->hys_scale = HYS_SCALE;
    this->tweak_sclr_amp = 1.;
    this->tweak_sclr_noa = 1.;
    this->nP = NP;
    this->nS = NS;
    this->mon_mod = MON_CHEM;
    this->sim_mod = SIM_CHEM;
    this->Vb_scale = VB_SCALE;
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
    Serial.printf("\nretained parameters (rp):\n");
    Serial.printf("                 local     memory\n");
    Serial.printf(" isum                           %d tbl ptr\n", isum);
    Serial.printf(" dq_cinf_amp%10.1f %10.1f C\n", -RATED_BATT_CAP*3600., delta_q_cinf_amp);
    Serial.printf(" dq_dinf_amp%10.1f %10.1f C\n", RATED_BATT_CAP*3600., delta_q_dinf_amp);
    Serial.printf(" dq_cinf_noa%10.1f %10.1f C\n", -RATED_BATT_CAP*3600., delta_q_cinf_noa);
    Serial.printf(" dq_dinf_noa%10.1f %10.1f C\n", RATED_BATT_CAP*3600., delta_q_dinf_noa);
    Serial.printf(" t_last          %5.2f      %5.2f dg C\n", RATED_TEMP, t_last);
    Serial.printf(" t_last_sim      %5.2f      %5.2f dg C\n", RATED_TEMP, t_last_model);
    Serial.printf(" shunt_gn_slr  %7.3f    %7.3f ?\n", 1., shunt_gain_sclr);  // TODO:  no talk value
    Serial.printf(" debug               %d          %d *v<>\n", 0, debug);
    Serial.printf(" delta_q    %10.1f %10.1f *Ca<>, C\n", 0., delta_q);
    Serial.printf(" dq_sim     %10.1f %10.1f *Ca<>, *Cm<>, C\n", 0., delta_q_model);
    Serial.printf(" scale_amp     %7.3f    %7.3f *SA<>\n", CURR_SCALE_AMP, Ib_scale_amp);
    Serial.printf(" bias_amp      %7.3f    %7.3f *DA<>\n", CURR_BIAS_AMP, ib_bias_amp);
    Serial.printf(" scale_noa     %7.3f    %7.3f *SB<>\n", CURR_SCALE_NOA, Ib_scale_noa);
    Serial.printf(" bias_noa      %7.3f    %7.3f *DB<>\n", CURR_BIAS_NOA, ib_bias_noa);
    Serial.printf(" ib_bias_all   %7.3f    %7.3f *Di<> A\n", CURR_BIAS_ALL, ib_bias_all);
    Serial.printf(" ib_select           %d          %d *s<> -1=noa, 0=auto, 1=amp\n", FAKE_FAULTS, ib_select);
    Serial.printf(" Vb_bias       %7.3f    %7.3f *Dv<>,*Dc<> V\n", VOLT_BIAS, Vb_bias);
    Serial.printf(" modeling            %d          %d *Xm<>\n", 0, modeling);
    Serial.printf(" inj amp       %7.3f    %7.3f *Xa<> A pk\n", 0., amp);
    Serial.printf(" inj frq       %7.3f    %7.3f *Xf<> r/s\n", 0., freq);
    Serial.printf(" inj typ             %d          %d *Xt<> 1=sin, 2=sq, 3=tri\n", 0, type);
    Serial.printf(" inj_bias      %7.3f    %7.3f *Xb<> A\n", 0., inj_bias);
    Serial.printf(" tb_bias_hdwe  %7.3f    %7.3f *Dt<> dg C\n", TEMP_BIAS, tb_bias_hdwe);
    Serial.printf(" s_cap_model   %7.3f    %7.3f *Sc<>\n", 1., s_cap_model);
    Serial.printf(" cut_gn_slr    %7.3f    %7.3f *Sk<>\n", 1., cutback_gain_scalar);
    Serial.printf(" hys_scale     %7.3f    %7.3f *Sh<>\n", HYS_SCALE, hys_scale);
    Serial.printf(" tweak_sclr_amp%7.3f    %7.3f *Mk<>\n", 1., tweak_sclr_amp);
    Serial.printf(" tweak_sclr_noa%7.3f    %7.3f *Nk<>\n", 1., tweak_sclr_noa);
    Serial.printf(" nP            %7.3f    %7.3f *BP<> eg '2P1S'\n", NP, nP);
    Serial.printf(" nS            %7.3f    %7.3f *BP<> eg '2P1S'\n", NS, nS);
    Serial.printf(" mon chem            %d          %d *Bm<> 0=Battle, 1=LION\n", MON_CHEM, mon_mod);
    Serial.printf(" sim chem            %d          %d *Bs<>\n", SIM_CHEM, sim_mod);
    Serial.printf(" sclr vb       %7.3f    %7.3f *SV<>\n\n", VB_SCALE, Vb_scale);
  }

};            

#endif
