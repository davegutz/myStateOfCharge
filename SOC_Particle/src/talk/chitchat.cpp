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
#include "application.h"
#include "chitchat.h"
#include "help.h"
#include "../subs.h"
#include "../command.h"
#include "../parameters.h"
#include "../debug.h"
#include "recall_H.h"
#include "recall_P.h"
#include "recall_R.h"
#include "recall_X.h"
#include "followup.h"

extern SavedPars sp;       // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap;    // Various adjustment parameters shared at system level
extern CommandPars cp;     // Various parameters shared at system level
extern Flt_st mySum[NSUM]; // Summaries for saving charge history

// Process asap commands
// void asap()
// {
// #ifdef DEBUG_QUEUE
//   if (cp.inp_str.length() || cp.asap_str.length())
//     Serial.printf("\nasap exit cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
//                   cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str());
// #endif

//   if (!cp.cmd_token && cp.asap_str.length())
//   {
//     cp.cmd_token = true;
//     cp.cmd_str = get_cmd(&cp.asap_str);
//     cp.cmd_token = false;
//   }

// #ifdef DEBUG_QUEUE
//   if (cp.inp_str.length() || cp.asap_str.length())
//     Serial.printf("\nasap exit cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
//                   cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str());
// #endif
// }

// Process chat strings
// void chat()
// {
// #ifdef DEBUG_QUEUE
//   if (!cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length()))
//     Serial.printf("\nchat enter cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] cmd_token %d\n",
//                   cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str(), cp.cmd_token);
// #endif

//   if (!cp.cmd_token && cp.soon_str.length())
//   {
//     cp.cmd_token = true;
//     cp.cmd_str = get_cmd(&cp.soon_str);
//     cp.cmd_token = false;

// #ifdef DEBUG_QUEUE
//     if (cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length()))
//       Serial.printf("\nSOON cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
//                     cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str());
// #endif
//   }

//   else if (!cp.cmd_token && cp.queue_str.length()) // Do QUEUE only after SOON empty
//   {
//     cp.cmd_token = true;
//     cp.cmd_str = get_cmd(&cp.queue_str);
//     cp.cmd_token = false;

// #ifdef DEBUG_QUEUE
//     if (cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length()))
//       Serial.printf("\nQUEUE cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
//                     cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str());
// #endif
//   }

//   else if (!cp.cmd_token && cp.last_str.length()) // Do END only after QUEUE empty
//   {
//     cp.cmd_token = true;
//     cp.cmd_str = get_cmd(&cp.last_str);
//     cp.cmd_token = false;

// #ifdef DEBUG_QUEUE
//     if (cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length()))
//       Serial.printf("\nEND cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] cmd_token %d\n",
//                     cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str(), cp.cmd_token);
// #endif
//   }

//   else
//   {
//     cp.cmd_token = true;
//     cp.cmd_str = cp.inp_str;
//     cp.cmd_token = false;
//   }

// #ifdef DEBUG_QUEUE
//   if (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length())
//     Serial.printf("\nchat exit cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
//                   cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str());
// #endif

//   return;
// }


// Process chat strings one at a time.  Leave this with one cmd_str
void chatter()
{
  if ( !cp.cmd_token )
  {
    cp.cmd_token = true;

    if      ( cp.asap_str.length()  ) cp.cmd_str = get_cmd(&cp.asap_str);

    else if ( cp.soon_str.length()  ) cp.cmd_str = get_cmd(&cp.soon_str);

    else if ( cp.queue_str.length() ) cp.cmd_str = get_cmd(&cp.queue_str);

    else if ( cp.last_str.length()  ) cp.cmd_str = get_cmd(&cp.last_str);

    cp.cmd_token = false;
  }

  #ifdef DEBUG_QUEUE
    debug_queue("chatter");
  #endif

  return;
}


