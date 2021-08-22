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


#ifndef _MY_RETAINED_H
#define _MY_RETAINED_H

#include "application.h"

// SRAM retention class
struct Retained
{
  int8_t soc_min;             // Minimum soc observed
  int8_t curr_charge_max;     // Maximum charge current, A
  int8_t curr_discharge_max;  // Maximum discharge current, A
  int8_t temp_max;            // Maximum tbatt observed, F
  int8_t temp_min;            // Minimum tbatt observed, F
  Retained(void){}             // This must be empty so that 'retained' declaration is not over-written on reboot
  Retained(const int8_t soc_min, const int8_t curr_charge_max, const int8_t curr_discharge_max, const int8_t temp_max, const int8_t temp_min)
  {
    this->soc_min = soc_min;
    this->curr_charge_max = curr_charge_max;
    this->curr_discharge_max = curr_discharge_max;
    this->temp_max = temp_max;
    this->temp_min = temp_min;
  }
  void operator=(const Retained & s)
  {
    soc_min = s.soc_min;
    curr_charge_max = s.curr_charge_max;
    curr_discharge_max = s.curr_discharge_max;
    temp_max = s.temp_max;
    temp_min = s.temp_min;
  }
  void add_from(const Retained & s)
  {
    soc_min += s.soc_min;
    curr_charge_max += s.curr_charge_max;
    curr_discharge_max += s.curr_discharge_max;
    temp_max += s.temp_max;
    temp_min += s.temp_min;
  }
  void print(void)
  {
    Serial.printf("retained ( soc_min, curr_charge_max, curr_discharge_max, temp_max, temp_min):  %3d,%3d,%3d,%3d,%3d,\n",
      soc_min, curr_charge_max, curr_discharge_max, temp_max, temp_min);
  }
};

#endif