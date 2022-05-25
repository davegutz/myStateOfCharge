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
  const double time_to_wait, float *rp_delta_q_inf, float *rp_tweak_bias, const double coul_eff)
  : name_(name), gain_(-1./gain), max_change_(max_change), max_tweak_(max_tweak), delta_q_sat_present_(0),
    delta_q_sat_past_(0), sat_(false), delta_q_max_(-(RATED_BATT_CAP*3600.)), time_sat_past_(millis()), time_to_wait_(time_to_wait),
    rp_delta_q_inf_(rp_delta_q_inf), rp_tweak_bias_(rp_tweak_bias), delta_hrs_(0), coul_eff_(coul_eff) {}
Tweak::~Tweak() {}
// operators
// functions
// Process new information and return indicator of new peak found

/* Do the tweak and display change
  Inputs:
    now                   Time since boot, ms
    delta_hrs_            Time since last allowed saturation see 'N/Mz', hr
    gain_                 Current correction to be made for charge error, A/Coulomb/day
    max_change_           Maximum allowed change to calibration adjustment, A
    max_tweak_            Maximum allowed calibration adjustment, A\n
  States:
    delta_q_max_          Running tab since last de-saturation of potential new delta_q_sat
    delta_q_sat_past_     Charge infinity at saturation, past see 'N/Mp', Coulombs
    delta_q_sat_present_  Charge infinity at saturation, present see 'N/MP', Coulombs
   *rp_delta_q_inf_       Charge infinity at past update see 'N/Mi', Coulombs
  Outputs:
    tweak_bias_           Bias on current see 'N/Mk', A
*/
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
  *rp_tweak_bias_ = new_Di;

  // Reset deltas to prevent windup
  double shift = delta_q_sat_past_;
  delta_q_sat_past_ = 1e-5;   // Avoid 0.0 so don't reset the counters
  *rp_delta_q_inf_ -= shift;
  delta_q_sat_present_ -= shift;
  delta_q_max_ -= shift;
  
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
    Serial.printf("  sat_ =                          %d; // Indication that battery is saturated, T=saturated\n", sat_);
    Serial.printf("  delta_hrs_ =           %10.6f; // Time since last allowed saturation see 'N/Mz', hr\n", double(millis()-time_sat_past_)/3600000.);
    Serial.printf("  time_to_wait =         %10.6f; // Time to wait before allowing saturation see 'N/Mw', hr\n", time_to_wait_);
    Serial.printf("  tweak_bias =              %7.3f; // Bias on current see 'N/Mk', A\n", *rp_tweak_bias_);
}

// reset:  Reset on call.   Reset all indicators and states to boot status.
void Tweak::reset(void)
{
  *rp_delta_q_inf_ = 0.;
  delta_q_sat_present_ = 0.;
  delta_q_sat_past_ = 0.;
  sat_ = false;
  delta_q_max_ = -(RATED_BATT_CAP*3600.);
}

/* save_new_sat:  Save new result
  Inputs:
    now                   Time since boot, ms
    delta_q_sat_present_  Charge infinity at saturation, present see 'N/MP', Coulombs
  Outputs:
    delta_q_max_          Running tab since last de-saturation of potential new delta_q_sat
    delta_q_sat_past_     Charge infinity at saturation, past see 'N/Mp', Coulombs
    delta_q_sat_present_  Charge infinity at saturation, present see 'N/MP', Coulombs
    sat_                  Indication that battery is saturated, T=saturated
    time_sat_past_        Time at last declared saturation, ms
*/
void Tweak::save_new_sat(unsigned long int now)
{
  delta_q_sat_past_ = delta_q_sat_present_;
  delta_q_sat_present_ = delta_q_max_;
  delta_q_max_ = -(RATED_BATT_CAP*3600.);                // Reset
  sat_ = false;
  time_sat_past_ = now;
}

/* Monitor the process and return status
  INPUTS:
    curr_in       Current into process, A
    T             Time since last call, sec
    is_sat        Is the battery in saturation, T=saturated
    now           Time since boot, ms
    coul_eff_     Coulombic efficiency - the fraction of charging input that gets turned into useable Coulombs
    delta_hrs_    Time since last allowed saturation see 'N/Mz', hr
    delta_q_max_  Running tab since last de-saturation of potential new delta_q_sat
    sat_          Indication that battery is saturated, T=saturated
*/
boolean Tweak::new_desat(const double curr_in, const double T, const boolean is_sat, unsigned long int now)
{
  double d_delta_q_inf = curr_in * T;
  if ( curr_in>0. ) d_delta_q_inf *= coul_eff_;
  *rp_delta_q_inf_ += d_delta_q_inf;
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
  if ( rp.debug==99 )  Serial.printf("Tweak:,%7.3f,%7.3f,", curr_in, d_delta_q_inf);

  return ( have_new );
}

