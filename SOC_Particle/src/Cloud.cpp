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

#include "subs.h"
#include "Cloud.h"
#include "constants.h"
#include <math.h>

// #include "Blynk/BlynkSimpleSerialBLE.h"
// #define BLYNK_PRINT Serial

extern CommandPars cp;            // Various parameters to be common at system level (reset on PLC reset)
extern PublishPars pp;            // For publishing

// Assignments
void assign_publist(Publish* pubList, const unsigned long long now, const String unit, const String hm_string,
  Sensors* Sen, const int num_timeouts, BatteryMonitor* Mon)
{
  String batt;
  if ( sp.Mon_chm() == 0 )
    batt = "_bb";
  else if ( sp.Mon_chm() == 1 )
    batt = "_ch";
  else
    batt = "_un";
  pubList->now = now;
  pubList->unit = unit + batt.c_str();
  pubList->hm_string =hm_string;
  pubList->Tb = Sen->Tb;
  pubList->Ib = Sen->Ib;
  pubList->tcharge = Mon->tcharge();
  pubList->Voc = Mon->voc()*sp.nS();
  pubList->sat = Mon->sat();
  pubList->Amp_hrs_remaining_ekf = Mon->amp_hrs_remaining_ekf()*sp.nS()*sp.nP();
  pubList->Amp_hrs_remaining_soc = Mon->amp_hrs_remaining_soc()*sp.nS()*sp.nP();
}
