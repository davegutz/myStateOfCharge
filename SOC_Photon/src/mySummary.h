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

#include "application.h"

// SRAM retention class
struct PickelJar
{
  int8_t soc_delta;           // Cycle height, frac
  int8_t curr_charge_max;     // Maximum charge current, A
  int8_t curr_discharge_max;  // Maximum discharge current, A
  int8_t temp_max;            // Maximum tbatt observed, F
  int8_t temp_min;            // Minimum tbatt observed, F
  int8_t cycle_dura;          // Cycle duration, 10xHr
  PickelJar(void){}       // This must be empty so that 'PickelJar' declaration is not over-written on reboot
  PickelJar(const int8_t soc_delta, const int8_t curr_charge_max, const int8_t curr_discharge_max,
      const int8_t temp_max, const int8_t temp_min, const int8_t cycle_dura)
  {
    this->soc_delta = soc_delta;
    this->curr_charge_max = curr_charge_max;
    this->curr_discharge_max = curr_discharge_max;
    this->temp_max = temp_max;
    this->temp_min = temp_min;
    this->cycle_dura = cycle_dura;
  }
  void operator=(const PickelJar & s)
  {
    soc_delta = s.soc_delta;
    curr_charge_max = s.curr_charge_max;
    curr_discharge_max = s.curr_discharge_max;
    temp_max = s.temp_max;
    temp_min = s.temp_min;
    cycle_dura = s.cycle_dura;
  }
  void add_from(const PickelJar & s)
  {
    soc_delta += s.soc_delta;
    curr_charge_max += s.curr_charge_max;
    curr_discharge_max += s.curr_discharge_max;
    temp_max += s.temp_max;
    temp_min += s.temp_min;
    cycle_dura += s.cycle_dura;
  }
  void print(void)
  {
    Serial.printf("PickelJar ( soc_delta, curr_charge_max, curr_discharge_max, temp_max, temp_min, cycle_dura):  %3d,%3d,%3d,%3d,%3d,%3d,\n",
      soc_delta, curr_charge_max, curr_discharge_max, temp_max, temp_min, cycle_dura);
  }
};

// Summary
class Summary
{
public:
  Summary();
  Summary(const double absorption_th, const double absorption_pers,
    const double soc_delta, const double curr_charge_max, const double curr_discharge_max, 
    const double temp_max, const double temp_min, const double cycle_dura);
  ~Summary();
  // operators
  void operator=(const Summary & s);
  // functions
  void load_from(struct PickelJar & R);
  void load_to(class PickelJar *r);
  void print(void);
  void update(const double soc, const double curr, const double temp, const unsigned now, const bool RESET, const double T);
protected:
  // Settings
  double absorption_th_;        // Absorption detection, frac
  double absorption_pers_;      // Absorption detection persistence, sce
  double dying_th_;             // Absorption detection, frac
  double dying_pers_;           // Dying detection persistence, sce
  double discharge_th_;         // Discharge detection, frac/sec
  double discharge_pers_;       // Discharge detection persistence, sec
  double charge_th_;            // Charge detection, frac/sec
  double charge_pers_;          // Discharge detection persistence, sec
  double dwell_dura_th_;        // Full cycle detection, sec
  // Calculations
  class TFDelay *Cycling_TF_;   // TF delay cycling detect
  bool cycling_detect_;         // Cycling detected
  bool cycling_;                // Cycling detected and persisted
  double soc_delta_;            // Delta soc observed
  double curr_charge_max_;      // Maximum charge current, A
  double curr_discharge_max_;   // Maximum discharge current, A
  double temp_max_;             // Maximum tbatt observed, F
  double temp_min_;             // Minimum tbatt observed, F
  double cycle_dura_;           // Duration of cycle, Hr
  double charge_dura_;          // Charge duration, Hr
  double discharge_dura_;       // Discharge duration, Hr
  double dwell_dura_;           // Dwell duration, Hr
  double soc_min_;              // Minimum soc observed, frac
  double soc_max_;              // Maximum soc observed, frac
  bool falling_;                // soc is falling
  bool rising_;                 // soc is rising
  bool absorbing_;              // state of charge absorption
  bool dying_;                  // state of total discharge
  bool cycle_cpt_;              // A cycle is declared complete...store it
};

#endif