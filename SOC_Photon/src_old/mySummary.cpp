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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "mySummary.h"

// print helpler
void print_all_summary(struct Sum_st *sum, const int isum, const int nsum)
{
  int i = isum;  // Last one written was isum
  int n = -1;
  while ( ++n < nsum )
  {
    if ( ++i>nsum-1 ) i = 0;  // Increment beyond last one written
    Serial.printf("%d,  ", n);
    sum[i].print();
    Serial.printf("\n");
  }
  Serial.printf("i,  date,                time,    Tbatt, Vbatt, Ibatt, soc, soc_ekf, voc_dyn, voc, tweak_bias_amp, tweak_bias_noa,\n");
}

// reset helper
void large_reset_summary(struct Sum_st *sum, const int isum, const int nsum)
{
  int i = isum;  // Last one written was isum
  int n = -1;
  while ( ++n < nsum )
  {
    if ( ++i>nsum-1 ) i = 0;  // Increment beyond last one written
    sum[i].nominal();
  }
}
