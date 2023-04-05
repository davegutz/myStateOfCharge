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

#ifndef _COMMAND_H
#define _COMMAND_H

#include "myCloud.h"

// Definition of structure for external control coordination
struct PublishPars
{
  Publish pubList;          // Publish object
  PublishPars(void)
  {
    pubList = Publish();
  }
};


struct CommandPars
{
  char buffer[280];         // Auxiliary print buffer
  String input_str;      // Hold incoming data
  String queue_str;         // Hold chit_chat queue data - queue with Control pass, 1 per Control pass
  String soon_str;          // Hold chit_chat soon data - priority with next Control pass, 1 per Control pass
  String asap_str;          // Hold chit_chat asap data - no waiting, ASAP all of now_str processed before Control pass
  boolean token;            // Whether input_str is complete
  boolean dc_dc_on;         // DC-DC charger is on
  uint8_t eframe_mult;      // Frame multiplier for EKF execution.  Number of READ executes for each EKF execution
  boolean fake_faults;      // Faults faked (ignored).  Used to evaluate a configuration, deploy it without disrupting use
  boolean inf_reset;        // Use talk to reset infinite counter
  boolean model_cutback;    // On model cutback
  boolean model_saturated;  // Sim on cutback and saturated
  unsigned long num_v_print;// Number of print echos made, for checking on BLE
  uint8_t print_mult;       // Print multiplier for objects
  boolean publishS;         // Print serial monitor data
  boolean soft_reset;       // Use talk to reset main
  float s_t_sat;            // Scalar on saturation test time set and reset
  float Tb_bias_model;      // Bias on Tb for model, C
  boolean write_summary;    // Use talk to issue a write command to summary

  CommandPars(void)
  {
    token = false;
    dc_dc_on = false;
    eframe_mult = EKF_EFRAME_MULT;
    fake_faults = FAKE_FAULTS;
    inf_reset = false;
    model_cutback = false;
    model_saturated = false;
    num_v_print = 0UL;
    print_mult = 4;
    publishS = false;
    soft_reset = false;
    s_t_sat = 1.;
    Tb_bias_model = 0.;
    write_summary = false;
  }

  void assign_eframe_mult(const uint8_t count)
  {
    eframe_mult = count;
  }

  void assign_print_mult(const uint8_t count)
  {
    print_mult = count;
  }

  void cmd_reset(void)
  {
    soft_reset = true;
  }

  void cmd_summarize(void)
  {
    write_summary = true;
  }

  void large_reset(void)
  {
    model_cutback = true;
    model_saturated = true;
    soft_reset = true;
    num_v_print = 0UL;
  }

  void pretty_print(void)
  {
    Serial.printf("command parameters(cp):\n");
    Serial.printf(" dc_dc_on %d\n", dc_dc_on);
    Serial.printf(" eframe_mult %d\n", eframe_mult);
    Serial.printf(" fake_faults %d\n", fake_faults);
    Serial.printf(" inf_reset %d\n", inf_reset);
    Serial.printf(" model_cutback %d\n", model_cutback);
    Serial.printf(" model_saturated %d\n", model_saturated);
    Serial.printf(" print_mult %d\n", print_mult);
    Serial.printf(" publishS %d\n", publishS);
    Serial.printf(" soft_reset %d\n", soft_reset);
    Serial.printf(" s_t_sat%7.3f\n", s_t_sat);
    Serial.printf(" tb_bias_mod%7.3f\n", Tb_bias_model);
    Serial.printf(" write_summary %d\n", write_summary);
  }

};            


#endif
