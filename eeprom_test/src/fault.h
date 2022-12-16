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


#ifndef _FAULT_H
#define _FAULT_H

#include "application.h"
#include "Battery.h"
String time_long_2_str(const unsigned long current_time, char *tempStr);

// SRAM retention summary
// Broken into 3 parts because Wire reliably supports a limited number of bytes per transmission
struct Flt1_st
{
  unsigned long t = 1L; // Timestamp seconds since start of epoch
  int16_t Tb_hdwe = 0;  // Battery temperature, hardware, C
  int16_t vb_hdwe = 0;  // Battery measured potential, hardware, V
  int16_t ib_amp_hdwe = 0;  // Battery measured input current, amp, A
  int16_t ib_noa_hdwe = 0;  // Battery measured input current, no amp, A
  Flt1_st()
  {
    nominal();
  }
  // void assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen);
  void copy_from(Flt1_st input)
  {
    t = input.t;
    Tb_hdwe = input.Tb_hdwe;
    vb_hdwe = input.vb_hdwe;
    ib_amp_hdwe = input.ib_amp_hdwe;
    ib_noa_hdwe = input.ib_noa_hdwe;
  }
  void nominal()
  {
    this->t = 1L;
    this->Tb_hdwe = int16_t(0);
    this->vb_hdwe = int16_t(0);
    this->ib_amp_hdwe = int16_t(0);
    this->ib_noa_hdwe = int16_t(0);
  }
  void pretty_print(const String code)
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
    Serial.printf("vb_hdwe %7.3f\n", double(this->vb_hdwe)/600.);
    Serial.printf("ib_amp_hdwe %7.3f\n", double(this->ib_amp_hdwe)/600.);
    Serial.printf("ib_noa_hdwe %7.3f\n", double(this->ib_noa_hdwe)/600.);
  }
  void print(const String code)
  {
    char buffer[32] = "---";
    if ( this->t>0 )
    {
      time_long_2_str(this->t, buffer);
    }
    Serial.printf("%s, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f,",
      code.c_str(), buffer, this->t,
      double(this->Tb_hdwe)/600.,
      double(this->vb_hdwe)/1200.,
      double(this->ib_amp_hdwe)/600.,
      double(this->ib_noa_hdwe)/600.);
    Serial1.printf("unit_f, %s, %ld, %7.3f, %7.3f, %7.3f, %7.3f,",
      buffer, this->t,
      double(this->Tb_hdwe)/600.,
      double(this->vb_hdwe)/1200.,
      double(this->ib_amp_hdwe)/600.,
      double(this->ib_noa_hdwe)/600.);
  }
};
struct Flt2_st
{
  int16_t Tb = 0;       // Battery temperature, filtered, C
  int16_t vb = 0;       // Battery measured potential, filtered, V
  int16_t ib = 0;       // Battery measured input current, filtered, A
  int16_t soc = 0;      // Battery state of charge, free Coulomb counting algorithm, frac
  int16_t soc_ekf = 0;  // Battery state of charge, ekf, frac
  int16_t voc = 0;      // Battery open circuit voltage measured vb-ib*Z, V
  int16_t voc_stat = 0; // Stored charge voltage from measurement, V
  Flt2_st()
  {
    nominal();
  }
  // void assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen);
  void copy_from(Flt2_st input)
  {
    Tb = input.Tb;
    vb = input.vb;
    ib = input.ib;
    soc = input.soc;
    soc_ekf = input.soc_ekf;
    voc = input.voc;
    voc_stat = input.voc_stat;
}
  void nominal()
  {
    this->Tb = int16_t(0);
    this->vb = int16_t(0);
    this->ib = int16_t(0);
    this->soc = int16_t(0);
    this->soc_ekf = int16_t(0);
    this->voc = int16_t(0);
    this->voc_stat = int16_t(0);
  }
  void pretty_print(const String code)
  {
    Serial.printf("Tb %7.3f\n", double(this->Tb)/1200.);
    Serial.printf("ib %7.3f\n", double(this->ib)/600.);
    Serial.printf("soc %7.4f\n", double(this->soc)/16000.);
    Serial.printf("soc_ekf %7.4f\n", double(this->soc_ekf)/16000.);
    Serial.printf("voc %7.3f\n", double(this->voc)/1200.);
    Serial.printf("voc_stat %7.3f\n", double(this->voc_stat)/1200.);
  }
  void print()
  {
    Serial.printf("%7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f,",
      double(this->Tb)/600.,
      double(this->vb)/1200.,
      double(this->ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->voc)/1200.,
      double(this->voc_stat)/1200.);
    Serial1.printf("%7.3f, %7.3f, %7.3f, %7.4f, %7.4f, %7.3f, %7.3f,",
      double(this->Tb)/600.,
      double(this->vb)/1200.,
      double(this->ib)/600.,
      double(this->soc)/16000.,
      double(this->soc_ekf)/16000.,
      double(this->voc)/1200.,
      double(this->voc_stat)/1200.);
  }
};
struct Flt3_st
{
  int16_t e_wrap_filt = 0; // Wrap model error, filtered, V
  uint16_t fltw = 0;    // Fault word
  uint16_t falw = 0;    // Fail word
  Flt3_st()
  {
    nominal();
  }
  // void assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen);
  void copy_from(Flt3_st input)
  {
    e_wrap_filt =input.e_wrap_filt;
    fltw = input.fltw;
    falw = input.falw;
  }
  void nominal()
  {
    this->e_wrap_filt = int16_t(0);
    this->fltw = uint16_t(2);
    this->falw = uint16_t(1);
  }
  void pretty_print(const String code)
  {
    Serial.printf("e_wrap_filt %7.3f\n", double(this->e_wrap_filt)/1200.);
    Serial.printf("fltw %d falw %d\n", this->fltw, this->falw);
  }
  void print()
  {
    Serial.printf("%7.3f, %d, %d,\n",
      double(this->e_wrap_filt)/1200.,
      this->fltw,
      this->falw);
    Serial1.printf("%7.3f, %d, %d,\n",
      double(this->e_wrap_filt)/1200.,
      this->fltw,
      this->falw);
  }
};

#endif