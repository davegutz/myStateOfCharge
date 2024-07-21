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

#include "constants.h"
#include "Battery.h"
#include "Fault.h"
#include "PrinterPars.h"
#include "Variable.h"
#include "Cloud.h"

void app_no();
void app_mon_chem();

class Parameters
{
public:
    Parameters();
    ~Parameters();
    // Do everything
    boolean find_adjust(const String &str);
    virtual void initialize() {}
    boolean is_corrupt();
    virtual void pretty_print(const boolean all){}
    void set_nominal();
    String value_str() { return value_str_; }
protected:
    int8_t n_;
    Variable **V_;
    String value_str_;
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
    float hys_state;            // Sim hysteresis state
    float ib_amp_add;           // Fault injection bias on amp, A
    float ib_amp_max;           // ib amp unit hardware model max, A
    float ib_amp_min;           // ib amp unit hardware model min, A
    float ib_diff_slr;          // Scale ib_diff detection thresh, scalar
    float ib_noa_add;           // Fault injection bias on non amp, A
    float ib_noa_max;           // ib noa unit hardware model max, A
    float ib_noa_min;           // ib noa unit hardware model min, A
    float Ib_amp_noise_amp;     // Ib bank noise on amplified sensor, amplitude model only, A pk-pk
    float Ib_noa_noise_amp;     // Ib bank noise on non-amplified sensor, amplitude model only, A pk-pk
    float ib_quiet_slr;         // Scale ib_quiet detection thresh, scalar
    float init_all_soc;         // Reinitialize all models to this soc
    float init_sim_soc;         // Reinitialize sim model only to this soc
    uint8_t print_mult;         // Print multiplier for objects
    unsigned long int read_delay; // Minor frame, ms
    float slr_res;              // Scalar Randles R0, slr
    float s_t_sat;              // Scalar on saturation test time set and reset
    unsigned long int sum_delay; // Minor frame divisor, div
    unsigned long int tail_inj; // Tail after end injection, ms
    unsigned long int talk_delay; // Talk frame, ms
    float Tb_bias_model;        // Bias on Tb for model
    float Tb_noise_amp;         // Tb noise amplitude model only, deg C pk-pk
    float tb_stale_time_slr;    // Scalar on persistences of Tb hardware stale check
    unsigned long int until_q;  // Time until set vv0, ms
    float vb_add;               // Fault injection bias, V
    float Vb_noise_amp;         // Vb bank noise amplitude model only, V pk-pk
    float vc_add;               // Shunt Vc/Vr Fault injection bias, V
    unsigned long int wait_inj; // Wait before start injection, ms
    FloatV *cc_diff_slr_p;
    FloatV *cycles_inj_p;
    BooleanV *dc_dc_on_p;
    BooleanV *disab_ib_fa_p;
    BooleanV *disab_tb_fa_p;
    BooleanV *disab_vb_fa_p;
    FloatV *ds_voc_soc_p;
    FloatV *dv_voc_soc_p;
    Uint8tV *eframe_mult_p;
    FloatV *ewhi_slr_p;
    FloatV *ewlo_slr_p;
    BooleanV *fail_tb_p;
    BooleanV *fake_faults_p;
    FloatV *hys_scale_p;
    FloatV *hys_state_p;
    FloatV *ib_amp_add_p;
    FloatV *ib_diff_slr_p;
    FloatV *ib_noa_add_p;
    FloatV *Ib_amp_noise_amp_p;
    FloatV *Ib_noa_noise_amp_p;
    FloatV *ib_quiet_slr_p;
    FloatV *init_all_soc_p;
    FloatV *init_sim_soc_p;
    Uint8tV *print_mult_p;
    ULongV *read_delay_p;
    FloatV *slr_res_p;
    FloatV *s_t_sat_p;
    ULongV *sum_delay_p;
    ULongV *tail_inj_p;
    ULongV *talk_delay_p;
    FloatV *Tb_bias_model_p;
    FloatV *Tb_noise_amp_p;
    FloatV *tb_stale_time_slr_p;
    ULongV *until_q_p;
    FloatV *vb_add_p;
    FloatV *Vb_noise_amp_p;
    FloatV *vc_add_p;
    ULongV *wait_inj_p;
    FloatV *ib_max_amp_p;
    FloatV *ib_min_amp_p;
    FloatV *ib_max_noa_p;
    FloatV *ib_min_noa_p;

protected:
};


// Definition of structure to be saved, either EERAM or retained backup SRAM.  Many are needed to calibrate.  Others are
// needed to allow testing with resets.  Others allow application to remember dynamic
// tweaks.  Default values below are important:  they prevent junk
// behavior on initial build. Don't put anything in here that you can't live with normal running
// because could get set by testing and forgotten.
// SavedPars Class
class SavedPars : public Parameters
{
public:
    SavedPars();
    SavedPars(SerialRAM *ram);
    SavedPars(Flt_st *hist, const uint16_t nhis, Flt_st *faults, const uint16_t nflt);
    ~SavedPars();
 
