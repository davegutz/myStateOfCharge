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
#include "fault.h"
#include "mySensors.h"

extern SavedPars sp;       // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level

// struct Flt_st.  This file needed to avoid circular reference to sp in header files
void Flt_st::assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen)
{
  char buffer[32];
  time_long_2_str(now, buffer);
  this->t = now;
  this->Tb_hdwe = int16_t(Sen->Tb_hdwe*600.);
  this->vb_hdwe = int16_t(Sen->Vb/sp.nS()*1200.);
  this->ib_amp_hdwe = int16_t(Sen->Ib_amp_hdwe/sp.nP()*600./sp.Ib_hist_slr());
  this->ib_noa_hdwe = int16_t(Sen->Ib_noa_hdwe/sp.nP()*600./sp.Ib_hist_slr());
  this->Tb = int16_t(Sen->Tb*600.);
  this->vb = int16_t(Sen->Vb/sp.nS()*1200.);
  this->ib = int16_t(Sen->Ib/sp.nP()*600./sp.Ib_hist_slr());
  if ( sp.Debug()==-1) Serial.printf("Ib %7.3f nP %7.3f ib %d\n", Sen->Ib, sp.nP(), this->ib);
  this->soc = int16_t(Mon->soc()*16000.);
  this->soc_min = int16_t(Mon->soc_min()*16000.);
  this->soc_ekf = int16_t(Mon->soc_ekf()*16000.);
  this->voc = int16_t(Mon->voc()*1200.);
  this->voc_stat = int16_t(Mon->voc_stat()*1200.);
  this->e_wrap_filt = int16_t(Sen->Flt->e_wrap_filt()*1200.);
  this->fltw = Sen->Flt->fltw();
  this->falw = Sen->Flt->falw();
}

// Copy function
void Flt_st::copy_to_Flt_ram_from(Flt_st input)
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
  soc_min = input.soc_min;
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
  this->soc_min = int16_t(0);
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
  if ( this->t>1 )
  {
    time_long_2_str(this->t, buffer);
    Serial.printf("code %s\n", code.c_str());
    Serial.printf("buffer %s\n", buffer);
    Serial.printf("t %ld\n", this->t);
    Serial.printf("Tb_hdwe %7.3f\n", float(this->Tb_hdwe)/600.);
    Serial.printf("vb_hdwe %7.3f\n", float(this->vb_hdwe)/1200.);
    Serial.printf("ib_amp_hdwe %7.3f\n", float(this->ib_amp_hdwe)/600.*sp.Ib_hist_slr());
    Serial.printf("ib_noa_hdwe %7.3f\n", float(this->ib_noa_hdwe)/600.*sp.Ib_hist_slr());
    Serial.printf("Tb %7.3f\n", float(this->Tb)/600.);
    Serial.printf("vb %7.3f\n", float(this->vb)/1200.);
    Serial.printf("ib %7.3f\n", float(this->ib)/600.*sp.Ib_hist_slr());
    Serial.printf("soc %7.4f\n", float(this->soc)/16000.);
    Serial.printf("soc_min %7.4f\n", float(this->soc_min)/16000.);
    Serial.printf("soc_ekf %7.4f\n", float(this->soc_ekf)/16000.);
    Serial.printf("voc %7.3f\n", float(this->voc)/1200.);
    Serial.printf("voc_stat %7.3f\n", float(this->voc_stat)/1200.);
    Serial.printf("e_wrap_filt %7.3f\n", float(this->e_wrap_filt)/1200.);
    Serial.printf("fltw %d falw %d\n", this->fltw, this->falw);
  }
}


