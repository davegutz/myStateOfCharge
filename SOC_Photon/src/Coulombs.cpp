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
  if ( mod==0 )  // "Battleborn";
  {
    *rp_mod_code = mod;
    dqdt    = 0.01;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
    low_voc = 10.;      // Voltage threshold for BMS to turn off battery;
    low_t   = 0;        // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
    if ( n_s )  delete x_soc;
    if ( m_t )  delete y_t;
    if ( m_t && n_s )  delete t_voc;
    n_s     = n_s_bb;   //Number of soc breakpoints voc table
    m_t     = m_t_bb;   // Number temperature breakpoints for voc table
    x_soc = new float[n_s];
    y_t = new float[m_t];
    t_voc = new float[m_t*n_s];
    for (i=0; i<n_s; i++) x_soc[i] = x_soc_bb[i];
    for (i=0; i<m_t; i++) y_t[i] = y_t_bb[i];
    for (i=0; i<m_t*n_s; i++) t_voc[i] = t_voc_bb[i];
    if ( n_n ) { delete x_soc_min; delete t_soc_min;}
    n_n     = n_n_bb;   // Number of temperature breakpoints for x_soc_min table
    x_soc_min = new float[n_n];
    t_soc_min = new float[n_n];
    for (i=0; i<n_n; i++) x_soc_min[i] = x_soc_min_bb[i];
    for (i=0; i<n_n; i++) t_soc_min[i] = t_soc_min_bb[i];
    hys_cap = 3.6e5;    // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data
    if ( n_h )  delete x_dv;
    if ( m_h )  delete y_soc;
    if ( m_h && n_h )  delete t_r;
    n_h     = n_h_bb;   // Number of dv breakpoints in r(dv) table t_r
    m_h     = m_h_bb;   // Number of soc breakpoints in r(soc, dv) table t_r
    x_dv = new float[n_h];
    y_soc = new float[m_h];
    t_r = new float[m_h*n_h];
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
    tau_sd      = 2.5e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
    r_sd        = 70;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  }
  else if ( mod==1 )  // "LION" placeholder.  Data fabricated
  {
    *rp_mod_code = mod;
    dqdt    = 0.02;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
    low_voc = 9.;       // Voltage threshold for BMS to turn off battery;
    low_t   = 0;        // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
    if ( n_s )  delete x_soc;
    if ( m_t )  delete y_t;
    if ( m_t && n_s )  delete t_voc;
    n_s     = n_s_li;   //Number of soc breakpoints voc table
    m_t     = m_t_li;   // Number temperature breakpoints for voc table
    x_soc = new float[n_s];
    y_t = new float[m_t];
    t_voc = new float[m_t*n_s];
    for (i=0; i<n_s; i++) x_soc[i] = x_soc_li[i];
    for (i=0; i<m_t; i++) y_t[i] = y_t_li[i];
    for (i=0; i<m_t*n_s; i++) t_voc[i] = t_voc_li[i];
    if ( n_n ) { delete x_soc_min; delete t_soc_min;}
    n_n     = n_n_li;   // Number of temperature breakpoints for x_soc_min table
    x_soc_min = new float[n_n];
    t_soc_min = new float[n_n];
    for (i=0; i<n_n; i++) x_soc_min[i] = x_soc_min_li[i];
    for (i=0; i<n_n; i++) t_soc_min[i] = t_soc_min_li[i];
    hys_cap = 3.6e5;    // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data
    if ( n_h )  delete x_dv;
    if ( m_h )  delete y_soc;
    if ( m_h && n_h )  delete t_r;
    n_h     = n_h_li;   // Number of dv breakpoints in r(dv) table t_r
    m_h     = m_h_li;   // Number of soc breakpoints in r(soc, dv) table t_r
    x_dv = new float[n_h];
    y_soc = new float[m_h];
    t_r = new float[m_h*n_h];
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
    tau_diff    = 166.;   // Randles diffusion time constant, s (=1/Rdif/Cdif)
    tau_sd      = 3.6e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec
    r_sd        = 140;    // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  }
  else Serial.printf("Battery::assign_mod:  unknown mod = %d.  Type 'h' for help\n", mod);
  r_ss = r_0 + r_ct + r_diff;
}

