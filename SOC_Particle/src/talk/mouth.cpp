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
#include "followup.h"
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
  int INT_in = -1;
  boolean found = false;
  urgency request;
  uint16_t modeling_past = sp.Modeling();

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
        followup(cp.input_str.charAt(1), Mon, Sen, modeling_past);
    }

    cp.input_str = "";
    cp.token = false;
  }  // if ( cp.token )
}
