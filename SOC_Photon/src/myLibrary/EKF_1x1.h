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


#ifndef EKF_1X1_H_
#define EKF_1X1_H_

// Lightweight general purpose state space for embedded application
class EKF_1x1
{
public:
  EKF_1x1();
  ~EKF_1x1();
  // operators
  // functions
  void predict_ekf(const double u);
  void update_ekf(const double z, double x_min, double x_max, const double dt);
  double x_ekf() { return ( x_ ); };
  double z_ekf() { return ( z_ ); };
  void init_ekf(double soc, double Pinit);
protected:
  double Fx_; // State transition
  double Bu_; // Control transition
  double Q_;  // Process uncertainty
  double R_;  // State uncertainty
  double P_;  // Uncertainty covariance
  double S_;  // System uncertainty
  double K_;  // Kalman gain
  double u_;  // Control input
  double x_;  // Kalman state variable
  double y_;  // Residual z- hx
  double z_;  // Observation of state x
  double x_prior_;  
  double P_prior_;
  double x_post_;
  double P_post_;
  double hx_; // Output of observation function h(x)
  double H_;  // Jacobian of h(x)
  /*
    Implement this function for your EKF model.
    @param fx gets output of state-transition function f(x)
    @param F gets Jacobian of f(x)
    @param hx gets output of observation function h(x)
    @param H gets Jacobian of h(x)
  */
  // virtual void ekf_model(double *Fx, double *Bu, double *hx, double *H) = 0;
  virtual void ekf_model_predict(double *Fx, double *Bu) = 0;
  virtual void ekf_model_update(double *hx, double *H) = 0;
};

// Methods

#endif