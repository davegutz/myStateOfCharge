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

class Sensors;

// Battery chemistry
struct Chemistry
{
  uint8_t *sp_mod_code;  // Chemistry code integer
  float dqdt;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
  float low_voc;    // Voltage threshold for BMS to turn off battery, V
  float low_t;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C 
  uint8_t m_t = 0;  // Number temperature breakpoints for voc table
  float *y_t;       // Temperature breakpoints for voc table
  uint8_t n_s = 0;  // Number of soc breakpoints voc table
  float *x_soc;     // soc breakpoints for voc table
  float *t_voc;     // voc(soc,t)
  uint8_t n_n = 0;  // Number temperature breakpoints for soc_min table
  float *x_soc_min; // Temperature breakpoints for soc_min table
  float *t_soc_min; // soc_min table
  float hys_cap;    // Capacitance of hysteresis, Farads
  uint8_t n_h = 0;  // Number of dv breakpoints in r(soc, dv) table t_r, t_s
  uint8_t m_h = 0;  // Number of soc breakpoints in r(soc, dv) table t_r, t_s
  float *x_dv;      // dv breakpoints for r(soc, dv) table t_r, t_s
  float *y_soc;     // soc breakpoints for r(soc, dv) tables t_r, t_s, t_x, t_n
  float *t_r;       // r(soc, dv) table
  float *t_s;       // s(soc, dv) table
  float *t_x;       // r_max(soc) table
  float *t_n;       // r_min(soc) table
  float v_sat;      // Saturation threshold at temperature, deg C
  float dvoc_dt;    // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  float dvoc;       // Adjustment for calibration error, V
  float r_0;        // ChargeTransfer R0, ohms
  float r_ct;     // ChargeTransfer charge transfer resistance, ohms
  float tau_ct;   // ChargeTransfer charge transfer time constant, s (=1/Rct/Cct)
  float tau_sd;     // Equivalent model for EKF reference.	Parasitic discharge time constant, sec
  float r_sd;       // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  float c_sd;       // Equivalent model for EKF reference.  Parasitic discharge equivalent, Farads
  float r_ss;       // Equivalent model for state space model initialization, ohms
  TableInterp2D *voc_T_;      // SOC-VOC 2-D table, V
  TableInterp1D *soc_min_T_;  // SOC-MIN 1-D table, V
  void assign_BB();   // Battleborn assignment
  void assign_CH();   // CHINS assignment
  void assign_SP();   // Spare assignment
  void assign_all_mod(const String mod_str);  // Assignment executive
  void assign_hys(const int _n_h, const int _m_h, const float *x, const float *y, const float *t, const float *s,
    const float *tx, const float *tn); // Worker bee Hys
  void assign_soc_min(const int _n_n, const float *x, const float *t);  // Worker bee SOC_MIN
  void assign_voc_soc(const int _n_s, const int _m_t, const float *x, const float *y, const float *t); // Worker bee VOC_SOC
  String decode(const uint8_t mod);
  uint8_t encode(const String mod_str);
  void pretty_print();
  Chemistry();
  Chemistry(const String mod_str)
  {
    sp_mod_code = new uint8_t(-1);
    assign_all_mod(mod_str);
  }
  Chemistry(uint8_t *mod_code)
  {
    sp_mod_code = mod_code;
    String mod_str = decode(*sp_mod_code);
    assign_all_mod(mod_str);
  }
};

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
  float coul_eff_;   // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
  double q_;          // Present charge available to use, except q_min_, C
  double q_capacity_; // Saturation charge at temperature, C
  double q_cap_rated_;// Rated capacity at t_rated_, saved for future scaling, C
  double q_cap_rated_scaled_;// Applied rated capacity at t_rated_, after scaling, C
  float q_min_;      // Floor on charge available to use, C
  boolean sat_;       // Indication that battery is saturated, T=saturated
  float soc_;        // Fraction of saturation charge (q_capacity_) available (0-1)
  float soc_min_;    // As battery cools, the voltage drops and there appears a minimum soc it can deliver
  double *sp_delta_q_;// Charge since saturated, C
  float *sp_t_last_;  // Last battery temperature for rate limit memory, deg C
  float t_rated_;    // Rated temperature, deg C
  float t_rlim_;     // Tb rate limit, deg C / s
  Chemistry chem_; // Storage of chemistry information
};

#endif
