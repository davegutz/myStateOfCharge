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
#include <math.h>
#include "Battery.h"
#include "Coulombs.h"
#include "parameters.h"
extern SavedPars sp;      // Various parameters to be static at system level and saved through power cycle
#include "command.h"
extern CommandPars cp;    // Various parameters to be static at system level
extern PublishPars pp;    // For publishing

// Structure Chemistry
// Assign parameters of model

// Chemistry Executive
void Chemistry::assign_all_mod(const String mod_str)
{
  uint8_t mod = encode(mod_str);
  if ( mod==0 )  // "Battleborn";
  {
    *sp_mod_code = mod;
    sp.put_mon_chm();
    sp.put_sim_chm();
    assign_BB();
  }
  else if ( mod==1 )  // "LION" placeholder.  Data fabricated
  {
    *sp_mod_code = mod;
    sp.put_mon_chm();
    sp.put_sim_chm();
    assign_LI();
  }
  else if ( mod==2 )  // "LION" EKF placeholder.  Data fabricated
  {
    *sp_mod_code = mod;
    sp.put_mon_chm();
    sp.put_sim_chm();
    assign_LIE();
  }
  else Serial.printf("Battery::assign_all_mod:  unknown mod = %d.  Type 'h' for help\n", mod);
  r_ss = r_0 + r_ct + r_diff;
}

// BattleBorn Chemistry
void Chemistry::assign_BB()
{
  // Constants
  dqdt    = 0.01;   // Change of charge with temperature, fraction/deg C (0.01 from literature)
  dvoc_dt = 0.004;  // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  dvoc    = 0.;     // Adjustment for calibration error, V (systematic error; may change in future)
  hys_cap = 3.6e3;  // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022
  low_voc = 9.0;     // Voltage threshold for BMS to turn off battery;
  low_t   = 0;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
  r_0     = 0.003;  // Randles R0, ohms   
  r_ct    = 0.0016; // Randles charge transfer resistance, ohms
  r_diff  = 0.0077; // Randles diffusion resistance, ohms
  r_sd    = 70;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  tau_ct  = 0.2;    // Randles charge transfer time constant, s (=1/Rct/Cct)
  tau_diff = 83.;   // Randles diffusion time constant, s (=1/Rdif/Cdif)
  tau_sd  = 2.5e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
  c_sd    = tau_sd / r_sd;
  v_sat   = 13.85;  // Saturation threshold at temperature, deg C

  // VOC_SOC table
  assign_voc_soc(N_S_BB, M_T_BB, X_SOC_BB, Y_T_BB, T_VOC_BB);

  // Min SOC table
  assign_soc_min(N_N_BB, X_SOC_MIN_BB, T_SOC_MIN_BB);

  // Hys table
  assign_hys(N_H_BB, M_H_BB, X_DV_BB, Y_SOC_BB, T_R_BB, T_DV_MAX_BB, T_DV_MIN_BB);
}

// LION Chemistry
void Chemistry::assign_LI()
{
  // Constants
  dqdt    = 0.01;   // Change of charge with temperature, fraction/deg C (0.01 from literature)
  dvoc_dt = 0.004;  // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  dvoc    = 0.;     // Adjustment for calibration error, V (systematic error; may change in future)
  hys_cap = 3.6e3;  // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022
  low_voc = 9.;     // Voltage threshold for BMS to turn off battery;
  low_t   = 0;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
  r_0     = 0.003;  // Randles R0, ohms
  r_ct    = 0.0016; // Randles charge transfer resistance, ohms
  r_diff  = 0.0077; // Randles diffusion resistance, ohms
  r_sd    = 70;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  tau_ct  = 0.2;    // Randles charge transfer time constant, s (=1/Rct/Cct)
  tau_diff = 83.;   // Randles diffusion time constant, s (=1/Rdif/Cdif)
  tau_sd  = 2.5e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
  c_sd    = tau_sd / r_sd;
  v_sat   = 13.85;  // Saturation threshold at temperature, deg C

  // VOC_SOC table
  assign_voc_soc(N_S_LI, M_T_LI, X_SOC_LI, Y_T_LI, T_VOC_LI);

  // Min SOC table
  assign_soc_min(N_N_LI, X_SOC_MIN_LI, T_SOC_MIN_LI);

  // Hys table
  assign_hys(N_H_LI, M_H_LI, X_DV_LI, Y_SOC_LI, T_R_LI, T_DV_MAX_LI, T_DV_MIN_LI);
}

