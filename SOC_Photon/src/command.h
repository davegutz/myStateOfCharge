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

#ifndef _COMMAND_H
#define _COMMAND_H

#include "myCloud.h"

// Definition of structure for external control coordination
// Default values below are important:  they determine behavior
// after a reset.   Also prevent junk behavior on initial build.
struct CommandPars
{
  char buffer[256];         // Auxiliary print buffer
  Publish pubList;          // Publish object
  String input_string;      // a string to hold incoming data
  boolean string_complete;  // whether the string is complete
  boolean stepping;         // active step adder
  double step_val;          // Step size
  boolean vectoring;        // Active battery test vector
  int8_t vec_num;           // Active vector number
  unsigned long vec_start;  // Start of active vector
  boolean enable_wifi;      // Enable wifi
  double socs_ekf;          // EKF output
  CommandPars(void)
  {
    this->string_complete = false;
    this->stepping = false;
    this->step_val = 0.;
    this->vectoring = false;
    this->vec_num = 0;
    this->vec_start = 0;
    this->enable_wifi = false;
    this->pubList = Publish();
  }
};            

#endif
