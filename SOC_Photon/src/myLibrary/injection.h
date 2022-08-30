
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
/*

                -   p/2
amp+bias     *    *
           +        + 
          /          \              p = 1/freq_hz 
bias---+ *            *             *---------
                       \           /
                         +        +
                          *     *
                             -  -amp+bias

*/
class SinInj
{
  public:
    SinInj(){};
    ~SinInj();
    double signal(const double amp, const double freq_rps, const double t, const double inj_bias)
    {
      return ( amp*(1. + sin(freq_rps*t)) + inj_bias );
    }

  protected:
};
// Cosine wave signal generation
// No attempt to control phase....may need to revise later
/*
         0                        p = 1/freq_hz
bias  ---+                        +----
            *                  *
              \              /
               *            *
                \          /
                   *     *
-2amp+bias            +

*/
class CosInj
{
  public:
    CosInj(){};
    ~CosInj();
    double signal(const double amp, const double freq_rps, const double t, const double inj_bias)
    {
      return ( amp*(cos(freq_rps*t)) + inj_bias );
    }

  protected:
};
// Square wave signal generation
/*

              p/2
amp+bias      +------+
              +      +      
       0      +      +
bias---+      +      +---------
       +      +      p = 1/freq_hz 
       +      +
       +------+  -amp+bias

*/
class SqInj
{
  public:
    SqInj(): t_last_sq_(0), square_bias_(0){};
    ~SqInj();    double signal(const double amp, const double freq_rps, const double t, const double inj_bias)
    {
      double sq_dt;
      if ( freq_rps>1e-6 )
        sq_dt = 1. / freq_rps * PI;
      else
        sq_dt = t;

      // Wave calculation
      if ( t-t_last_sq_ >= sq_dt )
      {
        t_last_sq_ = t;
        if ( square_bias_ == 0. )
          square_bias_ = 2.*amp;
        else
          square_bias_ = 0.0;
      }
      return ( square_bias_ + inj_bias );
    };
  protected:
    double t_last_sq_;
    double square_bias_;
};
// Triangle wave signal generation
/*

      p/4
amp+bias  *
         / \       
        /   \       p = 1/freq_hz 
bias---+     \      +---
        0      \   /
                \ /
-amp+bias        * 
                3p/4

*/
class TriInj
{
  public:
    TriInj(): t_last_(-1e6) {};
    ~TriInj();
    double signal(const double amp, const double freq_rps, const double t, const double inj_bias)
    {
      double res = 0.;   // return value
      double p = t;
      if ( freq_rps>1e-6 )
        p = 2. / freq_rps * PI;

      // Refresh or reset t_last base
      if ( t - t_last_ >= p || t < t_last_ )
        t_last_ = t;

      // Wave calculation
      double s = 4. * amp / p;
      double dt =  t - t_last_;

      if ( dt <= p/4. )
        res = dt * s;

      else if ( dt <= 3.*p/4.)
        res = (p/2. - dt) * s;

      else
        res = (dt - p) * s;
      
      res += inj_bias;
      return res;
    };
  protected:
    double t_last_;
};

#endif