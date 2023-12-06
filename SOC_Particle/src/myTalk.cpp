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

extern CommandPars cp;            // Various parameters shared at system level
extern SavedPars sp;              // Various parameters to be static at system level and saved through power cycle
extern Flt_st mySum[NSUM];        // Summaries for saving charge history

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
  chit("Pf", ASAP);  // Pf

  // Model
  Sen->Sim->hys_scale(1);  // Sh 1
  Sen->Sim->Sr(1);  // Sr
  Mon->Sr(1);  // Sr 1
  sp.Cutback_gain_sclr_p->print_adj_print(1); // Sk 1
  Sen->Sim->hys_state(0);  // SH 0
  Sen->Flt->wrap_err_filt_state(0);  // SH 0

  // Injection
  Sen->ib_amp_add(0);  // Dm 0
  Sen->ib_noa_add(0);  // Dn 0
  Sen->vb_add(0);  // Dv 0
  Sen->Sim->ds_voc_soc(0);  // Ds
  cp.Tb_bias_model = 0;  // D^
  Sen->Sim->Dv(0);  // Dy
  Sen->Flt->tb_stale_time_sclr(1);  // Xv 1
  Sen->Flt->fail_tb(0);  // Xu 0

  // Noise
  Sen->Tb_noise_amp(0);  // DT 0
  Sen->Vb_noise_amp(0);  // DV 0
  Sen->Ib_amp_noise_amp(0);  // DM 0
  Sen->Ib_noa_noise_amp(0);  // DN 0

  // Intervals
  cp.assign_eframe_mult(max(min(EKF_EFRAME_MULT, UINT8_MAX), 0)); // DE
  cp.assign_print_mult(max(min(DP_MULT, UINT8_MAX), 0));  // DP
  Sen->ReadSensors->delay(READ_DELAY);

  // Fault logic
  Sen->Flt->cc_diff_sclr(1);  // Fc 1
  Sen->Flt->ib_diff_sclr(1);  // Fd 1
  cp.fake_faults = 0;  // Ff 0
  sp.put_Ib_select(0);  // Ff 0
  Sen->Flt->ewhi_sclr(1);  // Fi 1
  Sen->Flt->ewlo_sclr(1);  // Fo 1
  Sen->Flt->ib_quiet_sclr(1);  // Fq 1
  Sen->Flt->disab_ib_fa(0);  // FI 0
  Sen->Flt->disab_tb_fa(0);  // FT 0
  Sen->Flt->disab_vb_fa(0);  // FV 0

}

