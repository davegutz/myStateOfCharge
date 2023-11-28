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

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

// #define t_float float

#include "local_config.h"
#include "Battery.h"
#include "hardware/SerialRAM.h"
#include "fault.h"
#include "command.h"

extern CommandPars cp;            // Various parameters shared at system level

#undef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

class Storage
{
public:
    Storage(){}

    Storage(const String &code, const String &description, const String &units, const boolean _uint8=false)
    {
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        is_eeram_ = false;
        units_ = units.substring(0, 10);
        is_uint8_t_ = _uint8;
    }

    Storage(const String &code, SerialRAM *ram, const String &description, const String &units, boolean _uint8=false)
    {
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        is_eeram_ = true;
        rP_ = ram;
        is_uint8_t_ = _uint8;
    }

    ~Storage(){}

    String code() { return code_; }

    const char* description() { return description_.c_str(); }

    virtual boolean is_off(){return true;};

    virtual void print(){};

    const char* units() { return units_.c_str(); }

protected:
    String code_;
    SerialRAM *rP_;
    address16b addr_;
    String units_;
    String description_;
    boolean is_eeram_;
    boolean is_uint8_t_;
};


class FloatStorage: public Storage
{
public:
    FloatStorage(){}

    FloatStorage(const String &code, const String &description, const String &units, const float min, const float max, const boolean _uint8=false, const float _default=0):
        Storage(code, description, units, _uint8)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    FloatStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max, boolean _uint8=false, const float _default=0):
        Storage(code, ram, description, units, _uint8)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    ~FloatStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(float);
    }

    float nominal() { return default_; }
    
    float get()
    {
        float value;
        rP_->get(addr_.a16, value);
        val_ = value; 
        return val_;
    }

    boolean is_corrupt()
    {
        return val_ >= max_ || val_ <= min_;
    }

    boolean is_off()
    {
        return val_ != default_;
    }

    void print()
    {
        sprintf(cp.buffer, "%-20s %7.3f is %7.3f, %10s (* %s)", description_.c_str(), default_, val_, units_.c_str(), code_.c_str());
        Serial.printf("%s\n", cp.buffer);
    }

    void print1()
    {
        sprintf(cp.buffer, "%-20s %7.3f is %7.3f, %10s (* %s)", description_.c_str(), default_, val_, units_.c_str(), code_.c_str());
        Serial1.printf("%s\n", cp.buffer);
    }
    
    void print_help()
    {
      Serial.printf(" *%s= %7.3f: %s, %s (%7.3f - %7.3f) [%7.3f]\n", code_.c_str(), val_, description_.c_str(), units_.c_str(), min_, max_, default_);
    }

    void set(float val)
    {
        val_ = val;
        if ( is_eeram_ ) rP_->put(addr_.a16, val_);
    }

    void set_default()
    {
        set(default_);
    }

protected:
    float val_;
    float default_;
    float min_;
    float max_;
};


class Int8tStorage: public Storage
{
public:
    Int8tStorage(){}

