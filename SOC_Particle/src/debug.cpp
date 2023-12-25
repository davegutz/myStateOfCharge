/*  Heart rate and pulseox calculation Constants

18-Dec-2020 	DA Gutz 	Created from MAXIM code.
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

*/

#include "debug.h"
#include "parameters.h"
#include "talk/chitchat.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle

#ifndef CONFIG_PHOTON
  // sp.debug()==12 EKF
  void debug_12(BatteryMonitor *Mon, Sensors *Sen)
  {
    Serial.printf("ib,ib_mod,   vb,vb_mod,  voc,voc_stat_mod,voc_mod,   K, y,    SOC_mod, SOC_ekf, SOC,   %7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
    Mon->ib(), Sen->Sim->ib(),
    Mon->vb(), Sen->Sim->vb(),
    Mon->voc(), Sen->Sim->voc_stat(), Sen->Sim->voc(),
    Mon->K_ekf(), Mon->y_ekf(),
    Sen->Sim->soc(), Mon->soc_ekf(), Mon->soc());
  }

// sp.debug()==-13 ib_dscn for Arduino.
  // Start Arduino serial plotter.  Toggle v like 'vv0;vv-13;' to produce legend
  void debug_m13(Sensors *Sen)
  {

    // Arduinio header
    static int8_t last_call = 0;
    if ( sp.debug()!=last_call && sp.debug()==-13 )
      Serial.printf("ib_sel_st:, ib_amph:, ib_noah:, ib_rate:, ib_quiet:,  dscn_flt:, dscn_fa:\n");
    last_call = sp.debug();

    // Plot
    if ( sp.debug()!=-13)
      return;
    else
        Serial.printf("%d, %7.3f,%7.3f,  %7.3f,%7.3f,   %d,%d\n",
    Sen->Flt->ib_sel_stat(),
    max(min(Sen->Ib_amp_hdwe, 2), -2), max(min(Sen->Ib_noa_hdwe, 2), -2),
    max(min(Sen->Flt->ib_rate(),2), -2), max(min(Sen->Flt->ib_quiet(), 2), -2),
    Sen->Flt->ib_dscn_fa(), Sen->Flt->ib_dscn_fa());
  }

  // sp.debug()==-23 vb for Arduino.
  // Start Arduino serial plotter.  Toggle v like 'vv0;vv-23;' to produce legend
  void debug_m23(Sensors *Sen)
  {

    // Arduinio header
    static int8_t last_call = 0;
    if ( sp.debug()!=last_call && sp.debug()==-23 )
      Serial.printf("Vb_hdwe-Vb_hdwe_f:\n");
    last_call = sp.debug();

    // Plot
    if ( sp.debug()!=-23)
      return;
    else
        Serial.printf("%7.3f\n", Sen->Vb_hdwe - Sen->Vb_hdwe_f);
  }

  // sp.debug()==-24 Vb, Ib for Arduino.
  // Start Arduino serial plotter.  Toggle v like 'vv0;vv-23;' to produce legend
  void debug_m24(Sensors *Sen)
  {

    // Arduinio header
    static int8_t last_call = 0;
    if ( sp.debug()!=last_call && sp.debug()==-23 )
      Serial.printf("Vb_hdwe-Vb_hdwe_f:, Ib_hdwe:\n");
    last_call = sp.debug();

    // Plot
    if ( sp.debug()!=-24)
      return;
    else
        Serial.printf("%7.3f, %7.3f\n", Sen->Vb_hdwe - Sen->Vb_hdwe_f, Sen->Ib_hdwe);
  }
#endif

// sp.debug()==5 Charge time
void debug_5(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("oled_display: Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem_wt, %3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
    pp.pubList.Tb, pp.pubList.Voc, pp.pubList.Ib, pp.pubList.Amp_hrs_remaining_ekf, pp.pubList.tcharge, pp.pubList.Amp_hrs_remaining_soc);
}

