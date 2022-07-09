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
  int16_t Tb;       // Battery temperature, filtered, C
  int16_t Vb;       // Battery measured potential, filtered, V
  int16_t Ib;       // Battery measured input current, filtered, A
  int16_t soc;      // Battery state of charge, free Coulomb counting algorithm, %
  int16_t soc_ekf;  // Battery state of charge, ekf, %
  int16_t Voc_dyn;  // Battery modeled charge voltage at soc, V
  int16_t Voc_ekf;  // Ekf estimated charge voltage, V
  int16_t tweak_sclr_amp;  // Amplified Coulombic Efficiency scalar
  int16_t tweak_sclr_noa;  // Non-Amplified Coulombic Efficiency scalar
  Sum_st(void){}
  void assign(const time32_t now, const double Tbatt, const double Vbatt, const double Ibatt,
    const double soc_ekf, const double soc, const double Voc_dyn, const double Voc_ekf,
    const double tweak_sclr_amp, const double tweak_sclr_noa)
  {
    char buffer[32];
    this->t = now;
    this->Tb = int16_t(Tbatt*600.);
    this->Vb = int16_t(Vbatt*1200.);
    this->Ib = int16_t(Ibatt*600.);
    this->soc = int16_t(soc*16000.);
    this->soc_ekf = int16_t(soc_ekf*16000.);
    this->Voc_dyn = int16_t(Voc_dyn*1200.);
    this->Voc_ekf = int16_t(Voc_ekf*1200.);
    this->tweak_sclr_amp = int16_t(tweak_sclr_amp*16000.);
    this->tweak_sclr_noa = int16_t(tweak_sclr_noa*16000.);
    time_long_2_str(now, buffer);
  }
  void print(void)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("%s, %ld, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %10.6f, %10.6f,",
          buffer, this->t,
          double(this->Tb)/600.,
          double(this->Tb)/1200.,
          double(this->Ib)/600.,
          double(this->soc)/16000.,
          double(this->soc_ekf)/16000.,
          double(this->Voc_dyn)/1200.,
          double(this->Voc_ekf)/1200.,
          double(this->tweak_sclr_amp)/16000.,
          double(this->tweak_sclr_noa)/16000.);
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
    this->Voc_ekf = 0.;
    this->tweak_sclr_amp = 1.;
    this->tweak_sclr_noa = 1.;
  }
};

// Function prototypes
void print_all_summary(struct Sum_st *sum, const int isum, const int nsum);
void large_reset_summary(struct Sum_st *sum, const int isum, const int nsum);

#endif