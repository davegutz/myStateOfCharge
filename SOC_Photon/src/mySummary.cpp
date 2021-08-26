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
#include "myFilters.h"

Summary::Summary(const double absorption_th, const double absorption_pers,
    const double soc_delta, const double curr_charge_max, const double curr_discharge_max, 
    const double temp_max, const double temp_min, const double cycle_dura)
      : absorption_th_(absorption_th), absorption_pers_(absorption_pers)
{
  this->soc_delta_ = soc_delta;
  this->curr_charge_max_ = curr_charge_max;
  this->curr_discharge_max_ = curr_discharge_max;
  this->temp_max_ = temp_max;
  this->temp_min_ = temp_min;
  this->cycle_dura_ = cycle_dura;
  this->Cycling_TF_ = new TFDelay(false, absorption_pers_, absorption_pers_, 1.);
}
Summary::Summary(){}  // Must be empty to avoid re-init on power up for instances 'retained'
Summary::~Summary() {}
void Summary::update(const double soc, const double curr, const double temp, const unsigned now, const bool RESET, const double T)
{
  cycling_detect_ = soc < absorption_th_;
  cycling_ = Cycling_TF_->calculate(cycling_detect_, absorption_pers_, absorption_pers_, T, RESET);
}
void Summary::operator=(const Summary & s)
{
  soc_delta_ = s.soc_delta_;
  curr_charge_max_ = s.curr_charge_max_;
  curr_discharge_max_ = s.curr_discharge_max_;
  temp_max_ = s.temp_max_;
  temp_min_ = s.temp_min_;
  cycle_dura_ = s.cycle_dura_;
}
void Summary::load_to(struct PickelJar *r)
{
  r->soc_delta = soc_delta_;
  r->curr_charge_max = curr_charge_max_;
  r->curr_discharge_max = curr_discharge_max_;
  r->temp_max = temp_max_;
  r->temp_min = temp_min_;
  r->cycle_dura = cycle_dura_;
}
void Summary::load_from(struct PickelJar & r)
{
  soc_delta_ = double(r.soc_delta);
  curr_charge_max_ = double(r.curr_charge_max);
  curr_discharge_max_ = double(r.curr_discharge_max);
  temp_max_ = double(r.temp_max);
  temp_min_ = double(r.temp_min);
  cycle_dura_ = double(r.cycle_dura);
}
void Summary::print(void)
{
  Serial.printf("summ ( soc_delta, curr_charge_max, curr_discharge_max, temp_max, temp_min, cycle_dura):  %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
    soc_delta_, curr_charge_max_, curr_discharge_max_, temp_max_, temp_min_, cycle_dura_);
}
