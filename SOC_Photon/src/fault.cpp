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
#include "fault.h"
#include "mySensors.h"
extern SavedPars sp;              // Various parameters to be static at system level

// struct Flt_st
void Flt_st::assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen)
{
  char buffer[32];
  time_long_2_str(now, buffer);
  this->t = now;
  this->Tb_hdwe = int16_t(Sen->Tb_hdwe*600.);
  this->vb_hdwe = int16_t(Sen->Vb/sp.nS*1200.);
  this->ib_amp_hdwe = int16_t(Sen->Ib_amp_hdwe/sp.nP*600.);
  this->ib_noa_hdwe = int16_t(Sen->Ib_noa_hdwe/sp.nP*600.);
  this->Tb = int16_t(Sen->Tb*600.);
  this->vb = int16_t(Sen->Vb/sp.nS*1200.);
  this->ib = int16_t(Sen->Ib/sp.nP*600.);
  this->soc = int16_t(Mon->soc()*16000.);
  this->soc_ekf = int16_t(Mon->soc_ekf()*16000.);
  this->voc = int16_t(Mon->voc()*1200.);
  this->voc_stat = int16_t(Mon->voc_stat()*1200.);
  this->e_wrap_filt = int16_t(Sen->Flt->e_wrap_filt()*1200.);
  this->fltw = Sen->Flt->fltw();
  this->falw = Sen->Flt->falw();
}
