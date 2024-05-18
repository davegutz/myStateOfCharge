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


#ifndef _FAULT_H
#define _FAULT_H

#include "Battery.h"
#include "./hardware/SerialRAM.h"

String time_long_2_str(const time_t current_time, char *tempStr);

// SRAM retention summary
struct Flt_st
{
  unsigned long t_flt = 1UL; // Timestamp seconds since start of epoch
  int16_t Tb_hdwe = 0;  // Battery temperature, hardware, C
  int16_t vb_hdwe = 0;  // Battery measured potential, hardware, V
  int16_t ib_amp_hdwe = 0;  // Battery measured input current, amp, A
  int16_t ib_noa_hdwe = 0;  // Battery measured input current, no amp, A
  int16_t Tb = 0;       // Battery temperature, filtered, C
  int16_t vb = 0;       // Battery measured potential, filtered, V
  int16_t ib = 0;       // Battery measured input current, filtered, A
  int16_t soc = 0;      // Battery state of charge, free Coulomb counting algorithm, frac
  int16_t soc_min = 0;  // Battery min state of charge, frac
  int16_t soc_ekf = 0;  // Battery state of charge, ekf, frac
  int16_t voc = 0;      // Battery open circuit voltage measured vb-ib*Z, V
  int16_t voc_stat = 0; // Stored charge voltage from measurement, V
  int16_t e_wrap_filt = 0; // Wrap model error, filtered, V
  uint16_t fltw = 0;    // Fault word
  uint16_t falw = 0;    // Fail word
  unsigned long dummy = 0;  // padding to absorb Wire.write corruption
  void assign(const unsigned long now, BatteryMonitor *Mon, Sensors *Sen);
  void copy_to_Flt_ram_from(Flt_st input);
  void get() {};
  void nominal();
  void pretty_print(const String code);
  void print_flt(const String code);
  void put(Flt_st source);
  void put_nominal();
};

class Flt_ram : public Flt_st
{
public:
  Flt_ram();
  ~Flt_ram();
  #ifdef HDWE_47L16_EERAM
    void get_t_flt()        { unsigned long value;  rP_->get(t_flt_eeram_.a16, value);        t_flt = value; };
    void get_Tb_hdwe()      { int16_t value;        rP_->get(Tb_hdwe_eeram_.a16, value);      Tb_hdwe = value; };
    void get_vb_hdwe()      { int16_t value;        rP_->get(vb_hdwe_eeram_.a16, value);      vb_hdwe = value; };
    void get_ib_amp_hdwe()  { int16_t value;        rP_->get(ib_amp_hdwe_eeram_.a16, value);  ib_amp_hdwe = value; };
    void get_ib_noa_hdwe()  { int16_t value;        rP_->get(ib_noa_hdwe_eeram_.a16, value);  ib_noa_hdwe = value; };
    void get_Tb()           { int16_t value;        rP_->get(Tb_eeram_.a16, value);           Tb = value; };
    void get_vb()           { int16_t value;        rP_->get(vb_eeram_.a16, value);           vb = value; };
    void get_ib()           { int16_t value;        rP_->get(ib_eeram_.a16, value);           ib = value; };
    void get_soc()          { int16_t value;        rP_->get(soc_eeram_.a16, value);          soc = value; };
    void get_soc_min()      { int16_t value;        rP_->get(soc_min_eeram_.a16, value);      soc_min = value; };
    void get_soc_ekf()      { int16_t value;        rP_->get(soc_ekf_eeram_.a16, value);      soc_ekf = value; };
    void get_voc()          { int16_t value;        rP_->get(voc_eeram_.a16, value);          voc = value; };
    void get_voc_stat()     { int16_t value;        rP_->get(voc_stat_eeram_.a16, value);     voc_stat = value; };
    void get_e_wrap_filt()  { int16_t value;        rP_->get(e_wrap_filt_eeram_.a16, value);  e_wrap_filt = value; };
    void get_fltw()         { uint16_t value;       rP_->get(fltw_eeram_.a16, value);         fltw = value; };
    void get_falw()         { uint16_t value;       rP_->get(falw_eeram_.a16, value);         falw = value; };
    void instantiate(SerialRAM *ram, uint16_t *next);
  #endif

  void get();
  void put(const Flt_st input);
  void put_nominal();

