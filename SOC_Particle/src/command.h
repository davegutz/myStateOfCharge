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


// A limited class of variables needed to tweak behavior.  Perhaps could be combined with CommandPars if can
// convince myself that CommandPars is needed to be treated special and separate for backup RAM use?
class AdjustPars
{
public:
  ~AdjustPars();

  // Small static value area for 'retained'
  boolean fail_tb;            // Make hardware bus read ignore Tb and fail it
  unsigned long int tail_inj; // Tail after end injection, ms
  float tb_stale_time_sclr;   // Scalar on persistences of Tb hardware stale chec, (1)
  unsigned long int wait_inj; // Wait before start injection, ms

  // Adjustment handling structure
  BooleanZ *fail_tb_p;
  FloatZ *tb_stale_time_sclr_p;
  ULongZ *tail_inj_p;
  ULongZ *wait_inj_p;
  uint8_t size_;
  Z *Z_[4];

  AdjustPars()
  {
    nominalize();
    fail_tb_p             = new BooleanZ("Xu", NULL, "Ignore Tb & fail", "1=Fail",  false, true, &fail_tb, 0, true);
    tb_stale_time_sclr_p  = new FloatZ("Xv", NULL, "scl Tb 1-wire stale pers", "slr",  0, 100, &tb_stale_time_sclr, 1, true);
    tail_inj_p            = new ULongZ("XT", NULL, "tail end inj", "ms", 0UL, 120000UL, &tail_inj, 0UL, true);
    wait_inj_p            = new ULongZ("XW", NULL, "wait start inj", "ms", 0UL, 120000UL, &wait_inj, 0UL, true);
    Z_[size_++] = fail_tb_p;
    Z_[size_++] = tb_stale_time_sclr_p;
    Z_[size_++] = tail_inj_p;
    Z_[size_++] = wait_inj_p;
  }

  void large_reset(void)
  {
    nominalize();
  }

  void nominalize(void)
  {
    tb_stale_time_sclr = 1;
  }

  void pretty_print(void)
  {
    Serial.printf("adjust parameters(ap):\n");
    for ( uint8_t i=0; i<size_; i++ ) Z_[i] -> print();
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
  float cycles_inj;         // Number of injection cycles
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
  Tb_union tb_info;         // Use cp to pass DS2482 I2C information
  boolean write_summary;    // Use talk to issue a write command to summary

  // Adjustment handling structure
  FloatZ *cycles_inj_p;
  Uint8tZ *eframe_mult_p;
  BooleanZ *fake_faults_p;
  Uint8tZ *print_mult_p;
  FloatZ *s_t_sat_p;
  FloatZ *Tb_bias_model_p;
  uint8_t size_;
  Z *Z_[6];


  CommandPars()
  {
    token = false;
    dc_dc_on = false;
    eframe_mult = EKF_EFRAME_MULT; // DE
    fake_faults = FAKE_FAULTS; // Ff
    inf_reset = false;
    model_cutback = false;
    model_saturated = false;
    num_v_print = 0UL;
    print_mult = DP_MULT;  // DP
    publishS = false;
    soft_reset = false;
    s_t_sat = 1.;
    Tb_bias_model = 0.;  // D^
    write_summary = false;
    tb_info.t_c = 0.;
    tb_info.ready = false;
    size_ = 0;
    cycles_inj_p  = new FloatZ("XC", NULL, "Number prog cycle", "float",  0, 1000, &cycles_inj, 0, true);
    eframe_mult_p = new Uint8tZ("DE", NULL, "EKF Multiframe rate x Dr",  "uint",  0, UINT8_MAX, &eframe_mult, EKF_EFRAME_MULT, true);
    fake_faults_p = new BooleanZ("Ff", NULL, "Faults ignored",  "0=False, 1=True",  0, 1, &fake_faults, FAKE_FAULTS, true);
    print_mult_p  = new Uint8tZ("DP", NULL, "Print multiplier x Dr", "uint",  0, UINT8_MAX, &print_mult, DP_MULT, true);
    s_t_sat_p     = new FloatZ("Xs", NULL, "scalar on T_SAT", "slr",  0, 100, &s_t_sat, 1, true);
    Tb_bias_model_p  = new FloatZ("D^", NULL, "Del model", "deg C",  -50, 50, &Tb_bias_model, TEMP_BIAS, true);
    Z_[size_++] = cycles_inj_p;
    Z_[size_++] = eframe_mult_p;
    Z_[size_++] = fake_faults_p;
    Z_[size_++] = print_mult_p;
    Z_[size_++] = s_t_sat_p;
    Z_[size_++] = Tb_bias_model_p;
  }

  void assign_eframe_mult(const uint8_t count) { eframe_mult = count; }

  void assign_print_mult(const uint8_t count)  { print_mult = count;  }

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
    Serial.printf("command parameters(cp):\n");
    Serial.printf(" dc_dc_on %d\n", dc_dc_on);
    Serial.printf(" inf_reset %d\n", inf_reset);
    Serial.printf(" model_cutback %d\n", model_cutback);
    Serial.printf(" model_saturated %d\n", model_saturated);
    Serial.printf(" publishS %d\n", publishS);
    Serial.printf(" soft_reset %d\n", soft_reset);
    Serial.printf(" tb_info.t_c %7.3f\n", tb_info.t_c);
    Serial.printf(" tb_info.ready %d\n", tb_info.ready);
    Serial.printf(" write_summary %d\n\n", write_summary);
    for ( uint8_t i=0; i<size_; i++ ) Z_[i] -> print();
  }

};            


#endif