// LION Chemistry monotonic for EKF
void Chemistry::assign_LIE()
{
  // Constants
  dqdt    = 0.01;   // Change of charge with temperature, fraction/deg C (0.01 from literature)
  dvoc_dt = 0.004;  // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  dvoc    = 0.;     // Adjustment for calibration error, V (systematic error; may change in future)
  hys_cap = 3.6e3;  // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022
  low_voc = 9.;     // Voltage threshold for BMS to turn off battery;
  low_t   = 0;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
  r_0     = 0.003;  // Randles R0, ohms   
  r_ct    = 0.0016; // Randles charge transfer resistance, ohms
  r_diff  = 0.0077; // Randles diffusion resistance, ohms
  r_sd    = 70;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  tau_ct  = 0.2;    // Randles charge transfer time constant, s (=1/Rct/Cct)
  tau_diff = 83.;   // Randles diffusion time constant, s (=1/Rdif/Cdif)
  tau_sd  = 2.5e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
  c_sd    = tau_sd / r_sd;
  v_sat   = 13.85;  // Saturation threshold at temperature, deg C

  // VOC_SOC table
  assign_voc_soc(N_S_LIE, M_T_LIE, X_SOC_LIE, Y_T_LIE, T_VOC_LIE);

  // Min SOC table
  assign_soc_min(N_N_LIE, X_SOC_MIN_LIE, T_SOC_MIN_LIE);

  // Hys table
  assign_hys(N_H_LI, M_H_LI, X_DV_LI, Y_SOC_LI, T_R_LI, T_DV_MAX_LI, T_DV_MIN_LI);
}

// Workhorse assignment function for Hysteresis
void Chemistry::assign_hys(const int _n_h, const int _m_h, const float *x, const float *y, const float *t, const float *tx, const float *tn)
{
  int i;

  // Delete old if exists, before assigning anything
  if ( n_h )  delete x_dv;
  if ( m_h )  delete y_soc;
  if ( m_h && n_h )  delete t_r;
  if ( m_h )  delete t_x;
  if ( m_h )  delete t_n;

  // Instantiate
  n_h = _n_h;
  m_h = _m_h;
  x_dv = new float[n_h];
  y_soc = new float[m_h];
  t_r = new float[m_h*n_h];
  t_x = new float[m_h];
  t_n = new float[m_h];

  // Assign
  for (i=0; i<n_h; i++) x_dv[i] = x[i];
  for (i=0; i<m_h; i++) y_soc[i] = y[i];
  for (i=0; i<m_h*n_h; i++) t_r[i] = t[i];
  for (i=0; i<m_h; i++) t_x[i] = tx[i];
  for (i=0; i<m_h; i++) t_n[i] = tn[i];
 
  // Finalize
  //  ...see Class Hysteresis
}

// Workhorse assignment function for VOC_SOC
void Chemistry::assign_voc_soc(const int _n_s, const int _m_t, const float *x, const float *y, const float *t)
{
  int i;

  // Delete old if exists, before assigning anything
  if ( n_s )  delete x_soc;
  if ( m_t )  delete y_t;
  if ( m_t && n_s ) { delete t_voc; delete voc_T_; }

  // Instantiate
  n_s     = _n_s;   //Number of soc breakpoints voc table
  m_t     = _m_t;   // Number temperature breakpoints for voc table
  x_soc = new float[n_s];
  y_t = new float[m_t];
  t_voc = new float[m_t*n_s];

  // Assign
  for (i=0; i<n_s; i++) x_soc[i] = x[i];
  for (i=0; i<m_t; i++) y_t[i] = y[i];
  for (i=0; i<m_t*n_s; i++) t_voc[i] = t[i];

  // Finalize
  voc_T_ = new TableInterp2D(n_s, m_t, x_soc, y_t, t_voc);
}

// Workhorse assignment function for SOC_MIN
void Chemistry::assign_soc_min(const int _n_n, const float *x, const float *t)
{
  int i;

  // Delete old if exists, before assigning anything
  if ( n_n ) { delete x_soc_min; delete t_soc_min; delete soc_min_T_; }

  // Instantiate
  n_n     = _n_n;   // Number of temperature breakpoints for x_soc_min table
  x_soc_min = new float[n_n];
  t_soc_min = new float[n_n];

  // Assign
  for (i=0; i<n_n; i++) x_soc_min[i] = x[i];
  for (i=0; i<n_n; i++) t_soc_min[i] = t[i];

  // Finalize
  soc_min_T_ = new TableInterp1D(n_n, x_soc_min, t_soc_min);
}

// Battery type model translate to plain English for display
String Chemistry::decode(const uint8_t mod)
{
    String result;
    if ( mod==0 ) result = "Battleborn";
    else if ( mod==1 ) result = "LION";
    else if ( mod==2 ) result = "LIE";
    else
    {
      result = "unknown";
      Serial.printf("C::decode:  unknown mod = %d. 'h'\n", mod);
    }
    return ( result );
}

