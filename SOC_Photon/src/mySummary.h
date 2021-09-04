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

// SRAM retention summary
struct Sum_st
{
  unsigned long time;         // Timestamp
  int16_t Tbatt;              // Battery temperature, filtered, F
  float Vbatt;                // Battery measured potential, filtered, V
  int8_t Ishunt;              // Batter measured input current, filtered, A
  int8_t SOC_f;               // Battery state of charge, free Coulomb counting algorithm, %
  int8_t dV;                  // Estimated adjustment to SOC-VOC algorithm to match free, V*100
  Sum_st(void){}
  void assign(const unsigned long now, const double Tbatt, const double Vbatt, const double Ishunt,
    const double soc_s, const double soc_f, const double dV_dsoc)
  {
    this->time = now;
    this->Tbatt = Tbatt;
    this->Vbatt = float(Vbatt);
    this->Ishunt = Ishunt;
    this->SOC_f = soc_f*100;
    this->dV = int8_t(min(max((soc_s - soc_f) * dV_dsoc, -1.2), 1.2) * 100.);
  }
  void print(void)
  {
    char tempStr[50];
    time_long_2_str(time, tempStr);
    Serial.printf("%s, %ld, %4d, %7.3f, %4d, %7d, %5d,", tempStr, time, Tbatt, Vbatt, Ishunt, SOC_f, dV);
  }
};

//
void print_all(struct Sum_st *sum, const int isum, const int nsum)
{
  Serial.printf("i,  date,  time,    Tbatt,  Vbatt, Ishunt,  SOC_f,  dV,\n");
  int i = isum;  // Last one written was isum
  int n = -1;
  while ( ++n < nsum )
  {
    if ( ++i>nsum-1 ) i = 0;  // Increment beyond last one written
    Serial.printf("%d,  ", n);
    sum[i].print();
    Serial.printf("\n");
  }
}

#endif