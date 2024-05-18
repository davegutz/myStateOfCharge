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
#include "../constants.h"
#include "../Summary.h"
#include "../parameters.h"
#include <math.h>
#include "../debug.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

#if defined(HDWE_PHOTON)
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
  sp.nP_p->print_help();  //* BP
  sp.nS_p->print_help();  //* BS

  Serial.printf("\nBZ Benignly zero test settings\n");
  
  Serial.printf("\ncc  clear talk queues end XQ\n");
  Serial.printf("\ncf  freeze talk queues\n");
  Serial.printf("\ncu  unfreeze talk queues\n");

  Serial.printf("\nC<?> Chg SOC e.g.:\n");
  ap.init_all_soc_p->print_help();  // Ca
  Serial.printf("  Cm=  model (& ekf if mod)- '(0-1.1)'\n"); 

  Serial.printf("\nD/S<?> Adj e.g.:\n");
  sp.ib_bias_amp_p->print_help();  //* DA
  sp.ib_bias_amp_p->print1_help();  //* DA
  sp.ib_bias_noa_p->print_help();  //* DB
  sp.ib_bias_noa_p->print1_help();  //* DB
  sp.Vb_bias_hdwe_p->print_help();  //* Dc
  sp.Vb_bias_hdwe_p->print1_help();  //* Dc
  ap.eframe_mult_p->print_help();  //  DE
  ap.sum_delay_p->print_help();  //  Dh
  Serial.printf("    set 'Dh0;' for nominal\n");
  sp.ib_bias_all_p->print_help();  //* DI
  sp.ib_bias_all_p->print1_help();  //* DI
  sp.ib_bias_amp_p->print_help();  //  Dm
  ap.Ib_amp_noise_amp_p->print_help();  // DM
  sp.ib_bias_noa_p->print_help();  //  Dn
  ap.Ib_noa_noise_amp_p->print_help();  // DN
  ap.vc_add_p->print_help();  // D3
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
  ap.talk_delay_p->print_help();  //  D>
  sp.ib_scale_amp_p->print_help();  //* SA
  sp.ib_scale_amp_p->print1_help();  //* SA
  sp.ib_scale_noa_p->print_help();  //* SB
  sp.ib_scale_noa_p->print1_help();  //* SB
  sp.ib_disch_slr_p->print_help();  //* SD
  sp.ib_disch_slr_p->print1_help();  //* SD
  ap.hys_scale_p->print_help();  //  Sh
  ap.hys_state_p->print_help();  //  SH
  sp.cutback_gain_slr_p->print_help();  //* Sk
  sp.s_cap_mon_p->print_help();  //* SQ
  sp.s_cap_mon_p->print1_help();  //* SQ
  sp.s_cap_sim_p->print_help();  //* Sq
  sp.s_cap_sim_p->print1_help();  //* Sq
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
  Serial.printf("  Ca=<val> "); Serial.printf("initialize_all to present inputs\n");
  Serial.printf("  Rb= "); Serial.printf("batteries to present inputs\n");
  Serial.printf("  Rf= "); Serial.printf("fault logic latches\n");
  Serial.printf("  Ri= "); Serial.printf("infinite counter\n");
  Serial.printf("  Rr= "); Serial.printf("saturate Mon and equalize Sim & Mon\n");
  Serial.printf("  RR= "); Serial.printf("DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small.  Reinitialize filters\n");
  Serial.printf("  RS= "); Serial.printf("SavedPars: Renominalize saved\n");
  Serial.printf("  RV= "); Serial.printf("Renominalize volatile\n");

  sp.ib_select_p->print_help();  //* si
  sp.Time_now_p->print_help();  //* UT
  sp.Time_now_p->print1_help();  //* UT
  sp.debug_p->print_help();  // v
  sp.debug_p->print1_help();  // v

  Serial.printf("  -<>: Negative - Arduino plot compatible\n");
  Serial.printf(" vv-2: ADS counts for throughput meas\n");
  #ifdef DEBUG_INIT
    Serial.printf("  v-1: Debug\n");
  #endif
  Serial.printf("  vv1: GP\n");
  Serial.printf("  vv2: GP, Sim & Sel\n");
  Serial.printf("  vv3: EKF\n");
  Serial.printf("  vv4: GP, Sim, Sel, & EKF\n");
  Serial.printf("  vv5: OLED display\n");
  #ifndef HDWE_PHOTON
    Serial.printf(" vv12: EKF\n");
    Serial.printf("vv-13: ib_dscn\n");
  #endif
  Serial.printf(" vv14: vshunt and Ib raw\n");
  Serial.printf(" vv15: vb raw\n");
  Serial.printf(" vv16: Tb\n");
  #ifndef HDWE_PHOTON
    Serial.printf("vv-23: Vb_hdwe_ac\n");
    Serial.printf("vv-24: Vb_hdwe_ac, Ib_hdwe\n");
    Serial.printf(" vv34: EKF detail\n");
    Serial.printf(" vv35: ChargeTransfer balance\n");
    Serial.printf(" vv37: EKF short\n");
    Serial.printf(" vv75: voc_low check mod\n");
    Serial.printf(" vv76: vb model\n");
    Serial.printf(" vv78: Batt model sat\n");
    Serial.printf(" vv79: sat_ib model\n");
  #endif
  Serial.printf(" vv98: shunt filtering check\n");
  Serial.printf(" vv99: calibration\n");

  Serial.printf("\nW<?> - iters to wait\n");

  #ifdef HDWE_PHOTON2
    Serial.printf("\nw - save * confirm adjustments to SRAM\n");
  #endif

  Serial.printf("\nX<?> - Test Mode.   For example:\n");
  ap.dc_dc_on_p->print_help();  // Xd
  ap.until_q_p->print_help();  // XQ
  sp.modeling_p->print_help();  //* Xm
  sp.pretty_print_modeling();

  #endif

  sp.amp_p->print_help();  //* Xa
  sp.freq_p->print_help();  //* Xf
  sp.Type_p->print_help();  //* Xt

  #ifndef HELPLESS
  Serial.printf(" Xp= <?>, scripted tests...\n"); 
  Serial.printf("  Xp0: reset tests\n");
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
  // sp.testY_p->print_help();  // XY
  Serial.printf("\nurgency of cmds: -=ASAP,*=SOON, '' or +=QUEUE, <=LAST\n");
  #endif
}
