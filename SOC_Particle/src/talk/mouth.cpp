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

#include "../mySubs.h"
#include "../command.h"
#include "../Battery.h"
#include "../local_config.h"
#include "../mySummary.h"
#include "../parameters.h"
#include <math.h>
#include "../debug.h"
#include "recall_H.h"
#include "recall_P.h"
#include "recall_R.h"
#include "recall_X.h"
#include "help.h"

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
    if ( cp.input_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() )
      Serial.printf("cp.input_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
        cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif
  if ( cp.soon_str.length() )  // Do SOON first
  {
    get_string(&cp.soon_str);
  #ifdef DEBUG_QUEUE
    if ( cp.token && (cp.input_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
      Serial.printf("chat (SOON):  cmd('%s;') ASAP[%s] SOON[%s] QUEUE[%s] LAST[%s]\n",
        cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif
  }
  else if ( cp.queue_str.length() ) // Do QUEUE only after SOON empty
  {
    get_string(&cp.queue_str);
    #ifdef DEBUG_QUEUE
      if ( cp.token && (cp.input_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
          Serial.printf("chat (QUEUE):  cmd('%s;') ASAP[%s] SOON[%s] QUEUE[%s] LAST[%s]\n",
           cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
    #endif
  }
  else if ( cp.end_str.length() ) // Do QUEUE only after SOON empty
  {
    get_string(&cp.end_str);
    #ifdef DEBUG_QUEUE
    if ( cp.token && (cp.input_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
      Serial.printf("chat (QUEUE):  cmd('%s;') ASAP[%s] SOON[%s] QUEUE[%s] LAST[%s]\n",
        cp.input_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
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
  boolean found = false;
  uint16_t modeling_past = sp.Modeling();
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
                found = ap.find_adjust(cp.input_str) || sp.find_adjust(cp.input_str);
                if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());
            }
            break;

          case ( 'B' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'Z' ):  // BZ :  Benign zeroing of settings to make clearing test easier
                benign_zero(Mon, Sen);
                Serial.printf("Benign Zero\n");
                break;

              default:
                found = ap.find_adjust(cp.input_str) || sp.find_adjust(cp.input_str);
                if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());
            }
            break;

          case ( 'c' ):  // c:  clear queues
            Serial.printf("***CLEAR QUEUES\n");
            clear_queues();
            break;

          case ( 'H' ):  // History
            found = recall_H(cp.input_str.charAt(1), Mon, Sen);
            break;

          case ( 'P' ):
            found = recall_P(cp.input_str.charAt(1), Mon, Sen);
            break;

          case ( 'Q' ):  // Q:  quick critical
            debug_q(Mon, Sen);
            break;

          case ( 'R' ):
            found = recall_R(cp.input_str.charAt(1), Mon, Sen);
            break;

          // Photon 2 O/S waits 10 seconds between backup SRAM saves.  To save time, you can get in the habit of pressing 'w;'
          // This was not done for all passes just to save only when an adjustment change verified by user (* parameters), to avoid SRAM life impact.
          #ifdef CONFIG_PHOTON2
          case ( 'w' ):  // w:  confirm write * adjustments to to SRAM
            System.backupRamSync();
            Serial.printf("SAVED *\n"); Serial1.printf("SAVED *\n");
            break;
          #endif

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

          case ( 'X' ):
            found = recall_X(cp.input_str.charAt(1), Mon, Sen);
            break;

          case ( 'h' ):  // h: help
            talkH(Mon, Sen);
            break;

          default:
            found = ap.find_adjust(cp.input_str) || sp.find_adjust(cp.input_str);
            if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());

        }

        ///////////PART 2/////// There may be followup to structures or new commands
        switch ( cp.input_str.charAt(0) )
        {

          case ( 'B' ):
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'm' ):  //* Bm
                if ( sp.Mon_chm_p->success() )
                switch ( sp.Mon_chm_z )
                {
                  case ( 0 ):  // Bm0: Mon Battleborn
                      Mon->assign_all_mod("Battleborn");
                      Mon->chem_pretty_print();
                      cp.cmd_reset();
                      break;

                    case ( 1 ):  // Bm1: Mon CHINS
                      Mon->assign_all_mod("CHINS");
                      Mon->chem_pretty_print();
                      cp.cmd_reset();
                      break;

                    case ( 2 ):  // Bm2: Mon Spare
                      Mon->assign_all_mod("Spare");
                      Mon->chem_pretty_print();
                      cp.cmd_reset();
                      break;

                  }
                  break;

              case ( 's' ):  //* Bs:  Simulation chemistry change
                if ( sp.Sim_chm_p->success() )
                switch ( sp.Sim_chm_z )
                {
                  case ( 0 ):  // Bs0: Sim Battleborn
                    Sen->Sim->assign_all_mod("Battleborn");
                    cp.cmd_reset();
                    break;

                  case ( 1 ):  // Bs1: Sim CHINS
                    Sen->Sim->assign_all_mod("CHINS");
                    cp.cmd_reset();
                    break;

                  case ( 2 ):  // Bs2: Sim Spare
                    Sen->Sim->assign_all_mod("Spare");
                    cp.cmd_reset();
                    break;

                }
                break;
            }
            break;

          case ( 'C' ):  // C:
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'a' ):  // Ca<>:  assign charge state in fraction to all versions including model
                if ( ap.init_all_soc_p->success() )
                {
                  initialize_all(Mon, Sen, ap.init_all_soc, true);
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
                  Serial.printf("skipping %s\n", cp.input_str.c_str());
                break;

              case ( 'm' ):  // Cm<>:  assign curve charge state in fraction to model only (ekf if modeling)
                if ( ap.init_sim_soc_p->success() )  // Apply crude limit to prevent user error
                {
                  Sen->Sim->apply_soc(ap.init_sim_soc, Sen->Tb_filt);
                  Serial.printf("soc%8.4f, dq%7.3f, soc_mod%8.4f, dq mod%7.3f,\n",
                      Mon->soc(), Mon->delta_q(), Sen->Sim->soc(), Sen->Sim->delta_q());
                  if ( sp.Modeling() ) cp.cmd_reset_sim(); // Does not block.  Commands a reset
                }
                else
                  Serial.printf("soc%8.4f; must be 0-1.1\n", FP_in);
                break;

            }
            break;

          case ( 'D' ):
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'r' ):  //   Dr<>:  READ sample time input
                if ( ap.read_delay_p->success() )
                  Sen->ReadSensors->delay(ap.read_delay);  // validated
                break;

              case ( 't' ):  //*  Dt<>:  Temp bias change hardware
                if ( sp.Tb_bias_hdwe_p->success() )
                  cp.cmd_reset();
                break;

              case ( 'v' ):  //     Dv<>:  voltage signal adder for faults
                if ( ap.vb_add_p->success() )
                  ap.vb_add_p->print1();
                break;
  
              case ( '>' ):  //   D><>:  TALK sample time input
                if ( ap.talk_delay_p->success() )
                  Sen->Talk->delay(ap.talk_delay);  // validated
                break;

            }
            break;

          case ( 'S' ):
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'H' ):  //   SH<>: state of all hysteresis
                if ( ap.hys_state_p->success() )
                {
                  Sen->Sim->hys_state(ap.hys_state);
                  Sen->Flt->wrap_err_filt_state(-ap.hys_state);
                }
                break;

              case ( 'q' ):  //*  Sq<>: scale capacity sim
                if ( sp.S_cap_sim_p->success() )
                {
                  Sen->Sim->apply_cap_scale(sp.S_cap_sim());
                  if ( sp.Modeling() ) Mon->init_soc_ekf(Sen->Sim->soc());
                }
                break;
            
              case ( 'Q' ):  //*  SQ<>: scale capacity mon
                if ( sp.S_cap_mon_p->success() )
                {
                  Mon->apply_cap_scale(sp.S_cap_mon());
                }
                break;
            
            }
            break;

          case ( 'F' ):  // Fault stuff
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'f' ):  //* si, Ff<>:  fake faults
                if ( ap.fake_faults_p->success() )
                  sp.put_Ib_select(cp.input_str.substring(2).toInt());
                break;

            }
            break;


          case ( 'l' ):
            if ( sp.Debug_p->success() )
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

          case ( 'U' ):
            if ( sp.Time_now_p->success() )
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'T' ):  //*  UT<>:  Unix time since epoch
              Time.setTime(sp.Time_now_z);
              break;

            }
            break;

          case ( 'X' ):  // X
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'm' ):  // Xm<>:  code for modeling level
                if ( sp.Modeling_p->success() )
                {
                  reset = sp.Modeling() != modeling_past;
                  if ( reset )
                  {
                    Serial.printf("Chg...reset\n");
                    cp.cmd_reset();
                  }
                }
                break;

              case ( 'a' ): // Xa<>:  injection amplitude
                if ( sp.Amp_p->success() )
                {
                  sp.Amp_z *= sp.nP();
                  Serial.printf("Inj amp, %s, %s set%7.3f & inj_bias set%7.3f\n", sp.Amp_p->units(), sp.Amp_p->description(), sp.Amp(), sp.Inj_bias());
                }
                break;

              case ( 'f' ): //*  Xf<>:  injection frequency
                if ( sp.Freq_p->success() ) sp.Freq_z = sp.Freq_z*(2. * PI);
                break;

              case ( 'b' ): //*  Xb<>:  injection bias
                Serial.printf("Inj amp, %s, %s set%7.3f & inj_bias set%7.3f\n", sp.Amp_p->units(), sp.Amp_p->description(), sp.Amp(), sp.Inj_bias());
                break;

              case ( 'Q' ): //  XQ<>: time to quiet
                Serial.printf("Going black in %7.1f seconds\n", float(ap.until_q) / 1000.);
                break;
            }
            break;

          default:
            // Serial.print(cp.input_str.charAt(0)); Serial.print(" ? 'h'\n");
            break;
        }
    }

    cp.input_str = "";
    cp.token = false;
  }  // if ( cp.token )
}
