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

// struct Flt_st.  This file needed to avoid circular reference to sp in header files
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

// Copy function
void Flt_st::copy_from(Flt_st input)
{
  t = input.t;
  Tb_hdwe = input.Tb_hdwe;
  vb_hdwe = input.vb_hdwe;
  ib_amp_hdwe = input.ib_amp_hdwe;
  ib_noa_hdwe = input.ib_noa_hdwe;
  Tb = input.Tb;
  vb = input.vb;
  ib = input.ib;
  soc = input.soc;
  soc_ekf = input.soc_ekf;
  voc = input.voc;
  voc_stat = input.voc_stat;
  e_wrap_filt =input.e_wrap_filt;
  fltw = input.fltw;
  falw = input.falw;
}

// Nominal values
void Flt_st::nominal()
{
  this->t = 1L;
  this->Tb_hdwe = int16_t(0);
  this->vb_hdwe = int16_t(0);
  this->ib_amp_hdwe = int16_t(0);
  this->ib_noa_hdwe = int16_t(0);
  this->Tb = int16_t(0);
  this->vb = int16_t(0);
  this->ib = int16_t(0);
  this->soc = int16_t(0);
  this->soc_ekf = int16_t(0);
  this->voc = int16_t(0);
  this->voc_stat = int16_t(0);
  this->e_wrap_filt = int16_t(0);
  this->fltw = uint16_t(0);
  this->falw = uint16_t(0);
  this->dummy = 0L;
}

// Print functions
void Flt_st::pretty_print(const String code)
{
  char buffer[32] = "---";
  if ( this->t>0 )
  {
    time_long_2_str(this->t, buffer);
  }
  Serial.printf("code %s\n", code.c_str());
  Serial.printf("buffer %s\n", buffer);
  Serial.printf("t %ld\n", this->t);
  Serial.printf("Tb_hdwe %7.3f\n", double(this->Tb_hdwe)/600.);
  Serial.printf("vb_hdwe %7.3f\n", double(this->vb_hdwe)/1200.);
  Serial.printf("ib_amp_hdwe %7.3f\n", double(this->ib_amp_hdwe)/600.);
  Serial.printf("ib_noa_hdwe %7.3f\n", double(this->ib_noa_hdwe)/600.);
  Serial.printf("Tb %7.3f\n", double(this->Tb)/600.);
  Serial.printf("vb %7.3f\n", double(this->vb)/1200.);
  Serial.printf("ib %7.3f\n", double(this->ib)/600.);
  Serial.printf("soc %7.4f\n", double(this->soc)/16000.);
  Serial.printf("soc_ekf %7.4f\n", double(this->soc_ekf)/16000.);
  Serial.printf("voc %7.3f\n", double(this->voc)/1200.);
  Serial.printf("voc_stat %7.3f\n", double(this->voc_stat)/1200.);
  Serial.printf("e_wrap_filt %7.3f\n", double(this->e_wrap_filt)/1200.);
  Serial.printf("fltw %d falw %d\n", this->fltw, this->falw);
}
void Flt_st::print(const String code)
{
  char buffer[32] = "---";
  if ( this->t>0 )
  {
    time_long_2_str(this->t, buffer);
  }
  Serial.printf("%s, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,\n",
    code.c_str(), buffer, this->t,
    double(this->Tb_hdwe)/600.,
    double(this->vb_hdwe)/1200.,
    double(this->ib_amp_hdwe)/600.,
    double(this->ib_noa_hdwe)/600.,
    double(this->Tb)/600.,
    double(this->vb)/1200.,
    double(this->ib)/600.,
    double(this->soc)/16000.,
    double(this->soc_ekf)/16000.,
    double(this->voc)/1200.,
    double(this->voc_stat)/1200.,
    double(this->e_wrap_filt)/1200.,
    this->fltw,
    this->falw);
  // Serial.printf("%s, %s, %ld, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, fltw0x%X, falw0x%X,\n",
  //   code.c_str(), buffer, this->t,
  //   this->Tb_hdwe,
  //   this->vb_hdwe,
  //   this->ib_amp_hdwe,
  //   this->ib_noa_hdwe,
  //   this->Tb,
  //   this->vb,
  //   this->ib,
  //   this->soc,
  //   this->soc_ekf,
  //   this->voc,
  //   this->voc_stat,
  //   this->e_wrap_filt,
  //   this->fltw,
  //   this->falw);
  Serial1.printf("unit_f, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,\n",
    buffer, this->t,
    double(this->Tb_hdwe)/600.,
    double(this->vb_hdwe)/1200.,
    double(this->ib_amp_hdwe)/600.,
    double(this->ib_noa_hdwe)/600.,
    double(this->Tb)/600.,
    double(this->vb)/1200.,
    double(this->ib)/600.,
    double(this->soc)/16000.,
    double(this->soc_ekf)/16000.,
    double(this->voc)/1200.,
    double(this->voc_stat)/1200.,
    double(this->e_wrap_filt)/1200.,
    this->fltw,
    this->falw);
}

// Class fault ram to interface Flt_st to ram
Flt_ram::Flt_ram()
{
  Flt_st();
}

// Initialize each structure
void Flt_ram::instantiate(SerialRAM *ram, uint16_t *next)
{
  rP_ = ram;
  t_eeram_.a16 = *next; *next += sizeof(t);
  Tb_hdwe_eeram_.a16 = *next; *next += sizeof(Tb_hdwe);
  vb_hdwe_eeram_.a16 = *next; *next += sizeof(vb_hdwe);
  ib_amp_hdwe_eeram_.a16 = *next; *next += sizeof(ib_amp_hdwe);
  ib_noa_hdwe_eeram_.a16 = *next; *next += sizeof(ib_noa_hdwe);
  Tb_eeram_.a16 = *next; *next += sizeof(Tb);
  vb_eeram_.a16 = *next; *next += sizeof(vb);
  ib_eeram_.a16 = *next; *next += sizeof(ib);
  soc_eeram_.a16 = *next; *next += sizeof(soc);
  soc_ekf_eeram_.a16 = *next; *next += sizeof(soc_ekf);
  voc_eeram_.a16 = *next; *next += sizeof(voc);
  voc_stat_eeram_.a16 = *next; *next += sizeof(voc_stat);
  e_wrap_filt_eeram_.a16 = *next; *next += sizeof(e_wrap_filt);
  fltw_eeram_.a16 = *next; *next += sizeof(fltw);
  falw_eeram_.a16 = *next; *next += sizeof(falw);
  nominal();
}

// Load all
void Flt_ram::load_all()
{
  get_t();
  get_Tb_hdwe();
  get_vb_hdwe();
  get_ib_amp_hdwe();
  get_ib_noa_hdwe();
  get_Tb();
  get_vb();
  get_ib();
  get_soc();
  get_soc_ekf();
  get_voc();
  get_voc_stat();
  get_e_wrap_filt();
  get_fltw();
  get_falw();
}