// Battery type model coding
uint8_t Chemistry::encode(const String mod_str)
{
    uint8_t result;
    if ( mod_str=="Battleborn" ) result = 0;
    else if ( mod_str=="LION" ) result = 1;
    else if ( mod_str=="LIE" ) result = 2;
    else
    {
        result = 99;
        Serial.printf("C::encode:  unknown mod = %s.  'h'\n", mod_str.c_str());
    }
    return ( result );
}

// Pretty print
void Chemistry::pretty_print(void)
{
  Serial.printf("Chemistry:\n");
  Serial.printf("  dqdt%7.3f, frac/dg C\n", dqdt);
  Serial.printf("  low_voc%7.3f, V\n", low_voc);
  Serial.printf("  hys_cap%7.3f, F\n", hys_cap);
  Serial.printf("  v_sat%7.3f, V\n", v_sat);
  Serial.printf("  dvoc_dt%7.3f, V/dg C\n", dvoc_dt);
  Serial.printf("  dvoc%7.3f, V\n", dvoc);
  Serial.printf("  Randles:\n");
  Serial.printf("  r_0%9.6f, ohm\n", r_0);
  Serial.printf("  r_ct%9.6f, ohm\n", r_ct);
  Serial.printf("  r_diff%9.6f, ohm\n", r_diff);
  Serial.printf("  tau_ct%7.3f, s\n", tau_ct);
  Serial.printf("  tau_diff%7.3f, s\n", tau_diff);
  Serial.printf("  tau_sd%9.3g; EKF, s\n", tau_sd);
  Serial.printf("  c_sd%9.3g; EKK, farad\n", c_sd);
  Serial.printf("  r_sd%9.6f; EKF, ohm\n", r_sd);
  Serial.printf("  r_ss%9.6f; SS init, ohm\n", r_ss);
  Serial.printf("  voc(t, soc):\n");
  voc_T_->pretty_print();
  Serial.printf("  soc_min(temp_c):\n");
  soc_min_T_->pretty_print();
}


// class Coulombs
Coulombs::Coulombs() {}
Coulombs::Coulombs(double *sp_delta_q, float *sp_t_last, const double q_cap_rated, const double t_rated, 
  const double t_rlim, uint8_t *sp_mod_code, const double coul_eff)
  : sp_delta_q_(sp_delta_q), sp_t_last_(sp_t_last), q_cap_rated_(q_cap_rated), q_cap_rated_scaled_(q_cap_rated), t_rated_(t_rated), t_rlim_(0.017),
  soc_min_(0), chem_(sp_mod_code), coul_eff_(coul_eff) {}
Coulombs::~Coulombs() {}


// operators
// Pretty print
void Coulombs::pretty_print(void)
{
  Serial.printf("Coulombs:\n");
  Serial.printf(" q_cap_rat%9.1f, C\n", q_cap_rated_);
  Serial.printf(" q_cap_rat_scl%9.1f, C\n", q_cap_rated_scaled_);
  Serial.printf(" q_cap%9.1f, C\n", q_capacity_);
  Serial.printf(" q%9.1f, C\n", q_);
  Serial.printf(" q_min%9.1f, C\n", q_min_);
  Serial.printf(" delta_q%9.1f, C\n", *sp_delta_q_);
  Serial.printf(" soc%8.4f\n", soc_);
  Serial.printf(" sat %d\n", sat_);
  Serial.printf(" t_rat%5.1f dg C\n", t_rated_);
  Serial.printf(" t_last%5.1f dg C\n", *sp_t_last_);
  Serial.printf(" t_rlim%7.3f dg C / s\n", t_rlim_);
  Serial.printf(" resetting %d\n", resetting_);
  Serial.printf(" soc_min%8.4f\n", soc_min_);
  Serial.printf(" mod %s\n", chem_.decode(mod_code()).c_str());
  Serial.printf(" mod_code %d\n", mod_code());
  Serial.printf(" coul_eff%9.5f\n", coul_eff_);
  Serial.printf("Coulombs::");
  chem_.pretty_print();
}

// functions