  #ifndef HDWE_47L16_EERAM
    void put_t_flt(const unsigned long value)     { t_flt = value; };
    void put_Tb_hdwe(const int16_t value)         { Tb_hdwe = value; };
    void put_vb_hdwe(const int16_t value)         { vb_hdwe = value; };
    void put_ib_amp_hdwe(const int16_t value)     { ib_amp_hdwe = value; };
    void put_ib_noa_hdwe(const int16_t value)     { ib_noa_hdwe = value; };
    void put_Tb(const int16_t value)              { Tb = value; };
    void put_vb(const int16_t value)              { vb = value; };
    void put_ib(const int16_t value)              { ib = value; };
    void put_soc(const int16_t value)             { soc = value; };
    void put_soc_min(const int16_t value)         { soc_min = value; };
    void put_soc_ekf(const int16_t value)         { soc_ekf = value; };
    void put_voc(const int16_t value)             { voc = value; };
    void put_voc_stat(const int16_t value)        { voc_stat = value; };
    void put_e_wrap_filt(const int16_t value)     { e_wrap_filt = value; };
    void put_fltw(const uint16_t value)           { fltw = value; };
    void put_falw(const uint16_t value)           { falw = value; };
#else
    void put_t_flt()        { rP_->put(t_flt_eeram_.a16, t_flt); };
    void put_Tb_hdwe()      { rP_->put(Tb_hdwe_eeram_.a16, Tb_hdwe); };
    void put_vb_hdwe()      { rP_->put(vb_hdwe_eeram_.a16, vb_hdwe); };
    void put_ib_amp_hdwe()  { rP_->put(ib_amp_hdwe_eeram_.a16, ib_amp_hdwe); };
    void put_ib_noa_hdwe()  { rP_->put(ib_noa_hdwe_eeram_.a16, ib_noa_hdwe); };
    void put_Tb()           { rP_->put(Tb_eeram_.a16, Tb); };
    void put_vb()           { rP_->put(vb_eeram_.a16, vb); };
    void put_ib()           { rP_->put(ib_eeram_.a16, ib); };
    void put_soc()          { rP_->put(soc_eeram_.a16, soc); };
    void put_soc_min()      { rP_->put(soc_min_eeram_.a16, soc_min); };
    void put_soc_ekf()      { rP_->put(soc_ekf_eeram_.a16, soc_ekf); };
    void put_voc()          { rP_->put(voc_eeram_.a16, voc); };
    void put_voc_stat()     { rP_->put(voc_stat_eeram_.a16, voc_stat); };
    void put_e_wrap_filt()  { rP_->put(e_wrap_filt_eeram_.a16, e_wrap_filt); };
    void put_fltw()         { rP_->put(fltw_eeram_.a16, fltw); };
    void put_falw()         { rP_->put(falw_eeram_.a16, falw); };
    void put_t_flt(const unsigned long value)     { rP_->put(t_flt_eeram_.a16, value);        t_flt = value; };
    void put_Tb_hdwe(const int16_t value)         { rP_->put(Tb_hdwe_eeram_.a16, value);      Tb_hdwe = value; };
    void put_vb_hdwe(const int16_t value)         { rP_->put(vb_hdwe_eeram_.a16, value);      vb_hdwe = value; };
    void put_ib_amp_hdwe(const int16_t value)     { rP_->put(ib_amp_hdwe_eeram_.a16, value);  ib_amp_hdwe = value; };
    void put_ib_noa_hdwe(const int16_t value)     { rP_->put(ib_noa_hdwe_eeram_.a16, value);  ib_noa_hdwe = value; };
    void put_Tb(const int16_t value)              { rP_->put(Tb_eeram_.a16, value);           Tb = value; };
    void put_vb(const int16_t value)              { rP_->put(vb_eeram_.a16, value);           vb = value; };
    void put_ib(const int16_t value)              { rP_->put(ib_eeram_.a16, value);           ib = value; };
    void put_soc(const int16_t value)             { rP_->put(soc_eeram_.a16, value);          soc = value; };
    void put_soc_min(const int16_t value)         { rP_->put(soc_min_eeram_.a16, value);      soc_min = value; };
    void put_soc_ekf(const int16_t value)         { rP_->put(soc_ekf_eeram_.a16, value);      soc_ekf = value; };
    void put_voc(const int16_t value)             { rP_->put(voc_eeram_.a16, value);          voc = value; };
    void put_voc_stat(const int16_t value)        { rP_->put(voc_stat_eeram_.a16, value);     voc_stat = value; };
    void put_e_wrap_filt(const int16_t value)     { rP_->put(e_wrap_filt_eeram_.a16, value);  e_wrap_filt = value; };
    void put_fltw(const uint16_t value)           { rP_->put(fltw_eeram_.a16, value);         fltw = value; };
    void put_falw(const uint16_t value)           { rP_->put(falw_eeram_.a16, value);         falw = value; };
  #endif

protected:
  SerialRAM *rP_;
  #ifdef HDWE_47L16_EERAM
    address16b t_flt_eeram_;
    address16b Tb_hdwe_eeram_;
    address16b vb_hdwe_eeram_;
    address16b ib_amp_hdwe_eeram_;
    address16b ib_noa_hdwe_eeram_;
    address16b Tb_eeram_;
    address16b vb_eeram_;
    address16b ib_eeram_;
    address16b soc_eeram_;
    address16b soc_min_eeram_;
    address16b soc_ekf_eeram_;
    address16b voc_eeram_;
    address16b voc_stat_eeram_;
    address16b e_wrap_filt_eeram_;
    address16b fltw_eeram_;
    address16b falw_eeram_;
  #endif
};

#endif