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
    int8_t debug;                   // Level of debug printing, int8_t
    double delta_q;                 // Charge change since saturated, C
    double delta_q_model;           // Charge change since saturated, C
    int isum;                       // Summary location.   Begins at -1 because first action is to increment isum
    uint8_t modeling;               // Driving saturation calculation with model.  Bits specify which signals use model, uint8_t
    float t_last;                   // Updated value of battery temperature injection when sp.modeling and proper wire connections made, deg C
    float t_last_model;             // Battery temperature past value for rate limit memory, deg C

    // functions
    boolean is_corrupt();
    void large_reset() { nominal(); }
    boolean mod_any() { return ( 0<modeling ); }        // Using any
    boolean mod_ib() { return ( 0x4 & modeling ); }     // Using Sim as source of ib
    boolean mod_none() { return ( 0==modeling ); }      // Using nothing
    boolean mod_tb() { return ( 0x1 & modeling ); }     // Using Sim as source of tb
    boolean mod_vb() { return ( 0x2 & modeling ); }     // Using Sim as source of vb
    int8_t debug_get() { int8_t value; rP_->get(debug_eeram_.a16, value); return value; }
    void debug_put(const int8_t input) { rP_->put(debug_eeram_.a16, input); debug = input; }
    double delta_q_get() { double value; rP_->get(delta_q_eeram_.a16, value); return value; }
    void delta_q_put(const double input) { rP_->put(delta_q_eeram_.a16, input); delta_q = input; }
    double delta_q_model_get() { double value; rP_->get(delta_q_model_eeram_.a16, value); return value; }
    void delta_q_model_put(const double input) { rP_->put(delta_q_model_eeram_.a16, input); delta_q_model = input; }
    double isum_get() { int value; rP_->get(isum_eeram_.a16, value); return value; }
    void isum_put(const int input) { rP_->put(isum_eeram_.a16, input); isum = input; }
    void load_all();
    uint8_t modeling_get() { return rP_->read(modeling_eeram_.a16); }
    void modeling_put(const uint8_t input) { rP_->write(modeling_eeram_.a16, input); modeling = input; }
    void nominal();
    int num_diffs();
    void pretty_print(const boolean all);
    int read_all();
    int assign_all();
    int8_t t_last_get() { float value; rP_->get(t_last_eeram_.a16, value); return value; }
    void t_last_put(const float input) { rP_->put(t_last_eeram_.a16, input); t_last = input; }
    int8_t t_last_model_get() { float value; rP_->get(t_last_model_eeram_.a16, value); return value; }
    void t_last_model_put(const float input) { rP_->put(t_last_model_eeram_.a16, input); t_last_model = input; }
    boolean tweak_test() { return ( 0x8 & modeling ); } // Driving signal injection completely using software inj_bias 
protected:
    address16b debug_eeram_;
    address16b delta_q_eeram_;
    address16b delta_q_model_eeram_;
//   float shunt_gain_sclr = 1.;             // Shunt gain scalar
//   float Ib_scale_amp = CURR_SCALE_AMP;    // Calibration scalar of amplified shunt sensor, A
//   float ib_bias_amp = CURR_BIAS_AMP;      // Calibration adder of amplified shunt sensor, A
//   float Ib_scale_noa = CURR_SCALE_NOA;    // Calibration scalar of non-amplified shunt sensor, A
//   float ib_bias_noa = CURR_BIAS_NOA;      // Calibration adder of non-amplified shunt sensor, A
//   float ib_bias_all = CURR_BIAS_ALL;      // Bias on all shunt sensors, A
//   int8_t ib_select = FAKE_FAULTS;         // Force current sensor (-1=non-amp, 0=auto, 1=amp)
//   float Vb_bias_hdwe = VOLT_BIAS;         // Calibrate Vb, V
    address16b isum_eeram_;
    address16b modeling_eeram_;
    //   uint8_t modeling = MODELING;  // Driving saturation calculation with model.  Bits specify which signals use model
//   float amp = 0.;               // Injected amplitude, A pk (0-18.3)
//   float freq = 0.;              // Injected frequency, Hz (0-2)
//   uint8_t type = 0;             // Injected waveform type.   0=sine, 1=square, 2=triangle
//   float inj_bias = 0.;          // Constant bias, A
//   float Tb_bias_hdwe = TEMP_BIAS; // Bias on Tb sensor, deg C
//   float s_cap_model = 1.;         // Scalar on battery model size
//   float cutback_gain_scalar = 1.; // Scalar on battery model saturation cutback function
//           // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
//   int iflt = -1;                // Fault snap location.   Begins at -1 because first action is to increment iflt
//   float hys_scale = HYS_SCALE;  // Hysteresis scalar
//   float nP = NP;                // Number of parallel batteries in bank, e.g. '2P1S'
//   float nS = NS;                // Number of series batteries in bank, e.g. '2P1S'
//   uint8_t mon_chm = MON_CHEM;   // Monitor battery chemistry type
//   uint8_t sim_chm = SIM_CHEM;   // Simulation battery chemistry type
//   float Vb_scale = 1.;          // Calibration scalar for Vb. V/count
    SerialRAM *rP_;
    address16b t_last_eeram_;
    address16b t_last_model_eeram_;
};

#endif