    // parameter list
    float Amp() { return amp_z * nP_z; }
    float cutback_gain_slr() { return cutback_gain_slr_z; }
    int debug() { return debug_z;}
    double delta_q() { return delta_q_z;}
    double delta_q_model() { return delta_q_model_z;}
    float Dw() { return Dw_z; }
    float freq() { return freq_z; }
    float s_cap_mon() { return s_cap_mon_z; }
    float s_cap_sim() { return s_cap_sim_z; }
    float ib_bias_all() { return ib_bias_all_z; }
    float ib_bias_amp() { return ib_bias_amp_z; }
    float ib_bias_noa() { return ib_bias_noa_z; }
    float ib_disch_slr() { return ib_disch_slr_z; }
    float ib_scale_amp() { return ib_scale_amp_z; }
    float ib_scale_noa() { return ib_scale_noa_z; }
    int8_t ib_force() { return ib_force_z; }
    uint16_t Iflt() { return iflt_z; }
    uint16_t Ihis() { return ihis_z; }
    float inj_bias() { return inj_bias_z; }
    uint16_t isum() { return isum_z; }
    uint8_t modeling() { return modeling_z; }
    float nP() { return nP_z; }
    float nS() { return nS_z; }
    uint8_t preserving() { return preserving_z; }
    float Tb_bias_hdwe() { return Tb_bias_hdwe_z; }
    unsigned long Time_now() { return Time_now_z; }
    uint8_t type() { return type_z; }
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
    void nsum(const uint16_t in) { nsum_ = in; }
    uint16_t nsum() { return nsum_; }
    void nominalize_fault_array();
    void nominalize_history_array();
    int num_diffs();
    virtual void pretty_print(const boolean all);
    void pretty_print_modeling();
    void print_fault_array();
    void print_fault_header(Publish *pubList);
    void print_history_array();
    void reset_flt();
    void reset_his();
    virtual void set_nominal();
    float ib_hist_slr() { if ( abs(amp_z) > 40. ) return 30000./abs(amp_z); else return 600.; }
    float vb_hist_slr() { if ( abs(amp_z) > 40. ) return 1500./abs(amp_z); else return 1200.; }
    boolean mod_all_dscn() { return ( 111<modeling() ); }                // Bare all
    boolean mod_any() { return ( mod_ib() || mod_tb() || mod_vb() ); }  // Modeling any
    boolean mod_any_dscn() { return ( 15<modeling() ); }                 // Bare any
    boolean mod_ib() { return ( 1<<2 & modeling() || mod_ib_all_dscn() ); }  // Using Sim as source of ib
    boolean mod_ib_all_dscn() { return ( 191<modeling() ); }             // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_amp_dscn() { return ( 1<<6 & modeling() ); }          // Nothing connected to amp ib sensors in I2C on SDA/SCL
    boolean mod_ib_any_dscn() { return ( mod_ib_amp_dscn() || mod_ib_noa_dscn() ); }  // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_noa_dscn() { return ( 1<<7 & modeling() ); }          // Nothing connected to noa ib sensors in I2C on SDA/SCL
    boolean mod_none() { return ( 0==modeling() ); }                     // Using all
    boolean mod_none_dscn() { return ( 16>modeling() ); }                // Bare nothing
    boolean mod_tb() { return ( 1<<0 & modeling() || mod_tb_dscn() ); }  // Using Sim as source of tb
    boolean mod_tb_dscn() { return ( 1<<4 & modeling() ); }              // Nothing connected to one-wire Tb sensor on D6
    boolean mod_vb() { return ( 1<<1 & modeling() || mod_vb_dscn() ); }  // Using Sim as source of vb
    boolean mod_vb_dscn() { return ( 1<<5 & modeling() ); }              // Nothing connected to vb on A1

    #ifdef HDWE_47L16_EERAM
        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        uint16_t next() { return next_; }
        void load_all();
    #endif

