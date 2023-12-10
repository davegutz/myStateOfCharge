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
#include "constants.h"
#include "Z.h"
// #include "Adjust.h"

// DS2482 data union
typedef union {
	float t_c;
	boolean ready;
} Tb_union;


// Definition of structure for external control coordination
struct PublishPars
{
  Publish pubList;          // Publish object
  PublishPars(void)
  {
    pubList = Publish();
  }
};


class CommandPars
{
public:
  ~CommandPars();

  // Small static value area for 'retained'
  String input_str;         // Hold incoming data
  String end_str;           // Hold chit_chat end data - after everything else, 1 per Control pass
  String queue_str;         // Hold chit_chat queue data - queue with Control pass, 1 per Control pass
  String soon_str;          // Hold chit_chat soon data - priority with next Control pass, 1 per Control pass
  String asap_str;          // Hold chit_chat asap data - no waiting, ASAP all of now_str processed before Control pass
  boolean token;            // Whether input_str is complete
  boolean inf_reset;        // Use talk to reset infinite counter
  boolean model_cutback;    // On model cutback
  boolean model_saturated;  // Sim on cutback and saturated
  unsigned long num_v_print;// Number of print echos made, for checking on BLE
  boolean publishS;         // Print serial monitor data
  boolean soft_reset;       // Use talk to reset main
  Tb_union tb_info;         // Use cp to pass DS2482 I2C information
  boolean write_summary;    // Use talk to issue a write command to summary

  CommandPars()
  {
    token = false;
    inf_reset = false;
    model_cutback = false;
    model_saturated = false;
    num_v_print = 0UL;
    publishS = false;
    soft_reset = false;
    write_summary = false;
    tb_info.t_c = 0.;
    tb_info.ready = false;
  }

  void cmd_reset(void) { soft_reset = true; }

  void cmd_summarize(void) { write_summary = true; }

  void large_reset(void)
  {
    model_cutback = true;
    model_saturated = true;
    soft_reset = true;
    num_v_print = 0UL;
  }

  void pretty_print(void)
  {
    #ifndef DEPLOY_PHOTON
      Serial.printf("command parameters(cp):\n");
      Serial.printf(" inf_reset %d\n", inf_reset);
      Serial.printf(" model_cutback %d\n", model_cutback);
      Serial.printf(" model_saturated %d\n", model_saturated);
      Serial.printf(" publishS %d\n", publishS);
      Serial.printf(" soft_reset %d\n", soft_reset);
      Serial.printf(" tb_info.t_c %7.3f\n", tb_info.t_c);
      Serial.printf(" tb_info.ready %d\n", tb_info.ready);
      Serial.printf(" write_summary %d\n\n", write_summary);
    #endif
  }

};            


#endif
