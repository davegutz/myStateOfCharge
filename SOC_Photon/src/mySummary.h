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


#ifndef _MY_SUMMARY_H
#define _MY_SUMMARY_H

#include "myRetained.h"

// Summary
class Summary
{
public:
  Summary();
  Summary(const double soc_min, const double curr_charge_max, const double curr_discharge_max, const double temp_max, const double temp_min);
  ~Summary();
  // operators
  void operator=(const Summary & s);
  // functions
  void load_from(struct Retained & R);
  void load_to(class Retained *r);
  void print(void);
protected:
  double soc_min;             // Minimum soc observed
  double curr_charge_max;     // Maximum charge current, A
  double curr_discharge_max;  // Maximum discharge current, A
  double temp_max;            // Maximum tbatt observed, F
  double temp_min;            // Minimum tbatt observed, F
};

#endif