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

#include "../subs.h"
#include "../command.h"
#include "../Battery.h"
#include "../local_config.h"
#include "../Summary.h"
#include "../parameters.h"
#include <math.h>
#include "../debug.h"
#include "chitchat.h"
#include "recall_H.h"
#include "recall_P.h"
#include "recall_R.h"
#include "recall_X.h"
#include "followup.h"
#include "help.h"
#include "chitchat.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern PrinterPars pr;  // Print buffer
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

// Talk Executive
void transcribe(BatteryMonitor *Mon, Sensors *Sen)
{
  int INT_in = -1;
  boolean found = false;
  urgency request;
  uint16_t modeling_past = sp.modeling();
  char letter_0 = '\0';
  char letter_1 = '\0';

  // Serial event
  request = NEW;
  if ( !cp.cmd_token && ( cp.cmd_str.length() || cp.inp_str.length() ) )
  {
    cp.cmd_token = true;

    // Categorize the requests

    // Priority cmd over input
    if ( cp.cmd_str.length() )
    {
      char key = cp.cmd_str.charAt(0);
      #ifdef DEBUG_QUEUE
        Serial.printf("cmd_str transcribe enter: request %d of {INCOMING, ASAP, SOON, QUEUE, NEW, LAST}; key %c input_are [%s] cmd_str [%s]\n", request, key, cp.inp_str.c_str(), cp.cmd_str.c_str());
      #endif
      if ( key == '>' )
      {
        cp.cmd_str = cp.cmd_str.substring(1);  // Delete any leading '>'
        request = INCOMING;
      }
      else
      {
        request = NEW;
      }
    }
    else if ( cp.inp_str.length() )
    {
      char key = cp.inp_str.charAt(0);
      #ifdef DEBUG_QUEUE
        Serial.printf("inp_str transcribe enter: request %d of {INCOMING, ASAP, SOON, QUEUE, NEW, LAST}; key %c input_are [%s] cmd_str [%s]\n", request, key, cp.inp_str.c_str(), cp.cmd_str.c_str());
      #endif
      if ( key == 'c' )
      {
        request = INCOMING;
      }
      else if ( key == '-' && cp.inp_str.charAt(1)!= 'c')
      {
        cp.inp_str = cp.inp_str.substring(1);  // Delete the leading '-'
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
        cp.inp_str = cp.cmd_str.substring(1);  // Delete any leading '>'
        request = INCOMING;
      }
      else
      {
        request = NEW;
      }
      // Now we know the letters
      letter_0 = cp.inp_str.charAt(0);
      letter_1 = cp.inp_str.charAt(1);

      // Limited echoing of Serial1 commands available
      if ( request==0 )
      {
        Serial.printf ("cmd: %s\n", cp.inp_str.c_str());
        Serial1.printf ("cmd: %s\n", cp.inp_str.c_str());
      }
      else
      {
        Serial.printf ("echo: %s, %d\n", cp.inp_str.c_str(), request);
        Serial1.printf("echo: %s, %d\n", cp.inp_str.c_str(), request);
      }

      // Deal with each request
      #ifdef DEBUG_QUEUE
        Serial.printf("transcribe: urgency %d of {INCOMING, ASAP, SOON, QUEUE, NEW, LAST}; str [%s]\n", request, cp.cmd_str.c_str());
      #endif
      switch ( request )
      {
        case ( NEW ):  // Defaults to QUEUE
          cp.cmd_str = chit( cp.inp_str.substring(0) + ";", QUEUE);
          break;

        case ( ASAP ):
          cp.cmd_str = chit( cp.inp_str.substring(1)+";", ASAP);
          break;

        case ( SOON ):
          cp.cmd_str = chit( cp.inp_str.substring(1)+";", SOON);
          break;

        case ( QUEUE ):
          cp.cmd_str = chit( cp.inp_str.substring(1)+";", QUEUE);
          break;

        case ( LAST ):
          cp.cmd_str = chit( cp.inp_str.substring(1)+";", LAST);
          break;
      }
    }
  }
  if ( cp.cmd_str.length() )
  {
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
