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
extern SavedPars sp; // Various parameters to be static at system level and saved through power cycle
#include "command.h"
extern CommandPars cp; // Various parameters to be static at system level
extern PublishPars pp; // For publishing

// Structure Chemistry
// Assign parameters of model

// Chemistry Executive
void Chemistry::assign_all_chm(const String mod_str)
{
    uint8_t mod = encode(mod_str);
    if (mod == 0) // "Battleborn";
    {
        *sp_mod_code = mod;
        assign_BB();
    }
    else if (mod == 1) // "CHINS"
    {
        *sp_mod_code = mod;
        assign_CH();
    }
    else if (mod == 2) // "Spare"  Data fabricated
    {
        *sp_mod_code = mod;
        assign_SP();
    }
    else
        Serial.printf("Battery::assign_all_mod:  unknown mod = %d.  Type 'h' for help\n", mod);
    r_ss = r_0 + r_ct;
}

// BattleBorn Chemistry
// BattleBorn 100 Ah, 12v LiFePO4
// See VOC_SOC data.xls.    T=40 values are only a notion.   Need data for it.
// >13.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
// 20230401:  Hysteresis tuned to soc=0.7 step data
const uint8_t M_T_BB = 5;    // Number temperature breakpoints for voc table
const uint8_t N_S_BB = 18;   // Number soc breakpoints for voc table
const float Y_T_BB[M_T_BB] = // Temperature breakpoints for voc table
    {5., 11.1, 20., 30., 40.};
const float X_SOC_BB[N_S_BB] = // soc breakpoints for voc table
    {-0.15, 0.00, 0.05, 0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00};
const float T_VOC_BB[M_T_BB * N_S_BB] = // r(soc, dv) table
    {4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
     4.00, 4.00, 4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
     4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
     4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
     4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50};
const uint8_t N_N_BB = 5;                                          // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_BB[N_N_BB] = {5., 11.1, 20., 30., 40.};      // Temperature breakpoints for soc_min table
const float T_SOC_MIN_BB[N_N_BB] = {0.10, 0.07, 0.05, 0.00, 0.20}; // soc_min(t).  At 40C BMS shuts off at 12V

// Battleborn Hysteresis
const uint8_t M_H_BB = 3;     // Number of soc breakpoints in r(soc, dv) table t_r, t_s
const uint8_t N_H_BB = 7;     // Number of dv breakpoints in r(dv) table t_r, t_s
const float X_DV_BB[N_H_BB] = // dv breakpoints for r(soc, dv) table t_r. // DAG 6/13/2022 tune x10 to match data
    {-0.7, -0.5, -0.3, 0.0, 0.15, 0.3, 0.7};
const float Y_SOC_BB[M_H_BB] = // soc breakpoints for r(soc, dv) table t_r, t_s
    {0.0, 0.5, 0.7};
const float T_R_BB[M_H_BB * N_H_BB] = // r(soc, dv) table.    // DAG 9/29/2022 tune to match hist data
    {0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
     0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
     0.016, 0.016, 0.013, 0.005, 0.007, 0.010, 0.010};
const float T_S_BB[M_H_BB * N_H_BB] = // r(soc, dv) table. Not used yet for BB
    {1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1};
const float T_DV_MAX_BB[M_H_BB] = // dv_max(soc) table.  Pulled values from insp of T_R_BB where flattens
    {0.7,  0.3,  0.2};
const float T_DV_MIN_BB[M_H_BB] = // dv_max(soc) table.  Pulled values from insp of T_R_BB where flattens
    {-0.7, -0.5, -0.3};
