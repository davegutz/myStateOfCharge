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

#define t_float float

//#include "math.h"  // Needed for Photon?
#include "local_config.h"
#include "Battery.h"
#include "hardware/SerialRAM.h"
#include "fault.h"

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
    // SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt);
    SavedPars(SerialRAM *ram);
    ~SavedPars();
    friend Sensors;
    // friend BatteryMonitor;
    // friend BatterySim;

    // operators

    // parameter list
    float amp() { return amp_; }
    float cutback_gain_sclr() { return cutback_gain_sclr_; }
    int debug() { return debug_;}
    double delta_q() { return delta_q_; }
    double delta_q_model() { return delta_q_model_; }
    float freq() { return freq_; }
    float hys_scale() { return hys_scale_; }
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
    int8_t ib_select() { return ib_select_; }
    int iflt() { return iflt_; }
    int ihis() { return ihis_; }
    float inj_bias() { return inj_bias_; }
    int isum() { return isum_; }
    float nP() { return nP_; }
    float nS() { return nS_; }
    uint8_t preserving() { return preserving_; }
    float shunt_gain_sclr() { return shunt_gain_sclr_; }
    float Tb_bias_hdwe() { return Tb_bias_hdwe_; }
    time_t time_now() { return time_now_; }
    uint8_t type() { return type_; }
    float Vb_bias_hdwe() { return Vb_bias_hdwe_; }
    float Vb_scale() { return Vb_scale_; }

    // functions
    boolean is_corrupt();
    int large_reset()
    {
        int N = 0;
        int n = 0;
        unsigned long now, then;
        then = micros();
        n = reset_pars();
        N += n;
        now = micros();
        Serial.printf("reset_pars n %d avg%10.6f\n", n, float(now-then)/1.e6/float(n));
        then = micros();
        n = reset_flt();
        N += n;
        now = micros();
        Serial.printf("reset_flt n %d avg%10.6f\n", n, float(now-then)/1.e6/float(n));
        then = micros();
        n = reset_his();
        N += n;
        now = micros();
        Serial.printf("reset_his n %d avg%10.6f\n", n, float(now-then)/1.e6/float(n));
        return N;
    }
    int reset_flt();
    int reset_his();
    int reset_pars();
    uint8_t modeling() { return modeling_; }
    void modeling(const uint8_t input, Sensors *Sen);
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
    #ifdef CONFIG_ARGON
        void get_amp() { float value; rP_->get(amp_eeram_.a16, value); amp_ = value; }
        void get_cutback_gain_sclr() { float value; rP_->get(cutback_gain_sclr_eeram_.a16, value); cutback_gain_sclr_ = value; }
        void get_debug() { int value; rP_->get(debug_eeram_.a16, value); debug_ = value; }
        void get_delta_q() { double value; rP_->get(delta_q_eeram_.a16, value); delta_q_ = value; }
        void get_delta_q_model() { double value; rP_->get(delta_q_model_eeram_.a16, value); delta_q_model_ = value; }
        void get_freq() { float value; rP_->get(freq_eeram_.a16, value); freq_ = value; }
        void get_hys_scale() { float value; rP_->get(hys_scale_eeram_.a16, value); hys_scale_ = value; }
        void get_Ib_bias_all() { float value; rP_->get(Ib_bias_all_eeram_.a16, value); Ib_bias_all_ = value; }
        void get_Ib_bias_amp() { float value; rP_->get(Ib_bias_amp_eeram_.a16, value); Ib_bias_amp_ = value; }
        void get_Ib_bias_noa() { float value; rP_->get(Ib_bias_noa_eeram_.a16, value); Ib_bias_noa_ = value; }
        void get_ib_scale_amp() { float value; rP_->get(ib_scale_amp_eeram_.a16, value); ib_scale_amp_ = value; }
        void get_ib_scale_noa() { float value; rP_->get(ib_scale_noa_eeram_.a16, value); ib_scale_noa_ = value; }
        void get_ib_select() { int8_t value; rP_->get(ib_select_eeram_.a16, value); ib_select_ = value; }
        void get_iflt() { int value; rP_->get(iflt_eeram_.a16, value); iflt_ = value; }
        void get_ihis() { int value; rP_->get(ihis_eeram_.a16, value); ihis_ = value; }
        void get_inj_bias() { float value; rP_->get(inj_bias_eeram_.a16, value); inj_bias_ = value; }
        void get_isum() { int value; rP_->get(isum_eeram_.a16, value); isum_ = value; }
        void get_modeling() { modeling_ = rP_->read(modeling_eeram_.a16); }
        void get_mon_chm() { mon_chm_ = rP_->read(mon_chm_eeram_.a16); }
        void get_nP() { float value; rP_->get(nP_eeram_.a16, value); nP_ = value; }
        void get_nS() { float value; rP_->get(nS_eeram_.a16, value); nS_ = value; }
        void get_preserving() { preserving_ = rP_->read(preserving_eeram_.a16); }
        void get_shunt_gain_sclr() { float value; rP_->get(shunt_gain_sclr_eeram_.a16, value); shunt_gain_sclr_ = value; }
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
        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        uint16_t next() { return next_; }
        int load_all();
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
    #if defined(CONFIG_PHOTON) || defined(CONFIG_PHOTON2)
        void put_all_dynamic();
        void put_amp(const float input) { amp_ = input; }
        void put_cutback_gain_sclr(const float input) { cutback_gain_sclr_ = input; }
        void put_debug(const int input) { debug_ = input; }
        void put_delta_q(const double input) { delta_q_ = input; }
        void put_delta_q() {}
        void put_delta_q_model(const double input) { delta_q_model_ = input; }
        void put_delta_q_model() {}
        void put_freq(const float input) { freq_ = input; }
        void put_hys_scale(const float input) { hys_scale_ = input; }
        void put_hys_scale() {}
        void put_Ib_bias_all(const float input) { Ib_bias_all_ = input; }
        void put_Ib_bias_amp(const float input) { Ib_bias_amp_ = input; }
        void put_Ib_bias_noa(const float input) { Ib_bias_noa_ = input; }
        void put_ib_scale_amp(const float input) { ib_scale_amp_ = input; }
        void put_ib_scale_noa(const float input) { ib_scale_noa_ = input; }
        void put_ib_select(const int8_t input) { ib_select_ = input; }
        void put_iflt(const int input) { iflt_ = input; }
        void put_ihis(const int input) { ihis_ = input; }
        void put_inj_bias(const float input) { inj_bias_ = input; }
        void put_isum(const int input) { isum_ = input; }
        void put_modeling(const uint8_t input) { modeling_ = input; }
        void put_mon_chm(const uint8_t input) { mon_chm_ = input; }
        void put_mon_chm() {}
        void put_nP(const float input) { nP_ = input; }
        void put_nS(const float input) { nS_ = input; }
        void put_preserving(const uint8_t input) { preserving_ = input; }
        void put_shunt_gain_sclr(const float input) { shunt_gain_sclr_ = input; }
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
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_all_dynamic();
        void put_amp(const float input) { rP_->put(amp_eeram_.a16, input); amp_ = input; }
        void put_cutback_gain_sclr(const float input) { rP_->put(cutback_gain_sclr_eeram_.a16, input); cutback_gain_sclr_ = input; }
        void put_debug(const int input) { rP_->put(debug_eeram_.a16, input); debug_ = input; }
        void put_delta_q(const double input) { rP_->put(delta_q_eeram_.a16, input); delta_q_ = input; }
        void put_delta_q() { rP_->put(delta_q_eeram_.a16, delta_q_); }
        void put_delta_q_model(const double input) { rP_->put(delta_q_model_eeram_.a16, input); delta_q_model_ = input; }
        void put_delta_q_model() { rP_->put(delta_q_model_eeram_.a16, delta_q_model_); }
        void put_freq(const float input) { rP_->put(freq_eeram_.a16, input); freq_ = input; }
        void put_hys_scale(const float input) { rP_->put(hys_scale_eeram_.a16, input); hys_scale_ = input; }
        void put_hys_scale() { rP_->put(hys_scale_eeram_.a16, hys_scale_); }
        void put_Ib_bias_all(const float input) { rP_->put(Ib_bias_all_eeram_.a16, input); Ib_bias_all_ = input; }
        void put_Ib_bias_amp(const float input) { rP_->put(Ib_bias_amp_eeram_.a16, input); Ib_bias_amp_ = input; }
        void put_Ib_bias_noa(const float input) { rP_->put(Ib_bias_noa_eeram_.a16, input); Ib_bias_noa_ = input; }
        void put_ib_scale_amp(const float input) { rP_->put(ib_scale_amp_eeram_.a16, input); ib_scale_amp_ = input; }
        void put_ib_scale_noa(const float input) { rP_->put(ib_scale_noa_eeram_.a16, input); ib_scale_noa_ = input; }
        void put_ib_select(const int8_t input) { rP_->put(ib_select_eeram_.a16, input); ib_select_ = input; }
        void put_iflt(const int input) { rP_->put(iflt_eeram_.a16, input); iflt_ = input; }
        void put_ihis(const int input) { rP_->put(ihis_eeram_.a16, input); ihis_ = input; }
        void put_inj_bias(const float input) { rP_->put(inj_bias_eeram_.a16, input); inj_bias_ = input; }
        void put_isum(const int input) { rP_->put(isum_eeram_.a16, input); isum_ = input; }
        void put_modeling(const uint8_t input) { rP_->write(modeling_eeram_.a16, input); modeling_ = input; }
        void put_mon_chm(const uint8_t input) { rP_->write(mon_chm_eeram_.a16, input); mon_chm_ = input; }
        void put_mon_chm() { rP_->write(mon_chm_eeram_.a16, mon_chm_); }
        void put_nP(const float input) { rP_->put(nP_eeram_.a16, input); nP_ = input; }
        void put_nS(const float input) { rP_->put(nS_eeram_.a16, input); nS_ = input; }
        void put_preserving(const uint8_t input) { rP_->write(preserving_eeram_.a16, input); preserving_ = input; }
        void put_shunt_gain_sclr(const float input) { rP_->put(shunt_gain_sclr_eeram_.a16, input); shunt_gain_sclr_ = input; }
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
        int put_fault(const Flt_st input, const uint8_t i) { return fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 0x8 & modeling_ ); } // Driving signal injection completely using software inj_bias 
