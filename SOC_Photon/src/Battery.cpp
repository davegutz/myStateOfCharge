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

extern const int8_t debug;
extern char buffer[256];

/*
 KI_T_ = new TableInterp1Dclip(sizeof(xALL[myKit_]) / sizeof(double), xALL[myKit_], yKI);
*/

// class Battery
// constructors
Battery::Battery()
    : b_(0), a_(0), c_(0), m_(0), n_(0), d_(0), nz_(1), soc_(1.0) {}
Battery::  Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d)
    : b_(0), a_(0), c_(0), m_(m), n_(n), d_(d), nz_(sizeof(*x_tab)/sizeof(double)), soc_(1.0)
{
  B_T_ = new TableInterp1Dclip(nz_, x_tab, b_tab);
  A_T_ = new TableInterp1Dclip(nz_, x_tab, a_tab);
  C_T_ = new TableInterp1Dclip(nz_, x_tab, c_tab);
}
Battery::~Battery() {}
// operators
// functions
double Battery::calculate(const double temp_C, const double soc_frac)
{
  b_ = B_T_ ->interp(temp_C);
  a_ = A_T_ ->interp(temp_C);
  c_ = C_T_ ->interp(temp_C);
  soc_ = max(min(soc_frac, 1.0), 0.0);
  double out = a_ + b_*pow(-log(soc_frac), m_) + c_*soc_frac + d_*exp(n_*(soc_frac-1));
  return ( out );
}
