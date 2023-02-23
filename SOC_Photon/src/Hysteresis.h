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


#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include "Chemistry_BMS.h"

// Hysteresis: reservoir model of battery electrical hysteresis
// Use variable resistor and capacitor to create hysteresis from an RC circuit
class Hysteresis
{
public:
  Hysteresis();
  Hysteresis(Chemistry *chem);
  ~Hysteresis();
  // operators
  // functions
  float calculate(const float ib, const float soc);
  float dv_max(const float soc) { return chem_->hys_Tx_->interp(soc); };
  float dv_min(const float soc) { return chem_->hys_Tn_->interp(soc); };
  void init(const float dv_init);
  float look_hys(const float dv, const float soc);
  float look_slr(const float dv, const float soc);
  void pretty_print();
  float update(const double dt, const boolean init_high, const boolean init_low, const float e_wrap, const boolean reset);
  float ibs() { return ibs_; };
  float ioc() { return ioc_; };
  float dv_hys() { return dv_hys_; };
  void dv_hys(const float st) { dv_hys_ = max(min(st, dv_max(soc_)), dv_min(soc_)); };
//   float scale() { return sp.hys_scale(); };
protected:
  boolean disabled_;    // Hysteresis disabled by low scale input < 1e-5, T=disabled
  float res_;          // Variable resistance value, ohms
  float slr_;          // Variable scalar
  float soc_;          // State of charge input, dimensionless
  float ib_;           // Current in, A
  float ibs_;          // Scaled current in, A
  float ioc_;          // Current out, A
  float dv_hys_;       // State, voc_-voc_stat_, V
  float dv_dot_;       // Calculated voltage rate, V/s
  Chemistry *chem_;    // Chemistry
};


// Methods

#endif