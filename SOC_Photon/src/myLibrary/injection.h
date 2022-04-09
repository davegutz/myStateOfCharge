
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


#ifndef INJECTION_H
#define INJECTION_H

#include <math.h>
#include <spark_wiring_arduino_constants.h>

// Signal construction classes - convenient because they control their own memory
// Sine wave signal generation
// No attempt to control phase....may need to revise later
class SinInj
{
  public:
    SinInj(){};
    ~SinInj();
    double signal(const double amp, const double freq_rps, const double t, const double inj_soft_bias)
    {
      return ( amp*(1. + sin(freq_rps*t)) + inj_soft_bias );
    }

  protected:
};
// Square wave signal generation
class SqInj
{
  public:
    SqInj(): t_last_sq_(0), square_bias_(0){};
    ~SqInj();
    double signal(const double amp, const double freq_rps, const double t, const double inj_soft_bias)
    {
      double sq_dt;
      if ( freq_rps>1e-6 )
        sq_dt = 1. / freq_rps * PI;
      else
        sq_dt = t;
      if ( t-t_last_sq_ >= sq_dt )
      {
        t_last_sq_ = t;
        if ( square_bias_ == 0. )
          square_bias_ = 2.*amp;
        else
          square_bias_ = 0.0;
      }
      return ( square_bias_ + inj_soft_bias );
    };
  protected:
    double t_last_sq_;
    double square_bias_;
};
// Triangle wave signal generation
class TriInj
{
  public:
    TriInj(): t_last_tri_(0) {};
    ~TriInj();
    double signal(const double amp, const double freq_rps, const double t, const double inj_soft_bias)
    {
      double tri_bias = 0.;   // return value
      double tri_dt = t;
      if ( freq_rps>1e-6 )
        tri_dt = 1. / freq_rps * PI;
      if ( t-t_last_tri_ >= tri_dt )
        t_last_tri_ = t;
      double dt = t-t_last_tri_;
      if ( dt <= tri_dt/2. )
        tri_bias = dt / (tri_dt/2.) * 2. * amp;
      else
        tri_bias = (tri_dt-dt) / (tri_dt/2.) * 2. * amp;
      return ( tri_bias + inj_soft_bias );
    };
  protected:
    double t_last_tri_;
};

#endif