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
#include "help.h"
#include "../subs.h"
#include "../command.h"
// #include "../local_config.h"
// #include "../Summary.h"
#include "../parameters.h"
#include "transcribe.h"
// #include <math.h>
// #include "../debug.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

// Process asap commands
void asap()
{
  #ifdef DEBUG_QUEUE
    if ( cp.inp_str.length() || cp.asap_str.length() )
      Serial.printf("\nasap exit cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
        cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif

  if ( !cp.cmd_token && cp.asap_str.length() )
  {
    cp.cmd_token = true;
    cp.cmd_str = get_cmd(&cp.asap_str);
    cp.cmd_token = false;
  }

  #ifdef DEBUG_QUEUE
    if ( cp.inp_str.length() || cp.asap_str.length() )
      Serial.printf("\nasap exit cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
        cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif
}

// Process chat strings
void chat()
{
  #ifdef DEBUG_QUEUE
    if ( !cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
      Serial.printf("\nchat enter cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] cmd_token %d\n",
        cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str(), cp.cmd_token);
  #endif

  if ( !cp.cmd_token && cp.soon_str.length() )
  {
    cp.cmd_token = true;
    cp.cmd_str = get_cmd(&cp.soon_str);
    cp.cmd_token = false;

    #ifdef DEBUG_QUEUE
      if ( cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
        Serial.printf("\nSOON cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
          cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
    #endif
  }

  else if ( !cp.cmd_token && cp.queue_str.length() ) // Do QUEUE only after SOON empty
  {
    cp.cmd_token = true;
    cp.cmd_str = get_cmd(&cp.queue_str);
    cp.cmd_token = false;

    #ifdef DEBUG_QUEUE
      if ( cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
        Serial.printf("\nQUEUE cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
          cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
    #endif
  }

  else if ( !cp.cmd_token && cp.end_str.length() ) // Do END only after QUEUE empty
  {
    cp.cmd_token = true;
    cp.cmd_str = get_cmd(&cp.end_str);
    cp.cmd_token = false;

    #ifdef DEBUG_QUEUE
      if ( cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() ))
        Serial.printf("\nEND cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] cmd_token %d\n",
          cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str(), cp.cmd_token);
    #endif
  }

  else
  {
    cp.cmd_token = true;
    cp.cmd_str = cp.inp_str;
    cp.cmd_token = false;

  }

  #ifdef DEBUG_QUEUE
    if ( cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() )
      Serial.printf("\nchat exit cp.inp_str [%s] cp.cmd_str [%s]:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s]\n",
        cp.inp_str.c_str(), cp.cmd_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str());
  #endif

  return;
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for transcribe()
String chit(const String cmd, const urgency when)
{
  String chit_str = "";
  #ifdef DEBUG_QUEUE
    String When;
    if ( when == NEW) When = "NEW";
    else if ( when == QUEUE) When = "QUEUE";
    else if (when == SOON) When = "SOON";
    else if (when == ASAP) When = "ASAP";
    else if (when == INCOMING) When = "INCOMING"; 
    Serial.printf("\nchit='%s'[%s]\n", cmd.c_str(), When.c_str());
  #endif

  if ( when == LAST )
    cp.end_str += cmd;
  if ( when == QUEUE )
    cp.queue_str += cmd;
  else if ( when == SOON )
    cp.soon_str += cmd;
  else if ( when == ASAP )
    cp.asap_str += cmd;
  else
    chit_str = cmd;
  
  #ifdef DEBUG_QUEUE
    if ( cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.end_str.length() )
      Serial.printf("\nchit exit:  ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] rtn[%s]\n\n",
        cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.end_str.c_str(), chit_str.c_str());
  #endif

  return chit_str;
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for transcribe()
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
  sp.cutback_gain_slr_p->print_adj_print(1); // Sk 1
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
  sp.put_ib_select(0);  // Ff 0
  ap.ewhi_slr = 1;  // Fi
  ap.ewlo_slr = 1;  // Fo
  ap.ib_quiet_slr = 1;  // Fq 1
  ap.disab_ib_fa = 0;  // FI 0
  ap.disab_tb_fa = 0;  // FT 0
  ap.disab_vb_fa = 0;  // FV 0

}

