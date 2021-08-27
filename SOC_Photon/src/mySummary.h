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
  int8_t SOC;                 // Battery state of charge, %
  Sum_st(void){}
  void assign(const unsigned long now, const double Tbatt, const double Vbatt, const double Ishunt, const double soc)
  {
    this->time = now;
    this->Tbatt = Tbatt;
    this->Vbatt = float(Vbatt);
    this->Ishunt = Ishunt;
    this->SOC = soc*100;
  }
  void print(void)
  {
    char tempStr[50];
    time_long_2_str(time, tempStr);
    Serial.printf("%s, %4d, %7.3f, %4d, %7d,", tempStr, Tbatt, Vbatt, Ishunt, SOC);
  }
};

//
void print_all(struct Sum_st *sum, const int n)
{
  Serial.printf("i,  time,      Tbatt,  Vbatt, Ishunt,  SOC\n");
  for ( int i=0; i<n; i++ )
  {
    Serial.printf("%d,  ", i);
    sum[i].print();
    Serial.printf("\n");
  }
}

#endif