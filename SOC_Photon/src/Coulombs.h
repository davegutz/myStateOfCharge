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


#ifndef COULOMBS_H_
#define COULOMBS_H_

#include "Battery.h"
#include "Chemistry_BMS.h"


// Coulomb Counter Class
class Coulombs
{
public:
  Coulombs();
  Coulombs(double *sp_delta_q, float *sp_t_last, const float q_cap_rated, const float t_rated, const float t_rlim,
    uint8_t *sp_mod_code, const float coul_eff);
  ~Coulombs();
  // operators
  // functions
  void apply_cap_scale(const float scale);
  void apply_delta_q(const double delta_q);
  void apply_resetting(const boolean resetting){ resetting_ = resetting; };
  void apply_soc(const float soc, const float temp_c);
  void apply_delta_q_t(const boolean reset);
  void apply_delta_q_t(const double delta_q, const float temp_c);
  void assign_all_mod(const String mod_str) { chem_.assign_all_mod(mod_str); };
  double calculate_capacity(const float temp_c);
  double coul_eff() { return ( coul_eff_ ); };
  void coul_eff(const double coul_eff) { coul_eff_ = coul_eff; };
  virtual float count_coulombs(const double dt, const boolean reset, const float temp_c, const float charge_curr,
    const boolean sat, const double delta_q_ekf);
  double delta_q() { return(*sp_delta_q_); };
  uint8_t mod_code() { return (*chem_.sp_mod_code); };
  virtual void pretty_print();
  double q(){ return (q_); };
  float q_cap_rated(){ return (q_cap_rated_); };
  float q_cap_scaled(){ return (q_cap_rated_scaled_); };
  float q_capacity(){ return (q_capacity_); };
  float soc() { return(soc_); };
  float soc_min() { return(soc_min_); };
  boolean sat() { return(sat_); };
  float t_last() { return(*sp_t_last_); };
  virtual float vsat(void) = 0;
protected:
  boolean resetting_ = false;  // Sticky flag to coordinate user testing of coulomb counters, T=performing an external reset of counter
  float coul_eff_;    // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
  double q_;          // Present charge available to use, except q_min_, C
  double q_capacity_; // Saturation charge at temperature, C
  double q_cap_rated_;// Rated capacity at t_rated_, saved for future scaling, C
  double q_cap_rated_scaled_;// Applied rated capacity at t_rated_, after scaling, C
  float q_min_;       // Floor on charge available to use, C
  boolean sat_;       // Indication that battery is saturated, T=saturated
  float soc_;         // Fraction of saturation charge (q_capacity_) available (0-1)
  float soc_min_;     // As battery cools, the voltage drops and there appears a minimum soc it can deliver
  double *sp_delta_q_;// Charge since saturated, C
  float *sp_t_last_;  // Last battery temperature for rate limit memory, deg C
  float t_rated_;     // Rated temperature, deg C
  float t_rlim_;      // Tb rate limit, deg C / s
  Chemistry chem_;    // Chemistry
  BMS bms_;           // Battery Management System
};

#endif
