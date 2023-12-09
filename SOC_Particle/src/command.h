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
#include "Adjust.h"
#include <list>

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
  float cc_diff_sclr;         // Scale cc_diff detection thresh, scalar
  boolean fail_tb;            // Make hardware bus read ignore Tb and fail it
  unsigned long int tail_inj; // Tail after end injection, ms
  float tb_stale_time_sclr;   // Scalar on persistences of Tb hardware stale chec, (1)
  unsigned long int wait_inj; // Wait before start injection, ms
  boolean testB;
  double testD;

  // Adjustment handling structure
  FloatZ *cc_diff_sclr_p;
  BooleanZ *fail_tb_p;
  FloatZ *tb_stale_time_sclr_p;
  ULongZ *tail_inj_p;
  ULongZ *wait_inj_p;
  uint8_t n_;
  uint8_t m_;
  AjBoolean *testB_p;
  AjDouble *testD_p;
  Z **Z_;
  std::list< Adjust<boolean>* > Xb_; 
  std::list< Adjust<double>* > Xd_;

  AdjustPars()
  {
    n_ = 0;
    cc_diff_sclr_p        = new FloatZ(&n_, "Fc", NULL, "Slr cc_diff thr ",          "slr",    0,      1000,     &cc_diff_sclr,      1,    true);
    fail_tb_p           = new BooleanZ(&n_, "Xu", NULL, "Ignore Tb & fail",          "1=Fail", false,  true,     &fail_tb,           0,    true);
    tb_stale_time_sclr_p  = new FloatZ(&n_, "Xv", NULL, "scl Tb 1-wire stale pers",  "slr",    0,      100,      &tb_stale_time_sclr,1,    true);
    tail_inj_p            = new ULongZ(&n_, "XT", NULL, "tail end inj",              "ms",     0UL,    120000UL, &tail_inj,          0UL,  true);
    wait_inj_p            = new ULongZ(&n_, "XW", NULL, "wait start inj",            "ms",     0UL,    120000UL, &wait_inj,          0UL,  true);

    Xb_.push_back(testB_p  = new AjBoolean("XB", NULL, "testB boolean",       "B-",     false,    true, &testB,          false,  true));
    Xd_.push_back(testD_p   = new AjDouble("XD", NULL, "testD double",        "D-",     0,        1,    &testD,          0.5,    true));

    Z_ = new Z*[n_];
    uint8_t i = 0;
    Z_[i++] = cc_diff_sclr_p;
    Z_[i++] = fail_tb_p;
    Z_[i++] = tb_stale_time_sclr_p;
    Z_[i++] = tail_inj_p;
    Z_[i++] = wait_inj_p;
    if ( i != n_ ) Serial.printf("WARN(command.h, AdjustPars):  size error i%d != n_%d\n", i, n_);
    set_nominal();
  }

  void large_reset(void)
  {
    set_nominal();
  }

  void pretty_print(void)
  {
    #ifndef DEPLOY_PHOTON
      Serial.printf("adjust parameters(ap):\n");
      for ( uint8_t i=0; i<n_; i++ ) Z_[i] -> print();
      std::for_each(  Xb_.begin(), Xb_.end(), std::mem_fun(&Adjust<boolean>::print) );
      std::for_each(  Xd_.begin(), Xd_.end(), std::mem_fun(&Adjust<double>::print) );
    #endif
    
    Serial.printf("\nOff-nominal:\n");
    for ( uint8_t i=0; i<n_; i++ ) if ( Z_[i]->off_nominal() ) Z_[i] -> print();
    std::for_each(  Xb_.begin(), Xb_.end(), std::mem_fun(&Adjust<boolean>::print_off) );
    std::for_each(  Xd_.begin(), Xd_.end(), std::mem_fun(&Adjust<double>::print_off) );
  }

  void set_nominal()
  {
      for ( uint16_t i=0; i<n_; i++ ) Z_[i]->set_nominal();
      // for ( uint16_t j=0; j<m_; i++ ) X_[j]->pull_set_nominal();
      testB_p->pull_set_nominal();
      testD_p->pull_set_nominal();
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
  unsigned long int until_q;  // Time until set v0, ms
  boolean write_summary;    // Use talk to issue a write command to summary

  // Adjustment handling structure
  FloatZ *cycles_inj_p;
  BooleanZ *dc_dc_on_p;
  Uint8tZ *eframe_mult_p;
  BooleanZ *fake_faults_p;
  Uint8tZ *print_mult_p;
  FloatZ *s_t_sat_p;
  FloatZ *Tb_bias_model_p;
  ULongZ *until_q_p;
  Z **Z_;
  uint8_t n_;


  CommandPars()
  {
    token = false;
    dc_dc_on = false;  // Xd
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
    until_q = 0UL;  // XQ
    n_ = 0;
    cycles_inj_p     = new FloatZ(&n_, "XC", NULL, "Number prog cycle",          "float",      0,    1000,       &cycles_inj, 0, true);
    dc_dc_on_p     = new BooleanZ(&n_, "Xd", NULL, "DC-DC charger on",           "0=F, 1=T",   0,    1,          &dc_dc_on, false, true);
    eframe_mult_p   = new Uint8tZ(&n_, "DE", NULL, "EKF Multiframe rate x Dr",   "uint",       0,    UINT8_MAX,  &eframe_mult, EKF_EFRAME_MULT, true);
    fake_faults_p  = new BooleanZ(&n_, "Ff", NULL, "Faults ignored",             "0=F, 1=T",   0,    1,          &fake_faults, FAKE_FAULTS, true);
    print_mult_p    = new Uint8tZ(&n_, "DP", NULL, "Print multiplier x Dr",      "uint",       0,    UINT8_MAX,  &print_mult, DP_MULT, true);
    s_t_sat_p        = new FloatZ(&n_, "Xs", NULL, "scalar on T_SAT",            "slr",        0,    100,        &s_t_sat, 1, true);
    Tb_bias_model_p  = new FloatZ(&n_, "D^", NULL, "Del model",                  "deg C",      -50,  50,         &Tb_bias_model, TEMP_BIAS, true);
    until_q_p        = new ULongZ(&n_, "XQ", NULL, "Time until v0",              "ms",         0UL,  500000UL,   &until_q, 0UL, true);
    Z_ = new Z*[n_];
    uint8_t i = 0;
    Z_[i++] = cycles_inj_p;
    Z_[i++] = dc_dc_on_p;
    Z_[i++] = eframe_mult_p;
    Z_[i++] = fake_faults_p;
    Z_[i++] = print_mult_p;
    Z_[i++] = s_t_sat_p;
    Z_[i++] = Tb_bias_model_p;
    Z_[i++] = until_q_p;
    if ( i != n_ ) Serial.printf("WARN(command.h, CommandPars):  size error i%d != n_%d\n", i, n_);
    set_nominal();
  }

  void cmd_reset(void) { soft_reset = true; }

  void cmd_summarize(void) { write_summary = true; }

  void large_reset(void)
  {
    model_cutback = true;
    model_saturated = true;
    soft_reset = true;
    num_v_print = 0UL;
    set_nominal();
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
      for ( uint8_t i=0; i<n_; i++ ) Z_[i] -> print();
    #endif
    Serial.printf("\nOff-nominal:\n");
    for ( uint8_t i=0; i<n_; i++ ) if ( Z_[i]->off_nominal() ) Z_[i] -> print();
  }

  void set_nominal()
  {
      for ( uint16_t i=0; i<n_; i++ ) Z_[i]->set_nominal();
  }
};            


#endif
