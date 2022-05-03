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

#include "local_config.h"

// Definition of structure to be saved in SRAM
// Default values below are important:  they prevent junk
// behavior on initial build.
// ********CAUTION:  any special includes or logic in here breaks retained function
struct RetainedPars
{
  int8_t debug = 2;         // Level of debug printing
  double delta_q = -0.5;    // Charge change since saturated, C
  float t_last = 25.;      //Updated value of battery temperature injection when rp.modeling and proper wire connections made, deg C
  double delta_q_model = -0.5;  // Coulomb Counter state for model, (-1 - 1)
  float t_last_model = 25.;  // Battery temperature past value for rate limit memory, deg C
  float curr_bias_amp = CURR_BIAS_AMP; // Calibrate amp current sensor, A 
  float curr_bias_noamp = CURR_BIAS_NOAMP; // Calibrate non-amplified current sensor, A 
  float curr_bias_all = CURR_BIAS_ALL; // Bias all current sensors, A 
  boolean curr_sel_noamp = false; // Use non-amplified sensor
  float vbatt_bias = VOLT_BIAS;    // Calibrate Vbatt, V
  boolean modeling = false; // Driving saturation calculation with model
  uint32_t duty = 0;        // Used in Test Mode to inject Fake shunt current (0 - uint32_t(255))
  float amp = 0.;          // Injected amplitude, A pk (0-18.3)
  float freq = 0.;         // Injected frequency, Hz (0-2)
  uint8_t type = 0;         // Injected waveform type.   0=sine, 1=square, 2=triangle
  float inj_soft_bias = 0.;       // Constant bias, A
  float t_bias = TEMP_BIAS;       // Sensed temp bias, deg C
  float s_cap_model = 1.;  // Scalar on battery model size
  float cutback_gain_scalar = 1.;  // Scalar on battery model saturation cutback function
          // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
  int isum = -1;                // Summary location.   Begins at -1 because first action is to increment isum
  float delta_q_inf_amp = 0.;  // delta_q since last reset.  Simple integration of amplified current
  float delta_q_inf_noamp = 0.;// delta_q since last reset.  Simple integration of non-amplified current
  float hys_scale = 1.;        // Hysteresis scalar
  boolean tweak_test = false;   // Driving signal injection completely using software inj_soft_bias 
  float tweak_bias_amp = 0.;   // Tweak calibration for amplified current sensor
  float tweak_bias_noamp = 0.; // Tweak calibration for non-amplified current sensor
  float nP = NP;               // Number parallel batteries in bank
  float nS = NS;               // Number series batteries in bank

  // Nominalize
  void nominal()
  {
    this->debug = 2;
    this->delta_q = -0.5;
    this->t_last = 25.;
    this->delta_q_model = -0.5;
    this->t_last_model = 25.;
    this->curr_bias_amp = CURR_BIAS_AMP;
    this->curr_bias_noamp = CURR_BIAS_NOAMP;
    this->curr_bias_all = CURR_BIAS_ALL;
    this->curr_sel_noamp = false;
    this->vbatt_bias = VOLT_BIAS;
    this->modeling = false;
    this->duty = 0;
    this->amp = 0.;
    this->freq = 0.;
    this->type = 0;
    this->inj_soft_bias = 0.;
    this->t_bias = TEMP_BIAS;
    this->s_cap_model = 1.0;
    this->cutback_gain_scalar = 1.;
    this->isum = -1;
    this->delta_q_inf_amp = 0.;
    this->delta_q_inf_noamp = 0.;
    this->hys_scale = 1.;
    this->tweak_test = false;
    this->tweak_bias_amp = 0.;
    this->tweak_bias_noamp = 0.;
    this->nP = 1.;
    this->nS = 1.;
  }
  void large_reset()
  {
    this->debug = 2;
    this->delta_q = 0.;           // saturate
    this->delta_q_model = 0.;     // saturate
    this->curr_bias_amp = CURR_BIAS_AMP;
    this->curr_bias_noamp = CURR_BIAS_NOAMP;
    this->curr_bias_all = CURR_BIAS_ALL;
    this->curr_sel_noamp = false;   // T=amplified
    this->vbatt_bias = VOLT_BIAS;
    this->modeling = false;
    this->duty = 0;
    this->amp = 0.;
    this->freq = 0.;
    this->type = 0;
    this->inj_soft_bias = 0.;
    this->t_bias = TEMP_BIAS;
    this->s_cap_model = 1.0;
    this->cutback_gain_scalar = 1.;
    this->isum = -1;
    this->delta_q_inf_amp = 0.;
    this->delta_q_inf_noamp = 0.;
    this->hys_scale = 1.;
    this->tweak_test = false;
    this->tweak_bias_amp = 0.;
    this->tweak_bias_noamp = 0.;
    this->nP = 1.;
    this->nS = 1.;
  }
  
