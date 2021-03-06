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

// rp.debug==-1 General purpose Arduino plot
void debug_m1(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("%7.3f,     %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
    Sen->Sim->soc()*100.-90,
    Sen->ShuntAmp->ishunt_cal(), Sen->ShuntNoAmp->ishunt_cal(),
    Sen->Vbatt*10-110, Sen->Sim->voc()*10-110, Sen->Sim->dv_dyn()*10, Sen->Sim->Vb()*10-110, Mon->dv_dyn()*10-110);
}

// rp.debug==-3  // Power Arduino plot
void debug_m3(BatteryMonitor *Mon, Sensors *Sen, const unsigned long elapsed, const boolean reset)
{
  Serial.printf("fast,et,reset,Wbatt,q_f,q,soc,T,\n %12.3f,%7.3f, %d, %7.3f,    %7.3f,\n",
  Sen->control_time, double(elapsed)/1000., reset, Sen->Wbatt, Sen->Sim->soc());
}

// rp.debug==-4  // General Arduino plot
void debug_m4(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("Tb,Vb*10-110,Ib, voc*10-110,dv_dyn*100,voc_ekf*10-110,voc*10-110,vsat*10-110,  y_ekf*1000,  soc_sim*100,soc_ekf*100,soc*100,soc_wt*100,\n\
    %7.3f,%7.3f,%7.3f,  %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,  %10.6f,  %7.3f,%7.4f,%7.4f,%7.4f,\n",
    Sen->Tbatt, Sen->Vbatt*10.-110., Sen->Ibatt,
    Mon->voc()*10.-110., Mon->dv_dyn()*100., Mon->z_ekf()*10.-110., Mon->voc()*10.-110., Mon->vsat()*10.-110.,
    Mon->y_ekf()*1000.,
    Sen->Sim->soc()*100., Mon->x_ekf()*100., Mon->soc()*100.,  Mon->soc_wt()*100.);
}

// rp.debug==12 EKF
void debug_12(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("ib,ib_mod,   vb,vb_mod,  voc,voc_stat_mod,voc_mod,   K, y,    SOC_mod, SOC_ekf, SOC,   %7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
  Mon->Ib(), Sen->Sim->Ib(),
  Mon->Vb(), Sen->Sim->Vb(),
  Mon->voc(), Sen->Sim->voc_stat(), Sen->Sim->voc(),
  Mon->K_ekf(), Mon->y_ekf(),
  Sen->Sim->soc(), Mon->soc_ekf(), Mon->soc());
}

// rp.debug==-12 EKF Arduino plot
void debug_m12(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("ib,ib_mod,   vb*10-110,vb_mod*10-110,  voc*10-110,voc_stat_mod*10-110,voc_mod*10-110,   K, y,    SOC_mod-90, SOC_ekf-90, SOC-90,\n%7.3f,%7.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,\n",
  Mon->Ib(), Sen->Sim->Ib(),
  Mon->Vb()*10-110, Sen->Sim->Vb()*10-110,
  Mon->voc()*10-110, Sen->Sim->voc_stat()*10-110, Sen->Sim->voc()*10-110,
  Mon->K_ekf(), Mon->y_ekf(),
  Sen->Sim->soc()*100-90, Mon->soc_ekf()*100-90, Sen->Sim->soc()*100-90);
}

// rp.debug==-35 EKF summary Arduino plot
void debug_m35(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("soc_mod,soc_ekf,voc_ekf= %7.3f, %7.3f, %7.3f\n", Sen->Sim->soc(), Mon->x_ekf(), Mon->z_ekf());
}

// rp.debug==5 Charge time
void debug_5(void)
{
  Serial.printf("oled_display: Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem_wt, %3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
    pp.pubList.Tbatt, pp.pubList.Vbatt, pp.pubList.Ibatt, pp.pubList.Amp_hrs_remaining_ekf, pp.pubList.tcharge, pp.pubList.Amp_hrs_remaining_wt);
}

// rp.debug==-5 Charge time Arduino plot
void debug_m5(void)
{
  if ( rp.debug==-5 ) Serial.printf("Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem_wt,\n%3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
    pp.pubList.Tbatt, pp.pubList.Vbatt, pp.pubList.Ibatt, pp.pubList.Amp_hrs_remaining_ekf, pp.pubList.tcharge, pp.pubList.Amp_hrs_remaining_wt);
}

// rp.debug==-7 Battery i/o Arduino plot
void debug_m7(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("%7.3f,%7.3f,%7.3f,   %7.3f, %7.3f, %7.3f,\n",
        Mon->soc(), Sen->ShuntAmp->ishunt_cal(), Sen->ShuntNoAmp->ishunt_cal(),
        Sen->Vbatt, Sen->Sim->voc_stat(), Sen->Sim->voc());
}
