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

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#define t_float float

#include "math.h"
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
// ********CAUTION:  any special includes or logic in here breaks retained function
// Coulomb Counter Class
class SavedPars
{
public:
    SavedPars();
    SavedPars(SerialRAM *ram);
    ~SavedPars();
    // operators

    // parameter list
    float amp;              // Injected amplitude, A pk (0-18.3)
    float cutback_gain_sclr; // Scalar on battery model saturation cutback function
                                // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
    int debug;              // Level of debug printing
    double delta_q;         // Charge change since saturated, C
    double delta_q_model;   // Charge change since saturated, C
    float freq;             // Injected frequency, Hz (0-2)
    float hys_scale;        // Hysteresis scalar
    float Ib_bias_all;      // Bias on all shunt sensors, A
    float Ib_bias_amp;      // Calibration adder of amplified shunt sensor, A
    float Ib_bias_noa;      // Calibration adder of non-amplified shunt sensor, A
    float ib_scale_amp;     // Calibration scalar of amplified shunt sensor
    float ib_scale_noa;     // Calibration scalar of non-amplified shunt sensor
    int8_t ib_select;       // Force current sensor (-1=non-amp, 0=auto, 1=amp)
    int iflt;               // Fault snap location.   Begins at -1 because first action is to increment iflt
    int ihis;               // History location.   Begins at -1 because first action is to increment ihis
    float inj_bias;         // Constant bias, A
    int isum;               // Summary location.   Begins at -1 because first action is to increment isum
    uint8_t modeling;       // Driving saturation calculation with model.  Bits specify which signals use model
    uint8_t mon_chm;        // Monitor battery chemistry type
    float nP;               // Number of parallel batteries in bank, e.g. '2P1S'
    float nS;               // Number of series batteries in bank, e.g. '2P1S'
    uint8_t preserving;     // Preserving fault buffer
    float s_cap_model;      // Scalar on battery model size
    float shunt_gain_sclr;  // Shunt gain scalar
    uint8_t sim_chm;        // Simulation battery chemistry type
    float Tb_bias_hdwe;     // Bias on Tb sensor, deg C
    uint8_t type;           // Injected waveform type.   0=sine, 1=square, 2=triangle
    float t_last;           // Updated value of battery temperature injection when sp.modeling and proper wire connections made, deg C
    float t_last_model;     // Battery temperature past value for rate limit memory, deg C
    float Vb_bias_hdwe;     // Calibrate Vb, V
    float Vb_scale;         // Calibrate Vb scale