// Q quick print critical parameters
void debug_q(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("ib_amp_fail %d\nib_noa_fail %d\nvb_fail %d\nTb%7.3f\nvb%7.3f\nvoc%7.3f\nvoc_filt%7.3f\nvsat%7.3f\nib%7.3f\nsoc_m%8.4f\n\
soc_ekf%8.4f\nsoc%8.4f\nsoc_min%8.4f\nsoc_inf%8.4f\nmodeling %d\n",
    Sen->Flt->ib_amp_fa(), Sen->Flt->ib_noa_fa(), Sen->Flt->vb_fail(),
    Mon->temp_c(), Mon->vb(), Mon->voc(), Mon->voc_filt(), Mon->vsat(), Mon->ib(), Sen->Sim->soc(), Mon->soc_ekf(),
    Mon->soc(), Mon->soc_min(), Mon->soc_inf(), sp.modeling());
  Serial.printf("dq_inf/dq_abs%10.1f/%10.1f %8.4f coul_eff*=%9.6f, DAB+=%9.6f\nDQn%10.1f Tn%10.1f DQp%10.1f Tp%10.1f\n",
    Mon->delta_q_inf(), Mon->delta_q_abs(), Mon->delta_q_inf()/Mon->delta_q_abs(),
    -Mon->delta_q_neg()/Mon->delta_q_pos(),
    -(Mon->delta_q_neg() + Mon->delta_q_pos()) / nice_zero(Mon->time_neg() + Mon->time_pos(), 1e-6),
    Mon->delta_q_neg(), Mon->time_neg(), Mon->delta_q_pos(), Mon->time_pos());

  Serial1.printf("ib_amp_fail %d\nib_noa_fail %d\nvb_fail %d\nTb%7.3f\nvb%7.3f\nvoc%7.3f\nvoc_filt%7.3f\nvsat%7.3f\nib%7.3f\nsoc_m%8.4f\n\
soc_ekf%8.4f\nsoc%8.4f\nsoc_min%8.4f\nsoc_inf%8.4f\nmodeling %d\n",
    Sen->Flt->ib_amp_fa(), Sen->Flt->ib_noa_fa(), Sen->Flt->vb_fail(),
    Mon->temp_c(), Mon->vb(), Mon->voc(), Mon->voc_filt(), Mon->vsat(), Mon->ib(), Sen->Sim->soc(), Mon->soc_ekf(),
    Mon->soc(), Mon->soc_min(), Mon->soc_inf(), sp.modeling());
  Serial1.printf("dq_inf/dq_abs%10.1f/%10.1f = %8.4f coul_eff*=%9.6f DAB+=%9.6f\nDQn%10.1f Tn%10.1f DQp%10.1f Tp%10.1f\n",
    Mon->delta_q_inf(), Mon->delta_q_abs(), Mon->delta_q_inf()/Mon->delta_q_abs(),
    -Mon->delta_q_neg()/Mon->delta_q_pos(),
    -(Mon->delta_q_neg() + Mon->delta_q_pos()) / nice_zero(Mon->time_neg() + Mon->time_pos(), 1e-6),
    Mon->delta_q_neg(), Mon->time_neg(), Mon->delta_q_pos(), Mon->time_pos());

  if ( Sen->Flt->falw() || Sen->Flt->fltw() ) chit("Pf;", QUEUE);
}

// Calibration
void debug_99(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("Tb Vb imh inh voc voc_soc |*SV,*Dc |*SA,*DA|*SB,*DB| *Dw: %6.2fC %7.3fv %6.2fA %6.2fA %6.2fv %6.2fv |%6.3f %6.3fv  |%6.3f %6.3fA | %6.3f %6.3fA |%6.3fv,\n",
  Sen->Tb_hdwe, Sen->Vb_hdwe_f, Sen->Ib_amp_hdwe_f, Sen->Ib_noa_hdwe_f, Mon->voc(), Mon->voc_soc(), sp.Vb_scale(), sp.Vb_bias_hdwe(), sp.ib_scale_amp(), sp.ib_bias_amp(), sp.ib_scale_noa(), sp.ib_bias_noa(), sp.Dw());
  Serial1.printf("Tb Vb imh inh voc voc_soc |*SV,*Dc |*SA,*DA|*SB,*DB| *Dw: %6.2fC %7.3fv %6.2fA %6.2fA %6.2fv %6.2fv |%6.3f %6.3fv  |%6.3f %6.3fA | %6.3f %6.3fA |%6.3fv,\n",
  Sen->Tb_hdwe, Sen->Vb_hdwe_f, Sen->Ib_amp_hdwe_f, Sen->Ib_noa_hdwe_f, Mon->voc(), Mon->voc_soc(), sp.Vb_scale(), sp.Vb_bias_hdwe(), sp.ib_scale_amp(), sp.ib_bias_amp(), sp.ib_scale_noa(), sp.ib_bias_noa(), sp.Dw());
}

#ifdef DEBUG_INIT
  // Various parameters to debug initialization stuff as needed
  void debug_m1(BatteryMonitor *Mon, Sensors *Sen)
  {
    Serial.printf("mod %d fake_f %d reset_temp %d Tb%7.3f Tb_f%7.3f Vb%7.3f Ib%7.3f\nTb_s%6.2f Tl_s%6.2f ib_s%7.3f soc_s%8.4f dq_s%10.1f\nTb  %6.2f Tl%6.2f ib%7.3f soc  %8.4f dq  %10.1f soc_ekf%8.4f dq_ekf%10.1f\nvoc_filt %7.3f vsat %7.3f sat %d dq_z%10.1f lf %d llf %d\n",
        sp.modeling(), ap.fake_faults, Sen->reset_temp(), Sen->Tb, Sen->Tb_filt, Sen->Vb, Sen->Ib,
        Sen->Sim->Tb(), sp.T_state_model(), Sen->Sim->ib(), Sen->Sim->soc(), Sen->Sim->delta_q(),
        Mon->Tb(), sp.T_state(), Mon->ib(), Mon->soc(), Mon->delta_q(), Mon->soc_ekf(), Mon->delta_q_ekf(),
        Mon->voc_filt(), Mon->vsat(), Mon->sat(), sp.delta_q_z, Sen->Flt->latched_fail(), Sen->Flt->latched_fail_fake());
  }
#endif

#ifdef DEBUG_QUEUE
void debug_queue()
{
  if (cp.inp_token && (cp.inp_str.length() || cp.asap_str.length() || cp.soon_str.length() || cp.queue_str.length() || cp.last_str.length()))
    Serial.printf("\nchatter:  cp.inp_str [%s] ASAP[%s] SOON[%s],QUEUE[%s] LAST[%s] CMD[%s]\n",
      cp.inp_str.c_str(), cp.asap_str.c_str(), cp.soon_str.c_str(), cp.queue_str.c_str(), cp.last_str.c_str(), cp.cmd_str.c_str());
}
#endif
