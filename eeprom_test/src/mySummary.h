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
#include "command.h"
#include "myTalk.h"
extern CommandPars cp;


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
  Flt_st(void){}
  void assign(const time32_t now)
  {
    char buffer[32];
    this->t = now;
    this->Tb_hdwe = int16_t(1*600.);
    this->vb_hdwe = int16_t(2*1200.);
    this->ib_amp_hdwe = int16_t(3*600.);
    this->ib_noa_hdwe = int16_t(4*600.);
    this->Tb = int16_t(5*600.);
    this->vb = int16_t(6*1200.);
    this->ib = int16_t(7*600.);
    this->soc = int16_t(8*16000.);
    this->soc_ekf = int16_t(9*16000.);
    this->voc = int16_t(10*1200.);
    this->voc_stat = int16_t(11*1200.);
    this->e_wrap_filt = int16_t(12*1200.);
    this->fltw = 13;
    this->falw = 14;
    time_long_2_str(now, buffer);
  }
  void print(void)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("unit_f, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,",
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
};


// SRAM retention summary
struct Sum_st
{
  unsigned long t;  // Timestamp seconds since start of epoch
  int16_t Tb;       // Battery temperature, filtered, C
  int16_t Vb;       // Battery measured potential, filtered, V
  int16_t Ib;       // Battery measured input current, filtered, A
  int16_t soc;      // Battery state of charge, free Coulomb counting algorithm, %
  int16_t soc_ekf;  // Battery state of charge, ekf, %
  int16_t Voc_dyn;  // Battery modeled charge voltage at soc, V
  int16_t Voc_stat; // Ekf reference charge voltage, V
  uint16_t falw;    // Fail word
  Sum_st(void){}
  void assign(const time32_t now)
  {
    char buffer[32];
    this->t = now;
    this->Tb = int16_t(1*600.);
    this->Vb = int16_t(2*1200.);
    this->Ib = int16_t(3*600.);
    this->soc = int16_t(4*16000.);
    this->soc_ekf = int16_t(5*16000.);
    this->Voc_dyn = int16_t(6*1200.);
    this->Voc_stat = int16_t(7*1200.);
    this->falw = 8;
    time_long_2_str(now, buffer);
  }
  void print(void)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("unit_h, %s, %ld, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %d,",
      buffer, this->t,
      double(this->Tb)/600.,
      double(this->Vb)/1200.,
      double(this->Ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->Voc_dyn)/1200.,
      double(this->Voc_stat)/1200.,
      this->falw);
    Serial1.printf("unit_h, %s, %ld, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %d,",
      buffer, this->t,
      double(this->Tb)/600.,
      double(this->Vb)/1200.,
      double(this->Ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->Voc_dyn)/1200.,
      double(this->Voc_stat)/1200.,
      this->falw);
  }
  void nominal()
  {
    this->t = 0L;
    this->Tb = 0.;
    this->Vb = 0.;
    this->Ib = 0.;
    this->soc = 0.;
    this->soc_ekf = 0.;
    this->Voc_dyn = 0.;
    this->Voc_stat = 0.;
    this->falw = 0;
  }
};

// Function prototypes
void large_reset_fault_buffer(struct Flt_st *flt, const int iflt, const int nflt);
void large_reset_summary(struct Sum_st *sum, const int isum, const int nsum);
void print_all_fault_buffer(struct Flt_st *sum, const int iflt, const int nflt);
void print_all_summary(struct Sum_st *sum, const int isum, const int nsum);

#endif