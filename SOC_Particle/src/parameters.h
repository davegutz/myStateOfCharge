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

void app_no();
void app_mon_chem();

class Parameters
{
public:
    Parameters();
    ~Parameters();
    virtual void initialize() {};
    boolean is_corrupt();
    virtual void pretty_print(const boolean all){};
    void set_nominal();
protected:
    int8_t n_;
    Z **Z_;
    // fptr fptr_;
};


// Volatile memory
class VolatilePars : public Parameters
{
public:
    VolatilePars();
    ~VolatilePars();
    virtual void initialize();
    virtual void pretty_print(const boolean all);

    // Declare
    float cc_diff_slr;          // Scale cc_diff detection thresh, scalar
    float cycles_inj;           // Number of injection cycles
    boolean dc_dc_on;           // DC-DC charger is on
    boolean disab_ib_fa;        // Disable hard fault range failures for ib
    boolean disab_tb_fa;        // Disable hard fault range failures for tb
    boolean disab_vb_fa;        // Disable hard fault range failures for vb
    float ds_voc_soc;           // VOC(SOC) delta soc on input, frac
    float dv_voc_soc;           // VOC(SOC) del v, V
    uint8_t eframe_mult;        // Frame multiplier for EKF execution.  Number of READ executes for each EKF execution
    float ewhi_slr;             // Scale wrap hi detection thresh, scalar
    float ewlo_slr;             // Scale wrap lo detection thresh, scalar
    boolean fail_tb;            // Make hardware bus read ignore Tb and fail it
    boolean fake_faults;        // Faults faked (ignored).  Used to evaluate a configuration, deploy it without disrupting use
    float hys_scale;            // Sim hysteresis scalar
    float ib_amp_add;           // Fault injection bias on amp, A
    float ib_diff_slr;          // Scale ib_diff detection thresh, scalar
    float ib_noa_add;           // Fault injection bias on non amp, A
    float Ib_amp_noise_amp;     // Ib bank noise on amplified sensor, amplitude model only, A pk-pk
    float Ib_noa_noise_amp;     // Ib bank noise on non-amplified sensor, amplitude model only, A pk-pk
    uint8_t print_mult;         // Print multiplier for objects
    float s_t_sat;              // Scalar on saturation test time set and reset
    unsigned long int tail_inj; // Tail after end injection, ms
    float Tb_bias_model;        // Bias on Tb for model
    float Tb_noise_amp;         // Tb noise amplitude model only, deg C pk-pk
    float tb_stale_time_slr;    // Scalar on persistences of Tb hardware stale check
    unsigned long int until_q;  // Time until set v0, ms
    float vb_add;               // Fault injection bias, V
    float Vb_noise_amp;         // Vb bank noise amplitude model only, V pk-pk
    unsigned long int wait_inj; // Wait before start injection, ms
    FloatZ *cc_diff_slr_p;
    FloatZ *cycles_inj_p;
    BooleanZ *dc_dc_on_p;
    BooleanZ *disab_ib_fa_p;
    BooleanZ *disab_tb_fa_p;
    BooleanZ *disab_vb_fa_p;
    FloatZ *ds_voc_soc_p;
    FloatZ *dv_voc_soc_p;
    Uint8tZ *eframe_mult_p;
    FloatZ *ewhi_slr_p;
    FloatZ *ewlo_slr_p;
    BooleanZ *fail_tb_p;
    BooleanZ *fake_faults_p;
    FloatZ *hys_scale_p;
    FloatZ *ib_amp_add_p;
    FloatZ *ib_diff_slr_p;
    FloatZ *ib_noa_add_p;
    FloatZ *Ib_amp_noise_amp_p;
    FloatZ *Ib_noa_noise_amp_p;
    Uint8tZ *print_mult_p;
    FloatZ *s_t_sat_p;
    FloatZ *Tb_bias_model_p;
    FloatZ *Tb_noise_amp_p;
    FloatZ *tb_stale_time_slr_p;
    ULongZ *tail_inj_p;
    ULongZ *until_q_p;
    FloatZ *vb_add_p;
    FloatZ *Vb_noise_amp_p;
    ULongZ *wait_inj_p;

protected:
};


// Definition of structure to be saved, either EERAM or retained backup SRAM.  Many are needed to calibrate.  Others are
// needed to allow testing with resets.  Others allow application to remember dynamic
// tweaks.  Default values below are important:  they prevent junk
// behavior on initial build. Don't put anything in here that you can't live with normal running
// because could get set by testing and forgotten.  Not reset by hard reset
// SavedPars Class
class SavedPars : public Parameters
{
public:
    SavedPars();
    SavedPars(SerialRAM *ram);
    SavedPars(Flt_st *hist, const uint16_t nhis, Flt_st *faults, const uint16_t nflt);
    ~SavedPars();
 