    Int8tStorage(const String &code, const String &description, const String &units, const int8_t min, const int8_t max, boolean _uint8=false, const int8_t _default=0):
        Storage(code, description, units, _uint8)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    Int8tStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const int8_t min, const int8_t max, boolean _uint8=false, const int8_t _default=0):
        Storage(code, ram, description, units, _uint8)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    ~Int8tStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(float);
    }

    int8_t nominal() { return default_; }
    
    int8_t get()
    {
        int8_t value;
        rP_->get(addr_.a16, value);
        val_ = value; 
        return val_;
    }

    boolean is_corrupt()
    {
        return val_ >= max_ || val_ <= min_;
    }

    boolean is_off()
    {
        return val_ != default_;
    }

    void print()
    {
        sprintf(cp.buffer, "%-20s %7d is %7d, %10s (* %s)", description_.c_str(), default_, val_, units_.c_str(), code_.c_str());
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1()
    {
        sprintf(cp.buffer, "%-20s %7d is %7d, %10s (* %s)", description_.c_str(), default_, val_, units_.c_str(), code_.c_str());
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help()
    {
      Serial.printf(" *%s= %d: %s, %s (%d - %d) [%d]\n", code_.c_str(), val_, description_.c_str(), units_.c_str(), min_, max_, default_);
    }
    
    void set(int8_t val)
    {
        val_ = val;
        if ( is_eeram_ ) rP_->put(addr_.a16, val_);
    }

    void set_default()
    {
        set(default_);
    }

protected:
    int8_t val_;
    int8_t min_;
    int8_t max_;
    int8_t default_;
};


class Uint8tStorage: public Storage
{
public:
    Uint8tStorage(){}

    Uint8tStorage(const String &code, const String &description, const String &units, const uint8_t min, const uint8_t max, const boolean _uint8=false, const uint8_t _default=0):
        Storage(code, description, units, _uint8)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    Uint8tStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const uint8_t min, const uint8_t max, boolean _uint8=false, const uint8_t _default=0):
        Storage(code, ram, description, units, _uint8)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    ~Uint8tStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(uint8_t);
    }

    uint8_t nominal() { return default_; }
    
    uint8_t get()
    {
        val_ = rP_->read(addr_.a16);
        return val_;
    }

    boolean is_corrupt()
    {
        return val_ >= max_ || val_ <= min_;
    }

    boolean is_off()
    {
        return val_ != default_;
    }

    void print()
    {
        sprintf(cp.buffer, "%-20s %7d is %7d, %10s (* %s)", description_.c_str(), default_, val_, units_.c_str(), code_.c_str());
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1()
    {
        sprintf(cp.buffer, "%-20s %7d is %7d, %10s (* %s)", description_.c_str(), default_, val_, units_.c_str(), code_.c_str());
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help()
    {
      Serial.printf(" *%s= %d: %s, %s (%df - %df) [%d]\n", code_.c_str(), val_, description_.c_str(), units_.c_str(), min_, max_, default_);
    }

    
    void set(uint8_t val)
    {
        val_ = val;
        if ( is_eeram_ ) rP_->write(addr_.a16, val_);
    }

    void set_default()
    {
        set(default_);
    }

protected:
    uint8_t val_;
    uint8_t min_;
    uint8_t max_;
    uint8_t default_;
};

// class FloatVariable : public Storage <float>
// {
// public:
//     FloatVariable(){}

//     FloatVariable(const String &description, const String &units, const float _default)
//     {
//         Storage <float>(description, units, _default, false);
//     }

//     FloatVariable(SerialRAM *ram, const String &description, const String &units, const float _default)
//     {
//         Storage <float>(ram, description, units, _default, false);
//     }

//     ~FloatVariable(){}
// protected:
// };


// Corruption test
template <typename T>
boolean is_val_corrupt(T val, T minval, T maxval)
{
    return isnan(val) || val < minval || val > maxval;
}


// Definition of structure to be saved in EERAM.  Many are needed to calibrate.  Others are
// needed to allow testing with resets.  Others allow application to remember dynamic
// tweaks.  Default values below are important:  they prevent junk
// behavior on initial build. Don't put anything in here that you can't live with normal running
// because could get set by testing and forgotten.  Not reset by hard reset
// SavedPars Class
class SavedPars
{
public:
    SavedPars();
    SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt);
    SavedPars(SerialRAM *ram);
    ~SavedPars();
    void init();
    void init_ram();
    friend Sensors;
    friend BatteryMonitor;
    friend BatterySim;

    // operators

    // parameter list
    float Amp() { return Amp_->get(); }
    float Cutback_gain_sclr() { return Cutback_gain_sclr_->get(); }
    float Dw() { return vb_table_bias_; }
    int debug() { return debug_;}
    double delta_q() { return delta_q_; }
    double delta_q_model() { return delta_q_model_; }
    float Freq() { return Freq_->get(); }
    uint8_t mon_chm() { return mon_chm_; }
    uint8_t sim_chm() { return sim_chm_; }
    float s_cap_mon() { return s_cap_mon_; }
    float s_cap_sim() { return s_cap_sim_; }
    float t_last() { return t_last_; }
    float t_last_model() { return t_last_model_; }
    float Ib_bias_all() { return Ib_bias_all_; }
    float Ib_bias_amp() { return Ib_bias_amp_; }
    float Ib_bias_noa() { return Ib_bias_noa_; }
    float ib_scale_amp() { return ib_scale_amp_; }
    float ib_scale_noa() { return ib_scale_noa_; }
    int8_t Ib_select() { return Ib_select_->get(); }
    int iflt() { return iflt_; }
    int ihis() { return ihis_; }
    float inj_bias() { return inj_bias_; }
    int isum() { return isum_; }
    uint8_t Modeling() { return Modeling_->get(); }
    float nP() { return nP_; }
    float nS() { return nS_; }
    uint8_t preserving() { return preserving_; }
    float Tb_bias_hdwe() { return Tb_bias_hdwe_; }
    time_t time_now() { return time_now_; }
    uint8_t type() { return type_; }
    float Vb_bias_hdwe() { return Vb_bias_hdwe_; }
    float Vb_scale() { return Vb_scale_; }
    float Zf() { return Zf_->get(); }
    float Zi() { return Zi_->get(); }
    float Zu() { return Zu_->get(); }

    // functions
    boolean is_corrupt();
    void large_reset() { reset_pars(); reset_flt(); reset_his(); }
    void reset_flt();
    void reset_his();
    void reset_pars();
    boolean mod_all_dscn() { return ( 111<modeling_ ); }                // Bare all
    boolean mod_any() { return ( mod_ib() || mod_tb() || mod_vb() ); }  // Modeing any
    boolean mod_any_dscn() { return ( 15<modeling_ ); }                 // Bare any
    boolean mod_ib() { return ( 1<<2 & modeling_ || mod_ib_all_dscn() ); }  // Using Sim as source of ib
    boolean mod_ib_all_dscn() { return ( 191<modeling_ ); }             // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_amp_dscn() { return ( 1<<6 & modeling_ ); }          // Nothing connected to amp ib sensors in I2C on SDA/SCL
    boolean mod_ib_any_dscn() { return ( mod_ib_amp_dscn() || mod_ib_noa_dscn() ); }  // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_noa_dscn() { return ( 1<<7 & modeling_ ); }          // Nothing connected to noa ib sensors in I2C on SDA/SCL
    boolean mod_none() { return ( 0==modeling_ ); }                     // Using all
    boolean mod_none_dscn() { return ( 16>modeling_ ); }                // Bare nothing
    boolean mod_tb() { return ( 1<<0 & modeling_ || mod_tb_dscn() ); }  // Using Sim as source of tb
    boolean mod_tb_dscn() { return ( 1<<4 & modeling_ ); }              // Nothing connected to one-wire Tb sensor on D6
    boolean mod_vb() { return ( 1<<1 & modeling_ || mod_vb_dscn() ); }  // Using Sim as source of vb
    boolean mod_vb_dscn() { return ( 1<<5 & modeling_ ); }              // Nothing connected to vb on A1
    // get
    #ifdef CONFIG_47L16
        float get_Amp() { return Amp_->get(); }
        float get_Cutback_gain_sclr() { return Cutback_gain_sclr_->get(); }
        void get_Dw() { float value; rP_->get(Dw_eeram_.a16, value); vb_table_bias_ = value; }
        void get_debug() { int value; rP_->get(debug_eeram_.a16, value); debug_ = value; }
        void get_delta_q() { double value; rP_->get(delta_q_eeram_.a16, value); delta_q_ = value; }
        void get_delta_q_model() { double value; rP_->get(delta_q_model_eeram_.a16, value); delta_q_model_ = value; }
        float get_Freq() { return Freq_->get(); }
        void get_Ib_bias_all() { float value; rP_->get(Ib_bias_all_eeram_.a16, value); Ib_bias_all_ = value; }
        void get_Ib_bias_amp() { float value; rP_->get(Ib_bias_amp_eeram_.a16, value); Ib_bias_amp_ = value; }
        void get_Ib_bias_noa() { float value; rP_->get(Ib_bias_noa_eeram_.a16, value); Ib_bias_noa_ = value; }
        void get_ib_scale_amp() { float value; rP_->get(ib_scale_amp_eeram_.a16, value); ib_scale_amp_ = value; }
        void get_ib_scale_noa() { float value; rP_->get(ib_scale_noa_eeram_.a16, value); ib_scale_noa_ = value; }
        void get_Ib_select() { return Ib_select_->get(); }
        void get_iflt() { int value; rP_->get(iflt_eeram_.a16, value); iflt_ = value; }
        void get_ihis() { int value; rP_->get(ihis_eeram_.a16, value); ihis_ = value; }
        void get_inj_bias() { float value; rP_->get(inj_bias_eeram_.a16, value); inj_bias_ = value; }
        void get_isum() { int value; rP_->get(isum_eeram_.a16, value); isum_ = value; }
        uint8_t get_Modeling() { return Modeling_->get(); }
        void get_mon_chm() { mon_chm_ = rP_->read(mon_chm_eeram_.a16); }
        void get_nP() { float value; rP_->get(nP_eeram_.a16, value); nP_ = value; }
        void get_nS() { float value; rP_->get(nS_eeram_.a16, value); nS_ = value; }
        void get_preserving() { preserving_ = rP_->read(preserving_eeram_.a16); }
        void get_sim_chm() { sim_chm_ = rP_->read(sim_chm_eeram_.a16); }
        void get_s_cap_mon() { float value; rP_->get(s_cap_mon_eeram_.a16, value); s_cap_mon_ = value; }
        void get_s_cap_sim() { float value; rP_->get(s_cap_sim_eeram_.a16, value); s_cap_sim_ = value; }
        void get_Tb_bias_hdwe() { float value; rP_->get(Tb_bias_hdwe_eeram_.a16, value); Tb_bias_hdwe_ = value; }
        void get_time_now() { time_t value; rP_->get(time_now_eeram_.a16, value); time_now_ = value; Time.setTime(value); }
        void get_type() { type_ = rP_->read(type_eeram_.a16); }
        void get_t_last() { float value; rP_->get(t_last_eeram_.a16, value); t_last_ = value; }
        void get_t_last_model() { float value; rP_->get(t_last_model_eeram_.a16, value); t_last_model_ = value; }
        void get_Vb_bias_hdwe() { float value; rP_->get(Vb_bias_hdwe_eeram_.a16, value); Vb_bias_hdwe_ = value; }
        void get_Vb_scale() { float value; rP_->get(Vb_scale_eeram_.a16, value); Vb_scale_ = value; }
        float get_Zf() { return Zf_->get(); }
        int8_t get_Zi() { return Zi_->get(); }
        uint8_t get_Zu() { return Zu_->get(); }
        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        uint16_t next() { return next_; }
        void load_all();
    #else
        float get_Amp() { return Amp_->get(); }
        float get_Cutback_gain_sclr() { return Cutback_gain_sclr_->get(); }
        float get_Freq() { return Freq_->get(); }
        uint8_t get_Modeling() { return Modeling_->get(); }
        float get_Zf() { return Zf_->get(); }
        int8_t get_Zi() { return Zi_->get(); }
        uint8_t get_Zu() { return Zu_->get(); }
    #endif
    //
    void mem_print();
    uint16_t nflt() { return nflt_; }
    uint16_t nhis() { return nhis_; }
    void nominalize_fault_array();
    void nominalize_history_array();
    int num_diffs();
    void pretty_print(const boolean all);
    void print_fault_array();
    void print_fault_header();
    void print_history_array();
    // put
    #ifndef CONFIG_47L16
        void put_all_dynamic();
        void put_Amp(const float input) { Amp_->set(input); }
        void put_Cutback_gain_sclr(const float input) { Cutback_gain_sclr_->set(input); }
        void put_Dw(const float input) { vb_table_bias_ = input; }
        void put_debug(const int input) { debug_ = input; }
        void put_delta_q(const double input) { delta_q_ = input; }
        void put_delta_q() {}
        void put_delta_q_model(const double input) { delta_q_model_ = input; }
        void put_delta_q_model() {}
        void put_Freq(const float input) { Freq_->set(input); }
        void put_Ib_bias_all(const float input) { Ib_bias_all_ = input; }
        void put_Ib_bias_amp(const float input) { Ib_bias_amp_ = input; }
        void put_Ib_bias_noa(const float input) { Ib_bias_noa_ = input; }
        void put_ib_scale_amp(const float input) { ib_scale_amp_ = input; }
        void put_ib_scale_noa(const float input) { ib_scale_noa_ = input; }
        void put_Ib_select(const int8_t input) { Ib_select_->set(input); }
        void put_iflt(const int input) { iflt_ = input; }
        void put_ihis(const int input) { ihis_ = input; }
        void put_inj_bias(const float input) { inj_bias_ = input; }
        void put_isum(const int input) { isum_ = input; }
        void put_Modeling(const uint8_t input) { Modeling_->set(input); }
        void put_mon_chm(const uint8_t input) { mon_chm_ = input; }
        void put_mon_chm() {}
        void put_nP(const float input) { nP_ = input; }
        void put_nS(const float input) { nS_ = input; }
        void put_preserving(const uint8_t input) { preserving_ = input; }
        void put_sim_chm(const uint8_t input) { sim_chm_ = input; }
        void put_sim_chm() {}
        void put_s_cap_mon(const float input) { s_cap_mon_ = input; }
        void put_s_cap_sim(const float input) { s_cap_sim_ = input; }
        void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe_ = input; }
        void put_time_now(const time_t input) { time_now_ = input; }
        void put_type(const uint8_t input) { type_ = input; }
        void put_t_last(const float input) { t_last_ = input; }
        void put_t_last() {}
        void put_t_last_model(const float input) { t_last_model_ = input; }
        void put_t_last_model() {}
        void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe_ = input; }
        void put_Vb_scale(const float input) { Vb_scale_ = input; }
        void put_Zf(const float input) { Zf_->set(input); }
        void put_Zi(const int8_t input) { Zi_->set(input); }
        void put_Zu(const uint8_t input) { Zu_->set(input); }
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_all_dynamic();
        void put_Amp(const float input) { Amp_->set(input); }
        void put_Cutback_gain_sclr(const float input) { Cutback_gain_sclr_->set(input); }
        void put_Dw(const float input) { rP_->put(Dw_eeram_.a16, input); vb_table_bias_ = input; }
        void put_debug(const int input) { rP_->put(debug_eeram_.a16, input); debug_ = input; }
        void put_delta_q(const double input) { rP_->put(delta_q_eeram_.a16, input); delta_q_ = input; }
        void put_delta_q() { rP_->put(delta_q_eeram_.a16, delta_q_); }
        void put_delta_q_model(const double input) { rP_->put(delta_q_model_eeram_.a16, input); delta_q_model_ = input; }
        void put_delta_q_model() { rP_->put(delta_q_model_eeram_.a16, delta_q_model_); }
        void put_Freq(const float input) { Freq_->set(input); }
        void put_Ib_bias_all(const float input) { rP_->put(Ib_bias_all_eeram_.a16, input); Ib_bias_all_ = input; }
        void put_Ib_bias_amp(const float input) { rP_->put(Ib_bias_amp_eeram_.a16, input); Ib_bias_amp_ = input; }
        void put_Ib_bias_noa(const float input) { rP_->put(Ib_bias_noa_eeram_.a16, input); Ib_bias_noa_ = input; }
        void put_ib_scale_amp(const float input) { rP_->put(ib_scale_amp_eeram_.a16, input); ib_scale_amp_ = input; }
        void put_ib_scale_noa(const float input) { rP_->put(ib_scale_noa_eeram_.a16, input); ib_scale_noa_ = input; }
        void put_Ib_select(const int8_t input) { Ib_select_->set(input); }
        void put_ib_select(const int8_t input) { rP_->put(ib_select_eeram_.a16, input); ib_select_ = input; }
        void put_iflt(const int input) { rP_->put(iflt_eeram_.a16, input); iflt_ = input; }
        void put_ihis(const int input) { rP_->put(ihis_eeram_.a16, input); ihis_ = input; }
        void put_inj_bias(const float input) { rP_->put(inj_bias_eeram_.a16, input); inj_bias_ = input; }
        void put_isum(const int input) { rP_->put(isum_eeram_.a16, input); isum_ = input; }
        void put_Modeling(const uint8_t input) { Modeling_->set(input); }
        void put_mon_chm(const uint8_t input) { rP_->write(mon_chm_eeram_.a16, input); mon_chm_ = input; }
        void put_mon_chm() { rP_->write(mon_chm_eeram_.a16, mon_chm_); }
        void put_nP(const float input) { rP_->put(nP_eeram_.a16, input); nP_ = input; }
        void put_nS(const float input) { rP_->put(nS_eeram_.a16, input); nS_ = input; }
        void put_preserving(const uint8_t input) { rP_->write(preserving_eeram_.a16, input); preserving_ = input; }
        void put_sim_chm(const uint8_t input) { rP_->write(sim_chm_eeram_.a16, input); sim_chm_ = input; }
        void put_sim_chm() { rP_->write(sim_chm_eeram_.a16, sim_chm_); }
        void put_s_cap_mon(const float input) { rP_->put(s_cap_mon_eeram_.a16, input); s_cap_mon_ = input; }
        void put_s_cap_sim(const float input) { rP_->put(s_cap_sim_eeram_.a16, input); s_cap_sim_ = input; }
        void put_Tb_bias_hdwe(const float input) { rP_->put(Tb_bias_hdwe_eeram_.a16, input); Tb_bias_hdwe_ = input; }
        void put_time_now(const time_t input) { rP_->put(time_now_eeram_.a16, input); time_now_ = input; Time.setTime(time_now_); }
        void put_type(const uint8_t input) { rP_->write(type_eeram_.a16, input); type_ = input; }
        void put_t_last(const float input) { rP_->put(t_last_eeram_.a16, input); t_last_ = input; }
        void put_t_last() { rP_->put(t_last_eeram_.a16, t_last_); }
        void put_t_last_model(const float input) { rP_->put(t_last_model_eeram_.a16, input); t_last_model_ = input; }
        void put_t_last_model() { rP_->put(t_last_model_eeram_.a16, t_last_model_); }
        void put_Vb_bias_hdwe(const float input) { rP_->put(Vb_bias_hdwe_eeram_.a16, input); Vb_bias_hdwe_ = input; }
        void put_Vb_scale(const float input) { rP_->put(Vb_scale_eeram_.a16, input); Vb_scale_ = input; }
        void put_Zf(const float input) { Zf_->set(input); }
        void put_Zi(const int8_t input) { Zi_->set(input); }
        void put_Zu(const uint8_t input) { Zu_->set(input); }
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 1<<3 & modeling_ ); } // Driving signal injection completely using software inj_bias 
    FloatStorage *Amp_;
    FloatStorage *Cutback_gain_sclr_;
    FloatStorage *Freq_;
    Int8tStorage *Ib_select_;
    Uint8tStorage *Modeling_;
    FloatStorage *Zf_;
    Int8tStorage *Zi_;
    Uint8tStorage *Zu_;
