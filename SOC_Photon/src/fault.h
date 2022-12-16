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
#include "./hardware/SerialRAM.h"

String time_long_2_str(const unsigned long current_time, char *tempStr);

// SRAM retention summary
struct Flt_st
{
  unsigned long t = 1L; // Timestamp seconds since start of epoch
  int16_t Tb_hdwe = 0;  // Battery temperature, hardware, C
  int16_t vb_hdwe = 0;  // Battery measured potential, hardware, V
  int16_t ib_amp_hdwe = 0;  // Battery measured input current, amp, A
  int16_t ib_noa_hdwe = 0;  // Battery measured input current, no amp, A
  int16_t Tb = 0;       // Battery temperature, filtered, C
  int16_t vb = 0;       // Battery measured potential, filtered, V
  int16_t ib = 0;       // Battery measured input current, filtered, A
  int16_t soc = 0;      // Battery state of charge, free Coulomb counting algorithm, frac
  int16_t soc_ekf = 0;  // Battery state of charge, ekf, frac
  int16_t voc = 0;      // Battery open circuit voltage measured vb-ib*Z, V
  int16_t voc_stat = 0; // Stored charge voltage from measurement, V
  int16_t e_wrap_filt = 0; // Wrap model error, filtered, V
  uint16_t fltw = 0;    // Fault word
  uint16_t falw = 0;    // Fail word
  unsigned long dummy = 0;  // padding to absorb Wire.write corruption
  void assign(const time32_t now, BatteryMonitor *Mon, Sensors *Sen);
  void copy_from(Flt_st input);
  void nominal();
  void pretty_print(const String code);
  void print(const String code);
};

class Flt_ram : public Flt_st
{
public:
  Flt_ram();
  ~Flt_ram();
  void get_t()            { unsigned long value;  rP_->get(t_eeram_.a16, value);            t = value; };
  void get_Tb_hdwe()      { float value;          rP_->get(Tb_hdwe_eeram_.a16, value);      Tb_hdwe = value; };
  void get_vb_hdwe()      { float value;          rP_->get(vb_hdwe_eeram_.a16, value);      vb_hdwe = value; };
  void get_ib_amp_hdwe()  { float value;          rP_->get(ib_amp_hdwe_eeram_.a16, value);  ib_amp_hdwe = value; };
  void get_ib_noa_hdwe()  { float value;          rP_->get(ib_noa_hdwe_eeram_.a16, value);  ib_noa_hdwe = value; };
  void get_Tb()           { float value;          rP_->get(Tb_eeram_.a16, value);           Tb = value; };
  void get_vb()           { float value;          rP_->get(vb_eeram_.a16, value);           vb = value; };
  void get_ib()           { float value;          rP_->get(ib_eeram_.a16, value);           ib = value; };
  void get_soc()          { float value;          rP_->get(soc_eeram_.a16, value);          soc = value; };
  void get_soc_ekf()      { float value;          rP_->get(soc_ekf_eeram_.a16, value);      soc_ekf = value; };
  void get_voc()          { float value;          rP_->get(voc_eeram_.a16, value);          voc = value; };
  void get_voc_stat()     { float value;          rP_->get(voc_stat_eeram_.a16, value);     voc_stat = value; };
  void get_e_wrap_filt()  { float value;          rP_->get(e_wrap_filt_eeram_.a16, value);  e_wrap_filt = value; };
  void get_fltw()         { float value;          rP_->get(fltw_eeram_.a16, value);         fltw = value; };
  void get_falw()         { float value;          rP_->get(falw_eeram_.a16, value);         falw = value; };
  void instantiate(SerialRAM *ram, uint16_t *next);
  void load_all();
protected:
  SerialRAM *rP_;
  address16b t_eeram_;
  address16b Tb_hdwe_eeram_;
  address16b vb_hdwe_eeram_;
  address16b ib_amp_hdwe_eeram_;
  address16b ib_noa_hdwe_eeram_;
  address16b Tb_eeram_;
  address16b vb_eeram_;
  address16b ib_eeram_;
  address16b soc_eeram_;
  address16b soc_ekf_eeram_;
  address16b voc_eeram_;
  address16b voc_stat_eeram_;
  address16b e_wrap_filt_eeram_;
  address16b fltw_eeram_;
  address16b falw_eeram_;
};

#endif