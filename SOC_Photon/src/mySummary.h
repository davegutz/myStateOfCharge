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
#include "mySubs.h"
#include "command.h"
extern CommandPars cp;

// SRAM retention summary
struct Sum_st
{
  unsigned long t;  // Timestamp
  int16_t Tb;       // Battery temperature, filtered, C
  int16_t Vb;       // Battery measured potential, filtered, V
  int16_t Ib;       // Battery measured input current, filtered, A
  int16_t soc;      // Battery state of charge, free Coulomb counting algorithm, %
  int16_t soc_ekf;  // Battery state of charge, ekf, %
  int16_t Voc_dyn;  // Battery modeled charge voltage at soc, V
  int16_t Voc_stat; // Ekf reference charge voltage, V
  int16_t tweak_sclr_amp;  // Amplified Coulombic Efficiency scalar
  int16_t tweak_sclr_noa;  // Non-Amplified Coulombic Efficiency scalar
  uint16_t falw;    // Fail word
  Sum_st(void){}
  void assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen)
  {
    char buffer[32];
    this->t = now;
    this->Tb = int16_t(Sen->Tb*600.);
    this->Vb = int16_t(Sen->Vb*1200.);
    this->Ib = int16_t(Sen->Ib*600.);
    this->soc = int16_t(Mon->soc()*16000.);
    this->soc_ekf = int16_t(Mon->soc_ekf()*16000.);
    this->Voc_dyn = int16_t(Mon->Voc()*1200.);
    this->Voc_stat = int16_t(Mon->Voc_stat()*1200.);
    this->tweak_sclr_amp = int16_t(Sen->ShuntAmp->tweak_sclr()*16000.);
    this->tweak_sclr_noa = int16_t(Sen->ShuntNoAmp->tweak_sclr()*16000.);
    this->falw = Sen->Flt->falw();
    time_long_2_str(now, buffer);
  }
  void print(void)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("%s, %ld, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %10.6f, %10.6f, %d,",
      buffer, this->t,
      double(this->Tb)/600.,
      double(this->Vb)/1200.,
      double(this->Ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->Voc_dyn)/1200.,
      double(this->Voc_stat)/1200.,
      double(this->tweak_sclr_amp)/16000.,
      double(this->tweak_sclr_noa)/16000.,
      this->falw);
    if ( !cp.blynking ) Serial1.printf("%s, %ld, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %10.6f, %10.6f, %d,",
      buffer, this->t,
      double(this->Tb)/600.,
      double(this->Vb)/1200.,
      double(this->Ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->Voc_dyn)/1200.,
      double(this->Voc_stat)/1200.,
      double(this->tweak_sclr_amp)/16000.,
      double(this->tweak_sclr_noa)/16000.,
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
    this->tweak_sclr_amp = 1.;
    this->tweak_sclr_noa = 1.;
    this->falw = 0;
  }
};

// Function prototypes
void print_all_summary(struct Sum_st *sum, const int isum, const int nsum);
void large_reset_summary(struct Sum_st *sum, const int isum, const int nsum);

#endif