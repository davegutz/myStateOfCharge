/*  Heart rate and pulseox calculation Constants

18-Dec-2020 	DA Gutz 	Created from MAXIM code.
// Copyright (C) 2020 - Dave Gutz
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
extern SavedPars sp;              // Various parameters to be static at system level

// sp.debug==-4  // General Arduino plot
void debug_m4(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("Tb,Vb*10-110,Ib, voc*10-110,dv_dyn*100,voc_ekf*10-110,voc*10-110,vsat*10-110,  y_ekf*1000,  soc_sim*100,soc_ekf*100,soc*100,\n\
    %7.3f,%7.3f,%7.3f,  %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,  %10.6f,  %7.3f,%7.4f,%7.4f,\n",
    Sen->Tb, Sen->Vb*10.-110., Sen->Ib,
    Mon->voc()*10.-110., Mon->dv_dyn()*100., Mon->z_ekf()*10.-110., Mon->voc()*10.-110., Mon->vsat()*10.-110.,
    Mon->y_ekf()*1000.,
    Sen->Sim->soc()*100., Mon->x_ekf()*100., Mon->soc()*100.);
}

// sp.debug==12 EKF
void debug_12(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("ib,ib_mod,   vb,vb_mod,  voc,voc_stat_mod,voc_mod,   K, y,    SOC_mod, SOC_ekf, SOC,   %7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
  Mon->Ib(), Sen->Sim->Ib(),
  Mon->Vb(), Sen->Sim->Vb(),
  Mon->voc(), Sen->Sim->voc_stat(), Sen->Sim->voc(),
  Mon->K_ekf(), Mon->y_ekf(),
  Sen->Sim->soc(), Mon->soc_ekf(), Mon->soc());
}

// sp.debug==-13 ib_dscn for Arduino.
// Start Arduino serial plotter.  Toggle v like 'v0;v-13;' to produce legend
/*
void debug_m13(Sensors *Sen)
{

  // Arduinio header
  static int8_t last_call = 0;
  if ( sp.debug!=last_call && sp.debug==-13 )
    Serial.printf("ib_sel_st:, ib_amph:, ib_noah:, ib_rate:, ib_quiet:,  dscn_flt:, dscn_fa:\n");
  last_call = sp.debug;

  // Plot
  if ( sp.debug!=-13)
    return;
  else
      Serial.printf("%d, %7.3f,%7.3f,  %7.3f,%7.3f,   %d,%d\n",
  Sen->Flt->ib_sel_stat(),
  max(min(Sen->Ib_amp_hdwe, 2), -2), max(min(Sen->Ib_noa_hdwe, 2), -2),
  max(min(Sen->Flt->ib_rate(),2), -2), max(min(Sen->Flt->ib_quiet(), 2), -2),
  Sen->Flt->ib_dscn_fa(), Sen->Flt->ib_dscn_fa());
}
*/

// sp.debug==5 Charge time
void debug_5(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("oled_display: Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem_wt, %3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
    pp.pubList.Tb, pp.pubList.Voc, pp.pubList.Ib, pp.pubList.Amp_hrs_remaining_ekf, pp.pubList.tcharge, pp.pubList.Amp_hrs_remaining_soc);
}

