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

// Definition of structure to be saved in SRAM
// Default values below are important:  they prevent junk
// behavior on initial build.
struct RetainedPars
{
  int8_t debug = 2;         // Level of debug printing
  double curr_bias_amp = 0; // Calibrate amp current sensor, A 
  double curr_bias_noamp = 0; // Calibrate non-amplified current sensor, A 
  boolean curr_sel_amp = true; // Use amplified sensor
  double socs = 0.5;        // Coulomb counter scaled, normalized, state (0 - 1)
  double socu = 0.5;        // Coulomb counter state (-0.1 - 1.5 approx)
  double socu_free = 0.5;   // Coulomb Counter state (0 - 1.5)
  double vbatt_bias = 0;    // Calibrate Vbatt, V
  double delta_soc = -0.5;  // Coulomb Counter state for ekf, (-1 - 1)
  double delta_socs = -0.5;  // Coulomb Counter state for ekf, (-1 - 1)
  double t_sat = 25.;       // Battery temperature at saturation, deg C
  double soc_sat = 1.;      // Battery charge at saturation, Ah
  double socs_sat = 1.;      // Battery charge at saturation, Ah
  boolean modeling = false; // Driving saturation calculation with model
  uint32_t duty = 0;        // Used in Test Mode to inject Fake shunt current (0 - uint32_t(255))
  double amp = 0.;          // Injected amplitude, A pk (0-18.3)
  double freq = 0.;         // Injected frequency, Hz (0-2)
  uint8_t type = 0;         // Injected waveform type.   0=sine, 1=square, 2=triangle
  double offset = 0;        // Constant bias, A
};            

#endif
