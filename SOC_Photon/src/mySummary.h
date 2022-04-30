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
  unsigned long t;    // Timestamp
  int16_t tb;       // Battery temperature, filtered, C
  int16_t vb;       // Battery measured potential, filtered, V
  int16_t ib;       // Battery measured input current, filtered, A
  int16_t soc;      // Battery state of charge, free Coulomb counting algorithm, %
  int16_t soc_ekf;  // Battery state of charge, ekf, %
  int16_t voc_dyn;  // Battery modeled charge voltage at soc, V
  int16_t voc_ekf;  // Ekf estimated charge voltage, V
  int16_t tweak_bias_amp;  // Accumulated amplified current bias, A
  int16_t tweak_bias_noa;  // Accumulated non-amplified current bias, A
  Sum_st(void){}
  void assign(const time32_t now, const double Tbatt, const double Vbatt, const double Ishunt,
    const double soc_ekf, const double soc, const double voc_dyn, const double voc_ekf,
    const double tweak_bias_amp, const double tweak_bias_noa)
  {
    char buffer[32];
    this->t = now;
    this->tb = int16_t(Tbatt*600.);
    this->vb = int16_t(Vbatt*1200.);
    this->ib = int16_t(Ishunt*600.);
    this->soc = int16_t(soc*16000.);
    this->soc_ekf = int16_t(soc_ekf*16000.);
    this->voc_dyn = int16_t(voc_dyn*1200.);
    this->voc_ekf = int16_t(voc_ekf*1200.);
    this->tweak_bias_amp = int16_t(tweak_bias_amp*1200.);
    this->tweak_bias_noa = int16_t(tweak_bias_noa*1200.);
    time_long_2_str(now, buffer);
  }
  void print(void)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("%s, %ld, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %7.3f,",
          buffer, this->t, double(this->tb)/600., double(this->vb)/1200., double(this->ib)/600., double(this->soc)/16000., double(this->soc_ekf)/16000.,
          double(this->voc_dyn)/1200., double(this->voc_ekf)/1200.,
          double(this->tweak_bias_amp)/1200.,  double(this->tweak_bias_noa)/1200.);
  }
  void nominal()
  {
    this->t = 0L;
    this->tb = 0.;
    this->vb = 0.;
    this->ib = 0.;
    this->soc = 0.;
    this->soc_ekf = 0.;
    this->voc_dyn = 0.;
    this->voc_ekf = 0.;
    this->tweak_bias_amp = 0.;
    this->tweak_bias_noa = 0.;
  }
};

// Function prototypes
void print_all_summary(struct Sum_st *sum, const int isum, const int nsum);
void large_reset_summary(struct Sum_st *sum, const int isum, const int nsum);

#endif