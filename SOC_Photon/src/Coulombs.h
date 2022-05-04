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


#ifndef COULOMBS_H_
#define COULOMBS_H_

// Battery chemistry
struct Chemistry
{
  float dqdt;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
  float low_voc;    // Voltage threshold for BMS to turn off battery, V
  float low_t;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C 
  uint8_t m_t;      // Number temperature breakpoints for voc table
  float *y_t;       // Temperature breakpoints for voc table
  uint8_t n_s;      // Number of soc breakpoints voc table
  float *x_soc;     // soc breakpoints for voc table
  float *t_voc;     // voc(soc,t)
  uint8_t n_n;      // Number temperature breakpoints for soc_min table
  float *x_soc_min; // Temperature breakpoints for soc_min table
  float *t_soc_min; // soc_min table
  float hys_cap;    // Capacitance of hysteresis, Farads
  uint8_t n_h;      // Number of dv breakpoints in r(soc, dv) table t_r
  uint8_t m_h;      // Number of soc breakpoints in r(soc, dv) table t_r
  float *x_dv;      // dv breakpoints for r(soc, dv) table t_r
  float *y_soc;     // soc breakpoints for r(soc, dv) table t_r
  float *t_r;       // r(soc, dv) table
  float v_sat;      // Saturation threshold at temperature, deg C
  float dvoc_dt;    // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  float dv;         // Adjustment for calibration error, V
  float r_0;        // Randles R0, ohms
  float r_ct;       // Randles charge transfer resistance, ohms
  float r_diff;     // Randles diffusion resistance, ohms
  float tau_ct;     // Randles charge transfer time constant, s (=1/Rct/Cct)
  float tau_diff;   // Randles diffusion time constant, s (=1/Rdif/Cdif)
  float tau_sd;     // Equivalent model for EKF reference.	Parasitic discharge time constant, sec
  float r_sd;       // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  float r_ss;       // Equivalent model for state space model initialization, ohms
  void assign_mod(const String mod_str);
  String decode(const uint8_t mod);
  uint8_t encode(const String mod_str);
  Chemistry(const String mod_str)
  {
    assign_mod(mod_str);
  }
  Chemistry(const uint8_t mod)
  {
    String mod_str = decode(mod);
    assign_mod(mod_str);
  }
};

// Coulomb Counter Class
class Coulombs
{
public:
  Coulombs();
  Coulombs(double *rp_delta_q, float *rp_t_last, const double q_cap_rated, const double t_rated, const double t_rlim,
    const String batt_mod);
  ~Coulombs();
  // operators
  // functions
  void apply_cap_scale(const double scale);
  void apply_delta_q(const double delta_q);
  void apply_resetting(const boolean resetting){ resetting_ = resetting; };
  void apply_soc(const double soc, const double temp_c);
  void apply_SOC(const double SOC, const double temp_c);
  void apply_delta_q_t(const double delta_q, const double temp_c);
  double calculate_capacity(const double temp_c);
  virtual double count_coulombs(const double dt, const boolean reset, const double temp_c, const double charge_curr,
    const boolean sat, const double t_last);
  double delta_q() { return(*rp_delta_q_); };
  virtual void pretty_print();
  double q(){ return (q_); };
  double q_cap_rated(){ return (q_cap_rated_); };
  double q_cap_scaled(){ return (q_cap_rated_scaled_); };
  double q_capacity(){ return (q_capacity_); };
  double soc() { return(soc_); };
  double SOC() { return(SOC_); };
  boolean sat() { return(sat_); };
  double t_last() { return(*rp_t_last_); };
  virtual double vsat(void) = 0;
protected:
  double *rp_delta_q_;// Charge since saturated, C
  float *rp_t_last_;  // Last battery temperature for rate limit memory, deg C
  double q_cap_rated_;// Rated capacity at t_rated_, saved for future scaling, C
  double q_cap_rated_scaled_;// Applied rated capacity at t_rated_, after scaling, C
  double q_capacity_; // Saturation charge at temperature, C
  double q_;          // Present charge available to use, except q_min_, C
  double soc_;        // Fraction of saturation charge (q_capacity_) available (0-1)
  double SOC_;        // Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries.
  boolean sat_;       // Indication calculated by caller that battery is saturated, T=saturated
  double t_rated_;    // Rated temperature, deg C
  double t_rlim_;     // Tbatt rate limit, deg C / s
  boolean resetting_ = false;  // Flag to coordinate user testing of coulomb counters, T=performing an external reset of counter
  double soc_min_;    // As battery cools, the voltage drops and there appears a minimum soc it can deliver
  double q_min_;      // Floor on charge available to use, C
  TableInterp1D *soc_min_T_;   // SOC-MIN 1-D table, V
  Chemistry chem_; // Storage of chemistry information
};

#endif
