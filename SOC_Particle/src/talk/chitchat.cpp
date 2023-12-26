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


// Prioritize commands to describe.  asap_str queue always run.  Others only with chitchat
void chatter()
{

  // Always pull from asap if available and run them
  if ( cp.asap_str.length()  ) cp.cmd_str = chat_cmd_from(&cp.asap_str);

  // Otherwise run the other queues when chitchat frame is running
  else if ( cp.chitchat )
  {
    if ( cp.soon_str.length()  ) cp.cmd_str = chat_cmd_from(&cp.soon_str);

    else if ( cp.queue_str.length() ) cp.cmd_str = chat_cmd_from(&cp.queue_str);

    else if ( cp.last_str.length()  ) cp.cmd_str = chat_cmd_from(&cp.last_str);
  }

  #ifdef DEBUG_QUEUE
    if ( cp.chitchat || cp.asap_str.length() ) debug_queue("chatter exit");
  #endif

  return;
}


// Parse commands to queue strings
void chit(const String from, const urgency when)
{
  String chit_str = "";

  if (when == ASAP)
  {
    cp.asap_str += from;
  }

  else if (when == SOON)
  {
    cp.soon_str += from;
  }

  if (when == QUEUE)
  {
    cp.queue_str += from;
  }

  else if (when == LAST)
  {
    cp.last_str += from;
  }

  #ifdef DEBUG_QUEUE
    if ( cp.chitchat || cp.asap_str.length() ) debug_queue("exit chit");
  #endif

}


// Parse inputs to queues
void chitter(const boolean chitchat)
{
  // Since this is first procedure in chitchat sequence, note the state of chitchat frame
  cp.chitchat = chitchat;

  // When info available
  if (cp.inp_str.length())
  {
    if ( !cp.inp_token )
    {
      cp.inp_token = true;
      String nibble = chit_nibble_inp();
      urgency request = chit_classify_nibble(&nibble);

      // Deal with each request.  Strip off to use up to ';'.  Leave the rest for next iteration.
      switch (request)
      {
        case (INCOMING):  // 0, really not used until chatter and then as a placeholder
          chit(nibble, INCOMING);
          break;

        case (ASAP):  // 1
          chit(nibble, ASAP);
          break;

        case (SOON):  // 2
          chit(nibble, SOON);
          break;

        case (QUEUE):  // 3
          chit(nibble, QUEUE);
          break;

        case (NEW): // 4
          chit(nibble, QUEUE);
          break;

        case (LAST):  // 5
          chit(nibble, LAST);
          break;
      }

      cp.inp_token = false;

      #ifdef DEBUG_QUEUE
        if ( cp.chitchat || cp.asap_str.length() ) debug_queue("chitter exit");
      #endif
    }
  }
}


// Decode key
urgency chit_classify_nibble(String *nibble)
{
  char key = nibble->charAt(0);
  urgency result;

  // Validate
  if (key == '>' || key == ';')
  {
    *nibble = nibble->substring(1); // Delete any leading 'junk' and redefine key
    key = nibble->charAt(0);
  }

  // Classify
  if (key == 'c')
  {
    result = ASAP;
  }
  else if (key == '-' && nibble->charAt(1) != 'c')
  {
    *nibble = nibble->substring(1); // Delete the leading '-'
    result = ASAP;
  }
  else if (key == '-')
  {
    *nibble = nibble->substring(1); // Delete the leading '-'
    result = ASAP;
  }
  else if (key == '+')
  {
    *nibble = nibble->substring(1); // Delete the leading '+'
    result = QUEUE;
  }
  else if (key == '*')
  {
    *nibble = nibble->substring(1); // Delete the leading '*'
    result = SOON;
  }
  else if (key == '<')
  {
    *nibble = nibble->substring(1); // Delete the leading '<'
    result = LAST;
  }
  else
  {
    result = NEW;
  }

  return result;
}


// Get next item up to next ';'
String chit_nibble_inp()
{
  int semi_loc = cp.inp_str.indexOf(';');
  String nibble = cp.inp_str.substring(0, semi_loc+1);  // 2nd index is exclusive
  cp.inp_str = cp.inp_str.substring(semi_loc+1);  // +1 to grab the semi-colon
  return nibble;
}


// Start over with clean queues
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
    Serial.printf ("cmd: %s\n", cp.cmd_str.c_str());
    Serial1.printf ("cmd: %s\n", cp.cmd_str.c_str());
  }
  else
  {
    Serial.printf ("echo: %s, %d\n", cp.cmd_str.c_str(), request);
    Serial1.printf("echo: %s, %d\n", cp.cmd_str.c_str(), request);
  }
}


// Run the commands
void describe(BatteryMonitor *Mon, Sensors *Sen)
{
  int INT_in = -1;
  boolean found = false;
  uint16_t modeling_past = sp.modeling();
  char letter_0 = '\0';
  char letter_1 = '\0';
  String value = "";

  // Serial event
  if ( cp.cmd_str.length() )
  {
    // Now we know the letters
    letter_0 = cp.cmd_str.charAt(0);
    letter_1 = cp.cmd_str.charAt(1);
    value = cp.cmd_str.substring(2);
    cmd_echo(INCOMING);

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
}
