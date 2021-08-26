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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "Battery.h"
#include <math.h>

extern int8_t debug;
extern char buffer[256];

// class Battery
// constructors
Battery::Battery()
    : b_(0), a_(0), c_(0), m_(0), n_(0), d_(0), nz_(1), soc_(1.0), r1_(0), r2_(0), c2_(0), vstat_(0),
    vdyn_(0), v_(0), curr_in_(0), num_cells_(4) {}
Battery::Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2)
    : b_(0), a_(0), c_(0), m_(m), n_(n), d_(d), nz_(nz), soc_(1.0), r1_(r1), r2_(r2), c2_(r2c2/r2_),
    vstat_(0), vdyn_(0), v_(0), curr_in_(0), num_cells_(num_cells)
{
  B_T_ = new TableInterp1Dclip(nz_, x_tab, b_tab);
  A_T_ = new TableInterp1Dclip(nz_, x_tab, a_tab);
  C_T_ = new TableInterp1Dclip(nz_, x_tab, c_tab);
}
Battery::~Battery() {}
// operators
// functions

// SOC-OCV curve fit method per Zhang, et al
double Battery::calculate(const double temp_C, const double soc_frac, const double curr_in)
{
  b_ = B_T_ ->interp(temp_C);
  a_ = A_T_ ->interp(temp_C);
  c_ = C_T_ ->interp(temp_C);
  soc_ = max(min(soc_frac, 1.0-1e-6), 1e-6);
  curr_in_ = curr_in;

  // Perform computationally expensive steps one time
  double log_soc = log(soc_);
  double exp_n_soc = exp(n_*(soc_-1));
  double pow_log_soc = pow(-log_soc, m_);

  // VOC-OCV model
  vstat_ = double(num_cells_) * ( a_ + b_*pow_log_soc + c_*soc_ + d_*exp_n_soc );
  dv_dsoc_ = double(num_cells_) * ( b_*m_/soc_*pow_log_soc/log_soc + c_ + d_*n_*exp_n_soc );
  // d2v_dsoc2_ = double(num_cells_) * ( b_*m_/soc_/soc_*pow_log_soc/log_soc*((m_-1.)/log_soc - 1.) + d_*n_*n_*exp_n_soc );

  // Dynamic emf
  vdyn_ = double(num_cells_) * curr_in*(r1_ + r2_);

  v_ = vstat_ + vdyn_;
  return ( v_ );
}