// Battery type model translate to plain English for display
String Chemistry::decode(const uint8_t mod)
{
    String result;
    if ( mod==0 ) result = "Battleborn";
    else if ( mod==1 ) result = "LION";
    else
    {
      result = "unknown";
      Serial.printf("Chemistry::decode:  unknown mod number = %d.  Type 'h' for help\n", mod);
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

// Pretty print
void Chemistry::pretty_print(void)
{
  Serial.printf("Chemistry:\n");
  Serial.printf("  dqdt =   %7.3f;  // frac/deg C\n", dqdt);
  Serial.printf("  low_voc =%7.3f;  // BMS turn off, V\n", low_voc);
  Serial.printf("  hys_cap =%7.3f;  // F\n", hys_cap);
  Serial.printf("  v_sat =  %7.3f;  // Sat at temp, V\n", v_sat);
  Serial.printf("  dvoc_dt =%7.3f;  // V/deg C\n", dvoc_dt);
  Serial.printf("  dv =     %7.3f;  // Cal error, V\n", dv);
  Serial.printf("  r_0 =  %9.6f;  // Randles R0, ohms\n", r_0);
  Serial.printf("  r_ct = %9.6f;  // Charge transfer, ohms\n", r_ct);
  Serial.printf("  r_diff =%9.6f;  // Diffusion, ohms\n", r_diff);
  Serial.printf("  tau_ct = %7.3f;  // Charge transfer, s (=1/Rct/Cct)\n", tau_ct);
  Serial.printf("  tau_diff=%7.3f;  // Diffusion, s (=1/Rdif/Cdif)\n", tau_diff);
  Serial.printf("  tau_sd = %7.3f;  // EKF. Parasitic disch, sec\n", tau_sd);
  Serial.printf("  r_sd =   %7.3f;  // EKF. Parasitic disch, ohms\n", r_sd);
  Serial.printf("  r_ss =   %7.3f;  // SS init, ohms\n", r_ss);
  Serial.printf("  voc(t, soc):\n");
  Serial.printf("    t=  {"); for ( int i=0; i<m_t; i++ ) Serial.printf("%7.3f, ", y_t[i]); Serial.printf("};\n");
  Serial.printf("    soc={"); for ( int i=0; i<n_s; i++ ) Serial.printf("%7.3f, ", x_soc[i]); Serial.printf("};\n");
  Serial.printf("    voc={\n");
  for ( int i=0; i<m_t; i++ )
  {
    Serial.printf("      {");
    for ( int j=0; j<n_s; j++ ) Serial.printf("%7.3f, ", t_voc[i*n_s+j]);
    Serial.printf("},\n");
  }
  Serial.printf("         };\n");
  Serial.printf("  soc_min(temp_c):\n");
  Serial.printf("    temp_c =  {"); for ( int i=0; i<n_n; i++ ) Serial.printf("%7.3f, ", x_soc_min[i]); Serial.printf("};\n");
  Serial.printf("    soc_min=  {"); for ( int i=0; i<n_n; i++ ) Serial.printf("%7.3f, ", t_soc_min[i]); Serial.printf("};\n");
  Serial.printf("  r(soc, dv):\n");
  Serial.printf("    soc={"); for ( int i=0; i<m_h; i++ ) Serial.printf("%8.6f, ", y_soc[i]); Serial.printf("};\n");
  Serial.printf("    dv ={"); for ( int i=0; i<n_h; i++ ) Serial.printf("%7.3f, ", x_dv[i]); Serial.printf("};\n");
  Serial.printf("    r  ={\n");
  for ( int i=0; i<m_h; i++ )
  {
    Serial.printf("      {");
    for ( int j=0; j<n_h; j++ ) Serial.printf("%9.6f, ", t_r[i*n_h+j]);
    Serial.printf("},\n");
  }
  Serial.printf("         };\n");
}


// class Coulombs
Coulombs::Coulombs() {}
Coulombs::Coulombs(double *rp_delta_q, float *rp_t_last, const double q_cap_rated, const double t_rated, 
  const double t_rlim, uint8_t *rp_mod_code, const double coul_eff)
  : rp_delta_q_(rp_delta_q), rp_t_last_(rp_t_last), q_cap_rated_(q_cap_rated), q_cap_rated_scaled_(q_cap_rated), t_rated_(t_rated), t_rlim_(0.017),
  soc_min_(0), chem_(rp_mod_code), coul_eff_(coul_eff)
  {
    soc_min_T_ = new TableInterp1D(chem_.n_n, chem_.x_soc_min, chem_.t_soc_min);
  }
Coulombs::~Coulombs() {}


// operators
// Pretty print
void Coulombs::pretty_print(void)
{
  Serial.printf("Coulombs:\n");
  Serial.printf("  q_cap_rated_=%9.1f;  // Rated capacity at t_rated, C\n", q_cap_rated_);
  Serial.printf("  q_cap_rated_scaled_= %9.1f;  // Applied rated capacity at t_rated_, C\n", q_cap_rated_scaled_);
  Serial.printf("  q_capacity_ =%9.1f;  // Saturation charge at temperature, C\n", q_capacity_);
  Serial.printf("  q_ =         %9.1f;  // Present charge available to use, except q_min_, C\n", q_);
  Serial.printf("  q_min_ =     %9.1f;  // Est charge at low voltage shutdown, C\n", q_min_);
  Serial.printf("  delta_q      %9.1f;  // Charge change since saturated, C\n", *rp_delta_q_);
  Serial.printf("  soc_ =        %8.4f;  // Fraction of q_capacity_available (0-1);\n", soc_);
  Serial.printf("  sat_ =               %d;  // Indication that battery is saturated, T=saturated\n", sat_);
  Serial.printf("  t_rated_ =       %5.1f;  // Rated temperature, deg C\n", t_rated_);
  Serial.printf("  t_last =         %5.1f;  // Last battery temperature for rate limit memory, deg C\n", *rp_t_last_);
  Serial.printf("  t_rlim_ =      %7.3f;  // Tbatt rate limit, deg C / s\n", t_rlim_);
  Serial.printf("  resetting_ =         %d;  // Flag to coord user testing of CC, T=perf external reset of counter\n", resetting_);
  Serial.printf("  soc_min_ =    %8.4f;  // Est soc where battery BMS shuts off current\n", soc_min_);
  Serial.printf("  mod_ =       %s;  // Model type of battery chemistry e.g. Battleborn or LION\n", chem_.decode(mod_code()).c_str());
  Serial.printf("  mod_code_ =          %d;  // Chemistry code integer\n", mod_code());
  Serial.printf("  coul_eff_ =  %9.5f;  // Coulombic Efficiency\n", coul_eff_);
  Serial.printf("Coulombs::");
  chem_.pretty_print();
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
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q, capacity, temp preserved
void Coulombs::apply_delta_q(const double delta_q)
{
  *rp_delta_q_ = delta_q;
  q_ = *rp_delta_q_ + q_capacity_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void Coulombs::apply_delta_q_t(const boolean reset)
{
  if ( !reset ) return;
  q_capacity_ = calculate_capacity(*rp_t_last_);
  q_ = q_capacity_ + *rp_delta_q_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;
}
void Coulombs::apply_delta_q_t(const double delta_q, const float temp_c)
{
  *rp_delta_q_ = delta_q;
  *rp_t_last_ = temp_c;
  apply_delta_q_t(true);
}


// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_soc(const double soc, const float temp_c)
{
  soc_ = soc;
  q_capacity_ = calculate_capacity(temp_c);
  q_ = soc*q_capacity_;
  *rp_delta_q_ = q_ - q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Capacity
double Coulombs::calculate_capacity(const float temp_c)
{
  return( q_cap_rated_scaled_ * (1-chem_.dqdt*(temp_c - t_rated_)) );
}

/* Coulombs::count_coulombs:  Count coulombs based on true=actual capacity
Inputs:
  dt              Integration step, s
  temp_c          Battery temperature, deg C
  charge_curr     Charge, A
  sat             Indication that battery is saturated, T=saturated
  tlast           Past value of battery temperature used for rate limit memory, deg C
  coul_eff_       Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
  sclr_coul_eff   Scalar on coul_eff determined by tweak test
Outputs:
  q_capacity_     Saturation charge at temperature, C
  *rp_delta_q_    Charge change since saturated, C
  *rp_t_last_     Updated value of battery temperature used for rate limit memory, deg C
  resetting_      Sticky flag for initialization, T=reset
  soc_            Fraction of saturation charge (q_capacity_) available (0-1) 
  soc_min_        Estimated soc where battery BMS will shutoff current, fraction
  q_min_          Estimated charge at low voltage shutdown, C\
*/
double Coulombs::count_coulombs(const double dt, const boolean reset, const float temp_c, const double charge_curr,
  const boolean sat, const double sclr_coul_eff, const double delta_q_ekf)
{
    // Rate limit temperature
    if ( reset ) *rp_t_last_ = temp_c;
    double temp_lim = max(min( temp_c, *rp_t_last_ + t_rlim_*dt), *rp_t_last_ - t_rlim_*dt);

    // State change
    double d_delta_q = charge_curr * dt;
    if ( charge_curr>0. ) d_delta_q *= coul_eff_ * sclr_coul_eff;
    d_delta_q -= chem_.dqdt*q_capacity_*(temp_lim - *rp_t_last_);
    sat_ = sat;

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
    else if ( reset ) *rp_delta_q_ = delta_q_ekf;  // Solution to booting up unsaturated
    resetting_ = false;     // one pass flag

    // Integration
    q_capacity_ = calculate_capacity(temp_lim);
    if ( !reset ) *rp_delta_q_ = max(min(*rp_delta_q_ + d_delta_q, 0.0), -q_capacity_);
    q_ = q_capacity_ + *rp_delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    soc_min_ = soc_min_T_->interp(temp_lim);
    q_min_ = soc_min_ * q_capacity_;

    if ( rp.debug==96 )
        Serial.printf("Coulombs::cc,             reset,dt,voc, Voc_filt, V_sat, temp_c, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc,      %d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%7.4f,\n",
                    reset, dt, pp.pubList.Voc, pp.pubList.Voc_filt,  this->Vsat(), temp_c, temp_lim, sat, charge_curr, d_delta_q, *rp_delta_q_, q_, q_capacity_, soc_);
    if ( rp.debug==-96 )
        Serial.printf("voc, Voc_filt, v_sat, sat, temp_lim, charge_curr, d_d_q, d_q, q, q_capacity,soc,          \n%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%7.4f,\n",
                    pp.pubList.Voc, pp.pubList.Voc_filt, this->Vsat(), temp_lim, sat, charge_curr, d_delta_q, *rp_delta_q_, q_, q_capacity_, soc_);

    // Save and return
    *rp_t_last_ = temp_lim;
    return ( soc_ );
}