  // Print
  void pretty_print(void)
  {
    Serial.printf("retained parameters (rp):\n");
    Serial.printf("  debug =                     %d;  // Level of debug printing\n", this->debug);
    Serial.printf("  delta_q =             %7.3f;  // Coulomb Counter state for ekf, (-1 - 1)\n", this->delta_q);
    Serial.printf("  t_last =              %7.3f;  // Battery temperature past value for rate limit memory, deg C\n", this->t_last);
    Serial.printf("  delta_q_model =    %10.1f;  // Coulomb Counter state for model, (-1 - 1)\n", this->delta_q_model);
    Serial.printf("  t_last_model =        %7.3f;  // Battery temperature past value for rate limit memory, deg C\n", this->t_last_model);
    Serial.printf("  curr_bias_amp =       %7.3f;  // Calibrate amp current sensor, A\n", this->curr_bias_amp);
    Serial.printf("  curr_bias_noamp =     %7.3f;  // Calibrate non-amplified current sensor, A\n", this->curr_bias_noamp);
    Serial.printf("  curr_bias_all =       %7.3f;  // Bias all current sensors, A \n", this->curr_bias_all);
    Serial.printf("  curr_sel_noamp =            %d;  // Use non-amplified sensor\n", this->curr_sel_noamp);
    Serial.printf("  vbatt_bias =          %7.3f;  // Calibrate Vbatt, V\n", this->vbatt_bias);
    Serial.printf("  modeling =                  %d;  // Driving saturation calculation with model\n", this->modeling);
    Serial.printf("  duty =                      %ld;  // Used in Test Mode to inject Fake shunt current (0 - uint32_t(255))\n", this->duty);
    Serial.printf("  amp =                 %7.3f;  // Injected amplitude, A pk (0-18.3)\n", this->amp);
    Serial.printf("  freq =                %7.3f;  // Injected frequency, Hz (0-2)\n", this->freq);
    Serial.printf("  type =                      %d;  //  Injected waveform type.   0=sine, 1=square, 2=triangle\n", this->type);
    Serial.printf("  inj_soft_bias =       %7.3f;  // Constant bias, A\n", this->inj_soft_bias);
    Serial.printf("  t_bias =              %7.3f;  // Sensed temp bias, deg C\n", this->t_bias);
    Serial.printf("  s_cap_model =         %7.3f;  // Scalar on battery model size\n", this->s_cap_model);
    Serial.printf("  cutback_gain_scalar = %7.3f;  // Scalar on battery model saturation cutback function\n", this->cutback_gain_scalar);
    Serial.printf("  isum =                      %d;  // Summary location.   Begins at -1 because first action is to increment isum\n", this->isum);
    Serial.printf("  delta_q_inf_amp =  %10.1f;  // delta_q since last reset.  Simple integration of amplified current\n", this->delta_q_inf_amp);
    Serial.printf("  delta_q_inf_noamp =%10.1f;  // delta_q since last reset.  Simple integration of amplified current\n", this->delta_q_inf_noamp);
    Serial.printf("  hys_scale =           %7.3f;  //  Hysteresis scalar\n", this->hys_scale);
    Serial.printf("  tweak_test =                %d;  // Driving signal injection completely using software inj_soft_bias \n", this->tweak_test);
    Serial.printf("  tweak_bias_amp =      %7.3f;  // Tweak calibration for amplified current sensor\n", this->tweak_bias_amp);
    Serial.printf("  tweak_bias_noamp =    %7.3f;  // Tweak calibration for non-amplified current sensor\n", this->tweak_bias_noamp);
    Serial.printf("  nP =                    %5.2f;  // Number of parallel batteries in bank, e.g. '2P1S'\n", this->tweak_bias_noamp);
    Serial.printf("  nS =                    %5.2f;  // Number of series batteries in bank, e.g. '2P1S'\n", this->tweak_bias_noamp);
  }

};            

#endif
