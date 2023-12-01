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

#include "application.h"
#include "parameters.h"
extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle


Hysteresis::Hysteresis()
: disabled_(false), res_(0), soc_(0), ib_(0), ibs_(0), ioc_(0), dv_hys_(0), dv_dot_(0){};
Hysteresis::Hysteresis(Chemistry *chem)
: disabled_(false), res_(0), soc_(0), ib_(0), ibs_(0), ioc_(0), dv_hys_(0), dv_dot_(0), chem_(chem){}

// Calculate
float Hysteresis::calculate(const float ib, const float soc, const float hys_scale)
{
    ib_ = ib;
    soc_ = soc;

    // Disabled logic
    disabled_ = hys_scale < 1e-5;

    // Calculate
    if ( disabled_ )
    {
        res_ = 0.;
        slr_ = 1.;
        ibs_ = ib;
        ioc_ = ib;
        dv_dot_ = 0.;
    }
    else
    {
        res_ = look_hys(dv_hys_, soc_);
        slr_ = look_slr(dv_hys_, soc_);
        ibs_ = ib_ * slr_;
        ioc_ = dv_hys_ / res_;
        dv_dot_ = (ibs_ - dv_hys_/res_) / chem_->hys_cap;  // Capacitor ode
    }

    return ( dv_dot_ );
}

// Initialize
void Hysteresis::init(const float dv_init)
{
    dv_hys_ = dv_init;
}

// Table lookup
float Hysteresis::look_hys(const float dv, const float soc)
{
    float res;         // return value
    if ( disabled_ )
        res = 0.;
    else
        res = chem_->hys_T_->interp(dv, soc);
    return res;
}

float Hysteresis::look_slr(const float dv, const float soc)
{
    float slr;         // return value
    if ( disabled_ )
        slr = 1.;
    else
        slr = chem_->hys_Ts_->interp(dv, soc);
    return slr;
}

// Print
void Hysteresis::pretty_print()
{
#ifndef DEPLOY_PHOTON
    float res = look_hys(0., 0.8);
    Serial.printf("Hysteresis:\n");
    Serial.printf("  cap%10.1f, F\n", chem_->hys_cap);
    Serial.printf("  disab %d\n", disabled_);
    Serial.printf("  dv_dot%7.3f, V/s\n", dv_dot_);
    Serial.printf("  dv_hys%7.3f, V, SH\n", dv_hys_);
    Serial.printf("  ib%7.3f, A\n", ib_);
    Serial.printf("  ibs%7.3f, A\n", ibs_);
    Serial.printf("  ioc%7.3f, A\n", ioc_);
    Serial.printf("  res%6.4f, null Ohm\n", res_);
    Serial.printf("  res%7.3f, ohm\n", res_);
    Serial.printf("  slr%7.3f,\n", slr_);
    Serial.printf("  soc%8.4f\n", soc_);
    Serial.printf("  tau%10.1f, null, s\n", res*chem_->hys_cap);
    chem_->pretty_print();
#else
     Serial.printf("Hysteresis: silent DEPLOY\n");
#endif
}

// Dynamic update
float Hysteresis::update(const double dt, const boolean init_high, const boolean init_low, const float e_wrap, const float hys_scale,
    const boolean reset_temp)
{
    float dv_max = chem_->hys_Tx_->interp(soc_);
    float dv_min = chem_->hys_Tn_->interp(soc_);

    if ( init_high )
    {
        dv_hys_ = -chem_->dv_min_abs;
        dv_dot_ = 0.;
    }
    else if ( init_low )
    {
        dv_hys_ = max(chem_->dv_min_abs, -e_wrap);
        dv_dot_ = 0.;
    }
    else if ( reset_temp )
    {
        ioc_ = ib_;
        res_ = look_hys(0., soc_);
        slr_ = 1.;
        dv_dot_ = 0.;
        dv_hys_ = 0.;
        res_ = look_hys(dv_hys_, soc_);
        slr_ = look_slr(dv_hys_, soc_);
        ioc_ = ib_ * slr_;
        #ifdef DEBUG_INIT
            if ( sp.Debug_z==-1 ) Serial.printf("ib%7.3f ibs%7.3f ioc%7.3f dv%9.6f res%7.3f slr%7.3f\n", ib_, ibs_, ioc_, dv_hys_, res_, slr_);
        #endif
    }

    // Normal ODE integration
    dv_hys_ += dv_dot_ * dt;
    dv_hys_ = max(min(dv_hys_, dv_max), dv_min);
    return (dv_hys_ * (hys_scale)); // Scale on output only.   Don't retain it for feedback to ode
}

