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
  : q_cap_rated_(0), t_rated_(25), t_rlim_(2.5){}
Coulombs::Coulombs(const double q_cap_rated, const double t_rated, const double t_rlim)
  : q_cap_rated_(q_cap_rated), t_rated_(t_rated), t_rlim_(2.5){}
Coulombs::~Coulombs() {}
// operators
// functions

// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_soc(const double soc)
{
  soc_ = soc;
  q_ = soc*q_capacity_;
  delta_q_ = q_ - q_capacity_;
  SOC_ = q_ / q_cap_rated_ * 100.;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_SOC(const double SOC)
{
  SOC_ = SOC;
  q_ = SOC / 100. * q_cap_rated_;
  delta_q_ = q_ - q_capacity_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void Coulombs::apply_delta_q(const double delta_q)
{
  delta_q_ = delta_q;
  q_ = q_capacity_ + delta_q;
  soc_ = q_ / q_capacity_;
  SOC_ = q_ / q_cap_rated_ * 100.;
  resetting_ = true;
}

// Capacity
double Coulombs::calculate_capacity(const double temp_c)
{
    return( q_cap_rated_ * (1-DQDT*(temp_c - t_rated_)) );
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
    if ( false )    // TODO:  BatteryModel needs to use something different than Battery.  TODO:  add Coulombs to Battery and separate BatteryModel
    // if ( sat_ )
    {
        if ( d_delta_q > 0 )
        {
            d_delta_q = 0.;
            if ( !resetting_ ) delta_q_ = 0.;
            else resetting_ = false;     // one pass flag.  Saturation debounce should reset next pass
        }
    }

    // Integration
    q_capacity_ = q_cap_rated_*(1. + DQDT*(temp_lim - t_rated_));
    delta_q_ = max(min(delta_q_ + d_delta_q - DQDT*q_capacity_*(temp_lim-t_last_), 1.1*(q_cap_rated_ - q_capacity_)), -q_capacity_);
    q_ = q_capacity_ + delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    SOC_ = q_ / q_cap_rated_ * 100;

    if ( rp.debug==76 )
        Serial.printf("Coulombs::count_coulombs:,  dt,voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc,SOC,       %7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%9.1f,%7.3f,\n",
                    dt,cp.pubList.VOC,  sat_voc(temp_c), temp_lim, sat, charge_curr, d_delta_q, delta_q_, q_, q_capacity_, soc_, SOC_);
    if ( rp.debug==-76 )
        Serial.printf("voc, v_sat, sat, temp_lim, charge_curr, d_d_q, d_q, q, q_capacity,soc, SOC,          \n%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%9.1f,%7.3f,\n",
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

void Coulombs::prime(const double init_q, const double init_t_c)
// void Coulombs::prime(const double nom_q_cap_, const double t_rated_, const double init_q, const double init_t_c, const double s_cap)
{
    q_ = init_q;
    q_capacity_ = calculate_capacity(init_t_c);
    soc_ = q_ / q_capacity_;
    SOC_ = q_ / q_cap_rated_ * 100.;
    Serial.printf("re-primed Coulombs\n");
}


// Update states to be saved in retained memory
void Coulombs::update(double *delta_q, double *t_last)
{
    *delta_q = delta_q_;
    *t_last = t_last_;
}




























// struct CoulombCounter
CoulombCounter::CoulombCounter(){}

// functions
void CoulombCounter::prime(const double nom_q_cap_, const double t_rated_, const double init_q, const double init_t_c, const double s_cap)
{
    nom_q_cap = nom_q_cap_;
    t_rated = t_rated_;
    q = init_q;
    t_sat = init_t_c;
    q_cap = nom_q_cap * s_cap;
    q_sat = calculate_saturation_charge(t_sat, q_cap);
    q_capacity = calculate_capacity(init_t_c);
    soc = q / q_capacity;
    SOC = q / nom_q_cap * 100.;
    Serial.printf("re-primed CoulombCounter\n");
}

// Scale size of battery and adjust as needed to preserve delta_q.  t_sat_ unchanged.
// Goal is to scale battery and see no change in delta_q on screen of 
// test comparisons.   The rationale for this is that the battery is frequently saturated which
// resets all the model parameters.   This happens daily.   Then both the model and the battery
// are discharged by the same current so the delta_q will be the same.
void CoulombCounter::apply_cap_scale(const double scale)
{
  q_cap = scale * nom_q_cap;
  q = SOC / 100. * q_cap;
  q_sat = calculate_saturation_charge(t_sat, q_cap);
  q_capacity = q_sat;
  soc = q / q_capacity;
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void CoulombCounter::apply_soc(const double soc_)
{
  soc = soc_;
  q = soc*q_capacity;
  delta_q = q - q_capacity;
  SOC = q / q_cap * 100.;
  resetting = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void CoulombCounter::apply_SOC(const double SOC_)
{
  SOC = SOC_;
  q = SOC / 100. * q_cap;
  delta_q = q - q_capacity;
  soc = q / q_capacity;
  resetting = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void CoulombCounter::apply_delta_q(const double delta_q_)
{
  delta_q = delta_q_;
  q = q_capacity + delta_q;
  soc = q / q_capacity;
  SOC = q / q_cap * 100.;
  resetting = true;
}

// Capacity
double CoulombCounter::calculate_capacity(const double temp_c_)
{
    return( q_sat * (1-DQDT*(temp_c_ - t_sat)) );
}

// Saturation charge
double CoulombCounter::calculate_saturation_charge(const double t_sat_, const double q_cap_)
{
    return( q_cap_ * ((t_sat_ - t_rated)*DQDT + 1.) );
}

// Count coulombs based on true=actual capacity
double CoulombCounter::count_coulombs(const double dt_, const double temp_c_, const double charge_curr_, const boolean sat_, const double t_last_)
{
    /* Count coulombs based on true=actual capacity
    Inputs:
        dt_             Integration step, s
        temp_c_         Battery temperature, deg C
        charge_curr_    Charge, A
        sat_            Indicator that battery is saturated (VOC>threshold(temp)), T/F
        tlast_          Past value of battery temperature used for rate limit memory, deg C
    */
    double d_delta_q_ = charge_curr_ * dt_;
    t_last = t_last_;

    // Rate limit temperature
    double temp_lim = t_last + max(min( (temp_c_-t_last), t_rat*dt_), -t_rat*dt_);

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    // TODO:   should we just use q_sat all the time in soc calculation?  (Memory behavior causes problems with saturation
    // detection).
    if ( false )    // TODO:  BatteryModel needs to use something different than Battery.  TODO:  add CoulombCounter to Battery and separate BatteryModel
    // if ( sat_ )
    {
        if ( d_delta_q_ > 0 )
        {
            d_delta_q_ = 0.;
            if ( !resetting ) delta_q = 0.;
            else resetting = false;     // one pass flag.  Saturation debounce should reset next pass
        }
        t_sat = temp_c_;  // TODO:  not used
        q_sat = ((t_sat - t_rated)*DQDT + 1.)*q_cap; // TODO:  not used
    }

    // Integration
    q_capacity = q_cap*(1. + DQDT*(temp_lim - t_rated));
    delta_q = max(min(delta_q + d_delta_q_ - DQDT*q_capacity*(temp_lim-t_last), 1.1*(q_cap - q_capacity)), -q_capacity);
    q = q_capacity + delta_q;

    // Normalize
    soc = q / q_capacity;
    SOC = q / nom_q_cap * 100;

    if ( rp.debug==76 )
        Serial.printf("CoulombCounter::count_coulombs:,  dt,voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, tsat,q_capacity,soc,SOC,       %7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%9.1f,%7.3f,%7.3f,\n",
                    dt_,cp.pubList.VOC,  sat_voc(temp_c_), temp_lim, sat_, charge_curr_, d_delta_q_, delta_q, q, t_sat, q_capacity, soc, SOC);
    if ( rp.debug==-76 )
        Serial.printf("voc, v_sat, sat, temp_lim, charge_curr, d_d_q, d_q, q_sat, tsat,q_capacity,soc, SOC,          \n%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%9.1f,%7.3f,%7.3f,\n",
                    cp.pubList.VOC,  sat_voc(temp_c_), temp_lim, sat_, charge_curr_, d_delta_q_, delta_q, q, t_sat, q_capacity, soc, SOC);

    // Save and return
    t_last = temp_lim;
    return ( soc );
}

// Load states from retained memory
void CoulombCounter::load(const double delta_q_, const double t_sat_, const double q_sat_, const double t_last_)
{
    delta_q = delta_q_;
    t_sat = t_sat_;
    q_sat = q_sat_;
    t_last = t_last_;
}

// Update states to be saved in retained memory
void CoulombCounter::update(double *delta_q_, double *t_sat_, double *q_sat_, double *t_last_)
{
    *delta_q_ = delta_q;
    *t_sat_ = t_sat;
    *q_sat_ = q_sat;
    *t_last_ = t_last;
}
