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
#include "local_config.h"

struct CommandPars
{
  char buffer[280];         // Auxiliary print buffer
  String input_string;      // Hold incoming data
  boolean token;            // Whether input_string is complete
  boolean model_cutback;    // On model cutback
  boolean model_saturated;  // Sim on cutback and saturated
  boolean soft_reset;       // Use talk to reset main
  boolean write_summary;    // Use talk to issue a write command to summary
  float ib_tot_bias_amp;    // Runtime bias of amplified shunt sensor, A
  float ib_tot_bias_noa;    // Runtime bias of non-amplified shunt sensor, A
  boolean dc_dc_on;         // DC-DC charger is on
  String queue_str;         // Hold chit_chat queue data - queue with Control pass, 1 per Control pass
  String soon_str;          // Hold chit_chat soon data - priority with next Control pass, 1 per Control pass
  String asap_str;          // Hold chit_chat asap data - no waiting, ASAP all of now_str processed before Control pass
  boolean publishS;         // Print serial monitor data
  uint8_t print_mult;       // Print multiplier for objects
  unsigned long num_v_print;// Number of print echos made, for checking on BLE
  float Tb_bias_model;      // Bias on Tb for model, C
  float s_t_sat;            // Scalar on saturation test time set and reset
  uint8_t eframe_mult;      // Frame multiplier for EKF execution.  Number of READ executes for each EKF execution
  boolean fake_faults;      // Faults faked (ignored).  Used to evaluate a configuration, deploy it without disrupting use

  CommandPars(void)
  {
    this->token = false;
    this->model_cutback = false;
    this->model_saturated = false;
    this->soft_reset = false;
    this->write_summary = false;
    ib_tot_bias_amp = 0.;
    ib_tot_bias_noa = 0.;
    dc_dc_on = false;
    publishS = false;
    print_mult = 4;
    num_v_print = 0UL;
    Tb_bias_model = 0.;
    s_t_sat = 1.;
    eframe_mult = 20;
    fake_faults = FAKE_FAULTS;
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
    this->model_cutback = true;
    this->model_saturated = true;
    this->soft_reset = true;
    this->num_v_print = 0UL;
  }

  void pretty_print(void)
  {
    Serial.printf("command parameters(cp):\n");
    Serial.printf(" model_cutback=%d;\n", this->model_cutback);
    Serial.printf(" model_saturated=%d;\n", this->model_saturated);
    Serial.printf(" soft_reset=%d;\n", this->soft_reset);
    Serial.printf(" write_summary=%d;\n", this->write_summary);
    Serial.printf(" ib_tot_bias_amp=%7.3f;\n", this->ib_tot_bias_amp);
    Serial.printf(" ib_tot_bias_noa=%7.3f;\n", this->ib_tot_bias_noa);
    Serial.printf(" dc_dc_on=%d;\n", this->dc_dc_on);
    Serial.printf(" publishS=%d;\n", this->publishS);
    Serial.printf(" print_mult=%d;\n", this->print_mult);
    Serial.printf(" tb_bias_mod=%7.3f;\n", this->Tb_bias_model);
    Serial.printf(" s_t_sat=%7.3f;\n", this->s_t_sat);
    Serial.printf(" eframe_mult=%d;\n", this->eframe_mult);
    Serial.printf(" fake_faults=%d;\n", this->fake_faults);
  }

  void assign_eframe_mult(const uint8_t count)
  {
    this->eframe_mult = count;
  }

  void assign_print_mult(const uint8_t count)
  {
    this->print_mult = count;
  }

};            


#endif
