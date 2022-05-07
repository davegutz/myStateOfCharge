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
#include "Tweak.h"
#include "Battery.h"
#include "retained.h"
extern RetainedPars rp;         // Various parameters to be static at system level

// class Tweak
// constructors
Tweak::Tweak()
  : name_("None"), gain_(0), max_change_(0), delta_q_sat_present_(0), delta_q_sat_past_(0), sat_(false),
  delta_q_max_(0), time_sat_past_(0UL), time_to_wait_(0), delta_hrs_(0) {}
Tweak::Tweak(const String name, const double gain, const double max_change, const double max_tweak,
  const double time_to_wait, t_float *rp_delta_q_inf, t_float *rp_tweak_bias)
  : name_(name), gain_(-1./gain), max_change_(max_change), max_tweak_(max_tweak), delta_q_sat_present_(0),
    delta_q_sat_past_(0), sat_(false), delta_q_max_(-(RATED_BATT_CAP*3600.)), time_sat_past_(millis()), time_to_wait_(time_to_wait),
    rp_delta_q_inf_(rp_delta_q_inf), rp_tweak_bias_(rp_tweak_bias), delta_hrs_(0) {}
Tweak::~Tweak() {}
// operators
// functions
// Process new information and return indicator of new peak found

// Do the tweak
void Tweak::adjust(unsigned long now)
{
  if ( delta_q_sat_past_==0.0 ) return;
  double new_Di;
  double error = delta_q_sat_present_ - delta_q_sat_past_;
  double gain = gain_*24./delta_hrs_;
  new_Di = *rp_tweak_bias_ + max(min(error*gain, max_change_), -max_change_);
  new_Di = max(min(new_Di, max_tweak_), -max_tweak_);

  Serial.printf("          Tweak(%s)::adjust:, past=%10.1f, pres=%10.1f, error=%10.1f, gain=%10.6f, delta_hrs=%10.6f, Di=%7.3f, new_Di=%7.3f,\n",
    name_.c_str(), delta_q_sat_past_, delta_q_sat_present_, error, gain, delta_hrs_, *rp_tweak_bias_, new_Di);

  // Reset delta to keep from wandering away
  double shift = delta_q_sat_past_;
  delta_q_sat_past_ = 1e-5;   // Avoid 0.0 so don't reset the counters
  *rp_delta_q_inf_ -= shift;
  delta_q_sat_present_ -= shift;
  delta_q_max_ -= shift;
  
  *rp_tweak_bias_ = new_Di;
  return;
}

// Print
void Tweak::pretty_print(void)
{
    Serial.printf("Tweak(%s)::\n", name_.c_str());
    Serial.printf("  gain_ =                %10.6f; // Current correction to be made for charge error see 'N/Mg', A/Coulomb/day\n", gain_);
    Serial.printf("  max_change_ =             %7.3f; // Maximum allowed change to calibration adjustment see 'N/MC', A\n", max_change_);
    Serial.printf("  max_tweak_ =              %7.3f; // Maximum allowed calibration adjustment see 'N/Mx', A\n", max_tweak_);
    Serial.printf("  rp_delta_q_inf_ =      %10.1f; // Charge infinity at past update see 'N/Mi', Coulombs\n", *rp_delta_q_inf_);
    Serial.printf("  delta_q_sat_present_ = %10.1f; // Charge infinity at saturation, present see 'N/MP', Coulombs\n", delta_q_sat_present_);
    Serial.printf("  delta_q_sat_past_ =    %10.1f; // Charge infinity at saturation, past see 'N/Mp', Coulombs\n", delta_q_sat_past_);
    Serial.printf("  sat_ =                          %d; // Saturation status, T=saturated\n", sat_);
    Serial.printf("  delta_hrs_ =           %10.6f; // Time since last allowed saturation see 'N/Mz', hr\n", double(millis()-time_sat_past_)/3600000.);
    Serial.printf("  time_to_wait =         %10.6f; // Time to wait before allowing saturation see 'N/Mw', hr\n", time_to_wait_);
    Serial.printf("  tweak_bias =              %7.3f; // Bias on current see 'N/Mk', A\n", *rp_tweak_bias_);
}

// Reset on demand
void Tweak::reset(void)
{
  *rp_delta_q_inf_ = 0.;
  delta_q_sat_present_ = 0.;
  delta_q_sat_past_ = 0.;
  sat_ = false;
  delta_q_max_ = -(RATED_BATT_CAP*3600.);
}

// Save new result
void Tweak::save_new_sat(unsigned long int now)
{
  delta_q_sat_past_ = delta_q_sat_present_;
  delta_q_sat_present_ = delta_q_max_;
  delta_q_max_ = -(RATED_BATT_CAP*3600.);                // Reset
  sat_ = false;
  time_sat_past_ = now;
}

// Monitor the process and return status
boolean Tweak::new_desat(const double curr_in, const double T, const boolean is_sat, unsigned long int now)
{
  *rp_delta_q_inf_ += curr_in * T;
  delta_hrs_ = double(now - time_sat_past_)/double(ONE_HOUR_MILLIS);
  boolean have_new = false;
  if ( sat_ )
  {
    if ( !is_sat && ( delta_hrs_>time_to_wait_ ) )
    {
      have_new = true;
      save_new_sat(now);
    }
    else
    {
      delta_q_max_ = max(delta_q_max_, *rp_delta_q_inf_);
    }
  }
  else
  {
    if ( is_sat )
    {
      sat_ = true;
      delta_q_max_ = max(delta_q_max_, *rp_delta_q_inf_);
    }
  }
  if ( rp.debug==88 ) Serial.printf("Tweak(%s)::update:,  delta_q_inf=%10.1f, is_sat=%d, now=%ld, sat=%d, delta_q_sat_past=%10.1f, delta_q_sat_present=%10.1f, time_sat_past=%ld,\n",
    name_.c_str(), *rp_delta_q_inf_, is_sat, now, sat_, delta_q_sat_past_, delta_q_sat_present_, time_sat_past_);

  return ( have_new );
}