void Chemistry::assign_BB()
{
    // Constants
    rated_temp = 25.;  // Temperature at NOM_UNIT_CAP, deg C (25)
    coul_eff = 0.9985; // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9985)
    dqdt = 0.01;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dv_min_abs = 0.3;  // Absolute value of +/- hysteresis limit, V (0.3)
    dvoc_dt = 0.004;   // Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
    dvoc = 0.;         // Adjustment for calibration error, V (systematic error; may change in future, 0.)
    hys_cap = 3.6e3;   // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022 (3.6e3)
                       // tau_null = 1 / 0.005 / 3.6e3 = 0.056 s
    low_voc = 9.0;  // Voltage threshold for BMS to turn off battery, V (9.)
    low_t = 0;      // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C (0)
    r_0 = 0.0113;   // ChargeTransfer R0, ohms (0.0113)
    r_ct = 0.001;  // ChargeTransfer diffusion resistance, ohms (0.001)
    r_sd = 70;      // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70.)
    tau_ct = 83.;   // ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (83.)
    tau_sd = 2.5e7; // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
    c_sd = tau_sd / r_sd;
    vb_off = 10.;         // Shutoff point in Mon, V (10.)
    vb_down = 9.8;        // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
    vb_down_sim = 9.5;    // Shutoff point in Sim, V (9.5)
    vb_rising = 10.3;     // Shutoff point when off, V (10.3)
    vb_rising_sim = 9.75; // Shutoff point in Sim when off, V (9.75)
    v_sat = 13.85;        // Saturation threshold at temperature, deg C (13.85)

    // VOC_SOC table
    assign_voc_soc(N_S_BB, M_T_BB, X_SOC_BB, Y_T_BB, T_VOC_BB);

    // Min SOC table
    assign_soc_min(N_N_BB, X_SOC_MIN_BB, T_SOC_MIN_BB);

    // Hys table
    assign_hys(N_H_BB, M_H_BB, X_DV_BB, Y_SOC_BB, T_R_BB, T_S_BB, T_DV_MAX_BB, T_DV_MIN_BB);
}

// CHINS Chemistry
// CHINS 100 Ah, 12v LiFePO4
// 2023-02-27:  tune to data.  Add slight slope 0.8-0.98 to make models deterministic
const uint8_t M_T_CH = 4;    // Number temperature breakpoints for voc table
const uint8_t N_S_CH = 20;   // Number soc breakpoints for voc table
const float Y_T_CH[M_T_CH] = // Temperature breakpoints for voc table
    {5., 11.1, 11.2, 40.};
const float X_SOC_CH[N_S_CH] = // soc breakpoints for voc table
    {-0.100,  -0.060,  -0.035,   0.000,   0.050,   0.100,   0.140,   0.170,   0.200,   0.250,   0.300,   0.400,   0.500,   0.600,   0.700,   0.800,   0.900,   0.980,   0.990,   1.000};
const float T_VOC_CH[M_T_CH * N_S_CH] = // r(soc, dv) table
    {4.000,   4.000,   4.000,   4.000,   4.000,   4.000,   4.000,   5.370,  10.042,  12.683,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700,
     4.000,   4.000,   4.000,   4.000,   4.000,   4.000,   6.963,  10.292,  12.971,  13.025,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700,
     4.000,   4.000,   4.000,   9.000,  12.453,  12.746,  12.869,  12.931,  12.971,  13.025,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700,
     4.000,   4.000,   4.000,   9.000,  12.453,  12.746,  12.869,  12.931,  12.971,  13.025,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700};
const uint8_t N_N_CH = 4;                                        // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_CH[N_N_CH] = {5.000,  11.100,  11.200,  40.000};  // Temperature breakpoints for soc_min table
const float T_SOC_MIN_CH[N_N_CH] = {0.200,   0.167,   0.014,   0.014};  // soc_min(t)

// CHINS Hysteresis
const uint8_t M_H_CH = 4;     // Number of soc breakpoints in r(soc, dv) table t_r, t_s
const uint8_t N_H_CH = 10;    // Number of dv breakpoints in r(dv) table t_r, t_s
const float X_DV_CH[N_H_CH] = // dv breakpoints for r(soc, dv) table t_r, t_s
    {-.10, -.05, -.04, 0.0, .02, .04, .05, .06, .07, .10};
const float Y_SOC_CH[M_H_CH] = // soc breakpoints for r(soc, dv) table t_r, t_s
    {.47, .75, .80, .86};
