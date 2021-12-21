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
  void prime(const double init_q, const double init_t_c);
  void apply_resetting(const boolean resetting){ resetting_ = resetting; };
  void apply_soc(const double soc);
  void apply_SOC(const double SOC);
  void apply_delta_q(const double delta_q);
  double calculate_capacity(const double temp_c);
  double count_coulombs(const double dt, const double temp_c, const double charge_curr, const boolean sat, const double t_last);
  void load(const double delta_q, const double t_last);
  void update(double *delta_q, double *t_last);
protected:
  double q_cap_rated_;// Rated capacity at t_rated_, C
  double q_capacity_; // Saturation charge at temperature, C
  double q_;          // Present charge available to use, C
  double delta_q_;    // Charge since saturated, C
  double soc_;        // Fraction of saturation charge (q_capacity_) available (0-1)
  double SOC_;        // Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries.
  double t_rated_;    // Rated temperature, deg C
  double t_last_;     // Last battery temperature for rate limit memory, deg C
  double t_rlim_;     // Tbatt rate limit, deg C / s
  boolean resetting_ = false;  // Flag to coordinate user testing of coulomb counters, T=performing an external reset of counter
};


// Coulomb Counter Structure
struct CoulombCounter
{
  double nom_q_cap = 0;   // Rated capacity at t_rated_, C
  double q_cap = 0;       // Capacity normalized to rated temperature, C
  double q_capacity = 0;  // Saturation charge at temperature, C
  double q = 0;           // Present charge available to use, C
  double delta_q = 0;     // Charge since saturated, C
  double q_sat = 0;       // Saturation charge, C
  double t_sat = 0;       // Battery temperature at saturation, deg C
  double soc = 0;         // Fraction of saturation charge (q_capacity_) available (0-1)
  double SOC = 0;         // Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries.
  double t_rated = 0;     // Rated temperature, deg C
  double t_last = 25.;    // Last battery temperature for rate limit memory, deg C
  double t_rat = 2.5;     // Tbatt rate limit, deg C / s
  boolean resetting = false;  // Flag to coordinate user testing of coulomb counters, T=performing an external reset of counter
  CoulombCounter();
  void prime(const double nom_q_cap, const double t_rate, const double init_q, const double init_t_c, const double s_cap);
  // operators
  // functions
  void apply_cap_scale(const double scale_);
  void apply_soc(const double soc_);
  void apply_SOC(const double SOC_);
  void apply_delta_q(const double delta_q_);
  double calculate_capacity(const double temp_c_);
  double calculate_saturation_charge(const double t_sat_, const double q_cap_);
  double count_coulombs(const double dt_, const double temp_c_, const double charge_curr_, const boolean sat_, const double t_last_);
  void load(const double delta_q_, const double t_sat_, const double q_sat_, const double t_last_);
  void update(double *delta_q_, double *t_sat_, double *q_sat_, double *t_last_);
};

#endif
