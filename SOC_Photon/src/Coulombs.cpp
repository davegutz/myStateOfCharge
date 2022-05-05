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
extern PublishPars pp;            // For publishing

// Structure Chemistry
// Assign parameters of model
void Chemistry::assign_mod(const String mod_str)
{
  int i;
  uint8_t mod = encode(mod_str);
  switch (mod)
  {
    case ( '0' ):  // "Battleborn";
      dqdt    = 0.01;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
      low_voc = 10.;      // Voltage threshold for BMS to turn off battery;
      low_t   = 0;        // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
      m_t     = m_t_bb;   // Number temperature breakpoints for voc table
      n_s     = n_s_bb;   //Number of soc breakpoints voc table
      delete y_t;     y_t = new float[m_t];
      delete x_soc;   x_soc = new float[n_s];
      delete t_voc;   t_voc = new float[m_t*n_s];
      for (i=0; i<m_t; i++) y_t[i] = y_t_bb[i];
      for (i=0; i<n_s; i++) x_soc[i] = x_soc_bb[i];
      for (i=0; i<m_t*n_s; i++) t_voc[i] = t_voc_bb[i];
      n_n     = n_n_bb;   // Number of temperature breakpoints for x_soc_min table
      delete x_soc_min;    x_soc_min = new float[n_n];
      delete t_soc_min;    t_soc_min = new float[n_n];
      for (i=0; i<n_n; i++) x_soc_min[i] = x_soc_min_bb[i];
      for (i=0; i<n_n; i++) t_soc_min[i] = t_soc_min_bb[i];
      hys_cap = 3.6e6;    // Capacitance of hysteresis, Farads
      n_h     = n_h_bb;
      m_h     = m_h_bb;
      delete x_dv;    x_dv = new float[n_h];
      delete y_soc;   y_soc = new float[m_h];
      delete t_r;     t_r = new float[m_h*n_h];
      for (i=0; i<n_h; i++) x_dv[i] = x_dv_bb[i];
      for (i=0; i<m_h; i++) y_soc[i] = y_soc_bb[i];
      for (i=0; i<m_h*n_h; i++) t_r[i] = t_r_bb[i];
      v_sat       = 13.85;  // Saturation threshold at temperature, deg C
      dvoc_dt     = 0.004;  // Change of VOC with operating temperature in range 0 - 50 C V/deg C
      dv          = 0.01;   // Adjustment for calibration error, V
      r_0         = 0.003;  // Randles R0, ohms   
      r_ct        = 0.0016; // Randles charge transfer resistance, ohms
      r_diff      = 0.0077; // Randles diffusion resistance, ohms
      tau_ct      = 0.2;    // Randles charge transfer time constant, s (=1/Rct/Cct)
      tau_diff    = 83.;    // Randles diffusion time constant, s (=1/Rdif/Cdif)
      tau_sd      = 1.8e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec
      r_sd        = 70;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
      break;
    case ( '1' ):  // "LION";
      dqdt    = 0.02;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
      low_voc = 9.;       // Voltage threshold for BMS to turn off battery;
      low_t   = 0;        // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
      m_t     = m_t_li;   // Number temperature breakpoints for voc table
      n_s     = n_s_li;   //Number of soc breakpoints voc table
      delete y_t;     y_t = new float[m_t];
      delete x_soc;   x_soc = new float[n_s];
      delete t_voc;   t_voc = new float[m_t*n_s];
      for (i=0; i<m_t; i++) y_t[i] = y_t_li[i];
      for (i=0; i<n_s; i++) x_soc[i] = x_soc_li[i];
      for (i=0; i<m_t*n_s; i++) t_voc[i] = t_voc_li[i];
      n_n     = n_n_li;   // Number of temperature breakpoints for x_soc_min table
      delete x_soc_min;    x_soc_min = new float[n_n];
      delete t_soc_min;    t_soc_min = new float[n_n];
      for (i=0; i<n_n; i++) x_soc_min[i] = x_soc_min_li[i];
      for (i=0; i<n_n; i++) t_soc_min[i] = t_soc_min_li[i];
      hys_cap = 7.2e6;    // Capacitance of hysteresis, Farads
      n_h     = n_h_li;
      m_h     = m_h_li;
      delete x_dv;    x_dv = new float[n_h];
      delete y_soc;   y_soc = new float[m_h];
      delete t_r;     t_r = new float[m_h*n_h];
      for (i=0; i<n_h; i++) x_dv[i] = x_dv_li[i];
      for (i=0; i<m_h; i++) y_soc[i] = y_soc_li[i];
      for (i=0; i<m_h*n_h; i++) t_r[i] = t_r_li[i];
      v_sat       = 12.85;  // Saturation threshold at temperature, deg C
      dvoc_dt     = 0.008;  // Change of VOC with operating temperature in range 0 - 50 C V/deg C
      dv          = 0.01;   // Adjustment for calibration error, V
      r_0         = 0.006;  // Randles R0, ohms   
      r_ct        = 0.0032; // Randles charge transfer resistance, ohms
      r_diff      = 0.0144; // Randles diffusion resistance, ohms
      tau_ct      = 0.4;    // Randles charge transfer time constant, s (=1/Rct/Cct)
      tau_diff    = 166.;    // Randles diffusion time constant, s (=1/Rdif/Cdif)
      tau_sd      = 3.6e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec
      r_sd        = 140;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
      break;
    default:       // "unknown"
        Serial.printf("Battery::assign_mod:  unknown mod = %s.  Type 'h' for help\n", mod_str.c_str());
        break;
  }
  r_ss = r_0 + r_ct + r_diff;
}

