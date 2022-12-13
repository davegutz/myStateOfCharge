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


#ifndef _FAULT_H
#define _FAULT_H

#include "application.h"
#include "Battery.h"
String time_long_2_str(const unsigned long current_time, char *tempStr);

// SRAM retention summary
struct Flt_st
{
  unsigned long t;  // Timestamp seconds since start of epoch
  int16_t Tb_hdwe;  // Battery temperature, hardware, C
  int16_t vb_hdwe;  // Battery measured potential, hardware, V
  int16_t ib_amp_hdwe;  // Battery measured input current, amp, A
  int16_t ib_noa_hdwe;  // Battery measured input current, no amp, A
  int16_t Tb;       // Battery temperature, filtered, C
  int16_t vb;       // Battery measured potential, filtered, V
  int16_t ib;       // Battery measured input current, filtered, A
  int16_t soc;      // Battery state of charge, free Coulomb counting algorithm, frac
  int16_t soc_ekf;  // Battery state of charge, ekf, frac
  int16_t voc;      // Battery open circuit voltage measured vb-ib*Z, V
  int16_t voc_stat; // Stored charge voltage from measurement, V
  int16_t voc_soc;  // Stored charge voltage from lookup, V
  int16_t e_wrap_filt; // Wrap model error, filtered, V
  uint16_t fltw;    // Fault word
  uint16_t falw;    // Fail word
  Flt_st()
  {
    nominal();
  }
  void assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen);
  void copy_from(Flt_st *input)
  {
    this->t = input->t;
    this->Tb_hdwe = input->Tb_hdwe;
    this->vb_hdwe = input->vb_hdwe;
    this->ib_amp_hdwe = input->ib_amp_hdwe;
    this->ib_noa_hdwe = input->ib_noa_hdwe;
    this->Tb = input->Tb;
    this->vb = input->vb;
    this->ib = input->ib;
    this->soc = input->soc;
    this->soc_ekf = input->soc_ekf;
    this->voc = input->voc;
    this->voc_stat = input->voc_stat;
    this->e_wrap_filt =input->e_wrap_filt;
    this->fltw = input->fltw;
    this->falw = input->falw;
  }
  void nominal()
  {
    this->t = 0L;
    this->Tb_hdwe = 0.;
    this->vb_hdwe = 0.;
    this->ib_amp_hdwe = 0.;
    this->ib_noa_hdwe = 0.;
    this->Tb = 0.;
    this->vb = 0.;
    this->ib = 0.;
    this->soc = 0.;
    this->soc_ekf = 0.;
    this->voc = 0.;
    this->voc_stat = 0.;
    this->e_wrap_filt = 0.;
    this->fltw = 0;
    this->falw = 0;
  }
  void print(const String code)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("%s, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,",
      code.c_str(), buffer, this->t,
      double(this->Tb_hdwe)/600.,
      double(this->vb_hdwe)/1200.,
      double(this->ib_amp_hdwe)/600.,
      double(this->ib_noa_hdwe)/600.,
      double(this->Tb)/600.,
      double(this->vb)/1200.,
      double(this->ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->voc)/1200.,
      double(this->voc_stat)/1200.,
      double(this->e_wrap_filt)/1200.,
      this->fltw,
      this->falw);
    Serial1.printf("unit_f, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,",
      buffer, this->t,
      double(this->Tb_hdwe)/600.,
      double(this->vb_hdwe)/1200.,
      double(this->ib_amp_hdwe)/600.,
      double(this->ib_noa_hdwe)/600.,
      double(this->Tb)/600.,
      double(this->vb)/1200.,
      double(this->ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->voc)/1200.,
      double(this->voc_stat)/1200.,
      double(this->e_wrap_filt)/1200.,
      this->fltw,
      this->falw);
  }
};

#endif