protected:
    int debug_;             // Level of debug printing
    double delta_q_;        // Charge change since saturated, C
    double delta_q_model_;  // Charge change since saturated, C
    float freq_;            // Injected frequency, Hz (0-2)
    float Ib_bias_all_;     // Bias on all shunt sensors, A
    float Ib_bias_amp_;     // Calibration adder of amplified shunt sensor, A
    float Ib_bias_noa_;     // Calibration adder of non-amplified shunt sensor, A
    float ib_scale_amp_;    // Calibration scalar of amplified shunt sensor
    float ib_scale_noa_;    // Calibration scalar of non-amplified shunt sensor
    int8_t ib_select_;      // Force current sensor (-1=non-amp, 0=auto, 1=amp)
    int iflt_;              // Fault snap location.   Begins at -1 because first action is to increment iflt
    int ihis_;              // History location.   Begins at -1 because first action is to increment ihis
    float inj_bias_;        // Constant bias, A
    int isum_;              // Summary location.   Begins at -1 because first action is to increment isum
    uint8_t mon_chm_;       // Monitor battery chemistry type
    float nP_;              // Number of parallel batteries in bank, e.g. '2P1S'
    float nS_;              // Number of series batteries in bank, e.g. '2P1S'
    uint8_t preserving_;    // Preserving fault buffer
    uint8_t sim_chm_;       // Simulation battery chemistry type
    float s_cap_mon_;       // Scalar on battery monitor size
    float s_cap_sim_;       // Scalar on battery model size
    float Tb_bias_hdwe_;    // Bias on Tb sensor, deg C
    time_t time_now_;       // Time now, Unix time since epoch
    uint8_t type_;          // Injected waveform type.   0=sine, 1=square, 2=triangle
    float Vb_bias_hdwe_;    // Calibrate Vb, V
    float Vb_scale_;        // Calibrate Vb scale
    float vb_table_bias_;   // Battery monitor curve bias, V
    float t_last_;          // Updated value of battery temperature injection when sp.modeling_ and proper wire connections made, deg C
    float t_last_model_;    // Battery temperature past value for rate limit memory, deg C
    uint8_t modeling_;       // Driving saturation calculation with model.  Bits specify which signals use model
    #ifndef CONFIG_47L16
        Flt_st *fault_;
        Flt_st *history_;
    #else
        address16b cutback_gain_sclr_eeram_;
        address16b debug_eeram_;
        address16b delta_q_eeram_;
        address16b delta_q_model_eeram_;
        address16b freq_eeram_;
        address16b hys_scale_eeram_;
        address16b Ib_bias_all_eeram_;
        address16b Ib_bias_amp_eeram_;
        address16b Ib_bias_noa_eeram_;
        address16b ib_scale_amp_eeram_;
        address16b ib_scale_noa_eeram_;
        address16b ib_select_eeram_;
        address16b iflt_eeram_;
        address16b ihis_eeram_;
        address16b inj_bias_eeram_;
        address16b isum_eeram_;
        address16b modeling_eeram_;
        address16b mon_chm_eeram_;
        address16b nP_eeram_;
        address16b nS_eeram_;
        address16b preserving_eeram_;
        address16b Dw_eeram_;
        address16b sim_chm_eeram_;
        address16b s_cap_mon_eeram_;
        address16b s_cap_sim_eeram_;
        address16b Tb_bias_hdwe_eeram_;
        address16b time_now_eeram_;
        address16b type_eeram_;
        address16b t_last_eeram_;
        address16b t_last_model_eeram_;
        address16b Vb_bias_hdwe_eeram_;
        address16b Vb_scale_eeram_;
        SerialRAM *rP_;
        Flt_ram *fault_;
        Flt_ram *history_;
    #endif
    uint16_t next_;
    uint16_t nflt_;         // Length of Flt_ram array for faults
    uint16_t nhis_;         // Length of Flt_ram array for history
    uint16_t size_;
    Storage *Z_[50];
};

#endif
