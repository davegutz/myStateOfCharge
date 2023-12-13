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

#include "mySubs.h"
#include "command.h"
#include "Battery.h"
#include "local_config.h"
#include "mySummary.h"
#include "parameters.h"
#include <math.h>
#include "debug.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern PrinterPars pr;  // Print buffer
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

// Process asap commands
void asap()
{
  get_string(&cp.asap_str);
  // if ( cp.token ) Serial.printf("asap:  talk('%s;')\n", cp.input_str.c_str());
}

// Process chat strings
void chat()
{
  #ifdef DEBUG_QUEUE
    Serial.printf("shebang [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n", cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif
  if ( cp.soon_str.length() )  // Do SOON first
  {
    get_string(&cp.soon_str);
  #ifdef DEBUG_QUEUE
    if ( cp.token ) Serial.printf("chat (SOON):  cmd('%s;') ASAP[%s] SOON[%s] QUEUE[%s] LAST[%s]\n", cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif
  }
  else if ( cp.queue_str.length() ) // Do QUEUE only after SOON empty
  {
    get_string(&cp.queue_str);
    #ifdef DEBUG_QUEUE
      if ( cp.token ) Serial.printf("chat (QUEUE):  cmd('%s;') ASAP[%s] SOON[%s] QUEUE[%s] LAST[%s]\n", cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
    #endif
  }
  else if ( cp.end_str.length() ) // Do QUEUE only after SOON empty
  {
    get_string(&cp.end_str);
    #ifdef DEBUG_QUEUE
      if ( cp.token ) Serial.printf("chat (QUEUE):  cmd('%s;') ASAP[%s] SOON[%s] QUEUE[%s] LAST[%s]\n", cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
    #endif
  }
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for talk()
void chit(const String cmd, const urgency when)
{
  #ifdef DEBUG_QUEUE
    String When;
    if ( when == NEW) When = "NEW";
    else if ( when == QUEUE) When = "QUEUE";
    else if (when == SOON) When = "SOON";
    else if (when == ASAP) When = "ASAP";
    else if (when == INCOMING) When = "INCOMING"; 
    Serial.printf("chit cmd=%s [%s]\n", cmd.c_str(), When.c_str());
  #endif
  if ( when == LAST )
    cp.end_str += cmd;
  if ( when == QUEUE )
    cp.queue_str += cmd;
  else if ( when == SOON )
    cp.soon_str += cmd;
  else
    cp.asap_str += cmd;
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for talk()
void clear_queues()
{
  cp.end_str = "";
  cp.queue_str = "";
  cp.soon_str = "";
  cp.asap_str = "";
}

// Clear adjustments that should be benign if done instantly
void benign_zero(BatteryMonitor *Mon, Sensors *Sen)  // BZ
{

  // Snapshots
  cp.cmd_summarize();  // Hs
  cp.cmd_summarize();  // Hs
  cp.cmd_summarize();  // Hs
  cp.cmd_summarize();  // Hs

  // Model
  ap.hys_scale = HYS_SCALE;  // Sh 1
  ap.slr_res = 1;  // Sr 1
  sp.Cutback_gain_slr_p->print_adj_print(1); // Sk 1
  ap.hys_state = 0;  // SH 0

  // Injection
  ap.ib_amp_add = 0;  // Dm 0
  ap.ib_noa_add = 0;  // Dn 0
  ap.vb_add = 0;  // Dv 0
  ap.ds_voc_soc = 0;  // Ds
  ap.Tb_bias_model = 0;  // D^
  ap.dv_voc_soc = 0;  // Dy
  ap.tb_stale_time_slr = 1;  // Xv 1
  ap.fail_tb = false;  // Xu 0

  // Noise
  ap.Tb_noise_amp = TB_NOISE;  // DT 0
  ap.Vb_noise_amp = VB_NOISE;  // DV 0
  ap.Ib_amp_noise_amp = IB_AMP_NOISE;  // DM 0
  ap.Ib_noa_noise_amp = IB_NOA_NOISE;  // DN 0

  // Intervals
  ap.eframe_mult = max(min(EKF_EFRAME_MULT, UINT8_MAX), 0); // DE
  ap.print_mult = max(min(DP_MULT, UINT8_MAX), 0);  // DP
  Sen->ReadSensors->delay(READ_DELAY);  // Dr

  // Fault logic
  ap.cc_diff_slr = 1;  // Fc 1
  ap.ib_diff_slr = 1;  // Fd 1
  ap.fake_faults = 0;  // Ff 0
  sp.put_Ib_select(0);  // Ff 0
  ap.ewhi_slr = 1;  // Fi
  ap.ewlo_slr = 1;  // Fo
  ap.ib_quiet_slr = 1;  // Fq 1
  ap.disab_ib_fa = 0;  // FI 0
  ap.disab_tb_fa = 0;  // FT 0
  ap.disab_vb_fa = 0;  // FV 0

}

// Talk Executive
void talk(BatteryMonitor *Mon, Sensors *Sen)
{
  float FP_in = -99.;
  int INT_in = -1;
  boolean reset = false;
  urgency request;

  // Serial event
  request = NEW;
  if (cp.token)
  {
    // Categorize the requests
    char key = cp.input_str.charAt(0);
    if ( key == 'c' )
    {
      request = INCOMING;
    }
    else if ( key == '-' && cp.input_str.charAt(1)!= 'c')
    {
      cp.input_str = cp.input_str.substring(1);  // Delete the leading '-'
      request = INCOMING;
    }
    else if ( key == '-' )
      request = ASAP;
    else if ( key == '+' )
      request = QUEUE;
    else if ( key == '*' )
      request = SOON;
    else if ( key == '<' )
      request = LAST;
    else if ( key == '>' )
    {
      cp.input_str = cp.input_str.substring(1);  // Delete the leading '>'
      request = INCOMING;
    }
    else
    {
      request = NEW;
    }

    // Limited echoing of Serial1 commands available
    if ( request==0 )
    {
      Serial.printf ("cmd: %s\n", cp.input_str.c_str());
      Serial1.printf ("cmd: %s\n", cp.input_str.c_str());
    }
    else
    {
      Serial.printf ("echo: %s, %d\n", cp.input_str.c_str(), request);
      Serial1.printf("echo: %s, %d\n", cp.input_str.c_str(), request);
    }

    // Deal with each request
    switch ( request )
    {
      case ( NEW ):  // Defaults to QUEUE
        // Serial.printf("new:%s,\n", cp.input_str.substring(0).c_str());
        chit( cp.input_str.substring(0) + ";", QUEUE);
        break;

      case ( ASAP ):
        // Serial.printf("asap:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", ASAP);
        break;

      case ( SOON ):
        // Serial.printf("soon:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", SOON);
        break;

      case ( QUEUE ):
        // Serial.printf("queue:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", QUEUE);
        break;

      case ( LAST ):
        // Serial.printf("last:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", LAST);
        break;

      case ( INCOMING ):
        switch ( cp.input_str.charAt(0) )
        {
          case ( 'b' ):  // Fault buffer
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'd' ):  // bd: fault buffer dump
                Serial.printf("\n");
                sp.print_history_array();
                sp.print_fault_header();
                sp.print_fault_array();
                sp.print_fault_header();
                break;

              case ( 'h' ):  // bh: History buffer reset
                sp.reset_his();
                break;

              case ( 'r' ):  // br: Fault buffer reset
                sp.reset_flt();
                break;

              case ( 'R' ):  // bR: Reset all buffers
                sp.reset_flt();
                sp.reset_his();
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" ? 'h'\n");
            }
            break;

          case ( 'B' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'm' ):  //* Bm
                INT_in = cp.input_str.substring(2).toInt();
                // sp.Mon_chm_p->print_adj_print(cp.input_str.substring(2).toInt());  // Bm
                // break;
                switch ( INT_in )
                {
                  case ( 0 ):  // Bm0: Mon Battleborn
                    sp.Mon_chm_p->print_adj_print(INT_in);
                    Mon->assign_all_mod("Battleborn");
                    Mon->chem_pretty_print();
                    cp.cmd_reset();
                    break;

                  case ( 1 ):  // Bm1: Mon CHINS
                    sp.Mon_chm_p->print_adj_print(INT_in);
                    Mon->assign_all_mod("CHINS");
                    Mon->chem_pretty_print();
                    cp.cmd_reset();
                    break;

                  case ( 2 ):  // Bm2: Mon Spare
                    sp.Mon_chm_p->print_adj_print(INT_in);
                    Mon->assign_all_mod("Spare");
                    Mon->chem_pretty_print();
                    cp.cmd_reset();
                    break;

                  default:
                    Serial.printf("%d ? 'h'", INT_in);
                }
                break;

              case ( 's' ):  //* Bs:  Simulation chemistry change
                INT_in = cp.input_str.substring(2).toInt();
                switch ( INT_in )
                {
                  case ( 0 ):  // Bs0: Sim Battleborn
                    sp.Sim_chm_p->print_adj_print(INT_in);
                    Sen->Sim->assign_all_mod("Battleborn");
                    cp.cmd_reset();
                    break;

                  case ( 1 ):  // Bs1: Sim CHINS
                    sp.Sim_chm_p->print_adj_print(INT_in);
                    Sen->Sim->assign_all_mod("CHINS");
                    cp.cmd_reset();
                    break;

                  case ( 2 ):  // Bs2: Sim Spare
                    sp.Sim_chm_p->print_adj_print(INT_in);
                    Sen->Sim->assign_all_mod("Spare");
                    cp.cmd_reset();
                    break;

                  default:
                    Serial.printf("%d ? 'h'", INT_in);
                }
                break;

              case ( 'P' ):  //* BP<>:  Number of parallel batteries in bank, e.g. '2P1S'
                FP_in = cp.input_str.substring(2).toFloat();
                if ( FP_in>0 )  // Apply crude limit to prevent user error
                {
                  Serial.printf("nP%5.2f to", sp.nP());
                  sp.put_nP(FP_in);
                  Serial.printf("%5.2f\n", sp.nP());
                }
                else
                  Serial.printf("err%5.2f; <=0\n", FP_in);
                break;

              case ( 'S' ):  //* BS<>:  Number of series batteries in bank, e.g. '2P1S'
                FP_in = cp.input_str.substring(2).toFloat();
                if ( FP_in>0 )  // Apply crude limit to prevent user error
                {
                  Serial.printf("nS%5.2f to", sp.nS());
                  sp.put_nS(FP_in);
                  Serial.printf("%5.2f\n", sp.nS());
                }
                else
                  Serial.printf("err%5.2f; <=0\n", FP_in);
                break;

              case ( 'Z' ):  // BZ :  Benign zeroing of settings to make clearing test easier
                  benign_zero(Mon, Sen);
                  Serial.printf("Benign Zero\n");
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" ? 'h'\n");
            }
            break;

          case ( 'c' ):  // c:  clear queues
            Serial.printf("***CLEAR QUEUES\n");
            clear_queues();
            break;

          case ( 'C' ):  // C:
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'a' ):  // Ca<>:  assign charge state in fraction to all versions including model
                FP_in = cp.input_str.substring(2).toFloat();
                if ( FP_in<1.1 )  // Apply crude limit to prevent user error
                {
                  initialize_all(Mon, Sen, FP_in, true);
                  #ifdef DEBUG_INIT
                    if ( sp.Debug()==-1 ){ Serial.printf("after initialize_all:"); debug_m1(Mon, Sen);}
                  #endif
                  if ( sp.Modeling() )
                  {
                    cp.cmd_reset();
                    chit("W3;", SOON);  // Wait 10 passes of Control
                  }
                  else
                  {
                    cp.cmd_reset();
                    chit("W3;", SOON);  // Wait 10 passes of Control
                  }
                }
                else
                  Serial.printf("soc%8.4f; err 0-1.1\n", FP_in);
                break;

              case ( 'm' ):  // Cm<>:  assign curve charge state in fraction to model only (ekf if modeling)
                FP_in = cp.input_str.substring(2).toFloat();
                if ( FP_in<1.1 )   // Apply crude limit to prevent user error
                {
                  Sen->Sim->apply_soc(FP_in, Sen->Tb_filt);
                  Serial.printf("soc%8.4f, dq%7.3f, soc_mod%8.4f, dq mod%7.3f,\n",
                      Mon->soc(), Mon->delta_q(), Sen->Sim->soc(), Sen->Sim->delta_q());
                  if ( sp.Modeling() ) cp.cmd_reset();
                }
                else
                  Serial.printf("soc%8.4f; must be 0-1.1\n", FP_in);
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" ? 'h'\n");
            }
            break;

          case ( 'D' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'A' ):  //*  DA<>:  Amp sensor bias
                sp.Ib_bias_amp_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'B' ):  //*  DB<>:  No Amp sensor bias
                sp.Ib_bias_noa_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'c' ):  //*  Dc<>:  Vb bias
                sp.Vb_bias_hdwe_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'E' ):  //   DE<>:  EKF execution frame multiplier
                ap.eframe_mult_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'i' ):  //*  Di<>:  Bias all current sensors (same way as Da and Db)
                chit("DI;", ASAP);  // Occur before reset
                cp.cmd_reset();
                break;

              case ( 'I' ):  //*  DI<>:  Bias all current sensors without reset (same way as Da and Db)
                sp.Ib_bias_all_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'm' ):  //   Dm<>:  Amp signal adder for faults
                ap.ib_amp_add_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'n' ):  //   Dn<>:  No Amp signal adder for faults
                ap.ib_noa_add_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'P' ):  //   DP<>:  PRINT multiplier
                ap.print_mult_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'r' ):  //   Dr<>:  READ sample time input
                ap.read_delay_p->print_adj_print(cp.input_str.substring(2).toInt());
                Sen->ReadSensors->delay(ap.read_delay);  // validated
                break;

              case ( 's' ):  //   Ds<>:  d_soc to Sim.voc_soc
                ap.ds_voc_soc_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 't' ):  //*  Dt<>:  Temp bias change hardware
                sp.Tb_bias_hdwe_p->print_adj_print(cp.input_str.substring(2).toFloat());
                cp.cmd_reset();
                break;

              case ( '^' ):  //   D^<>:  Temp bias change model for faults
                ap.Tb_bias_model_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'v' ):  //     Dv<>:  voltage signal adder for faults
                ap.vb_add_p->print_adj_print(cp.input_str.substring(2).toFloat());
                ap.vb_add_p->print1();
                break;

              case ( 'w' ):  //   * Dw<>:
                sp.Dw_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'y' ):  //   Dy<>:  delta Sim curve soc in
                ap.dv_voc_soc_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'T' ):  //   DT<>:  Tb noise
                ap.Tb_noise_amp_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'V' ):  //   DV<>:  Vb noise
                ap.Vb_noise_amp_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'M' ):  //   DM<>:  Ib amp noise
                ap.Ib_amp_noise_amp_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'N' ):  //   DN<>:  Ib noa noise
                ap.Ib_noa_noise_amp_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" ? 'h'\n");
            }
            break;

          case ( 'S' ):
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'A' ):  //*  SA<>:  Amp sensor scalar
                sp.Ib_scale_amp_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'B' ):  //*  SB<>:  No Amp sensor scalar
                sp.Ib_scale_noa_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'h' ):  //   Sh<>: scale hysteresis
                ap.hys_scale_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'H' ):  //   SH<>: state of all hysteresis
                ap.hys_state_p->print_adj_print(cp.input_str.substring(2).toFloat());
                Sen->Sim->hys_state(ap.hys_state);
                Sen->Flt->wrap_err_filt_state(-ap.hys_state);
                break;

              case ( 'q' ):  //*  Sq<>: scale capacity sim
                sp.S_cap_sim_p->print_adj_print(cp.input_str.substring(2).toFloat());
                Sen->Sim->apply_cap_scale(sp.S_cap_sim());
                if ( sp.Modeling() ) Mon->init_soc_ekf(Sen->Sim->soc());
                break;
            
              case ( 'Q' ):  //*  SQ<>: scale capacity mon
                sp.S_cap_mon_p->print_adj_print(cp.input_str.substring(2).toFloat());
                Mon->apply_cap_scale(sp.S_cap_mon());
                break;
            
              case ( 'r' ):  //   Sr<>:  scalar resistor
                ap.slr_res_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;
            
              case ( 'k' ):  //*  Sk<>:  scale cutback gain for sim rep of >print_adj_print(S
                sp.Cutback_gain_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;
            
              case ( 'V' ):  //*  SV<>:  Vb sensor scalar
                sp.Vb_scale_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.print(" ? 'h'\n");
            }
            break;

          case ( 'F' ):  // Fault stuff
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'c' ):  //   Fc<>: scale cc_diff threshold
                ap.cc_diff_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'd' ):  //   Fd<>: scale ib_diff threshold
                ap.ib_diff_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'f' ):  //* si, Ff<>:  fake faults
                ap.fake_faults_p->print_adj_print(cp.input_str.substring(2).toInt());
                sp.put_Ib_select(cp.input_str.substring(2).toInt());
                break;

              case ( 'I' ):  //   FI<>:  Fault disable ib hard
                ap.disab_ib_fa_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'i' ):  //   Fi<>: scale e_wrap_hi threshold
                ap.ewhi_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'o' ):  //   Fo<>: scale e_wrap_lo threshold
                ap.ewlo_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'q' ):  //   Fq<>: scale ib_quiet threshold
                ap.ib_quiet_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'T' ):  //   FT<>:  Fault disable tb stale
                ap.disab_tb_fa_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'V' ):  //   FV<>:  Fault disable vb hard
                ap.disab_vb_fa_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.print(" ? 'h'\n");
            }
            break;

          case ( 'H' ):  // History
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'd' ):  // Hd: History dump
                Serial.printf("\n");
                print_all_fault_buffer("unit_h", mySum, sp.Isum(), sp.nsum());
                sp.print_fault_header();
                chit("Pr;Q;", QUEUE);
                Serial.printf("\n");
                // sp.print_history_array();
                // sp.print_fault_header();
                // sp.print_fault_array();
                // sp.print_fault_header();
                break;

              case ( 'f' ):  // Hf: History dump faults only
                Serial.printf("\n");
                sp.print_fault_array();
                sp.print_fault_header();
                break;

              case ( 'R' ):  // HR: History reset
                Serial.printf("Reset his & flt...");
                sp.reset_his();
                sp.reset_flt();
                Serial.printf("done\n");
                break;

              case ( 's' ):  // Hs: History snapshot
                cp.cmd_summarize();
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.print(" ? 'h'\n");
            }
            break;

          case ( 'l' ):
            switch ( sp.Debug() )
            {
              case ( -1 ):  // l-1:
                // Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt_amp  ,Ishunt_noa  ,Vb_fo*10-110  ,voc_s*10-110  ,dv_dyn_s*10  ,v_s*10-110  , voc_dyn*10-110,,,,,,,,,,,\n");
                break;
              case ( 1 ):  // l1:
                print_serial_header();
                break;
              case ( 2 ):  // l2:
                print_signal_sel_header();
                print_serial_sim_header();
                print_serial_header();
                break;
              case ( 3 ):  // l3:
                print_serial_ekf_header();
                print_serial_sim_header();
                print_serial_header();
                break;
              default:
                print_serial_header();
            }
            break;

          case ( 'P' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'a' ):  // Pa:  Print all
                chit("Pm;", SOON);
                chit("Ps;", SOON);
                chit("Pr;", SOON);
                chit("PM;", SOON);
                chit("PN;", SOON);
                chit("Ph;", SOON);
                chit("Hd;", SOON);
                chit("Pf;", SOON);
                chit("Q;", SOON);
                break;

              case ( 'b' ):  // Pb:  Print Vb measure
                Serial.printf("\nVolt:");   Serial.printf("Vb_bias_hdwe,Vb_m,mod,Vb=,%7.3f,%7.3f,%d,%7.3f,\n", 
                  sp.Vb_bias_hdwe(), Sen->Vb_model, sp.Modeling(), Sen->Vb);
                break;

              case ( 'e' ):  // Pe:  Print EKF
                Serial.printf ("\nMon::"); Mon->EKF_1x1::pretty_print();
                Serial1.printf("\nMon::"); Mon->EKF_1x1::pretty_print();
                break;

              case ( 'f' ):  // Pf:  Print faults
                sp.print_history_array();
                sp.print_fault_header();
                sp.print_fault_array();
                sp.print_fault_header();
                Serial.printf ("\nSen::\n");
                Sen->Flt->pretty_print (Sen, Mon);
                Serial1.printf("\nSen::\n");
                Sen->Flt->pretty_print1(Sen, Mon);
                break;

              case ( 'm' ):  // Pm:  Print mon
                Serial.printf ("\nM:"); Mon->pretty_print(Sen);
                Serial.printf ("M::"); Mon->Coulombs::pretty_print();
                Serial.printf ("M::"); Mon->EKF_1x1::pretty_print();
                Serial.printf ("\nmodeling %d\n", sp.Modeling());
                break;

              case ( 'M' ):  // PM:  Print shunt Amp
                Serial.printf ("\n"); Sen->ShuntAmp->pretty_print();
                break;

              case ( 'N' ):  // PN:  Print shunt no amp
                Serial.printf ("\n"); Sen->ShuntNoAmp->pretty_print();
                break;

              case ( 'R' ):  // PR:  Print retained
                Serial.printf("\n"); sp.pretty_print( true );
                Serial.printf("\n"); sp.pretty_print( false );
                break;

              case ( 'r' ):  // Pr:  Print only off-nominal retained
                Serial.printf("\n"); sp.pretty_print( false );
                break;

              case ( 's' ):  // Ps:  Print sim
                Serial.printf("\nmodeling=%d\n", sp.Modeling());
                Serial.printf("S:");  Sen->Sim->pretty_print();
                Serial.printf("S::"); Sen->Sim->Coulombs::pretty_print();
                break;

              case ( 'V' ):  // PV:  Print all volatile
                Serial.printf("\n"); ap.pretty_print(true);
                Serial.printf("\n"); cp.pretty_print();
                Serial.printf("\n"); ap.pretty_print(false);
                break;

              case ( 'v' ):  // Pv:  Print only off-nominal volatile
                Serial.printf("\n"); ap.pretty_print(false);
                break;

              case ( 'x' ):  // Px:  Print shunt measure
                Serial.printf("\nAmp: "); Serial.printf("Vshunt_int,Vshunt,Vc,Vo,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
                  Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), Sen->ShuntAmp->Vc(), Sen->ShuntAmp->Vo(), Sen->ShuntAmp->Ishunt_cal());
                Serial.printf("Noa:"); Serial.printf("Vshunt_int,Vshunt,Vc,Vo,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
                  Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), Sen->ShuntNoAmp->Vc(), Sen->ShuntNoAmp->Vo(), Sen->ShuntNoAmp->Ishunt_cal());
                Serial.printf("Sel:Noa,Ib=,%d,%7.3f\n", sp.Ib_select(), Sen->Ib);
                break;

              default:
                Serial.printf("\n");Serial.print(cp.input_str.charAt(1)); Serial.printf("? 'h'\n");
            }
            break;

          case ( 'Q' ):  // Q:  quick critical
            debug_q(Mon, Sen);
            break;

          case ( 'R' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'b' ):  // Rb:  Reset battery states (also hys)
                Sen->Sim->init_battery_sim(true, Sen);  // Reset sim battery state
                Mon->init_battery_mon(true, Sen);       // Reset mon battery state
                break;

              case ( 'f' ):  // Rf:  Reset fault Rf
                Serial.printf("Reset latches\n");
                Sen->Flt->reset_all_faults(true);
                break;

              case ( 'i' ):  // Ri:  Reset infinite counter
                Serial.printf("Reset infinite counter\n");
                cp.inf_reset = true;
                break;

              case ( 'r' ):  // Rr:  small reset counters
                Serial.printf("CC reset\n");
                Sen->Sim->apply_soc(1.0, Sen->Tb_filt);
                Mon->apply_soc(1.0, Sen->Tb_filt);
                cp.cmd_reset();
                break;

              case ( 'R' ):  // RR:  large reset
                Serial.printf("RESET\n");
                Serial1.printf("RESET\n");
                Sen->Sim->apply_soc(1.0, Sen->Tb_filt);
                Mon->apply_soc(1.0, Sen->Tb_filt);
                cp.cmd_reset();
                Sen->ReadSensors->delay(READ_DELAY);
                sp.large_reset();
                sp.large_reset();
                cp.large_reset();
                cp.cmd_reset();
                chit("HR;", SOON);
                chit("Rf;", SOON);
                chit("Hs;", SOON);
                chit("Pf;", SOON);
                break;

              case ( 's' ):  // Rs:  small reset filters
                Serial.printf("reset\n");
                cp.cmd_reset();
                break;

              case ( 'S' ):  // RS: renominalize saved pars
                sp.set_nominal();
                sp.pretty_print(true);
                break;

              case ( 'V' ):  // RV: renominalize volatile pars
                ap.set_nominal();
                ap.pretty_print(true);
                break;


              default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" unk. 'h'\n");
            }
            break;

          case ( 's' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'i' ):  //* si:  selection battery  mode
                sp.Ib_select_p->print_adj_print(cp.input_str.substring(1).toInt());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" unk. 'h'\n");
            }
            break;

          case ( 'U' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'T' ):  //*  UT<>:  Unix time since epoch
              sp.Time_now_p->print_adj_print((unsigned long)cp.input_str.substring(2).toInt());
              break;

            default:
                Serial.print(cp.input_str.charAt(1)); Serial.printf(" unk. 'h'\n");
            }
            break;

          case ( 'v' ):  //*  v<>:  verbose level
            sp.Debug_p->print_adj_print(cp.input_str.substring(1).toInt());
            break;

          case ( 'W' ):  // W<>:  wait.  Skip
            if ( cp.input_str.substring(1).length() )
            {
              INT_in = cp.input_str.substring(1).toInt();
              if ( INT_in > 0 )
              {
                for ( int i=0; i<INT_in; i++ )
                {
                  chit("W;", SOON);
                }
              }
            }
            else
            {
              Serial.printf("..Wait.\n");
            }
            break;

          // Photon 2 O/S waits 10 seconds between backup SRAM saves.  To save time, you can get in the habit of pressing 'w;'
          // This was not done for all passes just to save only when an adjustment change verified by user (* parameters), to avoid SRAM life impact.
          #ifdef CONFIG_PHOTON2
            case ( 'w' ):  // w:  confirm write * adjustments to to SRAM
              System.backupRamSync();
              Serial.printf("SAVED *\n"); Serial1.printf("SAVED *\n");
              break;
          #endif

          case ( 'X' ):  // X
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'd' ):  // Xd<>:  on/off dc-dc charger manual setting
                ap.dc_dc_on_p->print_adj_print(cp.input_str.substring(2).toInt()>0);
                break;

              case ( 'm' ):  // Xm<>:  code for modeling level
                INT_in =  cp.input_str.substring(2).toInt();
                reset = sp.Modeling() != INT_in;
                sp.Modeling_p->print_adj_print(INT_in);
                if ( reset )
                {
                  Serial.printf("Chg...reset\n");
                  cp.cmd_reset();
                }
                break;

              case ( 'a' ): // Xa<>:  injection amplitude
                sp.put_Amp(cp.input_str.substring(2).toFloat()*sp.nP());
                Serial.printf("Inj amp, %s, %s set%7.3f & inj_bias set%7.3f\n", sp.Amp_p->units(), sp.Amp_p->description(), sp.Amp(), sp.Inj_bias());
                break;

              case ( 'f' ): //*  Xf<>:  injection frequency
                sp.Freq_p->print_adj_print(cp.input_str.substring(2).toFloat());
                sp.Freq_p->print_adj_print(sp.Freq()*(2. * PI));
                break;

              case ( 'b' ): //*  Xb<>:  injection bias
                sp.Inj_bias_p->print_adj_print(cp.input_str.substring(2).toFloat());
                Serial.printf("Inj amp, %s, %s set%7.3f & inj_bias set%7.3f\n", sp.Amp_p->units(), sp.Amp_p->description(), sp.Amp(), sp.Inj_bias());
                break;

              case ( 'Q' ): //  XQ<>: time to quiet
                ap.until_q_p->print_adj_print((unsigned long int) cp.input_str.substring(2).toInt());
                Serial.printf("Going black in %7.1f seconds\n", float(ap.until_q) / 1000.);
                break;

              case ( 't' ): //*  Xt<>:  injection type
                switch ( cp.input_str.charAt(2) )
                {
                  case ( 'n' ):  // Xtn:  none
                    sp.put_Type(0);
                    Serial.printf("Set none. sp.type() %d\n", sp.type());
                    break;

                  case ( 's' ):  // Xts:  sine
                    sp.put_Type(1);
                    Serial.printf("Set sin. sp.type() %d\n", sp.type());
                    break;

                  case ( 'q' ):  // Xtq:  square
                    sp.put_Type(2);
                    Serial.printf("Set square. sp.type() %d\n", sp.type());
                    break;

                  case ( 't' ):  // Xtt:  triangle
                    sp.put_Type(3);
                    Serial.printf("Set tri. sp.type() %d\n", sp.type());
                    break;

                  case ( 'c' ):  // Xtc:  charge rate
                    sp.put_Type(4);
                    Serial.printf("Set 1C charge. sp.type() %d\n", sp.type());
                    break;

                  case ( 'd' ):  // Xtd:  discharge rate
                    sp.put_Type(5);
                    Serial.printf("Set 1C disch. sp.type() %d\n", sp.type());
                    break;

                  case ( 'o' ):  // Xto:  cosine
                    sp.put_Type(8);
                    Serial.printf("Set cos. sp.type() %d\n", sp.type());
                    break;

                  default:
                    Serial.print(cp.input_str.charAt(1)); Serial.print(" ? 'h'\n");
                }
                break;

              case ( 'p' ): // Xp<>:  injection program
                INT_in = cp.input_str.substring(2).toInt();
                switch ( INT_in )
                {

                  case ( -1 ):  // Xp-1:  full reset
                    chit("Xp0;", ASAP);
                    chit("Ca.5;", SOON);
                    chit("Xm0;", SOON);
                    break;

                  case ( 0 ):  // Xp0:  reset stop
                    Serial.printf("**************Xp0\n");
                    if ( !sp.tweak_test() ) chit("Xm247;", ASAP);  // Prevent upset of time in Xp9, Xp10, Xp11, etc
                    chit("Xf0;Xa0;Xtn;", ASAP);
                    if ( !sp.tweak_test() ) chit("Xb0;", ASAP);
                    chit("Mk1;Nk1;", ASAP);  // Stop any injection
                    chit("BZ;", SOON);
                    break;

                  #ifndef CONFIG_PHOTON
                    case ( 2 ):  // Xp2:  
                      chit("Xp0;", QUEUE);
                      chit("Xtc;", QUEUE);
                      chit("Di-40;", QUEUE);
                      break;

                    case ( 3 ):  // Xp3:  
                      chit("Xp0;", QUEUE);
                      chit("Xtc;", QUEUE);
                      chit("Di40;", QUEUE);
                      break;

                    case ( 4 ):  // Xp4:  
                      chit("Xp0;", QUEUE);
                      chit("Xtc;", QUEUE);
                      chit("Di-100;", QUEUE);
                      break;

                    case ( 5 ):  // Xp5:  
                      chit("Xp0;", QUEUE);
                      chit("Xtc;", QUEUE);
                      chit("Di100;", QUEUE);
                      break;

                  #endif

                  case ( 6 ):  // Xp6:  Program a pulse for EKF test
                    chit("XS;Dm0;Dn0;v0;Xm255;Ca.5;Pm;Dr100;DP20;v4;", QUEUE);  // setup
                    chit("Dn.00001;Dm500;Dm-500;Dm0;", QUEUE);  // run
                    chit("W10;Pm;v0;", QUEUE);  // finish
                    break;

                  case ( 7 ):  // Xp7:  Program a sensor pulse for State Space test
                    chit("XS;Dm0;Dn0;v0;Xm255;Ca.5;Pm;Dr100;DP1;v2;", QUEUE);  // setup
                    chit("Dn.00001;Dm500;Dm-500;Dm0;", QUEUE);  // run
                    chit("W10;Pm;v0;", QUEUE);  // finish
                    break;

                  case ( 8 ):  // Xp8:  Program a hardware pulse for State Space test
                    chit("XS;Di0;v0;Xm255;Ca.5;Pm;Dr100;DP1;v2;", QUEUE);  // setup
                    chit("DI500;DI-500;DI0;", QUEUE);  // run
                    chit("W10;Pm;v0;", QUEUE);  // finish
                    break;

                  case ( 9 ): case( 10 ): case ( 11 ): case( 12 ): case( 13 ): // Xp9: Xp10: Xp11: Xp12: Xp13:  Program regression
                    // Regression tests 9=tweak, 10=tweak w data, 11=cycle, 12 1/2 cycle
                    chit("Xp0;", QUEUE);      // Reset nominal
                    chit("v0;", QUEUE);       // Turn off debug temporarily so not snowed by data dumps
                    chit("Xm255;", QUEUE);    // Modeling (for totally digital test of logic) and tweak_test=true to disable cutback in Sim.  Leaving cutback on would mean long run times (~30:00) (May need a way to test features affected by cutback, such as tweak, saturation logic)
                    chit("Xts;", QUEUE);      // Start up a sine wave
                    chit("Ca1;", QUEUE);      // After restarting with sine running, soc will not be at 1.  Reset them all to 1
                    chit("Dm1;Dn1;", ASAP);   // Slight positive current so sat logic is functional.  ASAP so synchronized and ib_diff flat.
                    chit("DP1;", QUEUE);      // Fast data collection (may cause trouble in CoolTerm.  if so, try Dr200)
                    chit("Rb;", QUEUE);       // Reset battery states
                    // chit("Pa;", QUEUE);       // Print all for record
                    if ( INT_in == 10 )  // Xp10:  rapid tweak
                    {
                      chit("Xf.02;", QUEUE);  // Frequency 0.02 Hz
                      chit("Xa-2000;", QUEUE);// Amplitude -2000 A
                      chit("XW5000;", QUEUE);    // Wait time before starting to cycle
                      chit("XT5000;", QUEUE);    // Wait time after cycle to print
                      chit("XC3;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v4;", QUEUE);     // Data collection
                    }
                    else if ( INT_in == 11 )  // Xp11:  slow tweak
                    {
                      chit("Xf.002;", QUEUE); // Frequency 0.002 Hz
                      chit("Xa-60;", QUEUE);  // Amplitude -60 A
                      chit("XW60000;", QUEUE);   // Wait time before starting to cycle
                      chit("XT60000;", QUEUE);  // Wait time after cycle to print
                      chit("XC1;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v2;", QUEUE);     // Data collection
                    }
                    else if ( INT_in == 12 )  // Xp12:  slow half tweak
                    {
                      chit("Xf.0002;", QUEUE);  // Frequency 0.002 Hz
                      chit("Xa-6;", QUEUE);     // Amplitude -60 A
                      chit("XW60000;", QUEUE);     // Wait time before starting to cycle
                      chit("XT240000;", QUEUE);   // Wait time after cycle to print
                      chit("XC.5;", QUEUE);     // Number of injection cycles
                      chit("W2;", QUEUE);       // Wait
                      chit("v2;", QUEUE);       // Data collection
                    }
                    else if ( INT_in == 13 )  // Xp13:  tri tweak
                    {
                      chit("Xtt;", QUEUE);      // Start up a triangle wave
                      chit("Xf.02;", QUEUE);  // Frequency 0.02 Hz
                      chit("Xa-29500;", QUEUE);// Amplitude -2000 A
                      chit("XW5000;", QUEUE);    // Wait time before starting to cycle
                      chit("XT5000;", QUEUE);    // Wait time after cycle to print
                      chit("XC3;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v2;", QUEUE);     // Data collection
                    }
                    chit("W2;", QUEUE);       // Wait
                    chit("XR;", QUEUE);       // Run cycle
                    break;

                  case( 20 ): case ( 21 ):    // Xp20:  Xp21:  20= 0.5 s sample/2.0s print, 21= 2 s sample/8 s print
                    chit("v0;", QUEUE);       // Turn off debug temporarily so not snowed by data dumps
                    chit("Pa;", QUEUE);       // Print all for record
                    if ( INT_in == 20 )
                    {
                      chit("Dr500;", QUEUE);  // 5x sample time, > ChargeTransfer_T_MAX.  ChargeTransfer dynamics disabled in Photon
                      chit("DP4;", QUEUE);    // 4x data collection, > ChargeTransfer_T_MAX.  ChargeTransfer dynamics disabled in Python
                      chit("v2;", QUEUE);     // Large data set
                    }
                    else if ( INT_in == 21 )
                    {
                      chit("DP20;", QUEUE);   // 20x data collection
                      chit("v2;", QUEUE);     // Slow data collection
                    }
                    chit("Rb;", QUEUE);       // Large data set
                    break;

                  default:
                    Serial.printf("Xp=%d unk.  see 'h'\n", INT_in);
                }
                break;

              case ( 'C' ): // XC:  injection number of cycles
                ap.cycles_inj_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'R' ): // XR:  Start injection now
                if ( Sen->now>TEMP_INIT_DELAY )
                {
                  Sen->start_inj = ap.wait_inj + Sen->now;
                  Sen->stop_inj = ap.wait_inj + (Sen->now + min((unsigned long int)(ap.cycles_inj / max(sp.Freq()/(2.*PI), 1e-6) *1000.), ULLONG_MAX));
                  Sen->end_inj = Sen->stop_inj + ap.tail_inj;
                  Serial.printf("**\n*** RUN: at %ld, %7.3f cycles %ld to %ld with %ld wait and %ld tail\n\n",
                    Sen->now, ap.cycles_inj, Sen->start_inj, Sen->stop_inj, ap.wait_inj, ap.tail_inj);
                }
                else Serial.printf("Wait%5.1fs for init\n", float(TEMP_INIT_DELAY-Sen->now)/1000.);
                break;

              case ( 'S' ): // XS:  Stop injection now
                Serial.printf("STOP\n");
                Sen->start_inj = 0UL;
                Sen->stop_inj = 0UL;
                Sen->end_inj = 0UL;
                Sen->elapsed_inj = 0UL;
                chit("v0;", ASAP);     // Turn off echo
                chit("Xm247;", SOON);  // Turn off tweak_test
                chit("Xp0;", SOON);    // Reset
                break;

              case ( 's' ): // Xs:  scale T_SAT
                ap.s_t_sat_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'W' ):  // XW<>:  Wait beginning of programmed transient
                ap.wait_inj_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'T' ):  // XT<>:  Tail end of programmed transient
                ap.tail_inj_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'u' ):  // Xu<>:  Tb, fail it
                ap.fail_tb_p->print_adj_print(cp.input_str.substring(2).toInt());
                break;

              case ( 'v' ):  // Xv<>:  Tb stale time scalar
                ap.tb_stale_time_slr_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.print(" ? 'h'\n");
            }
            break;

          case ( 'h' ):  // h: help
            talkH(Mon, Sen);
            break;

          default:
            Serial.print(cp.input_str.charAt(0)); Serial.print(" ? 'h'\n");
            break;
        }
    }

    cp.input_str = "";
    cp.token = false;
  }  // if ( cp.token )
}

