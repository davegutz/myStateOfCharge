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


// Coulomb Counter Class
class Coulombs
{
public:
  Coulombs();
  Coulombs(const double q_cap_rated, const double t_rated, const double t_rlim);
  ~Coulombs();
  // operators
  // functions
  void apply_cap_scale(const double scale);
  void apply_delta_q(const double delta_q);
  void apply_resetting(const boolean resetting){ resetting_ = resetting; };
  void apply_soc(const double soc, const double temp_c);
  void apply_SOC(const double SOC, const double temp_c);
  void apply_delta_q_inf(const double delta_q_inf);
  virtual void apply_delta_q_t(const double delta_q, const double temp_c) {};
  void apply_delta_q_t(const double delta_q, const double temp_c, const double delta_q_inf);
  double calculate_capacity(const double temp_c);
  virtual double count_coulombs(const double dt, const boolean reset, const double temp_c, const double charge_curr,
    const boolean sat, const double t_last);
  double delta_q() { return(delta_q_); };
  void load(const double delta_q, const double t_last, const double delta_q_inf);
  virtual void pretty_print();
  double q(){ return (q_); };
  double q_cap_rated(){ return (q_cap_rated_); };
  double q_cap_scaled(){ return (q_cap_rated_scaled_); };
  double q_capacity(){ return (q_capacity_); };
  double soc() { return(soc_); };
  double SOC() { return(SOC_); };
  boolean sat() { return(sat_); };
  double t_last() { return(t_last_); };
  double delta_q_inf() { return(delta_q_inf_); };
  virtual void update(double *delta_q, double *t_last) {};
  void update(double *delta_q, double *t_last, double *delta_q_inf);
protected:
  double q_cap_rated_;// Rated capacity at t_rated_, saved for future scaling, C
  double q_cap_rated_scaled_;// Applied rated capacity at t_rated_, after scaling, C
  double q_capacity_; // Saturation charge at temperature, C
  double q_;          // Present charge available to use, C
  double delta_q_;    // Charge since saturated, C
  double soc_;        // Fraction of saturation charge (q_capacity_) available (0-1)
  double SOC_;        // Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries.
  boolean sat_;       // Indication calculated by caller that battery is saturated, T=saturated
  double t_rated_;    // Rated temperature, deg C
  double t_last_;     // Last battery temperature for rate limit memory, deg C
  double t_rlim_;     // Tbatt rate limit, deg C / s
  boolean resetting_ = false;  // Flag to coordinate user testing of coulomb counters, T=performing an external reset of counter
  double soc_min_;    // As battery cools, the voltage drops and there appears a minimum soc it can deliver
  double q_min_;      // Floor on charge available to use, C
  TableInterp1D *soc_min_T_;   // SOC-MIN 1-D table, V
  double delta_q_inf_;  // Absolute simple integration of current since large reset, C
};

#endif
