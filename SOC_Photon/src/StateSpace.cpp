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
#include "StateSpace.h"
#include <math.h>

extern int8_t debug;


// class StateSpace
// constructors
StateSpace::StateSpace(){}
StateSpace::StateSpace(double *A, double *B, double *C, double *D, const int8_t n, const int8_t p,
  const int8_t q) : A_(A), B_(B), C_(C), D_(D), n_(n), p_(p), q_(q)
  {
    x_ = new double[n_];
    x_dot_ = new double[n_];
    x_past_ = new double[n_];
    for ( int i=0; i<n_; i++ )
    {
      x_[i] = 0.;
      x_past_[i] = 0.;
    }
    u_ = new double[p_];
    y_ = new double[q_];
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
  if ( debug==-33 )
  {
    Serial.printf("\nA_=[%10.6f, %10.6f,\n %10.6f, %10.6f,]\n", A_[0], A_[1], A_[2], A_[3]);
    Serial.printf("x_=[%10.6f, %10.6f]\n", x_[0], x_[1]);
    Serial.printf("AX=[%10.6f, %10.6f]\n", AX[0], AX[1]);
    Serial.printf("B_=[%10.6f, %10.6f,\n %10.6f, %10.6f,]\n", B_[0], B_[1], B_[2], B_[3]);
    Serial.printf("u_=[%10.6f, %10.6f]\n", u_[0], u_[1]);
    Serial.printf("BU=[%10.6f, %10.6f]\n", BU[0], BU[1]);
    Serial.printf("xdot_=[%10.6f, %10.6f]\n", x_dot_[0], x_dot_[1]);
  }
}

// y <- C@x + D@u
// Backward Euler integration of x
void StateSpace::update(const double dt)
{
  int i;
  double CX[q_];
  double DU[q_];
  for (i=0; i<n_; i++)
  {
    x_past_[i] = x_[i];
    x_[i] += x_dot_[i] * dt;
  }
  mulmat(C_, x_past_, CX, q_, n_, 1);  // Back Euler uses past
  mulmat(D_, u_, DU, q_, p_, 1);
  for (i=0; i<q_; i++) y_[i] = CX[i] + DU[i];
  if ( debug==-33 )
  {
    Serial.printf("C_=[%10.6f, %10.6f]\n", C_[0], C_[1]);
    Serial.printf("D_=[%10.6f, %10.6f]\n", D_[0], D_[1]);
    Serial.printf("CX=[%10.6f]\n", CX[0]);
    Serial.printf("DU=[%10.6f]\n", DU[0]);
    Serial.printf("y=[%10.6f]\n", y_[0]);
  }
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