#if defined(CONFIG_PHOTON)
  #define HELPLESS
#else
  #undef HELPLESS
#endif

// Talk Help
void talkH(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("No help photon for test. Look at code.\n");
  Serial.printf("\n\nHelp menu.  Omit '=' and end entry with ';'\n");

  #ifndef HELPLESS
  Serial.printf("\nb<?>   Manage fault buffer\n");
  Serial.printf("  bd= "); Serial.printf("dump fault buffer\n");
  Serial.printf("  bh= "); Serial.printf("reset history buffer\n");
  Serial.printf("  br= "); Serial.printf("reset fault buffer\n");
  Serial.printf("  bR= "); Serial.printf("reset all buffers\n");

  Serial.printf("\nB<?> Battery e.g.:\n");
  sp.Mon_chm_p->print_help();  //* Bm 
  sp.Sim_chm_p->print_help();  //* Bs
  sp.nP_p->print_help();  //* BP
  sp.nS_p->print_help();  //* BS

  Serial.printf("\nBZ Benignly zero test settings\n");
  
  Serial.printf("\nc  clear talk, esp '-c;'\n");

  Serial.printf("\nC<?> Chg SOC e.g.:\n");
  Serial.printf("  Ca=  all - '(0-1.1)'\n"); 
  Serial.printf("  Cm=  model (& ekf if mod)- '(0-1.1)'\n"); 

  Serial.printf("\nD/S<?> Adj e.g.:\n");
  sp.Ib_bias_amp_p->print_help();  //* DA
  sp.Ib_bias_amp_p->print1_help();  //* DA
  sp.Ib_bias_noa_p->print_help();  //* DB
  sp.Ib_bias_noa_p->print1_help();  //* DB
  sp.Vb_bias_hdwe_p->print_help();  //* Dc
  sp.Vb_bias_hdwe_p->print1_help();  //* Dc
  ap.eframe_mult_p->print_help();  //  DE
  sp.Ib_bias_all_nan_p->print_help();  //* DI
  sp.Ib_bias_all_nan_p->print1_help();  //* DI
  sp.Ib_bias_all_p->print_help();  //* Di
  sp.Ib_bias_all_p->print1_help();  //* Di
  sp.Ib_bias_amp_p->print_help();  //  Dm
  ap.Ib_amp_noise_amp_p->print_help();  // DM
  sp.Ib_bias_noa_p->print_help();  //  Dn
  ap.Ib_noa_noise_amp_p->print_help();  // DN
  ap.print_mult_p->print_help();  //  DP
  ap.read_delay_p->print_help();  //  Dr
  ap.ds_voc_soc_p->print_help();  //  Ds
  sp.Tb_bias_hdwe_p->print_help();  //* Dt
  sp.Tb_bias_hdwe_p->print1_help();  //* Dt
  ap.Tb_noise_amp_p->print_help();  // DT
  ap.vb_add_p->print_help();  // Dv
  ap.Vb_noise_amp_p->print_help();  // DV
  sp.Dw_p->print_help();  //* Dw
  sp.Dw_p->print1_help();  //* Dw
  ap.dv_voc_soc_p->print_help();  //  Dy
  ap.Tb_bias_model_p->print_help();  // D^
  sp.Ib_scale_amp_p->print_help();  //* SA
  sp.Ib_scale_amp_p->print1_help();  //* SA
  sp.Ib_scale_noa_p->print_help();  //* SB
  sp.Ib_scale_noa_p->print1_help();  //* SB
  ap.hys_scale_p->print_help();  //  Sh
  ap.hys_state_p->print_help();  //  SH
  sp.Cutback_gain_slr_p->print_help();  //* Sk
  sp.S_cap_mon_p->print_help();  //* SQ
  sp.S_cap_mon_p->print1_help();  //* SQ
  sp.S_cap_sim_p->print_help();  //* Sq
  sp.S_cap_sim_p->print1_help();  //* Sq
  sp.Vb_scale_p->print_help();  //* SV
  sp.Vb_scale_p->print1_help();  //* SV

  Serial.printf("\nF<?>   Faults\n");
  ap.cc_diff_slr_p->print_help();  // Fc
  ap.ib_diff_slr_p->print1_help();  // Fd
  ap.fake_faults_p->print_help();  // Ff
  ap.fake_faults_p->print1_help();  // Ff
  ap.ewhi_slr_p->print_help();  // Fi
  ap.ewlo_slr_p->print_help();  // Fo
  ap.ib_quiet_slr_p->print_help();  // Fq
  ap.disab_ib_fa_p->print_help();  // FI
  ap.disab_tb_fa_p->print_help();  // FT
  ap.disab_vb_fa_p->print_help();  // FV

  Serial.printf("\nH<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump summ log\n");
  Serial.printf("  HR= "); Serial.printf("reset summ log\n");
  Serial.printf("  Hs= "); Serial.printf("save and print log\n");

  Serial.printf("\nP<?>   Print values\n");
  Serial.printf("  Pa= "); Serial.printf("all\n");
  Serial.printf("  Pb= "); Serial.printf("vb details\n");
  Serial.printf("  Pe= "); Serial.printf("ekf\n");
  Serial.printf("  Pf= "); Serial.printf("faults\n");
  Serial.printf("  Pm= "); Serial.printf("Mon\n");
  Serial.printf("  PM= "); Serial.printf("amp shunt\n");
  Serial.printf("  PN= "); Serial.printf("noa shunt\n");
  Serial.printf("  PR= "); Serial.printf("all retained adj\n");
  Serial.printf("  Pr= "); Serial.printf("off-nom ret adj\n");
  Serial.printf("  Ps= "); Serial.printf("Sim\n");
  Serial.printf("  PV= "); Serial.printf("all vol adj\n");
  Serial.printf("  Pv= "); Serial.printf("off-nom vol adj\n");
  Serial.printf("  Px= "); Serial.printf("ib select\n");

  Serial.printf("\nQ      vital stats\n");

  Serial.printf("\nR<?>   Reset\n");
  Serial.printf("  Rb= "); Serial.printf("batteries to present inputs\n");
  Serial.printf("  Rf= "); Serial.printf("fault logic latches\n");
  Serial.printf("  Ri= "); Serial.printf("infinite counter\n");
  Serial.printf("  Rr= "); Serial.printf("saturate Mon and equalize Sim & Mon\n");
  Serial.printf("  RR= "); Serial.printf("DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small.  Reinitialize filters\n");
  Serial.printf("  RS= "); Serial.printf("SavedPars: Renominalize saved\n");
  Serial.printf("  RV= "); Serial.printf("Renominalize volatile\n");

  sp.Ib_select_p->print_help();  //* si
  sp.Time_now_p->print_help();  //* UT
  sp.Time_now_p->print1_help();  //* UT
  sp.Debug_p->print_help();  // v
  sp.Debug_p->print1_help();  // v

  Serial.printf("  -<>: Negative - Arduino plot compatible\n");
  Serial.printf("  v-2: ADS counts for throughput meas\n");
  #ifdef DEBUG_INIT
    Serial.printf("  v-1: Debug\n");
  #endif
  Serial.printf("   v1: GP\n");
  Serial.printf("   v2: GP, Sim & Sel\n");
  Serial.printf("   v3: EKF\n");
  Serial.printf("   v4: GP, Sim, Sel, & EKF\n");
  Serial.printf("   v5: OLED display\n");
  #ifndef CONFIG_PHOTON
    Serial.printf("  v12: EKF\n");
    Serial.printf(" v-13: ib_dscn\n");
  #endif
  Serial.printf("  v14: vshunt and Ib raw\n");
  Serial.printf("  v15: vb raw\n");
  Serial.printf("  v16: Tb\n");
  #ifndef CONFIG_PHOTON
    Serial.printf(" v-23: Vb_hdwe_ac\n");
    Serial.printf(" v-24: Vb_hdwe_ac, Ib_hdwe\n");
    Serial.printf("  v34: EKF detail\n");
    Serial.printf("  v35: ChargeTransfer balance\n");
    Serial.printf("  v37: EKF short\n");
    Serial.printf("  v75: voc_low check mod\n");
    Serial.printf("  v76: vb model\n");
    Serial.printf("  v78: Batt model sat\n");
    Serial.printf("  v79: sat_ib model\n");
  #endif
  Serial.printf("  v99: calibration\n");

  Serial.printf("\nW<?> - iters to wait\n");

  #ifdef CONFIG_PHOTON2
    Serial.printf("\nw - save * confirm adjustments to SRAM\n");
  #endif

  Serial.printf("\nX<?> - Test Mode.   For example:\n");
  ap.dc_dc_on_p->print_help();  // Xd
  ap.until_q_p->print_help();  // XQ
  sp.Modeling_p->print_help();  //* Xm
  sp.pretty_print_modeling();

  #endif

  sp.Amp_p->print_help();  //* Xa
  sp.Freq_p->print_help();  //* Xf
  sp.Type_p->print_help();  //* Xt

  #ifndef HELPLESS
  Serial.printf(" Xp= <?>, scripted tests...\n"); 
  Serial.printf("  Xp-1: Off, modeling false\n");
  Serial.printf("  Xp0: reset tests\n");
  #ifndef CONFIG_PHOTON
    Serial.printf("  Xp2: -0.4C soft disch, reset xp0 or Di0\n");
    Serial.printf("  Xp3: +0.4C soft chg\n");
    Serial.printf("  Xp4: -1C soft disch, reset xp0 or Di0\n");
    Serial.printf("  Xp5: +1C soft chg\n");
  #endif
  Serial.printf("  Xp6: +/-500 A pulse EKF\n");
  Serial.printf("  Xp7: +/-500 A sw pulse SS\n");
  Serial.printf("  Xp8: +/-500 A hw pulse SS\n");
  Serial.printf("  Xp10:tweak sin\n");
  Serial.printf("  Xp11:slow sin\n");
  Serial.printf("  Xp12:slow half sin\n");
  Serial.printf("  Xp13:tweak tri\n");
  Serial.printf("  Xp20:collect fast\n");
  Serial.printf("  Xp21:collect slow\n");
  ap.cycles_inj_p->print_help();  // XC
  Serial.printf(" XR  "); Serial.printf("RUN inj\n");
  Serial.printf(" XS  "); Serial.printf("STOP inj\n");
  ap.s_t_sat_p->print_help();  // Xs
  ap.tail_inj_p->print_help();  // XT
  ap.wait_inj_p->print_help();  // XW
  ap.fail_tb_p->print_help();  // Xu
  ap.tb_stale_time_slr_p->print_help();  // Xv
  // sp.testB_p->print_help();  // XB
  // sp.testD_p->print_help();  // XD
  Serial.printf("\nurgency of cmds: -=ASAP,*=SOON, '' or +=QUEUE, <=LAST\n");
  #endif
}
