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
#include "math.h"

// Definition of structure to be saved in SRAM
// Default values below are important:  they prevent junk
// behavior on initial build.
// Don't put anything in here that you can't live with normal running
// because could get set by testing and forgotten.  Not reset by hard reset
// ********CAUTION:  any special includes or logic in here breaks retained function
struct RetainedPars
{
  int8_t debug = 0;             // Level of debug printing
  double delta_q = -0.5;        // Charge change since saturated, C
  float t_last = 25.;           // Updated value of battery temperature injection when rp.modeling and proper wire connections made, deg C
  double delta_q_model = -0.5;  // Coulomb Counter state for model, (-1 - 1)
  float t_last_model = 25.;     // Battery temperature past value for rate limit memory, deg C
  float ibatt_bias_amp = CURR_BIAS_AMP;     // Calibration of amplified shunt sensor, A
  float ibatt_bias_noamp = CURR_BIAS_NOAMP; // Calibration of non-amplified shunt sensor, A
  float ibatt_bias_all = CURR_BIAS_ALL;     // Bias on all shunt sensors, A
  boolean ibatt_sel_noamp = false;          // Force non-amplified sensor (amplified sensor is default)
  float vbatt_bias = VOLT_BIAS; // Calibrate Vbatt, V
  uint8_t modeling = 0;         // Driving saturation calculation with model.  Bits specify which signals use model
  float amp = 0.;               // Injected amplitude, A pk (0-18.3)
  float freq = 0.;              // Injected frequency, Hz (0-2)
  uint8_t type = 0;             // Injected waveform type.   0=sine, 1=square, 2=triangle
  float inj_bias = 0.;     // Constant bias, A
  float tbatt_bias = TEMP_BIAS; // Bias on Tbatt sensor, deg C
  float s_cap_model = 1.;       // Scalar on battery model size
  float cutback_gain_scalar = 1.;  // Scalar on battery model saturation cutback function
          // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
  int isum = -1;                // Summary location.   Begins at -1 because first action is to increment isum
  float delta_q_cinf_amp = -RATED_BATT_CAP*3600.;   // Charge delta_q since last reset.  Simple integration of amplified current
  float delta_q_cinf_noamp = -RATED_BATT_CAP*3600.; // Charge delta_q since last reset.  Simple integration of non-amplified current
  float delta_q_dinf_amp = RATED_BATT_CAP*3600.;    // Discharge delta_q since last reset.  Simple integration of amplified current
  float delta_q_dinf_noamp = RATED_BATT_CAP*3600.;  // Discharge delta_q since last reset.  Simple integration of non-amplified current
  float hys_scale = 1.;         // Hysteresis scalar
  float tweak_sclr_amp = 1.;    // Tweak calibration for amplified current sensor
  float tweak_sclr_noamp = 1.;  // Tweak calibration for non-amplified current sensor
  float nP = NP;                // Number of parallel batteries in bank, e.g. '2P1S'
  float nS = NS;                // Number of series batteries in bank, e.g. '2P1S'
  uint8_t mon_mod = MOD_CODE;   // Monitor battery chemistry type
  uint8_t sim_mod = MOD_CODE;   // Simulation battery chemistry type

  // Corruption test
  boolean is_corrupt()
  {
    return ( this->nP==0 || this->nS==0 || this->mon_mod>10 || isnan(this->amp) || this->freq>2. ||
     abs(this->ibatt_bias_amp)>500. || abs(this->cutback_gain_scalar)>1000. || abs(this->ibatt_bias_noamp)>500. ||
     this->t_last_model<-10. || this->t_last_model>70. );
  }