protected:
    float amp_;             // Injected amplitude, A
    float cutback_gain_sclr_;// Scalar on battery model saturation cutback function
                                // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
    int debug_;             // Level of debug printing
    double delta_q_;        // Charge change since saturated, C
    double delta_q_model_;  // Charge change since saturated, C
    float freq_;            // Injected frequency, Hz (0-2)
    float hys_scale_;       // Hysteresis scalar
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
    float shunt_gain_sclr_; // Shunt gain scalar
    float s_cap_mon_;       // Scalar on battery monitor size
    float s_cap_sim_;       // Scalar on battery model size
    float Tb_bias_hdwe_;    // Bias on Tb sensor, deg C
    time_t time_now_;       // Time now, Unix time since epoch
    uint8_t type_;          // Injected waveform type.   0=sine, 1=square, 2=triangle
    float Vb_bias_hdwe_;    // Calibrate Vb, V
    float Vb_scale_;        // Calibrate Vb scale
    float t_last_;          // Updated value of battery temperature injection when sp.modeling_ and proper wire connections made, deg C
    float t_last_model_;    // Battery temperature past value for rate limit memory, deg C

    uint8_t modeling_;       // Driving saturation calculation with model.  Bits specify which signals use model
    #ifdef CONFIG_ARGON
        address16b amp_eeram_;
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
        address16b shunt_gain_sclr_eeram_;
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
    #else
        Flt_st *fault_;
        Flt_st *history_;
    #endif
    uint16_t next_;
    uint16_t nflt_;         // Length of Flt_ram array for faults
    uint16_t nhis_;         // Length of Flt_ram array for history
};