const float T_R_CH[M_H_CH * N_H_CH] = // r(soc, dv) table
    {0.003, 0.003, 0.4, 0.4, 0.4, 0.4, 0.010, 0.010, 0.010, 0.010,
     0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012,
     0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012,
     0.004, 0.004, 0.4, 0.4, .2, .09, 0.04, 0.006, 0.006, 0.006};
const float T_S_CH[M_H_CH * N_H_CH] = // s(soc, dv) table
    {1., 1., .2, .2, .2, .2, 1., 1., 1., 1.,
     1., 1., .2, .2, .2, 1., 1., 1., 1., 1.,
     1., 1., .2, .2, .2, 1., 1., 1., 1., 1.,
     1., 1., .1, .1, .2, 1., 1., 1., 1., 1.};
const float T_DV_MAX_CH[M_H_CH] = // dv_max(soc) table.  Pulled values from insp of T_R_CH where flattens
    {0.06, 0.1, 0.1, 0.06};
const float T_DV_MIN_CH[M_H_CH] = // dv_max(soc) table.  Pulled values from insp of T_R_CH where flattens
    {-0.06, -0.06, -0.06, -0.06};
void Chemistry::assign_CH()
{
    // Constants
    rated_temp = 25.;  // Temperature at NOM_UNIT_CAP, deg C (25)
    coul_eff = 0.9976; // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9976 for sres=1.6)
    dqdt = 0.01;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dv_min_abs = 0.06; // Absolute value of +/- hysteresis limit, V (0.06)
    dvoc_dt = 0.004;   // Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
    dvoc = 0.;         // Adjustment for calibration error, V (systematic error; may change in future, 0.)
    hys_cap = 1.e4;    // Capacitance of hysteresis, Farads.  tau_null = 1 / 0.001 / 1.8e4 = 0.056 s (1e4)
    Serial.printf("CH dv_min_abs=%7.3f, cap=%7.1f\n", dv_min_abs, hys_cap);
    low_voc = 9.;         // Voltage threshold for BMS to turn off battery (9.)
    low_t = 0;            // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C (0.)
    r_0 = 0.0046 * 3.;    // ChargeTransfer R0, ohms (0.0046*3)
    r_ct = 0.0077 * 0.76; // ChargeTransfer diffusion resistance, ohms (0.0077*0.76)
    r_sd = 70;            // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70)
    tau_ct = 24.9;        // ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (24.9)
    tau_sd = 2.5e7;       // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
    c_sd = tau_sd / r_sd;
    // These following are the same as BattleBorn.   I haven't observed what they really are
    vb_off = 10.;         // Shutoff point in Mon, V (10.)
    vb_down = 9.6;        // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.6)
    vb_down_sim = 9.5;    // Shutoff point in Sim, V (9.5)
    vb_rising = 10.3;     // Shutoff point when off, V (10.3)
    vb_rising_sim = 9.75; // Shutoff point in Sim when off, V (9.75)
    v_sat = 13.85;        // Saturation threshold at temperature, deg C (13.85)

    // VOC_SOC table
    assign_voc_soc(N_S_CH, M_T_CH, X_SOC_CH, Y_T_CH, T_VOC_CH);

    // Min SOC table
    assign_soc_min(N_N_CH, X_SOC_MIN_CH, T_SOC_MIN_CH);

    // Hys table
    assign_hys(N_H_CH, M_H_CH, X_DV_CH, Y_SOC_CH, T_R_CH, T_S_CH, T_DV_MAX_CH, T_DV_MIN_CH);
}

// Spare
// SP spare to reserve memory, perhaps for LION
const uint8_t M_T_SP = 4;    // Number temperature breakpoints for voc table
const uint8_t N_S_SP = 18;   // Number soc breakpoints for voc table
const float Y_T_SP[M_T_SP] = // Temperature breakpoints for voc table
    {5., 11.1, 20., 40.};
const float X_SOC_SP[N_S_SP] = // soc breakpoints for voc table
    {-0.15, 0.00, 0.05, 0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00};
const float T_VOC_SP[M_T_SP * N_S_SP] = // r(soc, dv) table
    {4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
     4.00, 4.00, 4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
     4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
     4.00, 4.00, 10.50, 13.10, 13.27, 13.31, 13.44, 13.46, 13.40, 13.44, 13.48, 13.52, 13.56, 13.60, 13.64, 13.68, 14.22, 15.00};