// Scale size of battery and adjust as needed to preserve delta_q.  Tb unchanged.
// Goal is to scale battery and see no change in delta_q on screen of 
// test comparisons.   The rationale for this is that the battery is frequently saturated which
// resets all the model parameters.   This happens daily.   Then both the model and the battery
// are discharged by the same current so the delta_q will be the same.
void Coulombs::apply_cap_scale(const double scale)
{
  q_cap_rated_scaled_ = scale * q_cap_rated_;
  q_capacity_ = calculate_capacity(*sp_t_last_);
  q_ = *sp_delta_q_ + q_capacity_; // preserve delta_q, deficit since last saturation (like real life)
  soc_ = q_ / q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q, capacity, temp preserved
void Coulombs::apply_delta_q(const double delta_q)
{
  *sp_delta_q_ = delta_q;
  sp.put_delta_q();
  q_ = *sp_delta_q_ + q_capacity_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void Coulombs::apply_delta_q_t(const boolean reset)
{
  if ( !reset ) return;
  q_capacity_ = calculate_capacity(*sp_t_last_);
  q_ = q_capacity_ + *sp_delta_q_;
  soc_ = q_ / q_capacity_;
  resetting_ = true;
}
void Coulombs::apply_delta_q_t(const double delta_q, const float temp_c)
{
  *sp_delta_q_ = delta_q;
  sp.put_delta_q();
  *sp_t_last_ = temp_c;
  sp.put_t_last();
  apply_delta_q_t(true);
}


// Memory set, adjust book-keeping as needed.  delta_q preserved
void Coulombs::apply_soc(const double soc, const float temp_c)
{
  soc_ = soc;
  q_capacity_ = calculate_capacity(temp_c);
  q_ = soc*q_capacity_;
  *sp_delta_q_ = q_ - q_capacity_;
  sp.put_delta_q();
  resetting_ = true;     // momentarily turn off saturation check
}

// Capacity
double Coulombs::calculate_capacity(const float temp_c)
{
  return( q_cap_rated_scaled_ * (1+chem_.dqdt*(temp_c - t_rated_)) );
}

/* Coulombs::count_coulombs:  Count coulombs based on true=actual capacity
Inputs:
  dt              Integration step, s
  temp_c          Battery temperature, deg C
  charge_curr     Charge, A
  sat             Indication that battery is saturated, T=saturated
  tlast           Past value of battery temperature used for rate limit memory, deg C
  coul_eff_       Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
Outputs:
  q_capacity_     Saturation charge at temperature, C
  *sp_delta_q_    Charge change since saturated, C
  *sp_t_last_     Updated value of battery temperature used for rate limit memory, deg C
  resetting_      Sticky flag for initialization, T=reset
  soc_            Fraction of saturation charge (q_capacity_) available (0-1) 
  soc_min_        Estimated soc where battery BMS will shutoff current, fraction
  q_min_          Estimated charge at low voltage shutdown, C\
*/
double Coulombs::count_coulombs(const double dt, const boolean reset_temp, const float temp_c, const double charge_curr,
  const boolean sat, const double delta_q_ekf)
{
    // Rate limit temperature.   When modeling, reset_temp.  In real world, rate limited Tb ramps Coulomb count since bms_off
    if ( reset_temp && sp.mod_vb() )
    {
      *sp_t_last_ = temp_c;
      sp.put_t_last();
    }
    double temp_lim = max(min( temp_c, *sp_t_last_ + t_rlim_*dt), *sp_t_last_ - t_rlim_*dt);

    // State change
    double d_delta_q = charge_curr * dt;
    if ( charge_curr>0. ) d_delta_q *= coul_eff_;
    d_delta_q -= chem_.dqdt*q_capacity_*(temp_lim - *sp_t_last_);
    sat_ = sat;

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    if ( sat )
    {
        if ( d_delta_q > 0 )
        {
            d_delta_q = 0.;
            if ( !resetting_ )
            {
              *sp_delta_q_ = 0.;
              sp.put_delta_q();
            }
        }
        else if ( reset_temp )
        {
          *sp_delta_q_ = 0.;
          sp.put_delta_q();
        }
    }
    // else if ( reset_temp && !cp.fake_faults ) *sp_delta_q_ = delta_q_ekf;  // Solution to booting up unsaturated
    resetting_ = false;     // one pass flag

    // Integration.   Can go to negative
    q_capacity_ = calculate_capacity(temp_lim);
    if ( !reset_temp )
    {
      *sp_delta_q_ = max(min(*sp_delta_q_ + d_delta_q, 0.0), -q_capacity_*1.5);
      sp.put_delta_q();
    }
    q_ = q_capacity_ + *sp_delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    soc_min_ = chem_.soc_min_T_->interp(temp_lim);
    q_min_ = soc_min_ * q_capacity_;

    // if ( sp.debug==96 )
    //     Serial.printf("cc:,reset_temp,dt,voc, Voc_filt, V_sat, temp_c, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_cap, soc, %d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%7.4f,\n",
    //                 reset, dt, pp.pubList.Voc, pp.pubList.Voc_filt,  this->Vsat(), temp_c, temp_lim, sat, charge_curr, d_delta_q, *sp_delta_q_, q_, q_capacity_, soc_);

    // Save and return
    *sp_t_last_ = temp_lim;
    sp.put_t_last();
    return ( soc_ );
}
