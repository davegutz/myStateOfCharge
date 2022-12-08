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
    // functions
    boolean is_corrupt();
    void large_reset() { nominal(); }
    boolean mod_any() { return ( 0<modeling() ); }        // Using any
    boolean mod_ib() { return ( 0x4 & modeling() ); }     // Using Sim as source of ib
    boolean mod_none() { return ( 0==modeling() ); }      // Using nothing
    boolean mod_tb() { return ( 0x1 & modeling() ); }     // Using Sim as source of tb
    boolean mod_vb() { return ( 0x2 & modeling() ); }     // Using Sim as source of vb
    int8_t debug_ram;                   // Level of debug printing, int8_t
    int8_t debug() { return rP_->read(debug_.a16); }
    void debug(const int8_t input) { rP_->write(debug_.a16, input); debug_ram = input; }
    double delta_q_ram;                 // Charge change since saturated, C
    double delta_q() { double value; rP_->get(delta_q_.a16, value); return value; }
    void delta_q(const double input) { rP_->put(delta_q_.a16, input); delta_q_ram = input; }
    void load_all();
    uint8_t modeling_ram;               // Driving saturation calculation with model.  Bits specify which signals use model, uint8_t
    uint8_t modeling() { return rP_->read(modeling_.a16); }
    void modeling(const uint8_t input) { rP_->write(modeling_.a16, input); modeling_ram = input; }
    void nominal();
    int num_diffs();
    void pretty_print(const boolean all);
    int read_all();
    int assign_all();
    float t_last_ram;                   // Updated value of battery temperature injection when rp.modeling and proper wire connections made, deg C
    int8_t t_last() { return rP_->read(t_last_.a16); }
    void t_last(const float input) { rP_->write(t_last_.a16, input); t_last_ram = input; }
    boolean tweak_test() { return ( 0x8 & modeling() ); } // Driving signal injection completely using software inj_bias 
protected:
    address16b debug_;
    address16b delta_q_;
//   float t_last = RATED_TEMP;    // Updated value of battery temperature injection when rp.modeling and proper wire connections made, deg C
//   double delta_q_model = 0.;    // Coulomb Counter state for model, C
//   float t_last_model = RATED_TEMP;        // Battery temperature past value for rate limit memory, deg C
//   float shunt_gain_sclr = 1.;             // Shunt gain scalar
//   float Ib_scale_amp = CURR_SCALE_AMP;    // Calibration scalar of amplified shunt sensor, A
//   float ib_bias_amp = CURR_BIAS_AMP;      // Calibration adder of amplified shunt sensor, A
//   float Ib_scale_noa = CURR_SCALE_NOA;    // Calibration scalar of non-amplified shunt sensor, A
//   float ib_bias_noa = CURR_BIAS_NOA;      // Calibration adder of non-amplified shunt sensor, A
//   float ib_bias_all = CURR_BIAS_ALL;      // Bias on all shunt sensors, A
//   int8_t ib_select = FAKE_FAULTS;         // Force current sensor (-1=non-amp, 0=auto, 1=amp)
//   float Vb_bias_hdwe = VOLT_BIAS;         // Calibrate Vb, V
    address16b modeling_;
    //   uint8_t modeling = MODELING;  // Driving saturation calculation with model.  Bits specify which signals use model
//   float amp = 0.;               // Injected amplitude, A pk (0-18.3)
//   float freq = 0.;              // Injected frequency, Hz (0-2)
//   uint8_t type = 0;             // Injected waveform type.   0=sine, 1=square, 2=triangle
//   float inj_bias = 0.;          // Constant bias, A
//   float Tb_bias_hdwe = TEMP_BIAS; // Bias on Tb sensor, deg C
//   float s_cap_model = 1.;         // Scalar on battery model size
//   float cutback_gain_scalar = 1.; // Scalar on battery model saturation cutback function
//           // Set this to 0. for one compile-upload cycle if get locked on saturation overflow loop
//   int isum = -1;                // Summary location.   Begins at -1 because first action is to increment isum
//   int iflt = -1;                // Fault snap location.   Begins at -1 because first action is to increment iflt
//   float hys_scale = HYS_SCALE;  // Hysteresis scalar
//   float nP = NP;                // Number of parallel batteries in bank, e.g. '2P1S'
//   float nS = NS;                // Number of series batteries in bank, e.g. '2P1S'
//   uint8_t mon_chm = MON_CHEM;   // Monitor battery chemistry type
//   uint8_t sim_chm = SIM_CHEM;   // Simulation battery chemistry type
//   float Vb_scale = 1.;          // Calibration scalar for Vb. V/count
    SerialRAM *rP_;
    address16b t_last_;
};

#endif