const uint8_t N_N_SP = 4;                                   // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_SP[N_N_SP] = {5., 11.1, 20., 40.};    // Temperature breakpoints for soc_min table
const float T_SOC_MIN_SP[N_N_SP] = {0.10, 0.07, 0.05, 0.0}; // soc_min(t)
void Chemistry::assign_SP()
{
    // Constants
    dqdt = 0.01;     // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dvoc_dt = 0.004; // Change of VOC with operating temperature in range 0 - 50 C V/deg C
    dvoc = 0.;       // Adjustment for calibration error, V (systematic error; may change in future)
    hys_cap = 3.6e3; // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022
    low_voc = 9.;    // Voltage threshold for BMS to turn off battery;
    low_t = 0;       // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C
    r_0 = 0.003;     // ChargeTransfer R0, ohms
    r_ct = 0.0077;   // ChargeTransfer diffusion resistance, ohms
    r_sd = 70;       // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
    tau_ct = 83.;    // ChargeTransfer diffusion time constant, s (=1/Rct/Cct)
    tau_sd = 2.5e7;  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
    c_sd = tau_sd / r_sd;
    v_sat = 13.85; // Saturation threshold at temperature, deg C
    Serial.printf("SP dv_min_abs=%7.3f, cap=%7.1f\n", dv_min_abs, hys_cap);

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
    if (n_h)
        delete x_dv;
    if (m_h)
    {
        delete y_soc;
        delete hys_Tn_;
        delete hys_Tx_;
    }
    if (m_h && n_h)
    {
        delete t_r;
        delete hys_T_;
        delete hys_Ts_;
    }
    if (m_h && n_h)
        delete t_s;
    if (m_h)
        delete t_x;
    if (m_h)
        delete t_n;

    // Instantiate
    n_h = _n_h;
    m_h = _m_h;
    x_dv = new float[n_h];
    y_soc = new float[m_h];
    t_r = new float[m_h * n_h];
    t_s = new float[m_h * n_h];
    t_x = new float[m_h];
    t_n = new float[m_h];

    // Assign
    for (i = 0; i < n_h; i++)
        x_dv[i] = x[i];
    for (i = 0; i < m_h; i++)
        y_soc[i] = y[i];
    for (i = 0; i < m_h * n_h; i++)
        t_r[i] = t[i];
    for (i = 0; i < m_h * n_h; i++)
        t_s[i] = s[i];
    for (i = 0; i < m_h; i++)
        t_x[i] = tx[i];
    for (i = 0; i < m_h; i++)
        t_n[i] = tn[i];

    // Finalize
    hys_T_ = new TableInterp2D(n_h, m_h, x_dv, y_soc, t_r);
    hys_Tn_ = new TableInterp1D(m_h, y_soc, t_n);
    hys_Ts_ = new TableInterp2D(n_h, m_h, x_dv, y_soc, t_s);
    hys_Tx_ = new TableInterp1D(m_h, y_soc, t_x);
}

// Workhorse assignment function for VOC_SOC
void Chemistry::assign_voc_soc(const int _n_s, const int _m_t, const float *x, const float *y, const float *t)
{
    int i;

    // Delete old if exists, before assigning anything
    if (n_s)
        delete x_soc;
    if (m_t)
        delete y_t;
    if (m_t && n_s)
    {
        delete t_voc;
        delete voc_T_;
    }

    // Instantiate
    n_s = _n_s; // Number of soc breakpoints voc table
    m_t = _m_t; // Number temperature breakpoints for voc table
    x_soc = new float[n_s];
    y_t = new float[m_t];
    t_voc = new float[m_t * n_s];

    // Assign
    for (i = 0; i < n_s; i++)
        x_soc[i] = x[i];
    for (i = 0; i < m_t; i++)
        y_t[i] = y[i];
    for (i = 0; i < m_t * n_s; i++)
        t_voc[i] = t[i];

    // Finalize
    voc_T_ = new TableInterp2D(n_s, m_t, x_soc, y_t, t_voc);
}

