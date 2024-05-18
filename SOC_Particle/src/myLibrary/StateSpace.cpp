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
#include "myLibrary/StateSpace.h"
#include <math.h>
#include "parameters.h"
extern SavedPars sp;  // Various parameters to be static at system level and saved through power cycle


// class StateSpace
// constructors
StateSpace::StateSpace(){}
StateSpace::StateSpace(double *A, double *B, double *C, double *D, const int8_t n, const int8_t p,
  const int8_t q) : dt_(0), A_(A), B_(B), C_(C), D_(D), n_(n), p_(p), q_(q)
  {
    x_ = new double[n_];
    x_dot_ = new double[n_];
    x_past_ = new double[n_];
    u_ = new double[p_];
    y_ = new double[q_];
    if ( n_==2 && p_==2)
    {
      double Adet = A_[0]*A_[3] - A_[1]*A_[2];
      double *mAinv_ = new double[n*n];
      AinvB_ = new double[n*n];
      mAinv_[0] =  A_[3] / Adet;
      mAinv_[1] = -A_[1] / Adet;
      mAinv_[2] = -A_[2] / Adet;
      mAinv_[3] =  A_[0] / Adet;
      mulmat(mAinv_, B_, AinvB_, n, n, p);
    }

  }
StateSpace::~StateSpace() {}

// operators

// functions
// xdot <- A@x + B@u
void StateSpace::calc_x_dot(double *u)
{
  int i;
  double AX[n_];
  double BU[n_];
  for (i=0; i<p_; i++) u_[i] = u[i];
  mulmat(A_, x_, AX, n_, n_, 1);
  mulmat(B_, u_, BU, n_, p_, 1);
  for (i=0; i<n_; i++) x_dot_[i] = AX[i] + BU[i];
}

// Initialize
void StateSpace::init_state_space(double *u)
{
  // Explicit initialization 2x2 (first use)
  for (int i=0; i<p_; i++) u_[i] = u[i];
  if ( n_==2 && p_==2 )
  {
    mulmat(AinvB_, u_, x_, n_, n_, 1);
    for (int i=0; i<n_; i++) x_[i] = -x_[i];
    calc_x_dot(u_);
  }
  else  // All zero.  (need more time)
  {
    for ( int i=0; i<n_; i++ )
    {
      x_[i] = 0.;
      x_past_[i] = 0.;
    }
  }
}

// Pretty print matrix
void StateSpace::pretty_print_mat(const String name, const uint8_t n, const uint8_t m, double *A)
{
  Serial.printf("   %s =  [", name.c_str());
  for ( int i=0; i<n; i++ )
  {
    for ( int j=0; j<m; j++)
    {
      Serial.printf("%10.6f", A[i*n+j]);
      if ( j==m-1 )
      {
        if ( i==n-1 ) Serial.printf("];\n");
        else Serial.printf(",\n         ");
      }
      else Serial.printf(",");
    }
  }
}

// Pretty print vector
void StateSpace::pretty_print_vec(const String name, const uint8_t n, double *x)
{
  Serial.printf("   %s =  [", name.c_str());
  for ( int i=0; i<n; i++ )
  {
    Serial.printf("%10.6f", x[i]);
    if ( i==n-1 ) Serial.printf("];\n");
    else Serial.printf(",");
  }
}

// Pretty Print
void StateSpace::pretty_print(void)
{
#ifndef SOFT_DEPLOY_PHOTON
  Serial.printf("StateSpace:\n");
  Serial.printf("  dt %9.6f\n", dt_);
  pretty_print_mat("A ", n_, n_, A_);
  pretty_print_vec("x ", n_, x_);
  pretty_print_mat("B ", n_, p_, B_);
  pretty_print_vec("u ", p_, u_);
  pretty_print_mat("C ", q_, n_, C_);
  pretty_print_mat("D ", q_, p_, D_);
  pretty_print_vec("x_dot ", n_, x_dot_);
  pretty_print_vec("y ", q_, y_);
  if ( n_==2 && p_==2)
  {
    pretty_print_mat("AinvB", n_, n_, AinvB_);
  }
#else
     Serial.printf("StateSpace: silent DEPLOY\n");
#endif

}

// Scale elements as requested
void StateSpace::insert_A(const uint8_t i, const uint8_t j, const double value)
{
  A_[i*n_+j] = value;
}
void StateSpace::insert_B(const uint8_t i, const uint8_t j, const double value)
{
  B_[i*n_+j] = value;
}
void StateSpace::insert_C(const uint8_t i, const uint8_t j, const double value)
{
  C_[i*q_+j] = value;  
}
void StateSpace::insert_D(const uint8_t i, const uint8_t j, const double value)
{
  D_[i*q_+j] = value;  
}

// y <- C@x + D@u
// Backward Euler integration of x
void StateSpace::update(const double dt)
{
  int i;
  dt_ = dt;
  double CX[q_];
  double DU[q_];
  for (i=0; i<n_; i++)
  {
    x_past_[i] = x_[i];
    x_[i] += x_dot_[i] * dt_;
  }
  mulmat(C_, x_past_, CX, q_, n_, 1);  // Back Euler uses past
  mulmat(D_, u_, DU, q_, p_, 1);
  for (i=0; i<q_; i++) y_[i] = CX[i] + DU[i];
}

// C <- A @ B  
// A  arows x acols
// B  arows x bcols
// C  arows
void StateSpace::mulmat(double * a, double * b, double * c, int arows, int acols, int bcols)
{
    int i, j,l;

    for(i=0; i<arows; ++i)
        for(j=0; j<bcols; ++j) {
            c[i*bcols+j] = 0;
            for(l=0; l<acols; ++l)
                c[i*bcols+j] += a[i*acols+l] * b[l*bcols+j];
        }
}

void StateSpace::mulvec(double * a, double * x, double * y, int m, int n)
{
    int i, j;

    for(i=0; i<m; ++i) {
        y[i] = 0;
        for(j=0; j<n; ++j)
            y[i] += x[j] * a[i*n+j];
    }
}
