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
struct PublishPars
{
  Publish pubList;          // Publish object
  PublishPars(void)
  {
    this->pubList = Publish();
  }
};


struct CommandPars
{
  char buffer[256];         // Auxiliary print buffer
  String input_string;      // A string to hold incoming data
  boolean string_complete;  // whether the string is complete
  boolean enable_wifi;      // Enable wifi
  boolean model_cutback;    // On model cutback
  boolean model_saturated;  // Sim on cutback and saturated
  boolean soft_reset;       // Use talk to reset main
  boolean write_summary;    // Use talk to issue a write command to summary
  float ibatt_tot_bias_amp; // Runtime bias of amplified shunt sensor, A
  float ibatt_tot_bias_noamp; // Runtime bias of non-amplified shunt sensor, A
  boolean dc_dc_on;         // DC-DC charger is on
  boolean blynking;         // Using Serial1 for Blynk.  Turn off normal Serial1 monitoring and echo
  CommandPars(void)
  {
    this->string_complete = false;
    this->enable_wifi = false;
    this->model_cutback = false;
    this->model_saturated = false;
    this->soft_reset = false;
    this->write_summary = false;
    ibatt_tot_bias_amp = 0.;
    ibatt_tot_bias_noamp = 0.;
    dc_dc_on = false;
    blynking = false;
  }
  void cmd_reset(void)
  {
    this->soft_reset = true;
  }
  void cmd_summarize(void)
  {
    this->write_summary = true;
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
    Serial.printf("command parameters(cp):\n");
    Serial.printf("  enable_wifi =                %d;\n", this->enable_wifi);
    Serial.printf("  model_cutback =              %d;\n", this->model_cutback);
    Serial.printf("  model_saturated =            %d;\n", this->model_saturated);
    Serial.printf("  soft_reset =                 %d;\n", this->soft_reset);
    Serial.printf("  write_summary =              %d;\n", this->write_summary);
    Serial.printf("  ibatt_tot_bias_amp =   %7.3f;\n", this->ibatt_tot_bias_amp);
    Serial.printf("  ibatt_tot_bias_noamp = %7.3f;\n", this->ibatt_tot_bias_noamp);
    Serial.printf("  dc_dc_on =                   %d;\n", this->dc_dc_on);
    Serial.printf("  blynking =                   %d;\n", this->blynking);
  }
};            


#endif