    // functions
    int assign_all();
    boolean is_corrupt();
    void large_reset() { nominal(); }
    boolean mod_any() { return ( 0<modeling ); }        // Using any
    boolean mod_ib() { return ( 0x4 & modeling ); }     // Using Sim as source of ib
    boolean mod_none() { return ( 0==modeling ); }      // Using nothing
    boolean mod_tb() { return ( 0x1 & modeling ); }     // Using Sim as source of tb
    boolean mod_vb() { return ( 0x2 & modeling ); }     // Using Sim as source of vb
    // get
    #if (PLATFORM_ID==6)  // Photon
        void get_amp() { }
        void get_cutback_gain_sclr() { }
        void get_debug() { }
        void get_delta_q() { }
        void get_delta_q_model() { }
        void get_freq() { }
        void get_hys_scale() { }
        void get_Ib_bias_all() { }
        void get_Ib_bias_amp() { }
        void get_Ib_bias_noa() { }
        void get_ib_scale_amp() { }
        void get_ib_scale_noa() { }
        void get_ib_select() { }
        void get_iflt() { }
        void get_ihis() { }
        void get_inj_bias() { }
        void get_isum() { }
        void get_modeling() { }
        void get_mon_chm() { }
        void get_nP() { }
        void get_nS() { }
        void get_preserving() { }
        void get_shunt_gain_sclr() { }
        void get_sim_chm() { }
        void get_s_cap_model() { }
        void get_Tb_bias_hdwe() { }
        void get_type() { }
        void get_t_last() { }
        void get_t_last_model() { }
        void get_Vb_bias_hdwe() { }
        void get_Vb_scale() { }
        void get_fault() { }
        void get_history() { }
    #elif PLATFORM_ID == PLATFORM_ARGON
        void get_amp() { float value; rP_->get(amp_eeram_.a16, value); amp = value; }
        void get_cutback_gain_sclr() { float value; rP_->get(cutback_gain_sclr_eeram_.a16, value); cutback_gain_sclr = value; }
        void get_debug() { int value; rP_->get(debug_eeram_.a16, value); debug = value; }
        void get_delta_q() { double value; rP_->get(delta_q_eeram_.a16, value); delta_q = value; }
        void get_delta_q_model() { double value; rP_->get(delta_q_model_eeram_.a16, value); delta_q_model = value; }
        void get_freq() { float value; rP_->get(freq_eeram_.a16, value); freq = value; }
        void get_hys_scale() { float value; rP_->get(hys_scale_eeram_.a16, value); hys_scale = value; }
        void get_Ib_bias_all() { float value; rP_->get(Ib_bias_all_eeram_.a16, value); Ib_bias_all = value; }
        void get_Ib_bias_amp() { float value; rP_->get(Ib_bias_amp_eeram_.a16, value); Ib_bias_amp = value; }
        void get_Ib_bias_noa() { float value; rP_->get(Ib_bias_noa_eeram_.a16, value); Ib_bias_noa = value; }
        void get_ib_scale_amp() { float value; rP_->get(ib_scale_amp_eeram_.a16, value); ib_scale_amp = value; }
        void get_ib_scale_noa() { float value; rP_->get(ib_scale_noa_eeram_.a16, value); ib_scale_noa = value; }
        void get_ib_select() { int8_t value; rP_->get(ib_select_eeram_.a16, value); ib_select = value; }
        void get_iflt() { int value; rP_->get(iflt_eeram_.a16, value); iflt = value; }
        void get_ihis() { int value; rP_->get(ihis_eeram_.a16, value); ihis = value; }
        void get_inj_bias() { float value; rP_->get(inj_bias_eeram_.a16, value); inj_bias = value; }
        void get_isum() { int value; rP_->get(isum_eeram_.a16, value); isum = value; }
        void get_modeling() { modeling = rP_->read(modeling_eeram_.a16); }
        void get_mon_chm() { mon_chm = rP_->read(mon_chm_eeram_.a16); }
        void get_nP() { float value; rP_->get(nP_eeram_.a16, value); nP = value; }
        void get_nS() { float value; rP_->get(nS_eeram_.a16, value); nS = value; }
        void get_preserving() { preserving = rP_->read(preserving_eeram_.a16); }
        void get_shunt_gain_sclr() { float value; rP_->get(shunt_gain_sclr_eeram_.a16, value); shunt_gain_sclr = value; }
        void get_sim_chm() { mon_chm = rP_->read(sim_chm_eeram_.a16); }
        void get_s_cap_model() { float value; rP_->get(s_cap_model_eeram_.a16, value); s_cap_model = value; }
        void get_Tb_bias_hdwe() { float value; rP_->get(Tb_bias_hdwe_eeram_.a16, value); Tb_bias_hdwe = value; }
        void get_type() { type = rP_->read(type_eeram_.a16); }
        void get_t_last() { float value; rP_->get(t_last_eeram_.a16, value); t_last = value; }
        void get_t_last_model() { float value; rP_->get(t_last_model_eeram_.a16, value); t_last_model = value; }
        void get_Vb_bias_hdwe() { float value; rP_->get(Vb_bias_hdwe_eeram_.a16, value); Vb_bias_hdwe = value; }
        void get_Vb_scale() { float value; rP_->get(Vb_scale_eeram_.a16, value); Vb_scale = value; }
        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        uint16_t next() { return next_; }
    #endif
    //
    void load_all();
    void mem_print();
    uint16_t nflt() { return nflt_; }
    uint16_t nhis() { return nhis_; }
    void nominal();
    void nominalize_fault_array();
    void nominalize_history_array();
    int num_diffs();
    void pretty_print(const boolean all);
    void print_fault_array();
    void print_fault_header();
    void print_history_array();
    // put
    #if (PLATFORM_ID==6)  // Photon
        void put_amp(const float input) { amp = input; }
        void put_cutback_gain_sclr(const float input) { cutback_gain_sclr = input; }
        void put_debug(const int input) { debug = input; }
        void put_delta_q(const double input) { delta_q = input; }
        void put_delta_q_model(const double input) { delta_q_model = input; }
        void put_freq(const float input) { freq = input; }
        void put_hys_scale(const float input) { hys_scale = input; }
        void put_Ib_bias_all(const float input) { Ib_bias_all = input; }
        void put_Ib_bias_amp(const float input) { Ib_bias_amp = input; }
        void put_Ib_bias_noa(const float input) { Ib_bias_noa = input; }
        void put_ib_scale_amp(const float input) { ib_scale_amp = input; }
        void put_ib_scale_noa(const float input) { ib_scale_noa = input; }
        void put_ib_select(const int8_t input) { ib_select = input; }
        void put_iflt(const int input) { iflt = input; }
        void put_ihis(const int input) { ihis = input; }
        void put_inj_bias(const float input) { inj_bias = input; }
        void put_isum(const int input) { isum = input; }
        void put_modeling(const uint8_t input) { modeling = input; }
        void put_mon_chm(const uint8_t input) { mon_chm = input; }
        void put_nP(const float input) { nP = input; }
        void put_nS(const float input) { nS = input; }
        void put_preserving(const uint8_t input) { preserving = input; }
        void put_shunt_gain_sclr(const float input) { shunt_gain_sclr = input; }
        void put_sim_chm(const uint8_t input) { sim_chm = input; }
        void put_s_cap_model(const float input) { s_cap_model = input; }
        void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe = input; }
        void put_type(const uint8_t input) { type = input; }
        void put_t_last(const float input) { t_last = input; }
        void put_t_last_model(const float input) { t_last_model = input; }
        void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe = input; }
        void put_Vb_scale(const float input) { Vb_scale = input; }
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_from(input); }
        void put_history(const Flt_st input, const uint8_t i) { history_[i].copy_from(input); }
    #elif PLATFORM_ID == PLATFORM_ARGON
        void put_amp(const float input) { rP_->put(amp_eeram_.a16, input); amp = input; }
        void put_cutback_gain_sclr(const float input) { rP_->put(cutback_gain_sclr_eeram_.a16, input); cutback_gain_sclr = input; }
        void put_debug(const int input) { rP_->put(debug_eeram_.a16, input); debug = input; }
        void put_delta_q(const double input) { rP_->put(delta_q_eeram_.a16, input); delta_q = input; }
        void put_delta_q_model(const double input) { rP_->put(delta_q_model_eeram_.a16, input); delta_q_model = input; }
        void put_freq(const float input) { rP_->put(freq_eeram_.a16, input); freq = input; }
        void put_hys_scale(const float input) { rP_->put(hys_scale_eeram_.a16, input); hys_scale = input; }
        void put_Ib_bias_all(const float input) { rP_->put(Ib_bias_all_eeram_.a16, input); Ib_bias_all = input; }
        void put_Ib_bias_amp(const float input) { rP_->put(Ib_bias_amp_eeram_.a16, input); Ib_bias_amp = input; }
        void put_Ib_bias_noa(const float input) { rP_->put(Ib_bias_noa_eeram_.a16, input); Ib_bias_noa = input; }
        void put_ib_scale_amp(const float input) { rP_->put(ib_scale_amp_eeram_.a16, input); ib_scale_amp = input; }
        void put_ib_scale_noa(const float input) { rP_->put(ib_scale_noa_eeram_.a16, input); ib_scale_noa = input; }
        void put_ib_select(const int8_t input) { rP_->put(ib_select_eeram_.a16, input); ib_select = input; }
        void put_iflt(const int input) { rP_->put(iflt_eeram_.a16, input); iflt = input; }
        void put_ihis(const int input) { rP_->put(ihis_eeram_.a16, input); ihis = input; }
        void put_inj_bias(const float input) { rP_->put(inj_bias_eeram_.a16, input); inj_bias = input; }
        void put_isum(const int input) { rP_->put(isum_eeram_.a16, input); isum = input; }
        void put_modeling(const uint8_t input) { rP_->write(modeling_eeram_.a16, input); modeling = input; }
        void put_mon_chm(const uint8_t input) { rP_->write(mon_chm_eeram_.a16, input); mon_chm = input; }
        void put_nP(const float input) { rP_->put(nP_eeram_.a16, input); nP = input; }
        void put_nS(const float input) { rP_->put(nS_eeram_.a16, input); nS = input; }
        void put_preserving(const uint8_t input) { rP_->write(preserving_eeram_.a16, input); preserving = input; }
        void put_shunt_gain_sclr(const float input) { rP_->put(shunt_gain_sclr_eeram_.a16, input); shunt_gain_sclr = input; }
        void put_sim_chm(const uint8_t input) { rP_->write(sim_chm_eeram_.a16, input); sim_chm = input; }
        void put_s_cap_model(const float input) { rP_->put(s_cap_model_eeram_.a16, input); s_cap_model = input; }
        void put_Tb_bias_hdwe(const float input) { rP_->put(Tb_bias_hdwe_eeram_.a16, input); Tb_bias_hdwe = input; }
        void put_type(const uint8_t input) { rP_->write(type_eeram_.a16, input); type = input; }
        void put_t_last(const float input) { rP_->put(t_last_eeram_.a16, input); t_last = input; }
        void put_t_last_model(const float input) { rP_->put(t_last_model_eeram_.a16, input); t_last_model = input; }
        void put_Vb_bias_hdwe(const float input) { rP_->put(Vb_bias_hdwe_eeram_.a16, input); Vb_bias_hdwe = input; }
        void put_Vb_scale(const float input) { rP_->put(Vb_scale_eeram_.a16, input); Vb_scale = input; }
        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].put(input); }
        Flt_st put_history(const Flt_st input, const uint8_t i);
    #endif
    //
    int read_all();
    boolean tweak_test() { return ( 0x8 & modeling ); } // Driving signal injection completely using software inj_bias 
protected:
    #if PLATFORM_ID == PLATFORM_ARGON
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
        address16b s_cap_model_eeram_;
        address16b Tb_bias_hdwe_eeram_;
        address16b type_eeram_;
        address16b t_last_eeram_;
        address16b t_last_model_eeram_;
        address16b Vb_bias_hdwe_eeram_;
        address16b Vb_scale_eeram_;
        SerialRAM *rP_;
        uint16_t next_;
        Flt_ram *fault_;
        uint16_t nflt_;         // Length of Flt_ram array for faults
        Flt_ram *history_;
        uint16_t nhis_;         // Length of Flt_ram array for history
    #endif
};

#endif
