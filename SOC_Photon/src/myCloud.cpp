//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "mySubs.h"
#include "myCloud.h"
#include "constants.h"
#include <math.h>

// #include "Blynk/BlynkSimpleSerialBLE.h"
// #define BLYNK_PRINT Serial

extern CommandPars cp;            // Various parameters to be common at system level (reset on PLC reset)
extern PublishPars pp;            // For publishing

// Assignments
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  Sensors* Sen, const int num_timeouts, BatteryMonitor* Mon)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hm_string =hm_string;
  pubList->control_time = Sen->control_time;
  pubList->Vb = Sen->Vb;
  pubList->Tb = Sen->Tb;
  pubList->Tb_filt = Sen->Tb_filt;
  pubList->Vshunt = Sen->Vshunt;
  pubList->Ib = Sen->Ib;
  pubList->Wb = Sen->Wb;
  pubList->num_timeouts = num_timeouts;
  pubList->T = Sen->T;
  pubList->tcharge = Mon->tcharge();
  pubList->Voc_stat = Mon->Voc_stat();
  pubList->Voc = Mon->Voc();
  pubList->Voc_filt = Mon->Voc_filt();
  pubList->Vsat = Mon->Vsat();
  pubList->sat = Mon->sat();
  pubList->soc_model = Sen->Sim->soc();
  pubList->soc_ekf = Mon->soc_ekf();
  pubList->soc = Mon->soc();
  pubList->Amp_hrs_remaining_ekf = Mon->Amp_hrs_remaining_ekf();
  pubList->Amp_hrs_remaining_soc = Mon->Amp_hrs_remaining_soc();
  pubList->dV_dyn = Mon->dV_dyn();
  pubList->Voc_ekf = Mon->Hx();
  pubList->y_ekf = Mon->y_ekf();
  pubList->ioc = Mon->ioc();
  pubList->voc_soc = Mon->voc_soc();
}
