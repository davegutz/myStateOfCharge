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


#ifndef BATTERY_H_
#define BATTERY_H_

#include "myTables.h"

class Battery
{
public:
  Battery();
  Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat,
    const double *x_dv_tab, const double *y_dv_tab, const double *dv_tab, const unsigned int nd, const unsigned int md);
  ~Battery();
  // operators
  // functions
  void Dv(const double dv) { dv_ = dv; };
  void Sr(const double sr) { sr_ = sr; };
  double fudge(const double soc, const double tc);
  double calculate(const double temp_C, const double soc_frac, const double curr_in);
  double num_cells() { return (num_cells_); };
  double soc() { return (soc_); };
  double voc() { return (voc_); };
  double vdyn() { return (vdyn_); };
  double v() { return (v_); };
  double tcharge() { return (tcharge_); };
  double dv_dsoc() { return (dv_dsoc_); };
  double Dv() { return (dv_); };
  double Sr() { return (sr_); };
  boolean sat() { return (sat_); };
  boolean sat(const double v, const double i) { return (v-i*(r1_+r2_)>= vsat_); };
  // double d2v_dsoc2() { return (d2v_dsoc2_); };
protected:
  TableInterp1Dclip *B_T_;  // Battery coeff
  TableInterp1Dclip *A_T_;  // Battery coeff
  TableInterp1Dclip *C_T_;  // Battery coeff
  TableInterp2D *dV_T_;     // Real-life fudge facto
  double b_;        // Battery coeff
  double a_;        // Battery coeff
  double c_;        // Battery coeff
  double m_;        // Battery coeff
  double n_;        // Battery coeff
  double d_;        // Battery coeff
  unsigned int nz_; // Number of breakpoints
  double soc_;      // State of charge
  double r1_;       // Randels resistance, Ohms per cell
  double r2_;       // Randels resistance, Ohms per cell
  double c2_;       // Randels capacitance, Farads per cell
  double voc_;      // Static model open circuit voltage, V
  double vdyn_;     // Model current induced back emf, V
  double v_;        // Total model voltage, V
  double curr_in_;  // Current into battery, A
  int num_cells_;   // Number of cells
  double dv_dsoc_;  // Derivative, V/fraction
  // double d2v_dsoc2_; // Derivative, V^2/fraction^2
  double tcharge_;  // Charging time to 100%, hr
  double pow_in_;   // Charging power, w
  double sr_;       // Resistance scalar
  double vsat_; // Saturation threshold
  boolean sat_;     // Saturation status
  unsigned int nd_; // Number x breakpoints in dV_T
  unsigned int md_; // Number y breakpoints in dV_T
  double fudge_;    // Fudge factor from test data table, V
  double dv_;       // Adjustment, V
};

// BattleBorn 100 Ah, 12v LiFePO4
#define NOM_SYS_VOLT          12        // Nominal system output, V, at which the reported amps are used (12)
#define NOM_BATT_CAP          100       // Nominal battery bank capacity, Ah (100).   Accounts for internal losses.  This is 
                                        // what gets delivered, e.g. Wshunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C currents
                                        // or 20-40 A for a 100 Ah battery
// >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 60-95 F
#define BATT_V_SAT            3.4625    // Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
#define BATT_R1               0.00126   // Battery Randels static resistance, Ohms (0.00126) for 3v cell matches transients
#define BATT_R2               0.00168   // Battery Randels dynamic resistance, Ohms (0.00168) for 3v cell matches transients
#define BATT_R2C2             100       // Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                                        // test so using with R2 and then multiplying by 4 for total result is valid,
                                        // though probably not for an individual cell
const int batt_num_cells = NOM_SYS_VOLT/3;  // Number of standard 3 volt LiFePO4 cells
const double batt_vsat = double(batt_num_cells)*double(BATT_V_SAT);  // Total bank saturation for 0.997=soc, V
const double batt_vmax = (14.3/4)*double(batt_num_cells); // Observed max voltage of 14.3 V for 12V prototype bank, V
const double batt_r1 = double(BATT_R1);     // Randels static resistance per cell, Ohms
const double batt_r2 = double(BATT_R2);     // Randels dynamic resistance per cell, Ohms
const double batt_r2c2 = double(BATT_R2C2);// Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                              // test so using with R2 and then multiplying by 4 for total result is valid,
                              // though probably not for an individual cell
const double batt_c2 = double(BATT_R2C2)/batt_r2;

// Battery model LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang etal.pdf'
// SOC-OCV curve fit
static const double t_bb[7] = {-10.,	 0.,	10.,	20.,	30.,	40.,	50.};
const double m_bb = 0.478;
static const double b_bb[7] = {-1.143251503,	-1.143251503,	-1.143251503,	-0.5779554,	-0.553297988,	-0.557104757,	-0.45551626};
static const double a_bb[7] = {3.348339977,	3.5247557,	3.553964435,	2.778271021,	2.806905998,	2.851255776,	2.731041762};
static const double c_bb[7] = {-1.770434633,	-1.770434633,	-1.770434633,	-1.099451796,	-1.125467829,	-1.159227563,	-1.059028383};
const double n_bb = 0.4;
const double d_bb = 1.734;
const unsigned int nz_bb = 7;

// Fudge factor to OCV calculation to match data
static const unsigned int n_dV = 6;
static const double x_dV[n_dV] = {0,  0.2,  0.4,  0.6,  0.8,  1.0};
static const unsigned int m_dV = 7;
static const double y_dV[m_dV] =      {-10., 0.,	10., 20.,	30., 40.,	50.};
static const double t_dV[m_dV*n_dV] = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                                        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
/*
static const double t_dV[m_dV*n_dV] = { -10, 0, 10, 20, 30, 40, 50,
                                        -12, 2, 12, 22, 32, 42, 52,
                                        -14, 4, 14, 24, 34, 44, 54,
                                        -16, 6, 16, 26, 36, 46, 56,
                                        -18, 8, 18, 28, 38, 48, 58,
                                        -19.999, 9.999, 19.999, 29.999, 39.999, 49.999, 59.999};
*/

// Charge test profiles
#define NUM_VEC           1   // Number of vectors defined here
static const unsigned int n_v1 = 10;
static const double t_min_v1[n_v1] =  {0,     0.2,   0.2001, 1.4,   1.4001, 2.0999, 2.0,    3.1999, 3.2,    3.6};
static const double v_v1[n_v1] =      {13.95, 13.95, 13.95,  13.0,  13.0,   13.0,   13.0,   13.95,  13.95,  13.95}; // Saturation 13.7
static const double i_v1[n_v1] =      {0.,    0.,    -500.,  -500., 0.,     0.,     500.,   500.,   0.,     0.};
static const double T_v1[n_v1] =      {72.,   72.,   72.,    72.,   72.,    72.,    72.,    72.,    72.,    72.};
static TableInterp1Dclip  *V_T1 = new TableInterp1Dclip(n_v1, t_min_v1, v_v1);
static TableInterp1Dclip  *I_T1 = new TableInterp1Dclip(n_v1, t_min_v1, i_v1);
static TableInterp1Dclip  *T_T1 = new TableInterp1Dclip(n_v1, t_min_v1, T_v1);

#endif