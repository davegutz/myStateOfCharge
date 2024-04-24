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


#ifndef CHEMISTRY_BMS_H_
#define CHEMISTRY_BMS_H_


// Battery Management System - built into battery
struct BMS
{
public:
  float low_voc;    // Voltage threshold for BMS to turn off battery, V
  float low_t;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
  float vb_off;     // Shutoff point in Mon, V
  float vb_down;    // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V
  float vb_rising;  // Shutoff point when off, V
  float vb_down_sim;    // Shutoff point in Sim, V
  float vb_rising_sim;  // Shutoff point in Sim when off, V
};


// Battery chemistry
struct Chemistry: public BMS
{
public:
  uint8_t *sp_mod_code;  // Chemistry code integer
  float rated_temp; // Temperature at NOM_UNIT_CAP, deg C
  double coul_eff;  // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
  float dqdt;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
  uint8_t m_t = 0;  // Number temperature breakpoints for voc table
  uint8_t n_s = 0;  // Number of soc breakpoints voc table
  uint8_t n_n = 0;  // Number temperature breakpoints for soc_min table
  float hys_cap;    // Capacitance of hysteresis, Farads
  uint8_t n_h = 0;  // Number of dv breakpoints in r(soc, dv) table t_r, t_s
  uint8_t m_h = 0;  // Number of soc breakpoints in r(soc, dv) table t_r, t_s
  float v_sat;      // Saturation threshold at temperature, deg C
  float dv_min_abs; // Absolute value of +/- hysteresis limit, V
  float dvoc_dt;    // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  float r_0;        // ChargeTransfer R0, ohms
  float r_ct;     // ChargeTransfer charge transfer resistance, ohms
  float tau_ct;   // ChargeTransfer charge transfer time constant, s (=1/Rct/Cct)
  float tau_sd;     // Equivalent model for EKF reference.	Parasitic discharge time constant, sec
  float r_sd;       // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  float c_sd;       // Equivalent model for EKF reference.  Parasitic discharge equivalent, Farads
  float r_ss;       // Equivalent model for state space model initialization, ohms
  TableInterp1D *hys_Tn_;     // soc 1-D table, V_min
  TableInterp2D *hys_Ts_;     // dv-soc 2-D table scalar
  TableInterp1D *hys_Tx_;     // soc 1-D table, V_max
  TableInterp2D *hys_T_;      // dv-soc 2-D table, V
  TableInterp2D *voc_T_;      // SOC-VOC 2-D table, V
  TableInterp1D *soc_min_T_;  // SOC-MIN 1-D table, V
  Chemistry();
  Chemistry(const String mod_str)
  {
    sp_mod_code = new uint8_t(-1);
    assign_all_chm(mod_str);
  }
  Chemistry(uint8_t *mod_code)
  {
    sp_mod_code = mod_code;
    String mod_str = decode(*sp_mod_code);
    assign_all_chm(mod_str);
  }
  void assign_BB();   // Battleborn assignment
  void assign_CH();   // CHINS assignment
  void assign_all_chm(const String mod_str);  // Assignment executive
  void assign_hys(const int _n_h, const int _m_h, float *x, float *y, float *t, float *s,
    float *tx, float *tn); // Worker bee Hys
  void assign_soc_min(const int _n_n, float *x, float *t);  // Worker bee SOC_MIN
  void assign_voc_soc(const int _n_s, const int _m_t, float *x, float *y, float *t); // Worker bee VOC_SOC
  String decode(const uint8_t mod);
  uint8_t encode(const String mod_str);
  void pretty_print();
};


#endif
