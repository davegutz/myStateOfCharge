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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include <math.h>
#include "Battery.h"
#include "Coulombs.h"
#include "retained.h"
extern RetainedPars rp; // Various parameters to be static at system level
#include "command.h"
extern CommandPars cp;

// class Coulombs
Coulombs::Coulombs()
  : q_cap_rated_(0), q_cap_scaled_(0), t_rated_(25), t_rlim_(2.5) {}
Coulombs::Coulombs(const double q_cap_rated, const double t_rated, const double t_rlim)
  : q_cap_rated_(q_cap_rated), q_cap_scaled_(q_cap_rated), t_rated_(t_rated), t_rlim_(0.017) {}
Coulombs::~Coulombs() {}
// t_rlim=0.017 allows 1 deg for 1 minute (the update time of the temp read; and the sensor has
// 1 deg resolution).
// operators
// functions

// Scale size of battery and adjust as needed to preserve delta_q.  t_sat_ unchanged.
// Goal is to scale battery and see no change in delta_q on screen of 
// test comparisons.   The rationale for this is that the battery is frequently saturated which
// resets all the model parameters.   This happens daily.   Then both the model and the battery
// are discharged by the same current so the delta_q will be the same.
void Coulombs::apply_cap_scale(const double scale)
{
  q_cap_scaled_ = scale * q_cap_rated_;
  q_ = SOC_ / 100. * q_cap_scaled_;
  q_capacity_ = calculate_saturation_charge(t_last_, q_cap_scaled_);
  delta_q_ = q_ - q_capacity_;
  soc_ = q_ / q_capacity_;
  SOC_ = q_ / q_cap_scaled_ * 100.;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_soc(const double soc)
{
  soc_ = soc;
  q_ = soc*q_capacity_;
  delta_q_ = q_ - q_capacity_;
  SOC_ = q_ / q_cap_scaled_ * 100.;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_SOC(const double SOC)
{
  SOC_ = SOC;
  q_ = SOC / 100. * q_cap_scaled_;
  delta_q_ = q_ - q_capacity_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void Coulombs::apply_delta_q_t(const double delta_q, const double temp_c)
{
  delta_q_ = delta_q;
  q_capacity_ = calculate_capacity(temp_c);
  q_ = q_capacity_ + delta_q;
  soc_ = q_ / q_capacity_;
  SOC_ = q_ / q_cap_scaled_ * 100.;
  resetting_ = true;
}

// Capacity
double Coulombs::calculate_capacity(const double temp_c)
{
    return( q_cap_scaled_ * (1-DQDT*(temp_c - t_rated_)) );
}

// Count coulombs based on true=actual capacity
double Coulombs::count_coulombs(const double dt, const double temp_c, const double charge_curr, const boolean sat, const double t_last)
{
    /* Count coulombs based on true=actual capacity
    Inputs:
        dt              Integration step, s
        temp_c          Battery temperature, deg C
        charge_curr     Charge, A
        sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
        tlast           Past value of battery temperature used for rate limit memory, deg C
    */
    double d_delta_q = charge_curr * dt;
    t_last_ = t_last;

    // Rate limit temperature
    double temp_lim = t_last_ + max(min( (temp_c-t_last_), t_rlim_*dt), -t_rlim_*dt);

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    // TODO:   should we just use q_sat all the time in soc calculation?  (Memory behavior causes problems with saturation
    // detection).
    if ( sat )
    {
        if ( d_delta_q > 0 )
        {
            d_delta_q = 0.;
            if ( !resetting_ ) delta_q_ = 0.;
            else resetting_ = false;     // one pass flag.  Saturation debounce should reset next pass
        }
    }

    // Integration
    q_capacity_ = q_cap_scaled_*(1. + DQDT*(temp_lim - t_rated_));
    delta_q_ = max(min(delta_q_ + d_delta_q - DQDT*q_capacity_*(temp_lim-t_last_), 1.1*(q_cap_scaled_ - q_capacity_)), -q_capacity_);
    q_ = q_capacity_ + delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    SOC_ = q_ / q_cap_scaled_ * 100;

    if ( rp.debug==96 )
        Serial.printf("Coulombs::cc,                 dt,voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc,SOC,       %7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%7.4f,%7.3f,\n",
                    dt,cp.pubList.VOC,  sat_voc(temp_c), temp_lim, sat, charge_curr, d_delta_q, delta_q_, q_, q_capacity_, soc_, SOC_);
    if ( rp.debug==-96 )
        Serial.printf("voc, v_sat, sat, temp_lim, charge_curr, d_d_q, d_q, q, q_capacity,soc, SOC,          \n%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%7.4f,%7.3f,\n",
                    cp.pubList.VOC,  sat_voc(temp_c), temp_lim, sat, charge_curr, d_delta_q, delta_q_, q_, q_capacity_, soc_, SOC_);

    // Save and return
    t_last_ = temp_lim;
    return ( soc_ );
}

// Load states from retained memory
void Coulombs::load(const double delta_q, const double t_last)
{
    delta_q_ = delta_q;
    t_last_ = t_last;
}

// Update states to be saved in retained memory
void Coulombs::update(double *delta_q, double *t_last)
{
    *delta_q = delta_q_;
    *t_last = t_last_;
}
