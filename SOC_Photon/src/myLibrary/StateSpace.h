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


#ifndef STATESPACE_H_
#define STATESPACE_H_

// Lightweight general purpose state space for embedded application
class StateSpace
{
public:
  StateSpace();
  StateSpace(double *A, double *B, double *C, double *D, const int8_t n,
    const int8_t p, const int8_t q);
  ~StateSpace();
  // operators
  // functions
  void calc_x_dot(double *u);
  void init_state_space(double *u);
  void update(const double dt);
  virtual void pretty_print();
  void pretty_print_mat(const String name, const uint8_t n, const uint8_t m, double *A);
  void pretty_print_vec(const String name, const uint8_t n, double *x);
  void mulmat(double * a, double * b, double * c, int arows, int acols, int bcols);
  void mulvec(double * a, double * x, double * y, int m, int n);
  void insert_A(const uint8_t i, const uint8_t j, const double value);
  void insert_B(const uint8_t i, const uint8_t j, const double value);
  void insert_C(const uint8_t i, const uint8_t j, const double value);
  void insert_D(const uint8_t i, const uint8_t j, const double value);
  double y(int i) { return ( y_[i] ); };
protected:
  double dt_; // Saved update time
  double *A_; // n x n state matrix
  double *B_; // n x p input matrix
  double *C_; // q x n state output matrix
  double *D_; // q x p input output matrix
  double *x_; // 1 x n state vector
  double *x_past_;// 1 x n state vector
  double *x_dot_; // 1 x n state vector
  double *u_;// 1 x p input vector
  double *y_;// q x1 output vector
  int8_t n_; // Length of state vector
  int8_t p_; // Length of input vector
  int8_t q_; // Length of output vector
  double *AinvB_;   // Inverse of A times B; valid for 2x2 only
};

// Methods

#endif