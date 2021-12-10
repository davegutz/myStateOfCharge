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
#include "EKF_1x1.h"
#include <math.h>

extern int8_t debug;


// class EKF_1x1
// constructors
EKF_1x1::EKF_1x1(){}
EKF_1x1::~EKF_1x1() {}

// operators

// functions
//1x1 Extended Kalman Filter predict
void EKF_1x1::predict_ekf(double u)
{
  /*
  1x1 Extended Kalman Filter predict
  Inputs:
    u   1x1 input, =ib, A
    Bu  1x1 control transition, Ohms
    Fx  1x1 state transition, V/V
  Outputs:
    x   1x1 Kalman state variable = Vsoc (0-1 fraction)
    P   1x1 Kalman probability
  */
  u_ = u;
  this->ekf_model_predict(&Fx_, &Bu_);
  x_ = Fx_*x_ + Bu_*u_;
  P_ = Fx_*P_*Fx_ + Q_;
  x_prior_ = x_;
  P_prior_ = P_;
}

// y <- C@x + D@u
// Backward Euler integration of x
void EKF_1x1::update_ekf(const double z, const double dt)
{
  /*1x1 Extended Kalman Filter update
  Inputs:
    z   1x1 input, =voc, dynamic predicted by other model, V
    R   1x1 Kalman state uncertainty
    Q   1x1 Kalman process uncertainty
    H   1x1 Jacobian sensitivity dV/dSOC
  Outputs:
    x   1x1 Kalman state variable = Vsoc (0-1 fraction)
    y   1x1 Residual z-hx, V
    P   1x1 Kalman uncertainty covariance
    K   1x1 Kalman gain
    S   1x1 system uncertainty
    SI  1x1 system uncertainty inverse
  */
  this->ekf_model_update(&hx_, &H_);
  z_ = z;
  double pht = P_*H_;
  S_ = H_*pht + R_;
  K_ = pht / S_;
  y_ = z_ - hx_;
  x_ += K_*y_;
  double i_kh = 1. - K_*H_;
  P_ *= i_kh;
  x_post_ = x_;
  P_post_ = P_;
}