class eSavedPars
{
public:
    // eSavedPars();
    // SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt);
    eSavedPars();
    ~eSavedPars();
    friend Sensors;
    // friend BatteryMonitor;
    // friend BatterySim;

    // operators

    // parameter list
    float amp() { return amp_; }
    float cutback_gain_sclr() { return cutback_gain_sclr_; }
    int debug() { return debug_;}
    double delta_q() { return delta_q_; }
    double delta_q_model() { return delta_q_model_; }
    float freq() { return freq_; }
    float hys_scale() { return hys_scale_; }
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
    int8_t ib_select() { return ib_select_; }
    int iflt() { return iflt_; }
    int ihis() { return ihis_; }
    float inj_bias() { return inj_bias_; }
    int isum() { return isum_; }
    float nP() { return nP_; }
    float nS() { return nS_; }
    uint8_t preserving() { return preserving_; }
    float shunt_gain_sclr() { return shunt_gain_sclr_; }
    float Tb_bias_hdwe() { return Tb_bias_hdwe_; }
    time_t time_now() { return time_now_; }
    uint8_t type() { return type_; }
    float Vb_bias_hdwe() { return Vb_bias_hdwe_; }
    float Vb_scale() { return Vb_scale_; }

    // functions
    boolean is_corrupt();
    int large_reset()
    {
        int N = 0;
        int n = 0;
        unsigned long now, then;
        then = micros();
        n = reset_pars();
        N += n;
        now = micros();
        Serial.printf("ereset_pars n %d avg%10.6f\n", n, float(now-then)/1.e6/float(n));
        then = micros();
        n = reset_flt();
        N += n;
        now = micros();
        Serial.printf("ereset_flt n %d avg%10.6f\n", n, float(now-then)/1.e6/float(n));
        then = micros();
        n = reset_his();
        N += n;
        now = micros();
        Serial.printf("ereset_his n %d avg%10.6f\n", n, float(now-then)/1.e6/float(n));
        return N;
    }
    int reset_flt();
    int reset_his();
    int reset_pars();
    uint8_t modeling() { return modeling_; }
    void modeling(const uint8_t input, Sensors *Sen);
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
    #ifdef CONFIG_ARGON
        void get_amp() { float value; EEPROM.get(amp_eeprom_, value); amp_ = value; }
        void get_cutback_gain_sclr() { float value; EEPROM.get(cutback_gain_sclr_eeprom_, value); cutback_gain_sclr_ = value; }
        void get_debug() { int value; EEPROM.get(debug_eeprom_, value); debug_ = value; }
        void get_delta_q() { double value; EEPROM.get(delta_q_eeprom_, value); delta_q_ = value; }
        void get_delta_q_model() { double value; EEPROM.get(delta_q_model_eeprom_, value); delta_q_model_ = value; }
        void get_freq() { float value; EEPROM.get(freq_eeprom_, value); freq_ = value; }
        void get_hys_scale() { float value; EEPROM.get(hys_scale_eeprom_, value); hys_scale_ = value; }
        void get_Ib_bias_all() { float value; EEPROM.get(Ib_bias_all_eeprom_, value); Ib_bias_all_ = value; }
        void get_Ib_bias_amp() { float value; EEPROM.get(Ib_bias_amp_eeprom_, value); Ib_bias_amp_ = value; }
        void get_Ib_bias_noa() { float value; EEPROM.get(Ib_bias_noa_eeprom_, value); Ib_bias_noa_ = value; }
        void get_ib_scale_amp() { float value; EEPROM.get(ib_scale_amp_eeprom_, value); ib_scale_amp_ = value; }
        void get_ib_scale_noa() { float value; EEPROM.get(ib_scale_noa_eeprom_, value); ib_scale_noa_ = value; }
        void get_ib_select() { int8_t value; EEPROM.get(ib_select_eeprom_, value); ib_select_ = value; }
        void get_iflt() { int value; EEPROM.get(iflt_eeprom_, value); iflt_ = value; }
        void get_ihis() { int value; EEPROM.get(ihis_eeprom_, value); ihis_ = value; }
        void get_inj_bias() { float value; EEPROM.get(inj_bias_eeprom_, value); inj_bias_ = value; }
        void get_isum() { int value; EEPROM.get(isum_eeprom_, value); isum_ = value; }
        void get_modeling() {  uint8_t value; modeling_ = EEPROM.get(modeling_eeprom_, value); }
        void get_mon_chm() {  uint8_t value; mon_chm_ = EEPROM.get(mon_chm_eeprom_, value); mon_chm_ = value;}
        void get_nP() { float value; EEPROM.get(nP_eeprom_, value); nP_ = value; }
        void get_nS() { float value; EEPROM.get(nS_eeprom_, value); nS_ = value; }
        void get_preserving() {  uint8_t value; preserving_ = EEPROM.get(preserving_eeprom_, value); preserving_ = value; }
        void get_shunt_gain_sclr() { float value; EEPROM.get(shunt_gain_sclr_eeprom_, value); shunt_gain_sclr_ = value; }
        void get_sim_chm() { uint8_t value;  sim_chm_ = EEPROM.get(sim_chm_eeprom_, value); sim_chm_ = value; }
        void get_s_cap_mon() { float value; EEPROM.get(s_cap_mon_eeprom_, value); s_cap_mon_ = value; }
        void get_s_cap_sim() { float value; EEPROM.get(s_cap_sim_eeprom_, value); s_cap_sim_ = value; }
        void get_Tb_bias_hdwe() { float value; EEPROM.get(Tb_bias_hdwe_eeprom_, value); Tb_bias_hdwe_ = value; }
        void get_time_now() { time_t value; EEPROM.get(time_now_eeprom_, value); time_now_ = value; Time.setTime(value); }
        void get_type() { uint8_t value; type_ = EEPROM.get(type_eeprom_, value); type_ = value;}
        void get_t_last() { float value; EEPROM.get(t_last_eeprom_, value); t_last_ = value; }
        void get_t_last_model() { float value; EEPROM.get(t_last_model_eeprom_, value); t_last_model_ = value; }
        void get_Vb_bias_hdwe() { float value; EEPROM.get(Vb_bias_hdwe_eeprom_, value); Vb_bias_hdwe_ = value; }
        void get_Vb_scale() { float value; EEPROM.get(Vb_scale_eeprom_, value); Vb_scale_ = value; }
        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        int next() { return next_; }
        int load_all();
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
    #if defined(CONFIG_PHOTON) || defined(CONFIG_PHOTON2)
        void put_all_dynamic();
        void put_amp(const float input) { amp_ = input; }
        void put_cutback_gain_sclr(const float input) { cutback_gain_sclr_ = input; }
        void put_debug(const int input) { debug_ = input; }
        void put_delta_q(const double input) { delta_q_ = input; }
        void put_delta_q() {}
        void put_delta_q_model(const double input) { delta_q_model_ = input; }
        void put_delta_q_model() {}
        void put_freq(const float input) { freq_ = input; }
        void put_hys_scale(const float input) { hys_scale_ = input; }
        void put_hys_scale() {}
        void put_Ib_bias_all(const float input) { Ib_bias_all_ = input; }
        void put_Ib_bias_amp(const float input) { Ib_bias_amp_ = input; }
        void put_Ib_bias_noa(const float input) { Ib_bias_noa_ = input; }
        void put_ib_scale_amp(const float input) { ib_scale_amp_ = input; }
        void put_ib_scale_noa(const float input) { ib_scale_noa_ = input; }
        void put_ib_select(const int8_t input) { ib_select_ = input; }
        void put_iflt(const int input) { iflt_ = input; }
        void put_ihis(const int input) { ihis_ = input; }
        void put_inj_bias(const float input) { inj_bias_ = input; }
        void put_isum(const int input) { isum_ = input; }
        void put_modeling(const uint8_t input) { modeling_ = input; }
        void put_mon_chm(const uint8_t input) { mon_chm_ = input; }
        void put_mon_chm() {}
        void put_nP(const float input) { nP_ = input; }
        void put_nS(const float input) { nS_ = input; }
        void put_preserving(const uint8_t input) { preserving_ = input; }
        void put_shunt_gain_sclr(const float input) { shunt_gain_sclr_ = input; }
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
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_all_dynamic();
        void put_amp(const float input) { EEPROM.put(amp_eeprom_, input); amp_ = input; }
        void put_cutback_gain_sclr(const float input) { EEPROM.put(cutback_gain_sclr_eeprom_, input); cutback_gain_sclr_ = input; }
        void put_debug(const int input) { EEPROM.put(debug_eeprom_, input); debug_ = input; }
        void put_delta_q(const double input) { EEPROM.put(delta_q_eeprom_, input); delta_q_ = input; }
        void put_delta_q() { EEPROM.put(delta_q_eeprom_, delta_q_); }
        void put_delta_q_model(const double input) { EEPROM.put(delta_q_model_eeprom_, input); delta_q_model_ = input; }
        void put_delta_q_model() { EEPROM.put(delta_q_model_eeprom_, delta_q_model_); }
        void put_freq(const float input) { EEPROM.put(freq_eeprom_, input); freq_ = input; }
        void put_hys_scale(const float input) { EEPROM.put(hys_scale_eeprom_, input); hys_scale_ = input; }
        void put_hys_scale() { EEPROM.put(hys_scale_eeprom_, hys_scale_); }
        void put_Ib_bias_all(const float input) { EEPROM.put(Ib_bias_all_eeprom_, input); Ib_bias_all_ = input; }
        void put_Ib_bias_amp(const float input) { EEPROM.put(Ib_bias_amp_eeprom_, input); Ib_bias_amp_ = input; }
        void put_Ib_bias_noa(const float input) { EEPROM.put(Ib_bias_noa_eeprom_, input); Ib_bias_noa_ = input; }
        void put_ib_scale_amp(const float input) { EEPROM.put(ib_scale_amp_eeprom_, input); ib_scale_amp_ = input; }
        void put_ib_scale_noa(const float input) { EEPROM.put(ib_scale_noa_eeprom_, input); ib_scale_noa_ = input; }
        void put_ib_select(const int8_t input) { EEPROM.put(ib_select_eeprom_, input); ib_select_ = input; }
        void put_iflt(const int input) { EEPROM.put(iflt_eeprom_, input); iflt_ = input; }
        void put_ihis(const int input) { EEPROM.put(ihis_eeprom_, input); ihis_ = input; }
        void put_inj_bias(const float input) { EEPROM.put(inj_bias_eeprom_, input); inj_bias_ = input; }
        void put_isum(const int input) { EEPROM.put(isum_eeprom_, input); isum_ = input; }
        void put_modeling(const uint8_t input) { EEPROM.put(modeling_eeprom_, input); modeling_ = input; }
        void put_mon_chm(const uint8_t input) { EEPROM.put(mon_chm_eeprom_, input); mon_chm_ = input; }
        void put_mon_chm() { EEPROM.put(mon_chm_eeprom_, mon_chm_); }
        void put_nP(const float input) { EEPROM.put(nP_eeprom_, input); nP_ = input; }
        void put_nS(const float input) { EEPROM.put(nS_eeprom_, input); nS_ = input; }
        void put_preserving(const uint8_t input) { EEPROM.put(preserving_eeprom_, input); preserving_ = input; }
        void put_shunt_gain_sclr(const float input) { EEPROM.put(shunt_gain_sclr_eeprom_, input); shunt_gain_sclr_ = input; }
        void put_sim_chm(const uint8_t input) { EEPROM.put(sim_chm_eeprom_, input); sim_chm_ = input; }
        void put_sim_chm() { EEPROM.put(sim_chm_eeprom_, sim_chm_); }
        void put_s_cap_mon(const float input) { EEPROM.put(s_cap_mon_eeprom_, input); s_cap_mon_ = input; }
        void put_s_cap_sim(const float input) { EEPROM.put(s_cap_sim_eeprom_, input); s_cap_sim_ = input; }
        void put_Tb_bias_hdwe(const float input) { EEPROM.put(Tb_bias_hdwe_eeprom_, input); Tb_bias_hdwe_ = input; }
        void put_time_now(const time_t input) { EEPROM.put(time_now_eeprom_, input); time_now_ = input; Time.setTime(time_now_); }
        void put_type(const uint8_t input) { EEPROM.put(type_eeprom_, input); type_ = input; }
        void put_t_last(const float input) { EEPROM.put(t_last_eeprom_, input); t_last_ = input; }
        void put_t_last() { EEPROM.put(t_last_eeprom_, t_last_); }
        void put_t_last_model(const float input) { EEPROM.put(t_last_model_eeprom_, input); t_last_model_ = input; }
        void put_t_last_model() { EEPROM.put(t_last_model_eeprom_, t_last_model_); }
        void put_Vb_bias_hdwe(const float input) { EEPROM.put(Vb_bias_hdwe_eeprom_, input); Vb_bias_hdwe_ = input; }
        void put_Vb_scale(const float input) { EEPROM.put(Vb_scale_eeprom_, input); Vb_scale_ = input; }
        int put_fault(const Flt_st input, const uint8_t i) { return fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 0x8 & modeling_ ); } // Driving signal injection completely using software inj_bias 
