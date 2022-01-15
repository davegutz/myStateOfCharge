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
// extern char buffer[256];

// SRAM retention summary
struct Sum_st
{
  unsigned long t;    // Timestamp
  int16_t Tbatt;      // Battery temperature, filtered, C
  float Vbatt;        // Battery measured potential, filtered, V
  int8_t Ishunt;      // Batter measured input current, filtered, A
  int8_t SOC_f;       // Battery state of charge, free Coulomb counting algorithm, %
  int8_t dV;          // Estimated adjustment to EKF-VOC algorithm to match free Coulomb Counter, V*100
  Sum_st(void){}
  void assign(const time32_t now, const double Tbatt, const double Vbatt, const double Ishunt,
    const double soc_ekf, const double soc_f, const double dV_dsoc)
  {
    char buffer[32];
    this->t = now;
    this->Tbatt = Tbatt;
    this->Vbatt = float(Vbatt);
    this->Ishunt = Ishunt*4;
    this->SOC_f = soc_f*100;
    this->dV = int8_t(min(max((soc_ekf - soc_f) * dV_dsoc, -1.25), 1.25) * 100.);
    time_long_2_str(now, buffer);
  }
  void print(void)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("%s, %ld, %4d, %7.3f, %6.2f, %7d, %5.2f,",
          buffer, this->t, this->Tbatt, this->Vbatt, double(this->Ishunt)/4., this->SOC_f, double(this->dV)/100.);
  }
  void nominal()
  {
    this->t = 0L;
    this->Tbatt = 0.;
    this->Vbatt = 0.;
    this->Ishunt = 0.;
    this->SOC_f = 0.;
    this->dV = 0.;
  }
};

// Function prototypes
void print_all_summary(struct Sum_st *sum, const int isum, const int nsum);
void large_reset_summary(struct Sum_st *sum, const int isum, const int nsum);

#endif