// Call talk from within, a crude macro feature.   'from' should by semi-colon delimited commands for transcribe()
String chit(const String from, const urgency when)
{
  String chit_str = "";
  String When = "";

  #ifdef DEBUG_QUEUE

    debug_queue("\nenter chit");

    Serial.printf("\nenter chit[%s] to[%s]\n", from.c_str(), When.c_str());

    if ( cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length() )
      Serial.printf("     inp[%s] ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] rtn[%s]\n\n",
                    cp.inp_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str(), chit_str.c_str());

  #endif

  if (when == LAST)
  {
    cp.last_str += from;
    When = "LAST";
  }
  if (when == QUEUE)
  {
    cp.queue_str += from;
    When = "QUEUE";
  }
  else if (when == SOON)
  {
    cp.soon_str += from;
    When = "SOON";
  }
  else if (when == ASAP)
  {
    cp.asap_str += from;
    When = "ASAP";
  }
  else if (when == NEW)
  {
    When = "NEW";
  }
  else if (when == INCOMING)
  {
    When = "INCOMING";
  }
  else
  {
    chit_str = from;
  }

  #ifdef DEBUG_QUEUE

    debug_queue("\nexit chit");

    Serial.printf("\nexit chit[%s] to[%s]\n", from.c_str(), When.c_str());

    if ( cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length() )
      Serial.printf("     inp[%s] ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] rtn[%s]\n\n",
                    cp.inp_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str(), chit_str.c_str());

  #endif

  return chit_str;
}


// Generate commands
void chitter()
{
  urgency request;

  // Serial event
  request = NEW;
  if (cp.inp_str.length())
  {
    if (!cp.inp_token && !cp.cmd_token)
    {
      cp.inp_token = true;
      cp.cmd_token = true;

      // Categorize the requests
      char key = cp.inp_str.charAt(0);
      request = decode_from_inp(key);
      Serial.printf("\nchitter enter: "); cmd_echo(request);

      // Deal with each request
      switch (request)
      {
      case (NEW): // Defaults to QUEUE
        chit(cp.inp_str.substring(0) + ";", QUEUE);
        break;

      case (ASAP):
        chit(cp.inp_str.substring(1) + ";", ASAP);
        break;

      case (SOON):
        chit(cp.inp_str.substring(1) + ";", SOON);
        break;

      case (QUEUE):
        chit(cp.inp_str.substring(1) + ";", QUEUE);
        break;

      case (LAST):
        chit(cp.inp_str.substring(1) + ";", LAST);
        break;
      }

      cp.inp_str = "";
      cp.inp_token = false;
      cp.cmd_token = false;

      #ifdef DEBUG_QUEUE
        debug_queue("chitter exit");
        Serial.printf("chitter exit: %d of {INCOMING, ASAP, SOON, QUEUE, NEW, LAST}; key %c inp_str [%s] cmd_str [%s]\n", request, key, cp.inp_str.c_str(), cp.cmd_str.c_str());
      #endif
    }
  }
}


// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for transcribe()
void clear_queues()
{
  cp.last_str = "";
  cp.queue_str = "";
  cp.soon_str = "";
  cp.asap_str = "";
}


// Limited echoing of Serial1 commands available
void cmd_echo(urgency request)
{
  if ( request==0 )
  {
    Serial.printf ("\ncmd: %s\n", cp.inp_str.c_str());
    Serial1.printf ("\ncmd: %s\n", cp.inp_str.c_str());
  }
  else
  {
    Serial.printf ("\necho: %s, %d\n", cp.inp_str.c_str(), request);
    Serial1.printf("\necho: %s, %d\n", cp.inp_str.c_str(), request);
  }
}


// Decode key
urgency decode_from_inp(const char key)
{
  urgency result = NEW;
  if (key == '>')
  {
    cp.cmd_str = cp.cmd_str.substring(1); // Delete any leading '>'
    result = INCOMING;
  }
  else if (key == 'c')
  {
    result = INCOMING;
  }
  else if (key == '-' && cp.inp_str.charAt(1) != 'c')
  {
    cp.inp_str = cp.inp_str.substring(1); // Delete the leading '-'
    result = INCOMING;
  }
  else if (key == '-')
    result = ASAP;
  else if (key == '+')
    result = QUEUE;
  else if (key == '*')
    result = SOON;
  else if (key == '<')
    result = LAST;
  else
  {
    result = NEW;
  }
  return result;
}