protected:
    float amp_;             // Injected amplitude, A
    float cutback_gain_sclr_;// Scalar on battery model saturation cutback function
                                // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
    int debug_;             // Level of debug printing
    double delta_q_;        // Charge change since saturated, C
    double delta_q_model_;  // Charge change since saturated, C
    float freq_;            // Injected frequency, Hz (0-2)
    float hys_scale_;       // Hysteresis scalar
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
    float shunt_gain_sclr_; // Shunt gain scalar
    float s_cap_mon_;       // Scalar on battery monitor size
    float s_cap_sim_;       // Scalar on battery model size
    float Tb_bias_hdwe_;    // Bias on Tb sensor, deg C
    time_t time_now_;       // Time now, Unix time since epoch
    uint8_t type_;          // Injected waveform type.   0=sine, 1=square, 2=triangle
    float Vb_bias_hdwe_;    // Calibrate Vb, V
    float Vb_scale_;        // Calibrate Vb scale
    float t_last_;          // Updated value of battery temperature injection when sp.modeling_ and proper wire connections made, deg C
    float t_last_model_;    // Battery temperature past value for rate limit memory, deg C

    uint8_t modeling_;       // Driving saturation calculation with model.  Bits specify which signals use model
    #ifdef CONFIG_ARGON
        int amp_eeprom_;
        int cutback_gain_sclr_eeprom_;
        int debug_eeprom_;
        int delta_q_eeprom_;
        int delta_q_model_eeprom_;
        int freq_eeprom_;
        int hys_scale_eeprom_;
        int Ib_bias_all_eeprom_;
        int Ib_bias_amp_eeprom_;
        int Ib_bias_noa_eeprom_;
        int ib_scale_amp_eeprom_;
        int ib_scale_noa_eeprom_;
        int ib_select_eeprom_;
        int iflt_eeprom_;
        int ihis_eeprom_;
        int inj_bias_eeprom_;
        int isum_eeprom_;
        int modeling_eeprom_;
        int mon_chm_eeprom_;
        int nP_eeprom_;
        int nS_eeprom_;
        int preserving_eeprom_;
        int shunt_gain_sclr_eeprom_;
        int sim_chm_eeprom_;
        int s_cap_mon_eeprom_;
        int s_cap_sim_eeprom_;
        int Tb_bias_hdwe_eeprom_;
        int time_now_eeprom_;
        int type_eeprom_;
        int t_last_eeprom_;
        int t_last_model_eeprom_;
        int Vb_bias_hdwe_eeprom_;
        int Vb_scale_eeprom_;
        Flt_prom *fault_;
        Flt_prom *history_;
    #else
        Flt_st *fault_;
        Flt_st *history_;
    #endif
    int next_;
    uint16_t nflt_;         // Length of Flt_ram array for faults
    uint16_t nhis_;         // Length of Flt_ram array for history
};
#endif