// Talk Executive
void talk(BatteryMonitor *Mon, Sensors *Sen)
{
  float FP_in = -99.;
  int INT_in = -1;
  float scale = 1.;
  boolean reset = false;
  urgency request;

  // Serial event
  if (cp.token)
  {
    // Categorize the requests
    char key = cp.input_str.charAt(0);
    if ( key == 'c' || (key == '-' && cp.input_str.charAt(1)!= 'c') )
    {
      Serial.printf("***CLEAR QUEUES\n");
      clear_queues();
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
              case ( 'm' ):  //* Bm:  Monitor chemistry change
                INT_in = cp.input_str.substring(2).toInt();
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
                Serial.printf("Eframe mult %d to ", cp.eframe_mult);
                cp.assign_eframe_mult(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.eframe_mult);
                break;

              case ( 'i' ):  //*  Di<>:  Bias all current sensors (same way as Da and Db)
                chit("DI;", ASAP);  // Occur before reset
                cp.cmd_reset();
                break;

              case ( 'I' ):  //*  DI<>:  Bias all current sensors without reset (same way as Da and Db)
                sp.Ib_bias_all_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'm' ):  //   Dm<>:  Amp signal adder for faults
                Serial.printf("ib_amp_add%7.3f to", Sen->ib_amp_add());
                Sen->ib_amp_add(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ib_amp_add());
                break;

              case ( 'n' ):  //   Dn<>:  No Amp signal adder for faults
                Serial.printf("ib_noa_add%7.3f to", Sen->ib_noa_add());
                Sen->ib_noa_add(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ib_noa_add());
                break;

              case ( 'P' ):  //   DP<>:  PRINT multiplier
                Serial.printf("Print int %d to ", cp.print_mult);
                cp.assign_print_mult(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.print_mult);
                break;

              case ( 'r' ):  //   Dr<>:  READ sample time input
                Serial.printf("ReadSensors %ld to ", Sen->ReadSensors->delay());
                Sen->ReadSensors->delay(cp.input_str.substring(2).toInt());
                Serial.printf("%ld\n", Sen->ReadSensors->delay());
                break;

              case ( 's' ):  //   Ds<>:  d_soc to Sim.voc_soc
                Serial.printf("Sim d_soc %7.3f to ", Sen->Sim->ds_voc_soc());
                Sen->Sim->ds_voc_soc(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Sim->ds_voc_soc());
                break;

              case ( 't' ):  //*  Dt<>:  Temp bias change hardware
                sp.Tb_bias_hdwe_p->print_adj_print(cp.input_str.substring(2).toFloat());
                cp.cmd_reset();
                break;

              case ( '^' ):  //   D^<>:  Temp bias change model for faults
                Serial.printf("cp.Tb_bias_model%7.3f to", cp.Tb_bias_model);
                cp.Tb_bias_model = cp.input_str.substring(2).toFloat();
                Serial.printf("%7.3f\n", cp.Tb_bias_model);
                break;

              case ( 'v' ):  //     Dv<>:  voltage signal adder for faults
                Serial.printf("*Sen->vb_add%7.3f to", Sen->vb_add());
                Serial1.printf("*Sen->vb_add%7.3f to", Sen->vb_add());
                Sen->vb_add(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->vb_add());
                Serial1.printf("%7.3f\n", Sen->vb_add());
                break;

              case ( 'w' ):  //   * Dw<>:
                sp.Dw_p->print_adj_print(cp.input_str.substring(2).toFloat());
                break;

              case ( 'y' ):  //   Dy<>:
                Serial.printf("Sim.Dv%7.3f to", Sen->Sim->Dv());
                Sen->Sim->Dv(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Sim->Dv());
                break;

              case ( 'T' ):  //   DT<>:  Tb noise
                Serial.printf("Sen.Tb_noise_amp_%7.3f to ", Sen->Tb_noise_amp());
                Sen->Tb_noise_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Tb_noise_amp());
                break;

              case ( 'V' ):  //   DV<>:  Vb noise
                Serial.printf("Sen.Vb_noise_amp_%7.3f to ", Sen->Vb_noise_amp());
                Sen->Vb_noise_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Vb_noise_amp());
                break;

              case ( 'M' ):  //   DM<>:  Ib amp noise
                Serial.printf("Sen.Ib_amp_noise_amp_%7.3f to ", Sen->Ib_amp_noise_amp());
                Sen->Ib_amp_noise_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Ib_amp_noise_amp());
                break;

              case ( 'N' ):  //   DN<>:  Ib noa noise
                Serial.printf("Sen.Ib_noa_noise_amp_%7.3f to ", Sen->Ib_noa_noise_amp());
                Sen->Ib_noa_noise_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Ib_noa_noise_amp());
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
                Serial.printf("Sen->Sim->hys_sale%7.3f to ", Sen->Sim->hys_scale());
                scale = cp.input_str.substring(2).toFloat();
                Sen->Sim->hys_scale(scale);
                Serial.printf("%7.3f\n", Sen->Sim->hys_scale());
                break;

              case ( 'H' ):  //   SH<>: state of all hysteresis
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("\nSim::Hys::dv_hys%7.3f\n", Sen->Sim->hys_state());
                Sen->Sim->hys_state(scale);
                Sen->Flt->wrap_err_filt_state(-scale);
                Serial.printf("to%7.3f\n", Sen->Sim->hys_state());
                break;

              case ( 'm' ):  //   Sm<>:  Amp signal scalar for faults
                Serial.printf("ShuntAmp.sclr%7.3f to", Sen->ib_amp_sclr());
                Sen->ib_amp_sclr(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ib_amp_sclr());
                break;

              case ( 'n' ):  //   Sn<>:  No Amp signal scalar for faults
                Serial.printf("ShuntNoAmp.sclr%7.3f to", Sen->ib_noa_sclr());
                Sen->ib_noa_sclr(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ib_noa_sclr());
                break;

              case ( 'q' ):  //*  Sq<>: scale capacity sim
                scale = cp.input_str.substring(2).toFloat();
                sp.S_cap_sim_p->print_adj_print(scale);
            
                Serial.printf("Sim.q_cap_rated%7.3f %7.3f to ", scale, Sen->Sim->q_cap_rated_scaled());
                Serial1.printf("Sim.q_cap_rated%7.3f %7.3f to ", scale, Sen->Sim->q_cap_rated_scaled());
            
                Sen->Sim->apply_cap_scale(sp.S_cap_sim());
                if ( sp.Modeling() ) Mon->init_soc_ekf(Sen->Sim->soc());
            
                Serial.printf("%7.3f\n", Sen->Sim->q_cap_rated_scaled());
                Serial.printf("Sim:"); Sen->Sim->pretty_print(); Sen->Sim->Coulombs::pretty_print();
                Serial1.printf("%7.3f\n", Sen->Sim->q_cap_rated_scaled());
                Serial1.printf("Sim:"); Sen->Sim->pretty_print(); Sen->Sim->Coulombs::pretty_print();
                break;
            
              case ( 'Q' ):  //*  SQ<>: scale capacity mon
                scale = cp.input_str.substring(2).toFloat();
                sp.S_cap_mon_p->print_adj_print(scale);
            
                Mon->apply_cap_scale(sp.S_cap_mon());
                Serial.printf("%7.3f\n", Mon->q_cap_rated_scaled());
                Serial.printf("Mon:"); Mon->pretty_print(Sen); Mon->Coulombs::pretty_print();
                break;
            
              case ( 'r' ):  //   Sr<>:  scalar resistor
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("\nSim resistance scale Sr%7.3f to", Sen->Sim->Sr());
                Sen->Sim->Sr(scale);
                Serial.printf("%7.3f\n", Sen->Sim->Sr());
                Serial.printf("\nMon resistance scale Sr%7.3f to", Mon->Sr());
                Mon->Sr(scale);
                Serial.printf("%7.3f\n", Mon->Sr());
                break;
            
              case ( 'k' ):  //*  Sk<>:  scale cutback gain for sim rep of BMS
                sp.Cutback_gain_sclr_p->print_adj_print(cp.input_str.substring(2).toFloat());
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
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("cc_diff scl%7.3f to", Sen->Flt->cc_diff_sclr());
                Sen->Flt->cc_diff_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->cc_diff_sclr());
                break;

              case ( 'd' ):  //   Fd<>: scale ib_diff threshold
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("ib_diff scl%7.3f to", Sen->Flt->ib_diff_sclr());
                Sen->Flt->ib_diff_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ib_diff_sclr());
                break;

              case ( 'f' ):  //* si, Ff<>:  fake faults
                INT_in = cp.input_str.substring(2).toInt();
                Serial.printf("cp.fake_faults, sp.ib_select %d, %d to ", cp.fake_faults, sp.Ib_select());
                cp.fake_faults = INT_in;
                sp.put_Ib_select(INT_in);
                Serial.printf("%d, %d\n", cp.fake_faults, sp.Ib_select());
                break;

              case ( 'I' ):  //   FI<>:  Fault disable ib hard
                INT_in = cp.input_str.substring(2).toInt();
                Serial.printf("Sen->Flt->disab_ib_fa %d to ", Sen->Flt->disab_ib_fa());
                Sen->Flt->disab_ib_fa(INT_in);
                Serial.printf("%d\n", Sen->Flt->disab_ib_fa());
                break;

              case ( 'i' ):  //   Fi<>: scale e_wrap_hi threshold
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("e_wrap_hi scl%7.3f to", Sen->Flt->ewhi_sclr());
                Sen->Flt->ewhi_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ewhi_sclr());
                break;

              case ( 'o' ):  //   Fo<>: scale e_wrap_lo threshold
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("e_wrap_lo scl%7.3f to", Sen->Flt->ewlo_sclr());
                Sen->Flt->ewlo_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ewlo_sclr());
                break;

              case ( 'q' ):  //   Fq<>: scale ib_quiet threshold
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("ib_quiet scl%7.3f to", Sen->Flt->ib_quiet_sclr());
                Sen->Flt->ib_quiet_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ib_quiet_sclr());
                break;

              case ( 'T' ):  //   FT<>:  Fault disable tb stale
                INT_in = cp.input_str.substring(2).toInt();
                Serial.printf("Sen->Flt->disab_tb_fa %d to ", Sen->Flt->disab_tb_fa());
                Sen->Flt->disab_tb_fa(INT_in);
                Serial.printf("%d\n", Sen->Flt->disab_tb_fa());
                break;

              case ( 'V' ):  //   FV<>:  Fault disable vb hard
                INT_in = cp.input_str.substring(2).toInt();
                Serial.printf("Sen->Flt->disab_vb_fa %d to ", Sen->Flt->disab_vb_fa());
                Sen->Flt->disab_vb_fa(INT_in);
                Serial.printf("%d\n", Sen->Flt->disab_vb_fa());
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

              case ( 'r' ):  // Pr:  Print retained
                Serial.printf("\n"); sp.pretty_print( true );
                Serial.printf("\n"); cp.pretty_print();
                break;

              case ( 's' ):  // Ps:  Print sim
                Serial.printf("\nmodeling=%d\n", sp.Modeling());
                Serial.printf("S:");  Sen->Sim->pretty_print();
                Serial.printf("S::"); Sen->Sim->Coulombs::pretty_print();
                break;

              case ( 'x' ):  // Px:  Print shunt measure
                Serial.printf("\nAmp: "); Serial.printf("Vshunt_int,Vshunt,Vc,Vo,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
                  Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), Sen->ShuntAmp->Vc(), Sen->ShuntAmp->Vo(), Sen->ShuntAmp->Ishunt_cal());
                Serial.printf("Noa:"); Serial.printf("Vshunt_int,Vshunt,Vc,Vo,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
                  Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), Sen->ShuntNoAmp->Vc(), Sen->ShuntNoAmp->Vo(), Sen->ShuntNoAmp->Ishunt_cal());
                Serial.printf("Sel:Noa,Ib=,%d,%7.3f\n", sp.Ib_select(), Sen->Ib);
                break;

              case ( 'v' ):  // Pv:  Print Vb measure
                Serial.printf("\nVolt:");   Serial.printf("Vb_bias_hdwe,Vb_m,mod,Vb=,%7.3f,%7.3f,%d,%7.3f,\n", 
                  sp.Vb_bias_hdwe(), Sen->Vb_model, sp.Modeling(), Sen->Vb);
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
                cp.large_reset();
                cp.cmd_reset();
                chit("HR;", SOON);
                chit("Rf;", SOON);
                // chit("W3;", SOON);
                chit("Hs;", SOON);
                chit("Pf;", SOON);
                break;

              case ( 's' ):  // Rs:  small reset filters
                Serial.printf("reset\n");
                cp.cmd_reset();
                break;

              case ( 'S' ):  // RS: reset saved pars
                sp.reset_pars();
                sp.pretty_print(true);
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

          case ( 'V' ):
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'm' ):  //   Vm<>:  delta Mon curve soc in
                Serial.printf("Mon ds_voc_soc%7.3f to", Mon->ds_voc_soc());
                Mon->ds_voc_soc(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Mon->ds_voc_soc());
                break;

              case ( 's' ):  //   Vs<>:  delta Sim curve soc in
                Serial.printf("Sim ds_voc_soc%7.3f to", Sen->Sim->ds_voc_soc());
                Sen->Sim->ds_voc_soc(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Sim->ds_voc_soc());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.print(" ? 'h'\n");
            }
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
                if ( cp.input_str.substring(2).toInt()>0 )
                  cp.dc_dc_on = true;
                else
                  cp.dc_dc_on = false;
                Serial.printf("dc_dc_on to %d\n", cp.dc_dc_on);
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
                Sen->until_q = (unsigned long int) cp.input_str.substring(2).toInt();
                Serial.printf("Going black in %7.1f seconds\n", float(Sen->until_q) / 1000.);
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
                      chit("XW5;", QUEUE);    // Wait time before starting to cycle
                      chit("XT5;", QUEUE);    // Wait time after cycle to print
                      chit("XC3;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v4;", QUEUE);     // Data collection
                    }
                    else if ( INT_in == 11 )  // Xp11:  slow tweak
                    {
                      chit("Xf.002;", QUEUE); // Frequency 0.002 Hz
                      chit("Xa-60;", QUEUE);  // Amplitude -60 A
                      chit("XW60;", QUEUE);   // Wait time before starting to cycle
                      chit("XT60;", QUEUE);  // Wait time after cycle to print
                      chit("XC1;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v2;", QUEUE);     // Data collection
                    }
                    else if ( INT_in == 12 )  // Xp12:  slow half tweak
                    {
                      chit("Xf.0002;", QUEUE);  // Frequency 0.002 Hz
                      chit("Xa-6;", QUEUE);     // Amplitude -60 A
                      chit("XW60;", QUEUE);     // Wait time before starting to cycle
                      chit("XT2400;", QUEUE);   // Wait time after cycle to print
                      chit("XC.5;", QUEUE);     // Number of injection cycles
                      chit("W2;", QUEUE);       // Wait
                      chit("v2;", QUEUE);       // Data collection
                    }
                    else if ( INT_in == 13 )  // Xp13:  tri tweak
                    {
                      chit("Xtt;", QUEUE);      // Start up a triangle wave
                      chit("Xf.02;", QUEUE);  // Frequency 0.02 Hz
                      chit("Xa-29500;", QUEUE);// Amplitude -2000 A
                      chit("XW5;", QUEUE);    // Wait time before starting to cycle
                      chit("XT5;", QUEUE);    // Wait time after cycle to print
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
                Sen->cycles_inj = max(min(cp.input_str.substring(2).toFloat(), 10000.), 0);
                Serial.printf("#inj cycles to%7.3f\n", Sen->cycles_inj);
                break;

              case ( 'R' ): // XR:  Start injection now
                if ( Sen->now>TEMP_INIT_DELAY )
                {
                  Sen->start_inj = Sen->wait_inj + Sen->now;
                  Sen->stop_inj = Sen->wait_inj + (Sen->now + min((unsigned long int)(Sen->cycles_inj / max(sp.Freq()/(2.*PI), 1e-6) *1000.), ULLONG_MAX));
                  Sen->end_inj = Sen->stop_inj + Sen->tail_inj;
                  Serial.printf("**\n*** RUN: at %ld, %7.3f cycles %ld to %ld with %ld wait and %ld tail\n\n",
                    Sen->now, Sen->cycles_inj, Sen->start_inj, Sen->stop_inj, Sen->wait_inj, Sen->tail_inj);
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
                chit("Pa;", ASAP);     // Print all for record.  Last so Pf last and visible
                chit("Xm247;", SOON);  // Turn off tweak_test
                chit("Xp0;", SOON);    // Reset
                break;

              case ( 's' ): // Xs:  scale T_SAT
                FP_in = cp.input_str.substring(2).toFloat();
                Serial.printf("s_t_sat%7.1f s to ", cp.s_t_sat);
                cp.s_t_sat = max(FP_in, 0.);
                Serial.printf("%7.1f\n", cp.s_t_sat);
                break;

              case ( 'W' ):  // XW<>:  Wait beginning of programmed transient
                FP_in = cp.input_str.substring(2).toFloat();
                Sen->wait_inj = (unsigned long int)(max(FP_in, 0.))*1000;
                Serial.printf("Wait%7.1f s to inj\n", FP_in);
                break;

              case ( 'T' ):  // XT<>:  Tail
                FP_in = cp.input_str.substring(2).toFloat();
                Sen->tail_inj = (unsigned long int)(max(FP_in, 0.))*1000;
                Serial.printf("Wait%7.1f s tail after\n", FP_in);
                break;

              case ( 'u' ):  // Xu<>:  Tb, fail it
                INT_in = cp.input_str.substring(2).toInt();
                Serial.printf("Fail tb %d to ", Sen->Flt->fail_tb());
                Sen->Flt->fail_tb(INT_in);
                Serial.printf("%d\n", Sen->Flt->fail_tb());
                break;

              case ( 'v' ):  // Xv<>:  Tb stale time scalar
                FP_in = cp.input_str.substring(2).toFloat();
                Serial.printf("Stale tb time sclr%9.4f to", Sen->Flt->tb_stale_time_sclr());
                Sen->Flt->tb_stale_time_sclr(FP_in);
                Serial.printf("%9.4f\n", Sen->Flt->tb_stale_time_sclr());
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

  Serial.printf("\nb<?>   Manage fault buffer\n");
  Serial.printf("  bd= "); Serial.printf("dump fault buffer\n");
  Serial.printf("  bh= "); Serial.printf("reset history buffer\n");
  Serial.printf("  br= "); Serial.printf("reset fault buffer\n");
  Serial.printf("  bR= "); Serial.printf("reset all buffers\n");

  #ifndef HELPLESS
  Serial.printf("\nB<?> Battery e.g.:\n");
  Serial.printf(" *Bm=  %d.  Mon chem 0='BB', 1='CH' [%d]\n", sp.Mon_chm(), MON_CHEM); 
  Serial.printf(" *Bs=  %d.  Sim chem 0='BB', 1='CH' [%d]\n", sp.Sim_chm(), SIM_CHEM); 
  Serial.printf(" *BP=  %4.2f.  parallel in bank [%4.2f]'\n", sp.nP(), NP);
  Serial.printf(" *BS=  %4.2f.  series in bank [%4.2f]'\n", sp.nS(), NS);

  Serial.printf("\nBZ Benignly zero test settings\n");
  
  Serial.printf("\nc  clear talk, esp '-c;'\n");

  Serial.printf("\nC<?> Chg SOC e.g.:\n");
  Serial.printf("  Ca=  all - '(0-1.1)'\n"); 
  Serial.printf("  Cm=  model (& ekf if mod)- '(0-1.1)'\n"); 
  #endif

  Serial.printf("\nD/S<?> Adj e.g.:\n");
  sp.Ib_bias_amp_p->print_help();  //* DA
  sp.Ib_bias_amp_p->print1_help();  //* DA
  sp.Ib_bias_noa_p->print_help();  //* DB
  sp.Ib_bias_noa_p->print1_help();  //* DB
  sp.Ib_bias_all_p->print_help();  //* Di
  sp.Ib_bias_all_p->print1_help();  //* Di
  sp.Ib_bias_all_nan_p->print_help();  //* DI
  sp.Ib_bias_all_nan_p->print1_help();  //* DI
  sp.Vb_bias_hdwe_p->print_help();  //* Dc
  sp.Vb_bias_hdwe_p->print1_help();  //* Dc

  #ifndef HELPLESS
  Serial.printf("  DE= "); Serial.printf("%d", cp.eframe_mult); Serial.printf(": eframe mult Dr [20]\n");
  Serial.printf("  DP=  "); Serial.print(cp.print_mult); Serial.printf(": print mult Dr [4]\n"); 
  Serial.printf("  Dr=  "); Serial.print(Sen->ReadSensors->delay()); Serial.printf(": minor frame, ms [100]\n"); 
  Serial.printf("  Ds=  "); Serial.print(Sen->Sim->ds_voc_soc()); Serial.printf(": d_soc to Sim.voc-soc, fraction [0]\n");
  #endif

  sp.Tb_bias_hdwe_p->print_help();  //* Dt
  sp.Tb_bias_hdwe_p->print1_help();  //* Dt
  
  #ifndef HELPLESS
  Serial.printf("  D^= "); Serial.printf("%6.3f", cp.Tb_bias_model); Serial.printf(": del model, deg C [%6.3f]\n", TEMP_BIAS); 
  Serial.printf("  Dv=  "); Serial.print(Sen->vb_add()); Serial.printf(": volt fault inj, V [0]\n"); 
  #endif

  sp.Dw_p->print_help();  //* Dw
  sp.Dw_p->print1_help();  //* Dw

  #ifndef HELPLESS
  Serial.printf("  Dy=  "); Serial.print(Sen->Sim->Dv()); Serial.printf(": Tab sim adj, V [0]\n"); 
  Serial.printf("  DT= "); Serial.printf("%6.3f", Sen->Tb_noise_amp()); Serial.printf(": noise, deg C pk-pk [%6.3f]\n", TB_NOISE); 
  Serial.printf("  DV= "); Serial.printf("%6.3f", Sen->Vb_noise_amp()); Serial.printf(": noise, V pk-pk [%6.3f]\n", VB_NOISE); 
  Serial.printf("  DM= "); Serial.printf("%6.3f", Sen->Ib_amp_noise_amp()); Serial.printf(": amp noise, A pk-pk [%6.3f]\n", IB_AMP_NOISE); 
  Serial.printf("  DN= "); Serial.printf("%6.3f", Sen->Ib_noa_noise_amp()); Serial.printf(": noa noise, A pk-pk [%6.3f]\n", IB_NOA_NOISE); 
  #endif

  sp.Ib_scale_amp_p->print_help();  //* SA
  sp.Ib_scale_amp_p->print1_help();  //* SA
  sp.Ib_scale_noa_p->print_help();  //* SB
  sp.Ib_scale_noa_p->print1_help();  //* SB
  
  #ifndef HELPLESS
  Serial.printf("  Sh= "); Serial.printf("%6.3f", Sen->Sim->hys_scale()); Serial.printf(": hys sclr [%5.2f]\n", HYS_SCALE);
  Serial.printf("  SH= "); Serial.printf("%6.3f", Sen->Sim->hys_state()); Serial.printf(": hys states [0]\n");
  #endif

  sp.S_cap_mon_p->print_help();  //* SQ
  sp.S_cap_mon_p->print1_help();  //* SQ
  sp.S_cap_sim_p->print_help();  //* Sq
  sp.S_cap_sim_p->print1_help();  //* Sq
  sp.Cutback_gain_sclr_p->print_help();  //* Sk
  sp.Vb_scale_p->print_help();  //* SV
  sp.Vb_scale_p->print1_help();  //* SV

  Serial.printf("\nF<?>   Faults\n");
  #ifndef HELPLESS
  Serial.printf("  Fc= "); Serial.printf("%6.3f", Sen->Flt->cc_diff_sclr()); Serial.printf(": sclr cc_diff thr ^ [1]\n"); 
  Serial.printf("  Fd= "); Serial.printf("%6.3f", Sen->Flt->ib_diff_sclr()); Serial.printf(": sclr ib_diff thr ^ [1]\n"); 
  Serial.printf("  Ff= "); Serial.printf("%d", cp.fake_faults); Serial.printf(": faults faked (ignored)[%d]\n", FAKE_FAULTS); 
  Serial.printf("  Fi= "); Serial.printf("%6.3f", Sen->Flt->ewhi_sclr()); Serial.printf(": sclr e_wrap_hi thr ^ [1]\n"); 
  Serial.printf("  Fo= "); Serial.printf("%6.3f", Sen->Flt->ewlo_sclr()); Serial.printf(": sclr e_wrap_lo thr ^ [1]\n"); 
  Serial.printf("  Fq= "); Serial.printf("%6.3f", Sen->Flt->ib_quiet_sclr()); Serial.printf(": sclr ib_quiet thr v [1]\n"); 
  Serial.printf("  FI=  "); Serial.print(Sen->Flt->disab_ib_fa()); Serial.printf(": disab Ib rng\n");
  Serial.printf("  FT=  "); Serial.print(Sen->Flt->disab_tb_fa()); Serial.printf(": disab Tb rng\n");
  Serial.printf("  FV=  "); Serial.print(Sen->Flt->disab_vb_fa()); Serial.printf(": disab Vb rng\n");

  Serial.printf("\nH<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump summ log\n");
  Serial.printf("  HR= "); Serial.printf("reset summ log\n");
  Serial.printf("  Hs= "); Serial.printf("save and print log\n");

  Serial.printf("\nP<?>   Print values\n");
  Serial.printf("  Pa= "); Serial.printf("all\n");
  Serial.printf("  Pe= "); Serial.printf("ekf\n");
  Serial.printf("  Pf= "); Serial.printf("faults\n");
  Serial.printf("  Pm= "); Serial.printf("Mon\n");
  Serial.printf("  PM= "); Serial.printf("amp shunt\n");
  Serial.printf("  PN= "); Serial.printf("noa shunt\n");
  Serial.printf("  Pr= "); Serial.printf("retained and command\n");
  Serial.printf("  Ps= "); Serial.printf("Sim\n");
  Serial.printf("  Px= "); Serial.printf("ib select\n");
  Serial.printf("  Pv= "); Serial.printf("vb details\n");

  Serial.printf("\nQ      vital stats\n");

  Serial.printf("\nR<?>   Reset\n");
  Serial.printf("  Rb= "); Serial.printf("batteries to present inputs\n");
  Serial.printf("  Rf= "); Serial.printf("fault logic latches\n");
  Serial.printf("  Ri= "); Serial.printf("infinite counter\n");
  Serial.printf("  Rr= "); Serial.printf("saturate Mon and equalize Sim & Mon\n");
  Serial.printf("  RR= "); Serial.printf("DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small.  Reinitialize filters\n");
  Serial.printf("  RS= "); Serial.printf("SavedPars: Reinitialize saved\n");
  #endif

  sp.Ib_select_p->print_help();  //* si
  sp.Time_now_p->print_help();  //* UT
  sp.Time_now_p->print1_help();  //* UT
  sp.Debug_p->print_help();  // v
  sp.Debug_p->print1_help();  // v

  #ifndef HELPLESS
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

  Serial.printf("\nV<?> - VOC(SOC) curve deltas\n");
  Serial.printf(" Vm= "); Serial.printf("%6.3f", Mon->ds_voc_soc()); Serial.printf(": Mon soc in [0]\n"); 
  Serial.printf(" Vs= "); Serial.printf("%6.3f", Sen->Sim->ds_voc_soc()); Serial.printf(": Sim soc in[0]\n"); 

  Serial.printf("\nW<?> - iters to wait\n");

  #ifdef CONFIG_PHOTON2
    Serial.printf("\nw - save * confirm adjustments to SRAM\n");
  #endif

  Serial.printf("\nX<?> - Test Mode.   For example:\n");
  Serial.printf(" Xd=  "); Serial.printf("%d,   dc-dc charger on [0]\n", cp.dc_dc_on);
  Serial.printf(" XQ=  "); Serial.printf("%ld,  Time until silent, s [0]\n", Sen->until_q);

  sp.Modeling_p->print_help();  //* Xm
  sp.pretty_print_modeling();

  #endif

  sp.Amp_p->print_help();  //* Xa
  sp.Freq_p->print_help();  //* Xf

  sp.Type_p->print_help();  //* Xt
  // Serial.printf(" *Xt=  "); Serial.printf("%d", sp.type()); Serial.printf(": Inj 'n'=none(0) 's'=sin(1) 'q'=square(2) 't'=tri(3) biases(4,5,6) 'o'=cos(8))\n");
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
  Serial.printf(" XC= "); Serial.printf("%6.3f cycles inj\n", Sen->cycles_inj);
  Serial.printf(" XR  "); Serial.printf("RUN inj\n");
  Serial.printf(" XS  "); Serial.printf("STOP inj\n");
  Serial.printf(" Xs= "); Serial.printf("%4.2f scalar on T_SAT\n", cp.s_t_sat);
  Serial.printf(" XW= "); Serial.printf("%7.2f s wait start inj\n", float(Sen->wait_inj)/1000.);
  Serial.printf(" XT= "); Serial.printf("%7.2f s tail end inj\n", float(Sen->tail_inj)/1000.);
  Serial.printf(" Xu= "); Serial.printf("%d T=ignore Tb read\n", Sen->Flt->fail_tb());
  Serial.printf(" Xv= "); Serial.printf("%4.2f scale Tb 1-wire stale persist\n", Sen->Flt->tb_stale_time_sclr());
  Serial.printf("\nurgency of cmds: -=ASAP,*=SOON, '' or +=QUEUE, <=LAST\n");
  #endif
}