    // parameter list
    float Amp() { return Amp_z; }
    float Cutback_gain_slr() { return Cutback_gain_slr_z; }
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
    virtual void initialize();
    void large_reset() { set_nominal(); reset_flt(); reset_his(); }
    void mem_print();
    uint16_t nflt() { return nflt_; }
    uint16_t nhis() { return nhis_; }
    uint16_t nsum() { return nsum_; }
    void nominalize_fault_array();
    void nominalize_history_array();
    int num_diffs();
    virtual void pretty_print(const boolean all);
    void pretty_print_modeling();
    void print_fault_array();
    void print_fault_header();
    void print_history_array();
    void reset_flt();
    void reset_his();
    virtual void set_nominal();

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

    // put
    void put_all_dynamic();
    void put_Amp(const float input) { Amp_p->check_set_put(input); }
    void put_Cutback_gain_slr(const float input) { Cutback_gain_slr_p->check_set_put(input); }
    void put_Debug(const int input) { Debug_p->check_set_put(input); }
    void put_Delta_q(const double input) { Delta_q_p->check_set_put(input); }
    void put_Delta_q() {}
    void put_Delta_q_model(const double input) { Delta_q_model_p->check_set_put(input); }
    void put_Delta_q_model() {}
    void put_Dw(const float input) { Dw_p->check_set_put(input); }
    void put_Freq(const float input) { Freq_p->check_set_put(input); }
    void put_Ib_bias_all(const float input) { Ib_bias_all_p->check_set_put(input); }
    void put_Ib_bias_amp(const float input) { Ib_bias_amp_p->check_set_put(input); }
    void put_Ib_bias_noa(const float input) { Ib_bias_noa_p->check_set_put(input); }
    void put_Ib_scale_amp(const float input) { Ib_scale_amp_p->check_set_put(input); }
    void put_Ib_scale_noa(const float input) { Ib_scale_noa_p->check_set_put(input); }
    void put_Ib_select(const int8_t input) { Ib_select_p->check_set_put(input); }
    void put_Iflt(const int input) { Iflt_p->check_set_put(input); }
    void put_Ihis(const int input) { Ihis_p->check_set_put(input); }
    void put_Isum(const int input) { Isum_p->check_set_put(input); }
    void put_Inj_bias(const float input) { Inj_bias_p->check_set_put(input); }
    void put_Mon_chm(const uint8_t input) { Mon_chm_p->check_set_put(input); }
    void put_Mon_chm() {}
    void put_nP(const float input) { nP_p->check_set_put(input); }
    void put_nS(const float input) { nS_p->check_set_put(input); }
    void put_Preserving(const uint8_t input) { Preserving_p->check_set_put(input); }
    void put_Sim_chm(const uint8_t input) { Sim_chm_p->check_set_put(input); }
    void put_Sim_chm() {}
    void put_S_cap_mon(const float input) { S_cap_mon_p->check_set_put(input); }
    void put_S_cap_sim(const float input) { S_cap_sim_p->check_set_put(input); }
    void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe_p->check_set_put(input); }
    void put_Time_now(const float input) { Time_now_p->check_set_put(input); }
    void put_Type(const uint8_t input) { Type_p->check_set_put(input); }
    void put_T_state(const float input) { T_state_p->check_set_put(input); }
    void put_T_state_model(const float input) { T_state_model_p->check_set_put(input); }
    void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe_p->check_set_put(input); }
    void put_Vb_scale(const float input) { Vb_scale_p->check_set_put(input); }
    #ifndef CONFIG_47L16_EERAM
        void put_Modeling(const uint8_t input) { Modeling_p->check_set_put(input); Modeling_z = Modeling();}
        void put_T_state() {}
        void put_T_state_model() {}
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_Modeling(const uint8_t input) { Modeling_p->check_set_put(input); }
        void put_T_state() { T_state_p->check_set_put(T_state_z); }
        void put_T_state_model() { T_state_p->check_set_put(T_state_model_z); }

        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 1<<3 & Modeling() ); } // Driving signal injection completely using software inj_bias 
    FloatZ *Amp_p;
    FloatZ *Cutback_gain_slr_p;
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
    float Cutback_gain_slr_z;
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
};

#endif
