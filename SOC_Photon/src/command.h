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
  boolean enable_wifi;      // Enable wifi
  boolean model_cutback;    // on model cutback
  boolean model_saturated;  //  Model on cutback and saturated
  boolean soft_reset;       // Use talk to reset main
  CommandPars(void)
  {
    this->string_complete = false;
    this->enable_wifi = false;
    this->pubList = Publish();
    this->model_cutback = false;
    this->model_saturated = false;
  }
  void small_reset(void)
  {
    this->soft_reset = true;
  }
  void large_reset(void)
  {
    this->enable_wifi = false;
    this->model_cutback = true;
    this->model_saturated = true;
    this->soft_reset = true;
  }
  void pretty_print(void)
  {
    Serial.printf("enable_wifi = %d, model_cutback = %d, model_saturated = %d, soft_reset = %d\n",
      this->enable_wifi, this->model_cutback, this->model_saturated, this->soft_reset);
  }
};            

#endif