// Hysteresis print
void debug_h(BatteryMonitor *Mon, Sensors *Sen)
{
  Mon->hys_pretty_print();
  Sen->Sim->hys_pretty_print();
  Serial.printf("\n Mon    Sim\n");
  Serial.printf("vb%5.2f,%5.2f\n", Mon->vb(), Sen->Sim->vb());
  Serial.printf("voc%5.2f,%5.2f\n", Mon->voc(), Sen->Sim->voc());
  Serial.printf("voc_stat%5.2f,%5.2f\n", Mon->Voc_stat(), Sen->Sim->voc_stat());
  Serial.printf("dh_hys%7.3f,%7.3f\n", Mon->hys_state(), Sen->Sim->hys_state());
  Serial.printf("voc_soc%5.2f\n", Mon->voc_soc());
  Serial.printf("e_wrap%6.3f\n", Sen->Flt->e_wrap());
  Serial.printf("e_wrap_f%6.3f\n", Sen->Flt->e_wrap_filt());
  Serial1.printf("\n Mon    Sim\n");
  Serial1.printf("vb%5.2f,%5.2f\n", Mon->vb(), Sen->Sim->vb());
  Serial1.printf("voc%5.2f,%5.2f\n", Mon->voc(), Sen->Sim->voc());
  Serial1.printf("voc_stat%5.2f,%5.2f\n", Mon->Voc_stat(), Sen->Sim->voc_stat());
  Serial1.printf("dh_hys%7.3f,%7.3f\n", Mon->hys_state(), Sen->Sim->hys_state());
  Serial1.printf("voc_soc%5.2f\n", Mon->voc_soc());
  Serial1.printf("e_wrap%6.3f\n", Sen->Flt->e_wrap());
  Serial1.printf("e_wrap_f%6.3f\n", Sen->Flt->e_wrap_filt());
}

// Q quick print critical parameters
void debug_q(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("ib_amp_fail = %d,\nib_noa_fail = %d,\nvb_fail = %d,\n\
Tb  = %7.3f,\nVb  = %7.3f,\nVoc = %7.3f,\nvoc_filt  = %7.3f,\nVsat = %7.3f,\nIb  = %7.3f,\nsoc_m = %8.4f,\n\
soc_ekf= %8.4f,\nsoc = %8.4f,\nmodeling = %d,\n",
    Sen->Flt->ib_amp_fa(), Sen->Flt->ib_noa_fa(), Sen->Flt->vb_fail(),
    Mon->temp_c(), Mon->Vb(), Mon->Voc(), Mon->Voc_filt(), Mon->Vsat(), Mon->Ib(), Sen->Sim->soc(), Mon->soc_ekf(),
    Mon->soc(), sp.modeling());
  Serial1.printf("ib_amp_fail = %d,\nib_noa_fail = %d,\nvb_fail = %d,\n\
Tb  = %7.3f,\nVb  = %7.3f,\nVoc = %7.3f,\nvoc_filt  = %7.3f,\nVsat = %7.3f,\nIb  = %7.3f,\nsoc_m = %8.4f,\n\
soc_ekf= %8.4f,\nsoc = %8.4f,\nmodeling = %d,\n",
      Sen->Flt->ib_amp_fa(), Sen->Flt->ib_noa_fa(), Sen->Flt->vb_fail(),
      Mon->temp_c(), Mon->Vb(), Mon->Voc(), Mon->Voc_filt(), Mon->Vsat(), Mon->Ib(), Sen->Sim->soc(), Mon->soc_ekf(),
      Mon->soc(), sp.modeling());
  if ( Sen->Flt->falw() || Sen->Flt->fltw() ) chit("Pf;", QUEUE);
}

// Various parameters to debug initialization stuff as needed
// void debug_m1(BatteryMonitor *Mon, Sensors *Sen)
// {
//   Serial.printf("mod %d fake_f %d reset_temp %d Tb%7.3f Tb_f%7.3f Vb%7.3f Ib%7.3f\nTb_s%6.2f Tl_s%6.2f ib_s%7.3f soc_s%8.4f dq_s%10.1f\nTb  %6.2f Tl%6.2f ib%7.3f soc  %8.4f dq  %10.1f soc_ekf%8.4f dq_ekf%10.1f\n",
//       sp.modeling(), cp.fake_faults, Sen->reset_temp(), Sen->Tb, Sen->Tb_filt, Sen->Vb, Sen->Ib,
//       Sen->Sim->Tb(), sp.t_last_model, Sen->Sim->ib(), Sen->Sim->soc(), Sen->Sim->delta_q(),
//       Mon->Tb(), sp.t_last, Mon->ib(), Mon->soc(), Mon->delta_q(), Mon->soc_ekf(), Mon->delta_q_ekf());
// }
