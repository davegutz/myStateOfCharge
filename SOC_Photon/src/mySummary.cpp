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

#include "mySummary.h"
#include "myRetained.h"

Summary::Summary(const double soc_min, const double curr_charge_max, const double curr_discharge_max, const double temp_max, const double temp_min)
{
  this->soc_min = soc_min;
  this->curr_charge_max = curr_charge_max;
  this->curr_discharge_max = curr_discharge_max;
  this->temp_max = temp_max;
  this->temp_min = temp_min;
}
Summary::Summary(){}
Summary::~Summary() {}
void Summary::operator=(const Summary & s)
{
  soc_min = s.soc_min;
  curr_charge_max = s.curr_charge_max;
  curr_discharge_max = s.curr_discharge_max;
  temp_max = s.temp_max;
  temp_min = s.temp_min;
}
void Summary::load_to(struct Retained *r)
{
  r->soc_min = soc_min;
  r->curr_charge_max = curr_charge_max;
  r->curr_discharge_max = curr_discharge_max;
  r->temp_max = temp_max;
  r->temp_min = temp_min;
}
void Summary::load_from(struct Retained & r)
{
  soc_min = double(r.soc_min);
  curr_charge_max = double(r.curr_charge_max);
  curr_discharge_max = double(r.curr_discharge_max);
  temp_max = double(r.temp_max);
  temp_min = double(r.temp_min);
}
void Summary::print(void)
{
  Serial.printf("summ ( soc_min, curr_charge_max, curr_discharge_max, temp_max, temp_min):  %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
    soc_min, curr_charge_max, curr_discharge_max, temp_max, temp_min);
}