// Talk Executive
void describe(BatteryMonitor *Mon, Sensors *Sen)
{
  int INT_in = -1;
  boolean found = false;
  urgency request;
  uint16_t modeling_past = sp.modeling();
  char letter_0 = '\0';
  char letter_1 = '\0';

  // Serial event
  request = NEW;
  if ( !cp.cmd_token && cp.cmd_str.length() )
  {
    request = INCOMING;
    cp.cmd_token = true;

    // Now we know the letters
    letter_0 = cp.inp_str.charAt(0);
    letter_1 = cp.inp_str.charAt(1);
    Serial.printf("\ndescribe:  processing %s; = '%c' + '%c'\n", cp.cmd_str.c_str(), letter_0, letter_1);
    cmd_echo(request);

    switch ( request )
    {
      case ( INCOMING ):
        switch ( letter_0 )
        {

          case ( 'b' ):  // Fault buffer
            switch ( letter_1 )
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
                found = ap.find_adjust(cp.cmd_str) || sp.find_adjust(cp.cmd_str);
                if (!found) Serial.printf("%s NOT FOUND\n", cp.cmd_str.substring(0,2).c_str());
            }
            break;

          case ( 'B' ):
            switch ( letter_1 )
            {
              case ( 'Z' ):  // BZ :  Benign zeroing of settings to make clearing test easier
                benign_zero(Mon, Sen);
                break;

              default:
                found = ap.find_adjust(cp.cmd_str) || sp.find_adjust(cp.cmd_str);
                if (!found) Serial.printf("%s NOT FOUND\n", cp.cmd_str.substring(0,2).c_str());
            }
            break;

          case ( 'c' ):  // c:  clear queues
            Serial.printf("***CLEAR QUEUES\n");
            clear_queues();
            break;

          case ( 'H' ):  // History
            found = recall_H(letter_1, Mon, Sen);
            break;

          case ( 'P' ):
            found = recall_P(letter_1, Mon, Sen);
            break;

          case ( 'Q' ):  // Q:  quick critical
            debug_q(Mon, Sen);
            break;

          case ( 'R' ):
            found = recall_R(letter_1, Mon, Sen);
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
            if ( cp.cmd_str.substring(1).length() )
            {
              INT_in = cp.cmd_str.substring(1).toInt();
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
            found = recall_X(letter_1, Mon, Sen);
            break;

          case ( 'h' ):  // h: help
            talkH(Mon, Sen);
            break;

          default:
            found = ap.find_adjust(cp.cmd_str) || sp.find_adjust(cp.cmd_str);
            if (!found) Serial.printf("%s NOT FOUND\n", cp.cmd_str.substring(0,2).c_str());

        }

      ///////////PART 2/////// There may be followup to structures or new commands
      followup(letter_0, letter_1, Mon, Sen, modeling_past);
    }

    cp.cmd_str = "";
    cp.cmd_token = false;
  }  // if ( cp.cmd_token )
}


// Clear adjustments that should be benign if done instantly
void benign_zero(BatteryMonitor *Mon, Sensors *Sen) // BZ
{

  // Snapshots
  cp.cmd_summarize(); // Hs
  cp.cmd_summarize(); // Hs
  cp.cmd_summarize(); // Hs
  cp.cmd_summarize(); // Hs

  // Model
  ap.hys_scale = HYS_SCALE;                  // Sh 1
  ap.slr_res = 1;                            // Sr 1
  sp.cutback_gain_slr_p->print_adj_print(1); // Sk 1
  ap.hys_state = 0;                          // SH 0

  // Injection
  ap.ib_amp_add = 0;        // Dm 0
  ap.ib_noa_add = 0;        // Dn 0
  ap.vb_add = 0;            // Dv 0
  ap.ds_voc_soc = 0;        // Ds
  ap.Tb_bias_model = 0;     // D^
  ap.dv_voc_soc = 0;        // Dy
  ap.tb_stale_time_slr = 1; // Xv 1
  ap.fail_tb = false;       // Xu 0

  // Noise
  ap.Tb_noise_amp = TB_NOISE;         // DT 0
  ap.Vb_noise_amp = VB_NOISE;         // DV 0
  ap.Ib_amp_noise_amp = IB_AMP_NOISE; // DM 0
  ap.Ib_noa_noise_amp = IB_NOA_NOISE; // DN 0

  // Intervals
  ap.eframe_mult = max(min(EKF_EFRAME_MULT, UINT8_MAX), 0); // DE
  ap.print_mult = max(min(DP_MULT, UINT8_MAX), 0);          // DP
  Sen->ReadSensors->delay(READ_DELAY);                      // Dr

  // Fault logic
  ap.cc_diff_slr = 1;  // Fc 1
  ap.ib_diff_slr = 1;  // Fd 1
  ap.fake_faults = 0;  // Ff 0
  sp.put_ib_select(0); // Ff 0
  ap.ewhi_slr = 1;     // Fi
  ap.ewlo_slr = 1;     // Fo
  ap.ib_quiet_slr = 1; // Fq 1
  ap.disab_ib_fa = 0;  // FI 0
  ap.disab_tb_fa = 0;  // FT 0
  ap.disab_vb_fa = 0;  // FV 0
}
