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
    assign_BB();
  }
  else if ( mod==1 )  // "CHINS"
  {
    *sp_mod_code = mod;
    assign_CH();
  }
  else if ( mod==2 )  // "Spare"  Data fabricated
  {
    *sp_mod_code = mod;
    assign_SP();
  }
  else Serial.printf("Battery::assign_all_mod:  unknown mod = %d.  Type 'h' for help\n", mod);
  r_ss = r_0 + r_ct;
}

// BattleBorn Chemistry
void Chemistry::assign_BB()
{
  // Constants
  rated_temp = 25.;   // Temperature at RATED_BATT_CAP, deg C (25)
  coul_eff = 0.9985;  // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9985)
  dqdt    = 0.01;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
  dvoc_dt = 0.004;    // Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
  dvoc    = 0.;       // Adjustment for calibration error, V (systematic error; may change in future, 0.)
  hys_cap = 3.6e3;    // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022 (3.6e3)
                      // tau_null = 1 / 0.005 / 3.6e3 = 0.056 s
  low_voc = 9.0;      // Voltage threshold for BMS to turn off battery, V (9.)
  low_t   = 0;        // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C (0)
  r_0     = 0.0046;   // ChargeTransfer R0, ohms (0.0046)
  r_ct  = 0.0077;     // ChargeTransfer diffusion resistance, ohms (0.0077)
  r_sd    = 70;       // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70.)
  tau_ct = 83.;       // ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (83.)
  tau_sd  = 2.5e7;    // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
  c_sd    = tau_sd / r_sd;
  vb_off  = 10.;        // Shutoff point in Mon, V (10.)
  vb_down = 9.8;        // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
  vb_down_sim = 9.5;    // Shutoff point in Sim, V (9.5)
  vb_rising = 10.3;     // Shutoff point when off, V (10.3)
  vb_rising_sim = 9.75; // Shutoff point in Sim when off, V (9.75)
  v_sat   = 13.85;      // Saturation threshold at temperature, deg C (13.85)

  // VOC_SOC table
  assign_voc_soc(N_S_BB, M_T_BB, X_SOC_BB, Y_T_BB, T_VOC_BB);

  // Min SOC table
  assign_soc_min(N_N_BB, X_SOC_MIN_BB, T_SOC_MIN_BB);

  // Hys table
  assign_hys(N_H_BB, M_H_BB, X_DV_BB, Y_SOC_BB, T_R_BB, T_S_BB, T_DV_MAX_BB, T_DV_MIN_BB);
}

// CHINS Chemistry
void Chemistry::assign_CH()
{
  // Constants
  rated_temp = 25.;   // Temperature at RATED_BATT_CAP, deg C (25)
  coul_eff = 0.9976;  // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9976 for sres=1.6)
  dqdt    = 0.01;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
  dvoc_dt = 0.004;    // Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
  dvoc    = 0.;       // Adjustment for calibration error, V (systematic error; may change in future, 0.)
  hys_cap = 1.e4;     // Capacitance of hysteresis, Farads.  tau_null = 1 / 0.001 / 1.8e4 = 0.056 s (1e4)
  low_voc = 9.;       // Voltage threshold for BMS to turn off battery (9.)
  low_t   = 0;        // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C (0.)
  r_0     = 0.0046*3.;  // ChargeTransfer R0, ohms (0.0046*3)
  r_ct    = 0.0077*0.76;// ChargeTransfer diffusion resistance, ohms (0.0077*0.76)
  r_sd    = 70;       // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70)
  tau_ct  = 24.9;     // ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (24.9)
  tau_sd  = 2.5e7;    // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
  c_sd    = tau_sd / r_sd;
  // These following are the same as BattleBorn.   I haven't observed what they really are
  vb_off  = 10.;        // Shutoff point in Mon, V (10.)
  vb_down = 9.8;        // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
  vb_down_sim = 9.5;    // Shutoff point in Sim, V (9.5)
  vb_rising = 10.3;     // Shutoff point when off, V (10.3)
  vb_rising_sim = 9.75; // Shutoff point in Sim when off, V (9.75)
  v_sat   = 13.85;      // Saturation threshold at temperature, deg C (13.85)

  // VOC_SOC table
  assign_voc_soc(N_S_CH, M_T_CH, X_SOC_CH, Y_T_CH, T_VOC_CH);

  // Min SOC table
  assign_soc_min(N_N_CH, X_SOC_MIN_CH, T_SOC_MIN_CH);

  // Hys table
  assign_hys(N_H_CH, M_H_CH, X_DV_CH, Y_SOC_CH, T_R_CH, T_S_CH, T_DV_MAX_CH, T_DV_MIN_CH);
}