// Battery type model translate to plain English for display
String Chemistry::decode(const uint8_t mod)
{
    String result;
    switch (mod)
    {
        case ( '0' ):
            result = "Battleborn";
            break;
        case ( '1' ):
            result = "LION";
            break;
        default:
            result = "unknown";
            Serial.printf("Chemistry::decode:  unknown mod number = %d.  Type 'h' for help\n", mod);
            break;
    }
    return ( result );
}

// Battery type model coding
uint8_t Chemistry::encode(const String mod_str)
{
    uint8_t result;
    if ( mod_str=="Battleborn" ) result = 0;
    else if ( mod_str=="LION" ) result = 1;
    else
    {
        result = 99;
        Serial.printf("Chemistry::encode:  unknown mod = %s.  Type 'h' for help\n", mod_str.c_str());
    }
    return ( result );
}


// class Coulombs
Coulombs::Coulombs() {}
Coulombs::Coulombs(double *rp_delta_q, float *rp_t_last, const double q_cap_rated, const double t_rated, 
  const double t_rlim, const uint8_t *rp_mod_code)
  : rp_delta_q_(rp_delta_q), rp_t_last_(rp_t_last), q_cap_rated_(q_cap_rated), q_cap_rated_scaled_(q_cap_rated), t_rated_(t_rated), t_rlim_(0.017),
  soc_min_(0), chem_(*rp_mod_code)
  {
    soc_min_T_ = new TableInterp1D(chem_.n_n, chem_.x_soc_min, chem_.t_soc_min);
  }
Coulombs::~Coulombs() {}

// operators
// Pretty print
void Coulombs::pretty_print(void)
{
  Serial.printf("Coulombs:\n");
  Serial.printf("  q_cap_rated_ =%9.1f;  // Rated capacity at t_rated_, saved for future scaling, C\n", q_cap_rated_);
  Serial.printf("  q_cap_rated_scaled_ = %9.1f;  // Applied rated capacity at t_rated_, after scaling, C\n", q_cap_rated_scaled_);
  Serial.printf("  q_capacity_ = %9.1f;  // Saturation charge at temperature, C\n", q_capacity_);
  Serial.printf("  q_ =          %9.1f;  // Present charge available to use, except q_min_, C\n", q_);
  Serial.printf("  q_min_ =      %9.1f;  // Estimated charge at low voltage shutdown, C\n", q_min_);
  Serial.printf("  delta_q       %9.1f;  // Charge change since saturated, C\n", *rp_delta_q_);
  Serial.printf("  soc_ =          %7.3f;  // Fraction of saturation charge (q_capacity_) available (0-1);\n", soc_);
  Serial.printf("  SOC_ =            %5.1f;  // Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries\n", SOC_);
  Serial.printf("  sat_ =                %d;  // Indication calculated by caller that battery is saturated, T=saturated\n", sat_);
  Serial.printf("  t_rated_ =        %5.1f;  // Rated temperature, deg C\n", t_rated_);
  Serial.printf("  t_last =          %5.1f;  // Last battery temperature for rate limit memory, deg C\n", *rp_t_last_);
  Serial.printf("  t_rlim_ =       %7.3f;  // Tbatt rate limit, deg C / s\n", t_rlim_);
  Serial.printf("  resetting_ =          %d;  // Flag to coordinate user testing of coulomb counters, T=performing an external reset of counter\n", resetting_);
  Serial.printf("  soc_min_ =      %7.3f;  // Estimated soc where battery BMS will shutoff current, fraction\n", soc_min_);
  Serial.printf("  mod_ =        %s;  // Model type of battery chemistry e.g. Battleborn or LION\n", chem_.decode(mod_code()).c_str());
  Serial.printf("  mod_code_ =           %d;  // Chemistry code integer\n", mod_code());
}

// functions

