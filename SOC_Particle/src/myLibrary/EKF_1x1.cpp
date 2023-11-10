//
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

#include "application.h"
#include "EKF_1x1.h"
#include <math.h>
#include "parameters.h"
extern SavedPars sp; // Various parameters to be static at system level and saved through power cycle
// extern int8_t debug();


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
  this->ekf_predict(&Fx_, &Bu_);
  x_ = Fx_*x_ + Bu_*u_;
  if ( isnan(P_) ) P_ = 0.;   // reset overflow
  P_ = Fx_*P_*Fx_ + Q_;
  x_prior_ = x_;
  P_prior_ = P_;
}

// Initialize
void EKF_1x1::init_ekf(double soc, double Pinit)
{
  x_ = soc;
  P_ = Pinit;
}

// Pretty Print
 void EKF_1x1::pretty_print(void)
 {
#ifndef DEPLOY_PHOTON
  Serial.printf("EKF_1x1:\n");
  Serial.printf("In:\n");
  Serial.printf("  z  =   %8.4f, V\n", z_);
  Serial.printf("  R  = %10.6f\n", R_);
  Serial.printf("  Q  = %10.6f\n", Q_);
  Serial.printf("  H  =    %7.3f\n", H_);
  Serial.printf("Out:\n");
  Serial.printf("  x  =   %8.4f, Vsoc (0-1 fraction)\n", x_);
  Serial.printf("  hx =   %8.4f\n", hx_);
  Serial.printf("  y  =   %8.4f, V\n", y_);
  Serial.printf("  P  = %10.6f\n", P_);
  Serial.printf("  K  = %10.6f\n", K_);
  Serial.printf("  S  = %10.6f\n", S_);
#else
     Serial.printf("EKF_1x1: silent for DEPLOY_PHOTON\n");
#endif
 }

// Serial print
 void EKF_1x1::serial_print(const double control_time, const unsigned long int now, const float dt)
 {
  double cTime;
  if ( sp.tweak_test() ) cTime = double(now)/1000.;
  else cTime = control_time;
  Serial.printf("unit_ekf,%13.3f,%7.3f,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,%10.7g,\n",
    cTime, dt, Fx_, Bu_, Q_, R_, P_, S_, K_, u_, x_, y_, z_, x_prior_, P_prior_, x_post_, P_post_, hx_, H_);
 }

// y <- C@x + D@u
// Backward Euler integration of x
void EKF_1x1::update_ekf(const double z, double x_min, double x_max)
{
  /*1x1 Extended Kalman Filter update
  Inputs:
    z   1x1 input, =voc, dynamic predicted by other model, V
    R   1x1 Kalman state uncertainty
    Q   1x1 Kalman process uncertainty
    H   1x1 Jacobian sensitivity dV/dSOC
  Outputs:
    x   1x1 Kalman state variable = Vsoc (0-1 fraction)
    hx  1x1 Output of observation function h(x)
    y   1x1 Residual z-hx, V
    P   1x1 Kalman uncertainty covariance
    K   1x1 Kalman gain
    S   1x1 system uncertainty
    SI  1x1 system uncertainty inverse
  */
  this->ekf_update(&hx_, &H_);
  z_ = z;
  double pht = P_*H_;
  S_ = H_*pht + R_;
  if ( abs(S_) > 1e-12) K_ = pht / S_;  // Using last-good-value if S_ = 0
  y_ = z_ - hx_;
  x_ = max(min( x_ + K_*y_, x_max), x_min);
  double i_kh = 1. - K_*H_;
  P_ *= i_kh;
  x_post_ = x_;
  P_post_ = P_;
}
