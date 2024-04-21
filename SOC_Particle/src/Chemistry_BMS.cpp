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
#include "command.h"

extern SavedPars sp; // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
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
    else
        Serial.printf("assign_all_mod:  unknown mod %d.  Type 'h' (Xm)\n", mod);
    r_ss = r_0 + r_ct;
}


// BattleBorn Chemistry
#if( defined(CONFIG_PRO1A) || defined(CONFIG_SOC1A) )
    // BattleBorn 100 Ah, 12v LiFePO4
    // See VOC_SOC data.xls.    T=40 values are only a notion.   Need data for it.
    // >13.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
    // 20230401:  Hysteresis tuned to soc=0.7 step data
    const uint8_t M_T = 5;    // Number temperature breakpoints for voc table
    const uint8_t N_S = 18;   // Number soc breakpoints for voc table
    const float Y_T[M_T] = // Temperature breakpoints for voc table
        {5., 11.1, 20., 30., 40.};
    const float X_SOC[N_S] = // soc breakpoints for voc table
        {-0.15, 0.00, 0.05, 0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00};
    // const float T_VOC[M_T * N_S] = // r(soc, dv) table
    //     {4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
    //      4.00, 4.00, 4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
    //      4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
    //      4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
    //      4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50};
    const float T_VOC[M_T * N_S] = // r(soc, dv) table  dag 20230726 tune by 0.3 nominal because data during slow discharge at -0.3 hysteresis
        {4.00, 4.00, 4.00,  4.00,  10.50, 12.00, 12.75, 13.00, 13.07, 13.20, 13.21, 13.28, 13.35, 13.41, 13.47, 13.52, 13.69, 14.25,
        4.00, 4.00, 4.00,  9.80,  12.30, 12.80, 13.00, 13.10, 13.20, 13.26, 13.31, 13.36, 13.41, 13.47, 13.50, 13.53, 13.70, 14.26,
        4.00, 4.00, 10.30, 12.90, 13.07, 13.15, 13.19, 13.25, 13.29, 13.33, 13.34, 13.39, 13.44, 13.51, 13.55, 13.57, 13.82, 14.30,
        4.00, 4.00, 12.30, 12.95, 13.05, 13.10, 13.15, 13.25, 13.30, 13.38, 13.42, 13.46, 13.50, 13.54, 13.56, 13.57, 13.82, 14.30,
        4.00, 4.00, 12.30, 12.95, 13.05, 13.10, 13.15, 13.25, 13.30, 13.38, 13.42, 13.46, 13.50, 13.54, 13.56, 13.57, 13.82, 14.30};
    const uint8_t N_N = 5;                                          // Number of temperature breakpoints for x_soc_min table
    const float X_SOC_MIN[N_N] = {5., 11.1, 20., 30., 40.};      // Temperature breakpoints for soc_min table
    const float T_SOC_MIN[N_N] = {0.10, 0.07, 0.05, 0.00, 0.20}; // soc_min(t).  At 40C BMS shuts off at 12V

    // Battleborn Hysteresis
    const uint8_t M_H = 3;     // Number of soc breakpoints in r(soc, dv) table t_r, t_s
    const uint8_t N_H = 7;     // Number of dv breakpoints in r(dv) table t_r, t_s
    const float X_DV[N_H] = // dv breakpoints for r(soc, dv) table t_r. // DAG 6/13/2022 tune x10 to match data
        {-0.7, -0.5, -0.3, 0.0, 0.15, 0.3, 0.7};
    const float Y_SOC[M_H] = // soc breakpoints for r(soc, dv) table t_r, t_s
        {0.0, 0.5, 0.7};
    const float T_R[M_H * N_H] = // r(soc, dv) table.    // DAG 9/29/2022 tune to match hist data
        {0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
        0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
        0.016, 0.016, 0.013, 0.005, 0.007, 0.010, 0.010};
    const float T_S[M_H * N_H] = // r(soc, dv) table. Not used yet for BB
        {1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1};
    const float T_DV_MAX[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {0.7,  0.3,  0.2};
    const float T_DV_MIN[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {-0.7, -0.5, -0.3};
#endif


// CHINS Chemistry
#if( defined(CONFIG_PRO3P2) )
    // CHINS 100 Ah, 12v LiFePO4
    // 2023-02-27:  tune to data.  Add slight slope 0.8-0.98 to make models deterministic
    // 2023-08-29:  tune to data
    // 2024-04-03:  tune to data
    const uint8_t M_T = 3;    // Number temperature breakpoints for voc table
    const uint8_t N_S = 21;   // Number soc breakpoints for voc table
    const float Y_T[M_T] = // Temperature breakpoints for voc table
        {5.1, 5.2, 21.5};
    const float X_SOC[N_S] = // soc breakpoints for voc table
        {-0.035,   0.000,   0.050,   0.100,   0.108,   0.120,   0.140,   0.170,   0.200,   0.250,   0.300,   0.340,   0.400,   0.500,   0.600,   0.700,   0.800,   0.900,   0.980,   0.990,   1.000};
    const float T_VOC[M_T * N_S] = // r(soc, dv) table
        {
        4.000,   4.000,  4.000,   4.000,    4.000,  4.000,  4.000,   4.000,   4.000,   4.000,   9.000,  11.770,  12.700,  12.950,  13.050,  13.100,  13.226,  13.259,  13.264,  13.460,  14.270,
        4.000,   4.000,  4.000,   4.000,    4.000,  4.000,  4.000,   4.000,   4.000,   4.000,   9.000,  11.770,  12.700,  12.950,  13.050,  13.100,  13.226,  13.259,  13.264,  13.460,  14.270,
        4.000,   4.000,  9.0000,  9.500,   11.260, 11.850, 12.400,  12.650,  12.730,  12.810,  12.920,  12.960,  13.020,  13.060,  13.220,  13.280,  13.284,  13.299,  13.310,  13.486,  14.700
        };
    const uint8_t N_N = 4;                                        // Number of temperature breakpoints for x_soc_min table
    const float X_SOC_MIN[N_N] = {0.000,  11.00,  21.5,  40.000, };  // Temperature breakpoints for soc_min table
    const float T_SOC_MIN[N_N] = {0.31,   0.31,   0.1,   0.1, };  // soc_min(t)
#elif ( defined(CONFIG_PRO0P) )
     // 2024-04-20T05-47-20:  tune to data
    const uint8_t M_T = 2;    // Number temperature breakpoints for voc table
    const uint8_t N_S = 22;   // Number soc breakpoints for voc table
    const float Y_T[M_T] = // Temperature breakpoints for voc table
        {21.5, 25.0, };
    const float X_SOC[N_S] = // soc breakpoints for voc table
        {-0.114, -0.044,  0.000,  0.016,  0.032,  0.055,  0.064,  0.114,  0.134,  0.154,  0.183,  0.214,  0.300,  0.400,  0.500,  0.600,  0.700,  0.800,  0.900,  0.960,  0.980,  1.000, }; 
    const float T_VOC[M_T * N_S] = // soc breakpoints for soc_min table
        {
        4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  9.481, 11.854, 12.408, 12.649, 12.733, 12.895, 13.010, 13.057, 13.210, 13.277, 13.284, 13.299, 13.307, 13.310, 14.700, 
        4.000,  8.950, 11.880, 12.198, 12.495, 12.700, 12.725, 12.820, 12.850, 12.881, 12.925, 12.972, 13.041, 13.084, 13.107, 13.162, 13.237, 13.273, 13.286, 13.300, 13.300, 14.760, 
        };
    const uint8_t N_N = 2; // Number of temperature breakpoints for x_soc_min table
    const float X_SOC_MIN[N_N] = {21.5, 25.0, };  // Temperature breakpoints for soc_min table
    const float T_SOC_MIN[N_N] = {0.12, -0.03, };  // soc_min(t)
#endif


// CHINS Hysteresis
#if( defined(CONFIG_PRO0P) || defined(CONFIG_PRO3P2) )
    const uint8_t M_H = 4;     // Number of soc breakpoints in r(soc, dv) table t_r, t_s
    const uint8_t N_H = 10;    // Number of dv breakpoints in r(dv) table t_r, t_s
    const float X_DV[N_H] = // dv breakpoints for r(soc, dv) table t_r, t_s
        {-.10, -.05, -.04, 0.0, .02, .04, .05, .06, .07, .10};
    const float Y_SOC[M_H] = // soc breakpoints for r(soc, dv) table t_r, t_s
        {.47, .75, .80, .86};
    const float T_R[M_H * N_H] = // r(soc, dv) table
        {0.003, 0.003, 0.4, 0.4, 0.4, 0.4, 0.010, 0.010, 0.010, 0.010,
        0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012,
        0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012,
        0.004, 0.004, 0.4, 0.4, .2, .09, 0.04, 0.006, 0.006, 0.006};
    const float T_S[M_H * N_H] = // s(soc, dv) table
        {1., 1., .2, .2, .2, .2, 1., 1., 1., 1.,
        1., 1., .2, .2, .2, 1., 1., 1., 1., 1.,
        1., 1., .2, .2, .2, 1., 1., 1., 1., 1.,
        1., 1., .1, .1, .2, 1., 1., 1., 1., 1.};
    const float T_DV_MAX[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {0.06, 0.1, 0.1, 0.06};
    const float T_DV_MIN[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {-0.06, -0.06, -0.06, -0.06};
#endif

void Chemistry::assign_BB()
{
    // Constants
    rated_temp = 25.;  // Temperature at NOM_UNIT_CAP, deg C (25)
    coul_eff = 0.9985; // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9985)
    dqdt = 0.01;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dv_min_abs = 0.3;  // Absolute value of +/- hysteresis limit, V (0.3)
    dvoc_dt = 0.004;   // Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
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
    assign_voc_soc(N_S, M_T, X_SOC, Y_T, T_VOC);

    // Min SOC table
    assign_soc_min(N_N, X_SOC_MIN, T_SOC_MIN);

    // Hys table
    assign_hys(N_H, M_H, X_DV, Y_SOC, T_R, T_S, T_DV_MAX, T_DV_MIN);
}


void Chemistry::assign_CH()
{
    // Constants
    rated_temp = 25.;  // Temperature at NOM_UNIT_CAP, deg C (25)
    coul_eff = 0.9976; // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9976 for sres=1.6)
    dqdt = 0.01;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dv_min_abs = 0.06; // Absolute value of +/- hysteresis limit, V (0.06)
    dvoc_dt = -0.01;   // Change of VOC with operating temperature in range 0 - 50 C V/deg C (-0.01)
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
    assign_voc_soc(N_S, M_T, X_SOC, Y_T, T_VOC);

    // Min SOC table
    assign_soc_min(N_N, X_SOC_MIN, T_SOC_MIN);

    // Hys table
    assign_hys(N_H, M_H, X_DV, Y_SOC, T_R, T_S, T_DV_MAX, T_DV_MIN);
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
    else
    {
        result = "unknown";
        Serial.printf("C::decode:  unknown mod %d. 'h' (Xm)\n", mod);
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
    else
    {
        result = 99;
        Serial.printf("C::encode:  unknown mod %s 'h' (Xm)\n", mod_str.c_str());
    }
    return (result);
}

// Pretty print
void Chemistry::pretty_print(void)
{
#ifndef DEPLOY_PHOTON
    Serial.printf("Chemistry:\n");
    Serial.printf("  dqdt%7.3f, frac/dg C\n", dqdt);
    Serial.printf("  dv_min_abs%7.3f, V\n", dv_min_abs);
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
#else
     Serial.printf("Chemistry: silent DEPLOY\n");
#endif
}