  // Nominalize
  void nominal()
  {
    this->debug = 0;
    this->delta_q = -0.5;
    this->t_last = 25.;
    this->delta_q_model = -0.5;
    this->t_last_model = 25.;
    this->ibatt_bias_amp = CURR_BIAS_AMP;
    this->ibatt_bias_noamp = CURR_BIAS_NOAMP;
    this->ibatt_bias_all = CURR_BIAS_ALL;
    this->ibatt_sel_noamp = false;
    this->vbatt_bias = VOLT_BIAS;
    this->modeling = 0;
    this->amp = 0.;
    this->freq = 0.;
    this->type = 0;
    this->inj_bias = 0.;
    this->tbatt_bias = TEMP_BIAS;
    this->s_cap_model = 1.;
    this->cutback_gain_scalar = 1.;
    this->isum = -1;
    this->delta_q_cinf_amp = -RATED_BATT_CAP*3600.;
    this->delta_q_cinf_noamp = -RATED_BATT_CAP*3600.;
    this->delta_q_dinf_amp = RATED_BATT_CAP*3600.;
    this->delta_q_dinf_noamp = RATED_BATT_CAP*3600.;
    this->hys_scale = 1.;
    this->tweak_sclr_amp = 1.;
    this->tweak_sclr_noamp = 1.;
    this->nP = NP;
    this->nS = NS;
    this->mon_mod = MOD_CODE;
    this->sim_mod = MOD_CODE;
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
    Serial.printf("  debug =                     %d;  // Level of debug printing\n", this->debug);
    Serial.printf("  delta_q =             %7.3f;  // CC state Monf, (-1 - 1)\n", this->delta_q);
    Serial.printf("  t_last =              %7.3f;  // Battery temp past rate limit mem, deg C\n", this->t_last);
    Serial.printf("  delta_q_model =    %10.1f;  // CC state model, (-1 - 1)\n", this->delta_q_model);
    Serial.printf("  t_last_model =        %7.3f;  // Battery temp past rate limit mem, deg C\n", this->t_last_model);
    Serial.printf("  ibatt_bias_amp =      %7.3f;  // Calibrate amp sensor, A\n", this->ibatt_bias_amp);
    Serial.printf("  ibatt_bias_noamp =    %7.3f;  // Calibrate non-amp sensor, A\n", this->ibatt_bias_noamp);
    Serial.printf("  ibatt_bias_all =      %7.3f;  // Bias all sensors, A \n", this->ibatt_bias_all);
    Serial.printf("  ibatt_sel_noamp =           %d;  // Use non-amp\n", this->ibatt_sel_noamp);
    Serial.printf("  vbatt_bias =          %7.3f;  // Calibrate Vbatt, V\n", this->vbatt_bias);
    Serial.printf("  modeling =                  %d;  // Bit mapped mod spec\n", this->modeling);
    Serial.printf("  amp =                 %7.3f;  // Injected amp, A pk (0-18.3)\n", this->amp);
    Serial.printf("  freq =                %7.3f;  // Injected freq, r/s (0-2*pi)\n", this->freq);
    Serial.printf("  type =                      %d;  //  Injected wave.  0=sin, 1=sq, 2=tri\n", this->type);
    Serial.printf("  inj_bias =            %7.3f;  // Injected bias, A\n", this->inj_bias);
    Serial.printf("  tbatt_bias =          %7.3f;  // Sense bias, deg C\n", this->tbatt_bias);
    Serial.printf("  s_cap_model =         %7.3f;  // Scalar on battery mod size\n", this->s_cap_model);
    Serial.printf("  cutback_gain_scalar = %7.3f;  // Scalar on battery mod saturation cutback function\n", this->cutback_gain_scalar);
    Serial.printf("  isum =                      %d;  // Summary location ptr.   Begins at -1\n", this->isum);
    Serial.printf("  delta_q_cinf_amp = %10.1f;  // Coul\n", this->delta_q_cinf_amp);
    Serial.printf("  delta_q_dinf_amp = %10.1f;  // Coul\n", this->delta_q_dinf_amp);
    Serial.printf("  delta_q_cinf_noamp=%10.1f;  // Coul\n", this->delta_q_cinf_noamp);
    Serial.printf("  delta_q_dinf_noamp=%10.1f;  // Coul\n", this->delta_q_dinf_noamp);
    Serial.printf("  hys_scale =           %7.3f;  //  Hysteresis scalar\n", this->hys_scale);
    Serial.printf("  tweak_sclr_amp =      %7.3f;  // Tweak calibration for amp current\n", this->tweak_sclr_amp);
    Serial.printf("  tweak_sclr_noamp =    %7.3f;  // Tweak calibration for non-amp current\n", this->tweak_sclr_noamp);
    Serial.printf("  nP =                    %5.2f;  // Num parallel batteries in bank, e.g. '2P1S'\n", this->nP);
    Serial.printf("  nS =                    %5.2f;  // Num series batteries in bank, e.g. '2P1S'\n", this->nS);
    Serial.printf("  mon_mod =                   %d;  //  Mon battery model chars.  0=Battleborn, 1=LION\n", this->mon_mod);
    Serial.printf("  sim_mod =                   %d;  //  Sim battery model chars.  0=Battleborn, 1=LION\n", this->sim_mod);
  }

};            

#endif
