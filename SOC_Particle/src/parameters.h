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

#include "local_config.h"
#include "Battery.h"
#include "fault.h"
#include "PrinterPars.h"
#include "Z.h"

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
    SavedPars(SerialRAM *ram);
    SavedPars(Flt_st *hist, const uint16_t nhis, Flt_st *faults, const uint16_t nflt);
    ~SavedPars();
 
    // operators

    // parameter list
    float Amp() { return Amp_z; }
    float Cutback_gain_sclr() { return Cutback_gain_sclr_z; }
    int Debug() { return Debug_z;}
    double Delta_q() { return Delta_q_z;}
    double Delta_q_model() { return Delta_q_model_z;}
    float Dw() { return Dw_z; }
    float Freq() { return Freq_z; }
    uint8_t Mon_chm() { return Mon_chm_z; }
    float S_cap_mon() { return S_cap_mon_z; }
    float S_cap_sim() { return S_cap_sim_z; }
    uint8_t Sim_chm() { return Sim_chm_z; }
    float Ib_bias_all() { return Ib_bias_all_z; }
    float Ib_bias_amp() { return Ib_bias_amp_z; }
    float Ib_bias_noa() { return Ib_bias_noa_z; }
    float ib_scale_amp() { return Ib_scale_amp_z; }
    float ib_scale_noa() { return Ib_scale_noa_z; }
    int8_t Ib_select() { return Ib_select_z; }
    uint16_t Iflt() { return Iflt_z; }
    uint16_t Ihis() { return Ihis_z; }
    float Inj_bias() { return Inj_bias_z; }
    uint16_t Isum() { return Isum_z; }
    uint8_t Modeling() { return Modeling_z; }
    float nP() { return nP_z; }
    float nS() { return nS_z; }
    uint8_t Preserving() { return Preserving_z; }
    float Tb_bias_hdwe() { return Tb_bias_hdwe_z; }
    unsigned long Time_now() { return Time_now_z; }
    uint8_t type() { return Type_z; }
    float T_state() { return T_state_z; }
    float T_state_model() { return T_state_model_z; }
    float Vb_bias_hdwe() { return Vb_bias_hdwe_z; }
    float Vb_scale() { return Vb_scale_z; }

    // functions
    void init_z();
    boolean is_corrupt();
    void large_reset() { reset_pars(); reset_flt(); reset_his(); }
    void reset_flt();
    void reset_his();
    void reset_pars();
    boolean mod_all_dscn() { return ( 111<Modeling() ); }                // Bare all
    boolean mod_any() { return ( mod_ib() || mod_tb() || mod_vb() ); }  // Modeing any
    boolean mod_any_dscn() { return ( 15<Modeling() ); }                 // Bare any
    boolean mod_ib() { return ( 1<<2 & Modeling() || mod_ib_all_dscn() ); }  // Using Sim as source of ib
    boolean mod_ib_all_dscn() { return ( 191<Modeling() ); }             // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_amp_dscn() { return ( 1<<6 & Modeling() ); }          // Nothing connected to amp ib sensors in I2C on SDA/SCL
    boolean mod_ib_any_dscn() { return ( mod_ib_amp_dscn() || mod_ib_noa_dscn() ); }  // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_noa_dscn() { return ( 1<<7 & Modeling() ); }          // Nothing connected to noa ib sensors in I2C on SDA/SCL
    boolean mod_none() { return ( 0==Modeling() ); }                     // Using all
    boolean mod_none_dscn() { return ( 16>Modeling() ); }                // Bare nothing
    boolean mod_tb() { return ( 1<<0 & Modeling() || mod_tb_dscn() ); }  // Using Sim as source of tb
    boolean mod_tb_dscn() { return ( 1<<4 & Modeling() ); }              // Nothing connected to one-wire Tb sensor on D6
    boolean mod_vb() { return ( 1<<1 & Modeling() || mod_vb_dscn() ); }  // Using Sim as source of vb
    boolean mod_vb_dscn() { return ( 1<<5 & Modeling() ); }              // Nothing connected to vb on A1

    #ifdef CONFIG_47L16_EERAM
        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        uint16_t next() { return next_; }
        void load_all();
    #endif

    void mem_print();
    uint16_t nflt() { return nflt_; }
    uint16_t nhis() { return nhis_; }
    uint16_t nsum() { return nsum_; }
    void nominalize_fault_array();
    void nominalize_history_array();
    int num_diffs();
    void pretty_print(const boolean all);
    void pretty_print_modeling();
    void print_fault_array();
    void print_fault_header();
    void print_history_array();

    // put
    void put_all_dynamic();
    void put_Amp(const float input) { Amp_p->set(input); }
    void put_Cutback_gain_sclr(const float input) { Cutback_gain_sclr_p->set(input); }
    void put_Debug(const int input) { Debug_p->set(input); }
    void put_Delta_q(const double input) { Delta_q_p->set(input); }
    void put_Delta_q() {}
    void put_Delta_q_model(const double input) { Delta_q_model_p->set(input); }
    void put_Delta_q_model() {}
    void put_Dw(const float input) { Dw_p->set(input); }
    void put_Freq(const float input) { Freq_p->set(input); }
    void put_Ib_bias_all(const float input) { Ib_bias_all_p->set(input); }
    void put_Ib_bias_amp(const float input) { Ib_bias_amp_p->set(input); }
    void put_Ib_bias_noa(const float input) { Ib_bias_noa_p->set(input); }
    void put_Ib_scale_amp(const float input) { Ib_scale_amp_p->set(input); }
    void put_Ib_scale_noa(const float input) { Ib_scale_noa_p->set(input); }
    void put_Ib_select(const int8_t input) { Ib_select_p->set(input); }
    void put_Iflt(const int input) { Iflt_p->set(input); }
    void put_Ihis(const int input) { Ihis_p->set(input); }
    void put_Isum(const int input) { Isum_p->set(input); }
    void put_Inj_bias(const float input) { Inj_bias_p->set(input); }
    void put_Mon_chm(const uint8_t input) { Mon_chm_p->set(input); }
    void put_Mon_chm() {}
    void put_nP(const float input) { nP_p->set(input); }
    void put_nS(const float input) { nS_p->set(input); }
    void put_Preserving(const uint8_t input) { Preserving_p->set(input); }
    void put_Sim_chm(const uint8_t input) { Sim_chm_p->set(input); }
    void put_Sim_chm() {}
    void put_S_cap_mon(const float input) { S_cap_mon_p->set(input); }
    void put_S_cap_sim(const float input) { S_cap_sim_p->set(input); }
    void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe_p->set(input); }
    void put_Time_now(const float input) { Time_now_p->set(input); }
    void put_Type(const uint8_t input) { Type_p->set(input); }
    void put_T_state(const float input) { T_state_p->set(input); }
    void put_T_state_model(const float input) { T_state_model_p->set(input); }
    void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe_p->set(input); }
    void put_Vb_scale(const float input) { Vb_scale_p->set(input); }
    #ifndef CONFIG_47L16_EERAM
        void put_Modeling(const uint8_t input) { Modeling_p->set(input); Modeling_z = Modeling();}
        void put_T_state() {}
        void put_T_state_model() {}

        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_Modeling(const uint8_t input) { Modeling_p->set(input); }
        void put_T_state() { T_state_p->set(T_state_z); }
        void put_T_state_model() { T_state_p->set(T_state_model_z); }

        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 1<<3 & Modeling() ); } // Driving signal injection completely using software inj_bias 
    FloatZ *Amp_p;
    FloatZ *Cutback_gain_sclr_p;
    IntZ *Debug_p;
    DoubleZ *Delta_q_p;
    DoubleZ *Delta_q_model_p;
    FloatZ *Dw_p;
    FloatZ *Freq_p;
    FloatZ *Ib_bias_all_p;
    FloatNoZ *Ib_bias_all_nan_p;
    FloatZ *Ib_bias_amp_p;
    FloatZ *Ib_bias_noa_p;
    FloatZ *Ib_scale_amp_p;
    FloatZ *Ib_scale_noa_p;
    Int8tZ *Ib_select_p;
    Uint16tZ *Iflt_p;
    Uint16tZ *Ihis_p;
    FloatZ *Inj_bias_p;
    Uint16tZ *Isum_p;
    Uint8tZ *Modeling_p;
    Uint8tZ *Mon_chm_p;
    FloatZ *nP_p;
    FloatZ *nS_p;
    Uint8tZ *Preserving_p;
    Uint8tZ *Sim_chm_p;
    FloatZ *S_cap_mon_p;
    FloatZ *S_cap_sim_p;
    FloatZ *Tb_bias_hdwe_p;
    ULongZ *Time_now_p;
    FloatZ *T_state_p;
    FloatZ *T_state_model_p;
    Uint8tZ *Type_p;
    FloatZ *Vb_bias_hdwe_p;
    FloatZ *Vb_scale_p;

    // SRAM storage state "retained" in SOC_Particle.ino.  Very few elements
    float Amp_z;
    float Cutback_gain_sclr_z;
    int Debug_z;
    double Delta_q_z;
    double Delta_q_model_z;
    float Dw_z;
    float Freq_z;
    float Ib_bias_all_z;
    float Ib_bias_amp_z;
    float Ib_bias_noa_z;
    float Ib_scale_amp_z;
    float Ib_scale_noa_z;
    int8_t Ib_select_z;
    uint16_t Iflt_z;
    uint16_t Ihis_z;
    float Inj_bias_z;
    uint16_t Isum_z;
    uint8_t Modeling_z;
    uint8_t Mon_chm_z;
    float nP_z;
    float nS_z;
    uint8_t Preserving_z;
    uint8_t Sim_chm_z;
    float S_cap_mon_z;
    float S_cap_sim_z;
    float Tb_bias_hdwe_z;
    unsigned long Time_now_z;
    uint8_t Type_z;
    float T_state_z;
    float T_state_model_z;
    float Vb_bias_hdwe_z;
    float Vb_scale_z;

protected:
    uint8_t n_;
    SerialRAM *rP_;
    #ifndef CONFIG_47L16_EERAM
        Flt_st *fault_;
        Flt_st *history_;
    #else
        Flt_ram *fault_;
        Flt_ram *history_;
    #endif
    uint16_t next_;
    uint16_t nflt_;         // Length of Flt_ram array for fault snapshot
    uint16_t nhis_;         // Length of Flt_ram array for fault history
    uint16_t nsum_;         // Length of Sum array for history
    uint16_t size_;
    Z *Z_[40];
};

#endif
