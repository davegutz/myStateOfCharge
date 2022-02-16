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
  double delta_q = -0.5;    // Coulomb Counter state for ekf, (-1 - 1)
  double t_last = 25.;      // Battery temperature past value for rate limit memory, deg C
  double delta_q_model = -0.5;  // Coulomb Counter state for model, (-1 - 1)
  double t_last_model = 25.;  // Battery temperature past value for rate limit memory, deg C
  double curr_bias_amp = CURR_BIAS_AMP; // Calibrate amp current sensor, A 
  double curr_bias_noamp = CURR_BIAS_NOAMP; // Calibrate non-amplified current sensor, A 
  double curr_bias_all = CURR_BIAS_ALL; // Bias all current sensors, A 
  boolean curr_sel_noamp = false; // Use non-amplified sensor
  double vbatt_bias = 0;    // Calibrate Vbatt, V
  boolean modeling = false; // Driving saturation calculation with model
  uint32_t duty = 0;        // Used in Test Mode to inject Fake shunt current (0 - uint32_t(255))
  double amp = 0.;          // Injected amplitude, A pk (0-18.3)
  double freq = 0.;         // Injected frequency, Hz (0-2)
  uint8_t type = 0;         // Injected waveform type.   0=sine, 1=square, 2=triangle
  double offset = 0.;       // Constant bias, A
  double t_bias = 0.;       // Sensed temp bias, deg C
  double s_cap_model = 1.02;// Scalar on battery model size
  double cutback_gain_scalar = 1.;  // Scalar on battery model saturation cutback function.
          // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop.
  int isum = -1;            // Summary location.   Begins at -1 because first action is to increment isum
  double delta_q_inf = 0.;  // delta_q since last reset.  Simple integration of current

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
    this->vbatt_bias = 0;
    this->modeling = false;
    this->duty = 0;
    this->amp = 0.;
    this->freq = 0.;
    this->type = 0;
    this->offset = 0.;
    this->t_bias = 0.;
    this->s_cap_model = 1.02;
    this->cutback_gain_scalar = 1.;
    this->isum = -1;
    this->delta_q_inf = 0.;
  }
  void large_reset()
  {
    this->debug = 2;
    this->delta_q = 0.;           // saturate
    this->delta_q_model = 0.;     // saturate
    this->curr_bias_amp = CURR_BIAS_AMP;
    this->curr_bias_noamp = CURR_BIAS_NOAMP;
    this->curr_bias_all = CURR_BIAS_ALL;
    this->curr_sel_noamp = false;   // non-amplified
    this->vbatt_bias = 0;
    this->modeling = false;
    this->duty = 0;
    this->amp = 0.;
    this->freq = 0.;
    this->type = 0;
    this->offset = 0.;
    this->t_bias = 0.;
    this->s_cap_model = 1.02;
    this->cutback_gain_scalar = 1.;
    this->isum = -1;
    this->delta_q_inf = 0.;
  }
  void print_part_1(char *buffer)
  {
    sprintf(buffer, "debug = %d, delta_q = %7.3f, t_last = %7.3f, delta_q_model = %7.3f, t_last_model = %7.3f, \n\
    curr_bias_amp = %7.3f, curr_bias_noamp = %7.3f, curr_bias_all = %7.3f, curr_sel_noamp = %d, \n",
      this->debug, this->delta_q, this->t_last, this->delta_q_model, this->t_last_model,
      this->curr_bias_amp, this->curr_bias_noamp, this->curr_bias_all, this->curr_sel_noamp);
  }
  void print_part_2(char *buffer)
  {
    sprintf(buffer, "    vbatt_bias = %7.3f, modeling = %d, duty = %ld, amp = %7.3f, freq = %7.3f, type = %d, offset = %7.3f, \n\
    t_bias = %7.3f, s_cap_model = %7.3f, cutback_gain_scalar = %7.3f, delta_q_inf = %7.3f,\n",
      this->vbatt_bias, this->modeling, this->duty, this->amp, this->freq, this->type, this->offset,
      this->t_bias, this->s_cap_model, this->cutback_gain_scalar, this->delta_q_inf);
  }
};            

#endif