void Flt_st::print(const String code)
{
  char buffer[32] = "---";
  if ( this->t>1 )
  {
    time_long_2_str(this->t, buffer);
    Serial.printf("%s, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,\n",
      code.c_str(), buffer, this->t,
      float(this->Tb_hdwe)/600.,
      float(this->vb_hdwe)/1200.,
      float(this->ib_amp_hdwe)/600.*sp.Ib_hist_slr(),
      float(this->ib_noa_hdwe)/600.*sp.Ib_hist_slr(),
      float(this->Tb)/600.,
      float(this->vb)/1200.,
      float(this->ib)/600.*sp.Ib_hist_slr(),
      float(this->soc)/16000.,
      float(this->soc_min)/16000.,
      float(this->soc_ekf)/16000.,
      float(this->voc)/1200.,
      float(this->voc_stat)/1200.,
      float(this->e_wrap_filt)/1200.,
      this->fltw,
      this->falw);
    Serial1.printf("unit_f, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.4f, %7.3f, %7.3f, %7.3f, %d, %d,\n",
      buffer, this->t,
      float(this->Tb_hdwe)/600.,
      float(this->vb_hdwe)/1200.,
      float(this->ib_amp_hdwe)/600.*sp.Ib_hist_slr(),
      float(this->ib_noa_hdwe)/600.*sp.Ib_hist_slr(),
      float(this->Tb)/600.,
      float(this->vb)/1200.,
      float(this->ib)/600.*sp.Ib_hist_slr(),
      float(this->soc)/16000.,
      float(this->soc_min)/16000.,
      float(this->soc_ekf)/16000.,
      float(this->voc)/1200.,
      float(this->voc_stat)/1200.,
      float(this->e_wrap_filt)/1200.,
      this->fltw,
      this->falw);
  }
}

// Regular put
void Flt_st::put(Flt_st source)
{
  copy_to_Flt_ram_from(source);
}

// nominalize
void Flt_st::put_nominal()
{
  Flt_st source;
  source.nominal();
  copy_to_Flt_ram_from(source);
}

// Class fault ram to interface Flt_st to ram
Flt_ram::Flt_ram()
{
  Flt_st();
}
Flt_ram::~Flt_ram(){}

// Load all
#ifdef CONFIG_47L16_EERAM
  void Flt_ram::get()
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

  // Initialize each structure
  void Flt_ram::instantiate(SerialRAM *ram, uint16_t *next)
  {
    t_eeram_.a16 = *next; *next += sizeof(t);
    Tb_hdwe_eeram_.a16 = *next; *next += sizeof(Tb_hdwe);
    vb_hdwe_eeram_.a16 = *next; *next += sizeof(vb_hdwe);
    ib_amp_hdwe_eeram_.a16 = *next; *next += sizeof(ib_amp_hdwe);
    ib_noa_hdwe_eeram_.a16 = *next; *next += sizeof(ib_noa_hdwe);
    Tb_eeram_.a16 = *next; *next += sizeof(Tb);
    vb_eeram_.a16 = *next; *next += sizeof(vb);
    ib_eeram_.a16 = *next; *next += sizeof(ib);
    soc_eeram_.a16 = *next; *next += sizeof(soc);
    soc_min_eeram_.a16 = *next; *next += sizeof(soc_min);
    soc_ekf_eeram_.a16 = *next; *next += sizeof(soc_ekf);
    voc_eeram_.a16 = *next; *next += sizeof(voc);
    voc_stat_eeram_.a16 = *next; *next += sizeof(voc_stat);
    e_wrap_filt_eeram_.a16 = *next; *next += sizeof(e_wrap_filt);
    fltw_eeram_.a16 = *next; *next += sizeof(fltw);
    falw_eeram_.a16 = *next; *next += sizeof(falw);
    rP_ = ram;
    nominal();
  }
#endif

// Save all
void Flt_ram::put(const Flt_st value)
{
  copy_to_Flt_ram_from(value);
  #ifdef CONFIG_47L16_EERAM
    put_t();
    put_Tb_hdwe();
    put_vb_hdwe();
    put_ib_amp_hdwe();
    put_ib_noa_hdwe();
    put_Tb();
    put_vb();
    put_ib();
    put_soc();
    put_soc_ekf();
    put_voc();
    put_voc_stat();
    put_e_wrap_filt();
    put_fltw();
    put_falw();
  #endif
}

// nominalize
void Flt_ram::put_nominal()
{
  Flt_st source;
  source.nominal();
  put(source);
}
