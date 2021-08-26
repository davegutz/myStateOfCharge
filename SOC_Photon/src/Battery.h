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
    const double r1, const double r2, const double r2c2);
  ~Battery();
  // operators
  // functions
  double calculate(const double temp_C, const double soc_frac, const double curr_in);
  double num_cells() { return (num_cells_); };
  double soc() { return (soc_); };
  double vstat() { return (vstat_); };
  double vdyn() { return (vdyn_); };
  double v() { return (v_); };
  double dv_dsoc() { return (dv_dsoc_); };
  // double d2v_dsoc2() { return (d2v_dsoc2_); };
protected:
  TableInterp1Dclip *B_T_;  // Battery coeff
  TableInterp1Dclip *A_T_;  // Battery coeff
  TableInterp1Dclip *C_T_;  // Battery coeff
  double b_;  // Battery coeff
  double a_;  // Battery coeff
  double c_;  // Battery coeff
  double m_;  // Battery coeff
  double n_;  // Battery coeff
  double d_;  // Battery coeff
  unsigned int nz_;  // Number of breakpoints
  double soc_; // State of charge
  double r1_;   // Randels resistance, Ohms per cell
  double r2_;   // Randels resistance, Ohms per cell
  double c2_;   // Randels capacitance, Farads per cell
  double vstat_;  // Static model voltage, V
  double vdyn_;   // Model current induced back emf, V
  double v_;      // Total model voltage, V
  double curr_in_;  // Current into battery, A
  int num_cells_;   // Number of cells
  double dv_dsoc_;  // Derivative, V/fraction
  // double d2v_dsoc2_; // Derivative, V^2/fraction^2
};

// BattleBorn 100 Ah, 12v LiFePO4
#define NOM_SYS_VOLT          12        // Nominal system output, V, at which the reported amps are used (12)
#define NOM_BATT_CAP          100       // Nominal battery bank capacity, Ah (100)
// >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 60-95 F
#define BATT_V_SAT            3.425     // Normal battery cell saturation for SOC=99.7, V (3.425)
#define BATT_SOC_SAT          0.997     // Normal battery cell saturation, fraction (0.997)
#define BATT_R1               0.00126   // Battery Randels static resistance, Ohms (0.00126) for 3v cell matches transients
#define BATT_R2               0.00168   // Battery Randels dynamic resistance, Ohms (0.00168) for 3v cell matches transients
#define BATT_R2C2             100       // Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                                        // test so using with R2 and then multiplying by 4 for total result is valid,
                                        // though probably not for an individual cell
const int batt_num_cells = NOM_SYS_VOLT/3;  // Number of standard 3 volt LiFePO4 cells
const double batt_vsat = double(batt_num_cells)*double(BATT_V_SAT);  // Total bank saturation for 0.997=soc, V
const double batt_vmax = (14.3/4)*double(batt_num_cells); // Observed max voltage of 14.3 V for 12V prototype bank, V
const double batt_r1 = double(BATT_R1);
const double batt_r2 = double(BATT_R2);
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
// const double r1_bb = BATT_R1;  // Battery Randels static resistance 3v cell, Ohms (0.0018)*0.7 to match transients
// const double r2_bb = BATT_R2;  // Battery Randels dynamic resistance 3v cel, Ohms (0.0024)*0.7 to match transients
// const double r2c2_bb = BATT_R2C2; 

// Charge test profiles
#define NUM_VEC           1   // Number of vectors defined here
static const unsigned int n_v1 = 10;
static const double t_min_v1[n_v1] =  {0,     0.2,   0.2001, 1.4,   1.4001, 2.4999, 2.5,    3.6999, 3.7,    4.5};
static const double v_v1[n_v1] =      {13.65, 13.65, 13.65,  13.0,  13.0,   13.0,   13.0,   13.65,  13.65,  13.65};
static const double i_v1[n_v1] =      {0.,    0.,    -500.,  -500., 0.,     0.,     500.,   500.,   0.,     0.};
static const double T_v1[n_v1] =      {72.,   72.,   72.,    72.,   72.,    72.,    72.,    72.,    72.,    72.};
static TableInterp1Dclip  *V_T1 = new TableInterp1Dclip(n_v1, t_min_v1, v_v1);
static TableInterp1Dclip  *I_T1 = new TableInterp1Dclip(n_v1, t_min_v1, i_v1);
static TableInterp1Dclip  *T_T1 = new TableInterp1Dclip(n_v1, t_min_v1, T_v1);

#endif