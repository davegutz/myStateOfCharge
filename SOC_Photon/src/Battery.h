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
  double soc() { return (soc_); };
  double vstat() { return (vstat_); };
  double vdyn() { return (vdyn_); };
  double v() { return (v_); };
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
  double r1_;   // Randels resistance, Ohms
  double r2_;   // Randels resistance, Ohms
  double c2_;   // Randels capacitance, Farads
  double vstat_;  // Static model voltage, V
  double vdyn_;   // Model current induced back emf, V
  double v_;      // Total model voltage, V
  double curr_in_;// Current into battery, A
  int num_cells_; // Number of cells
};

// Battery model LiFePO4 BattleBorn.xlsx and 'GeneralizedSOC-OCV Model Zhang etal.pdf'
static const double t_bb[7] = {-10.,	 0.,	10.,	20.,	30.,	40.,	50.};
const double m_bb = 0.478;
static const double b_bb[7] = {-1.143251503,	-1.143251503,	-1.143251503,	-0.5779554,	-0.553297988,	-0.557104757,	-0.45551626};
static const double a_bb[7] = {3.348339977,	3.5247557,	3.553964435,	2.778271021,	2.806905998,	2.851255776,	2.731041762};
static const double c_bb[7] = {-1.770434633,	-1.770434633,	-1.770434633,	-1.099451796,	-1.125467829,	-1.159227563,	-1.059028383};
const double n_bb = 0.4;
const double d_bb = 1.734;
const unsigned int nz_bb = 7;
const double r1_bb = 0.0018*0.7;  // Battery Randels static resistance, Ohms (0.0018) 0.7 to match transients
const double r2_bb = 0.0024*0.7;  // Battery Randels dynamic resistance, Ohms (0.0024) 0.7 to match transients
const double r2c2_bb = 100.0; // Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                              // test so using with R2 and then multiplying by 4 for total result is valid,
                              // though probably not for an individual cell
const int num_cells_bb = 1;   // Above values for single cell (~3 volts)
							
#endif