    // put
    void put_all_dynamic();
    void put_amp(const float input) { amp_p->check_set_put(input); }
    void put_cutback_gain_slr(const float input) { cutback_gain_slr_p->check_set_put(input); }
    void put_Debug(const int input) { debug_p->check_set_put(input); }
    void put_Delta_q(const double input) { delta_q_p->check_set_put(input); }
    void put_delta_q() {}
    void put_delta_q_model(const double input) { delta_q_model_p->check_set_put(input); }
    void put_delta_q_model() {}
    void put_Dw(const float input) { Dw_p->check_set_put(input); }
    void put_Freq(const float input) { freq_p->check_set_put(input); }
    void put_ib_bias_all(const float input) { ib_bias_all_p->check_set_put(input); }
    void put_ib_bias_amp(const float input) { ib_bias_amp_p->check_set_put(input); }
    void put_ib_bias_noa(const float input) { ib_bias_noa_p->check_set_put(input); }
    void put_ib_disch_slr(const float input) { ib_disch_slr_p->check_set_put(input); }
    void put_ib_scale_amp(const float input) { ib_scale_amp_p->check_set_put(input); }
    void put_ib_scale_noa(const float input) { ib_scale_noa_p->check_set_put(input); }
    void put_ib_force(const int8_t input) { ib_force_p->check_set_put(input); }
    void put_Iflt(const int input) { iflt_p->check_set_put(input); }
    void put_Ihis(const int input) { ihis_p->check_set_put(input); }
    void put_Isum(const int input) { isum_p->check_set_put(input); }
    void put_Inj_bias(const float input) { inj_bias_p->check_set_put(input); }
    void put_nP(const float input) { nP_p->check_set_put(input); }
    void put_nS(const float input) { nS_p->check_set_put(input); }
    void put_Preserving(const uint8_t input) { preserving_p->check_set_put(input); }
    void put_s_cap_mon(const float input) { s_cap_mon_p->check_set_put(input); }
    void put_s_cap_sim(const float input) { s_cap_sim_p->check_set_put(input); }
    void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe_p->check_set_put(input); }
    void put_Time_now(const unsigned long input) { Time_now_p->check_set_put(input); }
    void put_Type(const uint8_t input) { Type_p->check_set_put(input); }
    void put_T_state(const float input) { T_state_p->check_set_put(input); }
    void put_T_state_model(const float input) { T_state_model_p->check_set_put(input); }
    void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe_p->check_set_put(input); }
    void put_Vb_scale(const float input) { Vb_scale_p->check_set_put(input); }
    #ifndef HDWE_47L16_EERAM
        void put_modeling(const uint8_t input) { modeling_p->check_set_put(input); modeling_z = modeling();}
        void put_T_state() {}
        void put_T_state_model() {}
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_modeling(const uint8_t input) { modeling_p->check_set_put(input); }
        void put_T_state() { T_state_p->check_set_put(T_state_z); }
        void put_T_state_model() { T_state_p->check_set_put(T_state_model_z); }

        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 1<<3 & modeling() ); } // Driving signal injection completely using software inj_bias 
    FloatV *amp_p;
    FloatV *cutback_gain_slr_p;
    IntV *debug_p;
    DoubleV *delta_q_p;
    DoubleV *delta_q_model_p;
    FloatV *Dw_p;
    FloatV *freq_p;
    FloatV *ib_bias_all_p;
    FloatV *ib_bias_amp_p;
    FloatV *ib_bias_noa_p;
    FloatV *ib_disch_slr_p;
    FloatV *ib_scale_amp_p;
    FloatV *ib_scale_noa_p;
    Int8tV *ib_force_p;
    Uint16tV *iflt_p;
    Uint16tV *ihis_p;
    FloatV *inj_bias_p;
    Uint16tV *isum_p;
    Uint8tV *modeling_p;
    FloatV *nP_p;
    FloatV *nS_p;
    Uint8tV *preserving_p;
    FloatV *s_cap_mon_p;
    FloatV *s_cap_sim_p;
    FloatV *Tb_bias_hdwe_p;
    ULongV *Time_now_p;
    FloatV *T_state_p;
    FloatV *T_state_model_p;
    Uint8tV *Type_p;
    FloatV *Vb_bias_hdwe_p;
    FloatV *Vb_scale_p;

    // SRAM storage state "retained" in SOC_Particle.ino.  Very few elements
    float amp_z;
    float cutback_gain_slr_z;
    int debug_z;
    double delta_q_z;
    double delta_q_model_z;
    float Dw_z;
    float freq_z;
    float ib_bias_all_z;
    float ib_bias_amp_z;
    float ib_bias_noa_z;
    float ib_disch_slr_z;
    float ib_scale_amp_z;
    float ib_scale_noa_z;
    int8_t ib_force_z;
    uint16_t iflt_z;
    uint16_t ihis_z;
    float inj_bias_z;
    uint16_t isum_z;
    uint8_t modeling_z;
    float nP_z;
    float nS_z;
    uint8_t preserving_z;
    float s_cap_mon_z;
    float s_cap_sim_z;
    float Tb_bias_hdwe_z;
    unsigned long Time_now_z;
    uint8_t type_z;
    float T_state_z;
    float T_state_model_z;
    float Vb_bias_hdwe_z;
    float Vb_scale_z;

protected:
    SerialRAM *rP_;
    #ifndef HDWE_47L16_EERAM
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