// Scale size of battery and adjust as needed to preserve delta_q.  Tbatt unchanged.
// Goal is to scale battery and see no change in delta_q on screen of 
// test comparisons.   The rationale for this is that the battery is frequently saturated which
// resets all the model parameters.   This happens daily.   Then both the model and the battery
// are discharged by the same current so the delta_q will be the same.
void Coulombs::apply_cap_scale(const double scale)
{
  q_cap_rated_scaled_ = scale * q_cap_rated_;
  q_capacity_ = calculate_capacity(*rp_t_last_);
  q_ = *rp_delta_q_ + q_capacity_; // preserve delta_q, deficit since last saturation (like real life)
  soc_ = q_ / q_capacity_;
  SOC_ = q_ / q_cap_rated_scaled_ * 100.;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q, capacity, temp preserved
void Coulombs::apply_delta_q(const double delta_q)
{
  *rp_delta_q_ = delta_q;
  q_ = *rp_delta_q_ + q_capacity_;
  soc_ = q_ / q_capacity_;
  SOC_ = q_ / q_cap_rated_scaled_ * 100.;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void Coulombs::apply_delta_q_t(const double delta_q, const double temp_c)
{
  *rp_delta_q_ = delta_q;
  q_capacity_ = calculate_capacity(temp_c);
  q_ = q_capacity_ + *rp_delta_q_;
  soc_ = q_ / q_capacity_;
  SOC_ = q_ / q_cap_rated_scaled_ * 100.;
  resetting_ = true;
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_soc(const double soc, const double temp_c)
{
  soc_ = soc;
  q_capacity_ = calculate_capacity(temp_c);
  q_ = soc*q_capacity_;
  *rp_delta_q_ = q_ - q_capacity_;
  SOC_ = q_ / q_cap_rated_scaled_ * 100.;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_SOC(const double SOC, const double temp_c)
{
  SOC_ = SOC;
  q_ = SOC / 100. * q_cap_rated_scaled_;
  q_capacity_ = calculate_capacity(temp_c);
  *rp_delta_q_ = q_ - q_capacity_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Capacity
double Coulombs::calculate_capacity(const double temp_c)
{
  return( q_cap_rated_scaled_ * (1-chem_.dqdt*(temp_c - t_rated_)) );
}

/* Coulombs::count_coulombs:  Count coulombs based on true=actual capacity
Inputs:
  dt              Integration step, s
  temp_c          Battery temperature, deg C
  charge_curr     Charge, A
  sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
  tlast           Past value of battery temperature used for rate limit memory, deg C
Outputs:
  q_capacity_     Saturation charge at temperature, C
  *rp_delta_q_    Charge change since saturated, C
  *rp_t_last_     Updated value of battery temperature used for rate limit memory, deg C
  resetting_      Sticky flag for initialization, T=reset
  soc_            Fraction of saturation charge (q_capacity_) available (0-1) 
  SOC_            Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries
  soc_min_        Estimated soc where battery BMS will shutoff current, fraction
  q_min_          Estimated charge at low voltage shutdown, C\
*/
double Coulombs::count_coulombs(const double dt, const boolean reset, const double temp_c, const double charge_curr,
  const boolean sat, const double t_last)
{
    double d_delta_q = charge_curr * dt;
    sat_ = sat;

    // Rate limit temperature
    double temp_lim = max(min( temp_c, t_last + t_rlim_*dt), t_last - t_rlim_*dt);
    if ( reset ) temp_lim = temp_c;
    if ( reset ) *rp_t_last_ = temp_c;

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    if ( sat )
    {
        if ( d_delta_q > 0 )
        {
            d_delta_q = 0.;
            if ( !resetting_ ) *rp_delta_q_ = 0.;
        }
        else if ( reset )
          *rp_delta_q_ = 0.;
    }
    resetting_ = false;     // one pass flag.  Saturation debounce should reset next pass

    // Integration
    q_capacity_ = calculate_capacity(temp_lim);
    if ( !reset ) *rp_delta_q_ = max(min(*rp_delta_q_ + d_delta_q - chem_.dqdt*q_capacity_*(temp_lim-*rp_t_last_), 0.0), -q_capacity_);
    q_ = q_capacity_ + *rp_delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    soc_min_ = soc_min_T_->interp(temp_lim);
    q_min_ = soc_min_ * q_capacity_;
    SOC_ = q_ / q_cap_rated_scaled_ * 100;

    if ( rp.debug==96 )
        Serial.printf("Coulombs::cc,             reset,dt,voc, voc_filt, v_sat, temp_c, temp_lim, sat, charge_curr, d_d_q, d_q, d_q_i, q, q_capacity,soc,SOC,      %d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%7.4f,%7.3f,\n",
                    reset, dt, pp.pubList.voc, pp.pubList.voc_filt,  this->vsat(), temp_c, temp_lim, sat, charge_curr, d_delta_q, *rp_delta_q_, q_, q_capacity_, soc_, SOC_);
    if ( rp.debug==-96 )
        Serial.printf("voc, voc_filt, v_sat, sat, temp_lim, charge_curr, d_d_q, d_q, d_q_i, q, q_capacity,soc, SOC,          \n%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%7.4f,%7.3f,\n",
                    pp.pubList.voc, pp.pubList.voc_filt, this->vsat(), temp_lim, sat, charge_curr, d_delta_q, *rp_delta_q_, q_, q_capacity_, soc_, SOC_);

    // Save and return
    *rp_t_last_ = temp_lim;
    return ( soc_ );
}