// Spare
void Chemistry::assign_SP()
{
  // Constants
  dqdt    = 0.01;   // Change of charge with temperature, fraction/deg C (0.01 from literature)
  dvoc_dt = 0.004;  // Change of VOC with operating temperature in range 0 - 50 C V/deg C
  dvoc    = 0.;     // Adjustment for calibration error, V (systematic error; may change in future)
  hys_cap = 3.6e3;  // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022
  low_voc = 9.;     // Voltage threshold for BMS to turn off battery;
  low_t   = 0;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
  r_0     = 0.003;  // ChargeTransfer R0, ohms   
  r_ct  = 0.0077; // ChargeTransfer diffusion resistance, ohms
  r_sd    = 70;     // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
  tau_ct = 83.;   // ChargeTransfer diffusion time constant, s (=1/Rct/Cct)
  tau_sd  = 2.5e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
  c_sd    = tau_sd / r_sd;
  v_sat   = 13.85;  // Saturation threshold at temperature, deg C

  // VOC_SOC table
  assign_voc_soc(N_S_SP, M_T_SP, X_SOC_SP, Y_T_SP, T_VOC_SP);

  // Min SOC table
  assign_soc_min(N_N_SP, X_SOC_MIN_SP, T_SOC_MIN_SP);

  // Hys table
  assign_hys(N_H_CH, M_H_CH, X_DV_CH, Y_SOC_CH, T_R_CH, T_S_CH, T_DV_MAX_CH, T_DV_MIN_CH);
}

// Workhorse assignment function for Hysteresis
void Chemistry::assign_hys(const int _n_h, const int _m_h, const float *x, const float *y, const float *t, const float *s, const float *tx, const float *tn)
{
  int i;

  // Delete old if exists, before assigning anything
  if ( n_h )  delete x_dv;
  if ( m_h )  delete y_soc;
  if ( m_h && n_h )  delete t_r;
  if ( m_h && n_h )  delete t_s;
  if ( m_h )  delete t_x;
  if ( m_h )  delete t_n;

  // Instantiate
  n_h = _n_h;
  m_h = _m_h;
  x_dv = new float[n_h];
  y_soc = new float[m_h];
  t_r = new float[m_h*n_h];
  t_s = new float[m_h*n_h];
  t_x = new float[m_h];
  t_n = new float[m_h];

  // Assign
  for (i=0; i<n_h; i++) x_dv[i] = x[i];
  for (i=0; i<m_h; i++) y_soc[i] = y[i];
  for (i=0; i<m_h*n_h; i++) t_r[i] = t[i];
  for (i=0; i<m_h*n_h; i++) t_s[i] = s[i];
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
    else if ( mod==1 ) result = "CHINS";
    else if ( mod==2 ) result = "Spare";
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
    else if ( mod_str=="CHINS" ) result = 1;
    else if ( mod_str=="Spare" ) result = 2;
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
  Serial.printf("  dvoc%7.3f, V\n", dvoc);
  Serial.printf("  dvoc_dt%7.3f, V/dg C\n", dvoc_dt);
  Serial.printf("  hys_cap%7.3f, F\n", hys_cap);
  Serial.printf("  low_voc%7.3f, V\n", low_voc);
  Serial.printf("  v_sat%7.3f, V\n", v_sat);
  Serial.printf("  ChargeTransfer:\n");
  Serial.printf("  c_sd%9.3g; EKF, farad\n", c_sd);
  Serial.printf("  r_0%9.6f, ohm\n", r_0);
  Serial.printf("  r_ct%9.6f, ohm\n", r_ct);
  Serial.printf("  r_sd%9.3f; EKF, ohm\n", r_sd);
  Serial.printf("  r_ss%9.6f; SS init, ohm\n", r_ss);
  Serial.printf("  tau_ct%7.3f, s\n", tau_ct);
  Serial.printf("  tau_sd%9.3g; EKF, s\n", tau_sd);
  Serial.printf("  voc(t, soc):\n");
  voc_T_->pretty_print();
  Serial.printf("  soc_min(temp_c):\n");
  soc_min_T_->pretty_print();
}
