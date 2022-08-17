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
  : name_("None"), max_change_(0), sat_(false),time_sat_past_(0UL), time_to_wait_(0), delta_hrs_(0) {}
Tweak::Tweak(const String name, const double max_change, const double max_tweak, const double time_to_wait,
  float *rp_delta_q_cinf, float *rp_delta_q_dinf, float *rp_tweak_sclr, const double coul_eff)
  : name_(name), max_change_(max_change), max_tweak_(max_tweak), sat_(false), time_sat_past_(millis()),
    time_to_wait_(time_to_wait), rp_delta_q_cinf_(rp_delta_q_cinf), rp_delta_q_dinf_(rp_delta_q_dinf),
    rp_tweak_sclr_(rp_tweak_sclr), delta_hrs_(0), coul_eff_(coul_eff)
    {
      // Always reinit these on boot because they get saved on previous run
      *rp_delta_q_cinf_ = -RATED_BATT_CAP*3600.;
      *rp_delta_q_dinf_ = RATED_BATT_CAP*3600.;
    }
Tweak::~Tweak() {}
// operators
// functions
// Process new information and return indicator of new peak found

/* Do the tweak and display change
  Inputs:
    now                   Time since boot, ms
    delta_hrs_            Time since last allowed saturation see 'N/Mz', hr
    max_change_           Maximum allowed change to calibration scalar see 'N/Mc'
    max_tweak_            Maximum allowed calibration scaler
  States:
   *rp_delta_q_cinf_      Charge infinity at past update see 'N/Mi', Coulombs
   *rp_delta_q_dinf_      Discharge infinity at past update see 'N/Mi', Coulombs
  Outputs:
    tweak_sclr_           Scalar on Coulombic Efficiency see 'N/Mk'
*/
void Tweak::adjust(unsigned long now)
{
  // Candidate change scalar
  double new_Si = -(*rp_delta_q_dinf_ / (*rp_delta_q_cinf_));

  // Soft landing / stability
  new_Si = TWEAK_GAIN*(new_Si-1.) + 1.;

  // Apply limits
  double new_tweak_sclr = *rp_tweak_sclr_ * new_Si;
  new_tweak_sclr = max(min(new_tweak_sclr, *rp_tweak_sclr_+max_change_), *rp_tweak_sclr_-max_change_);
  new_tweak_sclr = max(min(new_tweak_sclr, 1.+max_tweak_), 1.-max_tweak_);

  // Result
  if ( *rp_delta_q_cinf_>=0. && *rp_delta_q_dinf_<=0. )  // Exclude first cycle after boot:  uncertain history
  {
    *rp_tweak_sclr_ = new_tweak_sclr;
    Serial.printf("          Tweak(%s)::adjust:, cinf=%10.1f, dinf=%10.1f, coul_eff=%9.6f, scaler=%9.6f, effective coul_eff=%9.6f\n",
      name_.c_str(), *rp_delta_q_cinf_, *rp_delta_q_dinf_, coul_eff_, *rp_tweak_sclr_, coul_eff_*(*rp_tweak_sclr_));
  }
  else
  {
    Serial.printf("          Tweak(%s)::ignore:, cinf=%10.1f, dinf=%10.1f, coul_eff=%9.6f, scaler=%9.6f, effective coul_eff=%9.6f\n",
      name_.c_str(), *rp_delta_q_cinf_, *rp_delta_q_dinf_, coul_eff_, *rp_tweak_sclr_, coul_eff_*(*rp_tweak_sclr_));
  }


  // Reset for next charge cycle
  *rp_delta_q_cinf_ = 0.;
  *rp_delta_q_dinf_ = 0.;
  
  return;
}

// Print
void Tweak::pretty_print(void)
{
    Serial.printf("Tweak(%s)::\n", name_.c_str());
    Serial.printf("  max_change=%7.3f; 'N/MC'\n", max_change_);
    Serial.printf("  max_tweak=%7.3f;  'N/Mx'\n", max_tweak_);
    Serial.printf("  rp_delta_q_cinf=%10.1f; 'N/Mi', Coulombs\n", *rp_delta_q_cinf_);
    Serial.printf("  rp_delta_q_dinf=%10.1f; 'N/Mi', Coulombs\n", *rp_delta_q_dinf_);
    Serial.printf("  sat=%d; T=sat\n", sat_);
    Serial.printf("  delta_hrs=%10.6f; 'N/Mz', hr\n", double(millis()-time_sat_past_)/3600000.);
    Serial.printf("  time_to_wait=%10.6f; 'N/Mw', hr\n", time_to_wait_);
    Serial.printf("  tweak_sclr=%7.3f; 'N/Mk'\n", *rp_tweak_sclr_);
    Serial.printf("  coul_eff=%9.5f;\n", coul_eff_);
}

// reset:  Reset on call.   Reset all indicators and states to boot status.
void Tweak::reset(void)
{
  *rp_delta_q_cinf_ = 0.;
  *rp_delta_q_dinf_ = 0.;
  sat_ = false;
}

/* save_new_sat:  Save new result
  Inputs:
    now                   Time since boot, ms
  Outputs:
    sat_                  Indication that battery is saturated, T=saturated
    time_sat_past_        Time at last declared saturation, ms
*/
void Tweak::save_new_sat(unsigned long int now)
{
  sat_ = false;
  time_sat_past_ = now;
}

/* Monitor the process and return status
  INPUTS:
    curr_in       Current into process, A
    T             Time since last call, sec
    is_sat        Is the battery in saturation, T=saturated
    now           Time since boot, ms
    coul_eff_     Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
    delta_hrs_    Time since last allowed saturation see 'N/Mz', hr
    sat_          Indication that battery is saturated, T=saturated
*/
boolean Tweak::new_desat(const double curr_in, const double T, const boolean is_sat, unsigned long int now)
{
  double d_delta_q_inf = curr_in * T;
  if ( curr_in>0. )
  {
    d_delta_q_inf *= coul_eff_ * (*rp_tweak_sclr_);
    *rp_delta_q_cinf_ += d_delta_q_inf;
  }
  else
  {
    *rp_delta_q_dinf_ += d_delta_q_inf;
  }
  delta_hrs_ = double(now - time_sat_past_)/double(ONE_HOUR_MILLIS);
  boolean have_new = false;
  if ( sat_ )
  {
    if ( !is_sat && ( delta_hrs_>time_to_wait_ ) )
    {
      have_new = true;
      save_new_sat(now);
    }
  }
  else
  {
    if ( is_sat )
    {
      sat_ = true;
    }
  }

  return ( have_new );
}

