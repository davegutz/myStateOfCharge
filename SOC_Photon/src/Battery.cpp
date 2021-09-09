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
    : b_(0), a_(0), c_(0), m_(0), n_(0), d_(0), nz_(1), socs_(1.0), socu_(1.0), r1_(0), r2_(0), c2_(0), voc_(0),
    vdyn_(0), v_(0), curr_in_(0), num_cells_(4), dv_dsocs_(0), dv_dsocu_(0), tcharge_(24), pow_in_(0.0), sr_(1), vsat_(13.7),
    sat_(false), dv_(0), dvoc_dt_(0) {}
Battery::Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt)
    : b_(0), a_(0), c_(0), m_(m), n_(n), d_(d), nz_(nz), socs_(1.0), socu_(1.0), r1_(r1), r2_(r2), c2_(r2c2/r2_),
    voc_(0), vdyn_(0), v_(0), curr_in_(0), num_cells_(num_cells), dv_dsocs_(0), dv_dsocu_(0), tcharge_(24.), pow_in_(0.0),
    sr_(1.), nom_vsat_(batt_vsat), sat_(false), dv_(0), dvoc_dt_(dvoc_dt)
{
  B_T_ = new TableInterp1Dclip(nz_, x_tab, b_tab);
  A_T_ = new TableInterp1Dclip(nz_, x_tab, a_tab);
  C_T_ = new TableInterp1Dclip(nz_, x_tab, c_tab);
}
Battery::~Battery() {}
// operators
// functions

// SOC-OCV curve fit method per Zhang, et al
double Battery::calculate(const double temp_C, const double socu_frac, const double curr_in)
{
  b_ = B_T_ ->interp(temp_C);
  a_ = A_T_ ->interp(temp_C);
  c_ = C_T_ ->interp(temp_C);

  socu_ = socu_frac;
  socs_ = 1-(1-socu_)*cu_bb/cs_bb;
  double socs_lim = max(min(socs_, mxeps_bb), mneps_bb);
  curr_in_ = curr_in;

  // Perform computationally expensive steps one time
  double log_socs = log(socs_lim);
  double exp_n_socs = exp(n_*(socs_lim-1));
  double pow_log_socs = pow(-log_socs, m_);

  // VOC-OCV model
  dv_dsocs_ = double(num_cells_) * ( b_*m_/socs_lim*pow_log_socs/log_socs + c_ + d_*n_*exp_n_socs );
  dv_dsocu_ = dv_dsocs_ * cu_bb / cs_bb;
  voc_ = double(num_cells_) * ( a_ + b_*pow_log_socs + c_*socs_lim + d_*exp_n_socs )
                                                  + (socs_ - socs_lim) * dv_dsocs_;  // slightly beyond
  voc_ +=  dv_;  // Experimentally varied
  // d2v_dsocs2_ = double(num_cells_) * ( b_*m_/soc_/soc_*pow_log_socs/log_socs*((m_-1.)/log_socs - 1.) + d_*n_*n_*exp_n_socs );

  // Dynamic emf
  if ( curr_in_ < 0. ) vdyn_ = double(num_cells_) * curr_in*(r1_ + r2_)*sr_;
  else vdyn_ = double(num_cells_) * curr_in*(r1_ + r2_)*sr_;

  // Summarize
  v_ = voc_ + vdyn_;
  pow_in_ = v_*curr_in_ - curr_in_*curr_in_*(r1_+r2_)*sr_*num_cells_;  // Internal resistance of battery is a loss
  if ( pow_in_>1. )  tcharge_ = min(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * (1.-socs_lim), 24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
  else if ( pow_in_<-1. ) tcharge_ = max(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * socs_lim, -24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
  else if ( pow_in_>=0. ) tcharge_ = 24.*(1.-socs_lim);
  else tcharge_ = -24.*socs_lim;
  vsat_ = nom_vsat_ + (temp_C-25.)*dvoc_dt_;
  sat_ = voc_ >= vsat_;

  if ( debug == -8 ) Serial.printf("SOCU_in,v,curr,pow,tcharge, %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
      socu_frac, v_, curr_in_, pow_in_, tcharge_);

  if ( debug == -9 )Serial.printf("tempC,tempF,curr,a,b,c,d,n,m,r,soc,logsoc,expnsoc,powlogsoc,voc,vdyn,v\n%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
     temp_C, temp_C*9./5.+32., curr_in_, a_, b_, c_, d_, n_, m_, (r1_+r2_)*sr_ , socs_, log_socs, exp_n_socs, pow_log_socs, voc_, vdyn_,v_);

  return ( v_ );
}