// Workhorse assignment function for SOC_MIN
void Chemistry::assign_soc_min(const int _n_n, const float *x, const float *t)
{
    int i;

    // Delete old if exists, before assigning anything
    if (n_n)
    {
        delete x_soc_min;
        delete t_soc_min;
        delete soc_min_T_;
    }

    // Instantiate
    n_n = _n_n; // Number of temperature breakpoints for x_soc_min table
    x_soc_min = new float[n_n];
    t_soc_min = new float[n_n];

    // Assign
    for (i = 0; i < n_n; i++)
        x_soc_min[i] = x[i];
    for (i = 0; i < n_n; i++)
        t_soc_min[i] = t[i];

    // Finalize
    soc_min_T_ = new TableInterp1D(n_n, x_soc_min, t_soc_min);
}

// Battery type model translate to plain English for display
String Chemistry::decode(const uint8_t mod)
{
    String result;
    if (mod == 0)
        result = "Battleborn";
    else if (mod == 1)
        result = "CHINS";
    else if (mod == 2)
        result = "Spare";
    else
    {
        result = "unknown";
        Serial.printf("C::decode:  unknown mod = %d. 'h'\n", mod);
    }
    return (result);
}

// Battery type model coding
uint8_t Chemistry::encode(const String mod_str)
{
    uint8_t result;
    if (mod_str == "Battleborn")
        result = 0;
    else if (mod_str == "CHINS")
        result = 1;
    else if (mod_str == "Spare")
        result = 2;
    else
    {
        result = 99;
        Serial.printf("C::encode:  unknown mod = %s.  'h'\n", mod_str.c_str());
    }
    return (result);
}

// Pretty print
void Chemistry::pretty_print(void)
{
    Serial.printf("Chemistry:\n");
    Serial.printf("  dqdt%7.3f, frac/dg C\n", dqdt);
    Serial.printf("  dv_min_abs%7.3f, V\n", dv_min_abs);
    Serial.printf("  dvoc%7.3f, V\n", dvoc);
    Serial.printf("  dvoc_dt%7.3f, V/dg C\n", dvoc_dt);
    Serial.printf("  hys_cap%7.0f, F\n", hys_cap);
    Serial.printf("  low_t%7.3f, V\n", low_t);
    Serial.printf("  low_voc%7.3f, V\n", low_voc);
    Serial.printf("  v_sat%7.3f, V\n", v_sat);
    Serial.printf("  vb_down%7.3f, shutoff, V\n", vb_down);
    Serial.printf("  vb_down_sim%7.3f, shutoff, V\n", vb_down_sim);
    Serial.printf("  vb_off%7.3f, shutoff, V (unused)\n", vb_off);
    Serial.printf("  vb_rising%7.3f, turnon, V\n", vb_rising);
    Serial.printf("  vb_rising_sim%7.3f, turnon, V\n", vb_rising_sim);
    Serial.printf("  ChargeTransfer:\n");
    Serial.printf("  c_sd%9.3g; EKF, farad\n", c_sd);
    Serial.printf("  r_0%9.6f, ohm\n", r_0);
    Serial.printf("  r_ct%9.6f, ohm\n", r_ct);
    Serial.printf("  r_sd%7.0f, EKF, ohm\n", r_sd);
    Serial.printf("  r_ss%9.6f, SS init, ohm\n", r_ss);
    Serial.printf("  tau_ct%7.3f, s\n", tau_ct);
    Serial.printf("  tau_sd%9.3g; EKF, s\n", tau_sd);
    Serial.printf("  voc(t, soc):\n");
    voc_T_->pretty_print();
    Serial.printf("  soc_min(temp_c):\n");
    soc_min_T_->pretty_print();
    Serial.printf("  r(soc, dv):\n");
    hys_T_->pretty_print();
    Serial.printf("  s(soc, dv):\n");
    hys_Ts_->pretty_print();
    Serial.printf("  r_max(soc):\n");
    hys_Tx_->pretty_print();
    Serial.printf("  r_min(soc):\n");
    hys_Tn_->pretty_print();
}
