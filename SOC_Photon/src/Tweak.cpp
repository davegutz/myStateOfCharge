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
  : gain_(0), max_change_(0), delta_q_inf_past_(0), delta_q_sat_present_(0), delta_q_sat_past_(0), sat_(false), delta_q_max_(0){}
Tweak::Tweak(const double gain, const double max_change)
  : gain_(-1./gain), max_change_(max_change), delta_q_inf_past_(0), delta_q_sat_present_(0), delta_q_sat_past_(0), sat_(false),
  delta_q_max_(0) {}
Tweak::~Tweak() {}
// operators
// functions
// Process new information and return indicator of new peak found

// Do the tweak
double Tweak::adjust(const double Di)
{
  double new_Di;
  new_Di = Di + max(min(gain_*(delta_q_sat_present_ - delta_q_sat_past_), max_change_), -max_change_);

  Serial.printf("              Tweak::adjust:, Di=%7.3f, new_Di=%7.3f,\n", Di, new_Di);
  
  return ( new_Di );
}

// Print
void Tweak::pretty_print(void)
{
    Serial.printf("Tweak::");
    Serial.printf("  gain_ =                %7.3f; // Current correction to be made for charge error, A/Coulomb\n", gain_);
    Serial.printf("  max_change_ =          %7.3f; // Maximum allowed calibration adjustment, A\n", max_change_);
    Serial.printf("  delta_q_inf_past_ =    %7.3f; // Charge infinity at past update, Coulombs\n", delta_q_inf_past_);
    Serial.printf("  delta_q_sat_present_ = %7.3f; // Charge infinity at saturation, present, Coulombs\n", delta_q_sat_present_);
    Serial.printf("  delta_q_sat_past_ =    %7.3f; // Charge infinity at saturation, past, Coulombs\n", delta_q_sat_past_);
    Serial.printf("  sat_ =                 %d;    // Saturation status, T=saturated\n", sat_);
    Serial.printf("  tweak_bias =           %7.3f; // Bias on current, A\n", rp.tweak_bias);
}

// Reset on demand
void Tweak::reset(void)
{
  delta_q_inf_past_ = 0.;
  delta_q_sat_present_ = 0.;
  delta_q_sat_past_ = 0.;
  sat_ = false;
  delta_q_max_ = 0.;
}

// Save new result
void Tweak::save_new_sat(void)
{
  delta_q_sat_past_ = delta_q_sat_present_;
  delta_q_sat_present_ = delta_q_max_;
  delta_q_max_ = -q_cap_rated;                // Reset
  sat_ = false;
}

// Monitor the process and return status
boolean Tweak::update(const double delta_q_inf, const boolean is_sat)
{
  boolean have_new = false;
  if ( sat_ )
  {
    if ( !is_sat )
    {
      have_new = true;
      save_new_sat();
    }
    else
    {
      delta_q_max_ = max(delta_q_max_, delta_q_inf);
    }
  }
  else
  {
    if ( is_sat )
    {
      sat_ = true;
      delta_q_max_ = max(delta_q_max_, delta_q_inf);
    }
  }
  delta_q_inf_past_ = delta_q_inf;
  if ( rp.debug==88 ) Serial.printf("Tweak::update:,  sat=%d, delta_q_inf_past=%10.1f, delta_q_sat_past=%10.1f, delta_q_sat_present=%10.1f,\n", sat_, delta_q_inf_past_, delta_q_sat_past_, delta_q_sat_present_);

  return ( have_new );
}

