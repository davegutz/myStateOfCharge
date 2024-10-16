//
// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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

#include "Summary.h"
#include "parameters.h"

// print helper
void print_all_fault_buffer(const String code, struct Flt_st *flt, const uint16_t iflt, const uint16_t nflt)
{
Serial.printf("print_all_fault_buffer: iflt %d nflt %d\n", iflt, nflt);
  uint16_t i = iflt;  // Last one written was iflt
  uint16_t n = 0;
  while ( n++ < nflt )
  {
    if ( ++i > (nflt-1) ) i = 0; // circular buffer
    flt[i].print_flt(code);
  }
}

void reset_all_fault_buffer(const String code, struct Flt_st *flt, const uint16_t iflt, const uint16_t nflt)
{
  uint16_t i = iflt;  // Last one written was iflt
  uint16_t n = 0;
  while ( n++ < nflt )
  {
    if ( ++i > (nflt-1) ) i = 0; // circular buffer
    flt[i].put_nominal();
  }
}
