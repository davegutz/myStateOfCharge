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
void Chemistry::assign_all_chm()
{
    if (CHEM == 0) // "Battleborn";
    {
        mod_code = 0;
        assign_BB();
    }
    else if (CHEM == 1) // "CHINS"
    {
        mod_code = 1;
        assign_CH();
    }
    else if (CHEM == 2) // "CHINS Garage"
    {
        mod_code = 2;
        assign_CH();
    }
    else
        Serial.printf("assign_all_mod:  unknown mod %d.  Type 'h' (Xm)\n", CHEM);
    r_ss = r_0 + r_ct;
}


// BattleBorn Chemistry
#if CHEM == 0
    // BattleBorn 100 Ah, 12v LiFePO4
    // See VOC_SOC data.xls.    T=40 values are only a notion.   Need data for it.
    // >13.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
    // 20230401:  Hysteresis tuned to soc=0.7 step data
    const uint8_t M_T = 5;    // Number temperature breakpoints for voc table
    const uint8_t N_S = 18;   // Number soc breakpoints for voc table
    float Y_T[M_T] = // Temperature breakpoints for voc table
        {5., 11.1, 20., 30., 40.};
    float X_SOC[N_S] = // soc breakpoints for voc table
        {-0.15, 0.00, 0.05, 0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00};
    // float T_VOC[M_T * N_S] = // r(soc, dv) table
    //     {4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
    //      4.00, 4.00, 4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
    //      4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
    //      4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
    //      4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50};
    float T_VOC[M_T * N_S] = // r(soc, dv) table  dag 20230726 tune by 0.3 nominal because data during slow discharge at -0.3 hysteresis
        {4.00, 4.00, 4.00,  4.00,  10.50, 12.00, 12.75, 13.00, 13.07, 13.20, 13.21, 13.28, 13.35, 13.41, 13.47, 13.52, 13.69, 14.25,
        4.00, 4.00, 4.00,  9.80,  12.30, 12.80, 13.00, 13.10, 13.20, 13.26, 13.31, 13.36, 13.41, 13.47, 13.50, 13.53, 13.70, 14.26,
        4.00, 4.00, 10.30, 12.90, 13.07, 13.15, 13.19, 13.25, 13.29, 13.33, 13.34, 13.39, 13.44, 13.51, 13.55, 13.57, 13.82, 14.30,
        4.00, 4.00, 12.30, 12.95, 13.05, 13.10, 13.15, 13.25, 13.30, 13.38, 13.42, 13.46, 13.50, 13.54, 13.56, 13.57, 13.82, 14.30,
        4.00, 4.00, 12.30, 12.95, 13.05, 13.10, 13.15, 13.25, 13.30, 13.38, 13.42, 13.46, 13.50, 13.54, 13.56, 13.57, 13.82, 14.30};
    const uint8_t N_N = 5;                                          // Number of temperature breakpoints for x_soc_min table
    float X_SOC_MIN[N_N] = {5., 11.1, 20., 30., 40.};      // Temperature breakpoints for soc_min table
    float T_SOC_MIN[N_N] = {0.10, 0.07, 0.05, 0.00, 0.20}; // soc_min(t).  At 40C BMS shuts off at 12V

    // Battleborn Hysteresis
    const uint8_t M_H = 3;     // Number of soc breakpoints in r(soc, dv) table t_r, t_s
    const uint8_t N_H = 7;     // Number of dv breakpoints in r(dv) table t_r, t_s
    float X_DV[N_H] = // dv breakpoints for r(soc, dv) table t_r. // DAG 6/13/2022 tune x10 to match data
        {-0.7, -0.5, -0.3, 0.0, 0.15, 0.3, 0.7};
    float Y_SOC[M_H] = // soc breakpoints for r(soc, dv) table t_r, t_s
        {0.0, 0.5, 0.7};
    float T_R[M_H * N_H] = // r(soc, dv) table.    // DAG 9/29/2022 tune to match hist data
        {0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
        0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
        0.016, 0.016, 0.013, 0.005, 0.007, 0.010, 0.010};
    float T_S[M_H * N_H] = // r(soc, dv) table. Not used yet for BB
        {1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1};
    float T_DV_MAX[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {0.7,  0.3,  0.2};
    float T_DV_MIN[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {-0.7, -0.5, -0.3};
#endif


// CHINS Chemistry
#if CHEM == 1
    // CHINS 100 Ah, 12v LiFePO4
    // 2023-02-27:  tune to data.  Add slight slope 0.8-0.98 to make models deterministic
    // 2023-08-29:  tune to data
    // 2024-04-03:  tune to data
    const uint8_t M_T = 3;    // Number temperature breakpoints for voc table
    const uint8_t N_S = 21;   // Number soc breakpoints for voc table
    float Y_T[M_T] = // Temperature breakpoints for voc table
        {5.1, 5.2, 21.5};
    float X_SOC[N_S] = // soc breakpoints for voc table
        {-0.035,   0.000,   0.050,   0.100,   0.108,   0.120,   0.140,   0.170,   0.200,   0.250,   0.300,   0.340,   0.400,   0.500,   0.600,   0.700,   0.800,   0.900,   0.980,   0.990,   1.000};
    float T_VOC[M_T * N_S] = // r(soc, dv) table
        {
        4.000,   4.000,  4.000,   4.000,    4.000,  4.000,  4.000,   4.000,   4.000,   4.000,   9.000,  11.770,  12.700,  12.950,  13.050,  13.100,  13.226,  13.259,  13.264,  13.460,  14.270,
        4.000,   4.000,  4.000,   4.000,    4.000,  4.000,  4.000,   4.000,   4.000,   4.000,   9.000,  11.770,  12.700,  12.950,  13.050,  13.100,  13.226,  13.259,  13.264,  13.460,  14.270,
        4.000,   4.000,  9.0000,  9.500,   11.260, 11.850, 12.400,  12.650,  12.730,  12.810,  12.920,  12.960,  13.020,  13.060,  13.220,  13.280,  13.284,  13.299,  13.310,  13.486,  14.700
        };
    const uint8_t N_N = 4;                                        // Number of temperature breakpoints for x_soc_min table
    float X_SOC_MIN[N_N] = {0.000,  11.00,  21.5,  40.000, };  // Temperature breakpoints for soc_min table
    float T_SOC_MIN[N_N] = {0.31,   0.31,   0.1,   0.1, };  // soc_min(t)
#elif CHEM == 2
    // 2024-04-24T14-51-24:  tune to data
    const uint8_t M_T = 3;    // Number temperature breakpoints for voc table
    const uint8_t N_S = 28;   // Number soc breakpoints for voc table
    float Y_T[M_T] = // Temperature breakpoints for voc table
        {21.5, 25.0, 35.0, };
    float X_SOC[N_S] = // soc breakpoints for voc table
        {-0.400, -0.300, -0.230, -0.200, -0.150, -0.130, -0.114, -0.044,  0.000,  0.016,  0.032,  0.055,  0.064,  0.114,  0.134,  0.154,  0.183,  0.214,  0.300,  0.400,  0.500,  0.600,  0.700,  0.800,  0.900,  0.960,  0.980,  1.000, }; 
    float T_VOC[M_T * N_S] = // soc breakpoints for soc_min table
        {
        4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  8.170, 11.285, 12.114, 12.558, 12.707, 12.875, 13.002, 13.054, 13.201, 13.275, 13.284, 13.299, 13.307, 13.310, 14.700, 
        4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  4.000,  7.947, 11.000, 11.946, 12.252, 12.588, 12.670, 12.797, 12.833, 12.864, 12.908, 12.957, 13.034, 13.081, 13.106, 13.159, 13.234, 13.272, 13.286, 13.300, 13.300, 14.760, 
        4.000,  4.000,  6.686,  8.206, 10.739, 12.045, 12.411, 12.799, 12.866, 12.890, 12.914, 12.949, 12.963, 13.037, 13.052, 13.067, 13.089, 13.112, 13.146, 13.196, 13.284, 13.318, 13.320, 13.320, 13.320, 13.320, 13.320, 14.760, 
        };
    const uint8_t N_N = 3; // Number of temperature breakpoints for x_soc_min table
    float X_SOC_MIN[N_N] = {21.5, 25.0, 35.0, };  // Temperature breakpoints for soc_min table
    float T_SOC_MIN[N_N] = {0.13, 0.00, -0.14, };  // soc_min(t)
#endif


// CHINS Hysteresis
#if( CHEM == 1 || CHEM == 2 )
    const uint8_t M_H = 4;     // Number of soc breakpoints in r(soc, dv) table t_r, t_s
    const uint8_t N_H = 10;    // Number of dv breakpoints in r(dv) table t_r, t_s
    float X_DV[N_H] = // dv breakpoints for r(soc, dv) table t_r, t_s
        {-.10, -.05, -.04, 0.0, .02, .04, .05, .06, .07, .10};
    float Y_SOC[M_H] = // soc breakpoints for r(soc, dv) table t_r, t_s
        {.47, .75, .80, .86};
    float T_R[M_H * N_H] = // r(soc, dv) table
        {0.003, 0.003, 0.4, 0.4, 0.4, 0.4, 0.010, 0.010, 0.010, 0.010,
        0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012,
        0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012,
        0.004, 0.004, 0.4, 0.4, .2, .09, 0.04, 0.006, 0.006, 0.006};
    float T_S[M_H * N_H] = // s(soc, dv) table
        {1., 1., .2, .2, .2, .2, 1., 1., 1., 1.,
        1., 1., .2, .2, .2, 1., 1., 1., 1., 1.,
        1., 1., .2, .2, .2, 1., 1., 1., 1., 1.,
        1., 1., .1, .1, .2, 1., 1., 1., 1., 1.};
    float T_DV_MAX[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {0.06, 0.1, 0.1, 0.06};
    float T_DV_MIN[M_H] = // dv_max(soc) table.  Pulled values from insp of T_R where flattens
        {-0.06, -0.06, -0.06, -0.06};
#endif

void Chemistry::assign_BB()
{
    // Constants
    rated_temp = 25.;   // Temperature at NOM_UNIT_CAP, deg C (25)
    coul_eff = 0.9985;  // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9985)
    dqdt = 0.01;        // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dv_min_abs = 0.3;   // Absolute value of +/- hysteresis limit, V (0.3)
    dvoc = 0.11;        // Baked-in table bias
    dvoc_dt = 0.004;    // Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
    hys_cap = 3.6e3;    // Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data. // div 10 again 9/29/2022 // div 10 again 11/30/2022 (3.6e3)
                        // tau_null = 1 / 0.005 / 3.6e3 = 0.056 s
    low_voc = 9.0;      // Voltage threshold for BMS to turn off battery, V (9.)
    low_t = 0;          // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C (0)
    r_0 = 0.0113;       // ChargeTransfer R0, ohms (0.0113)
    r_ct = 0.001;       // ChargeTransfer diffusion resistance, ohms (0.001)
    r_sd = 70;          // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70.)
    tau_ct = 83.;       // ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (83.)
    tau_sd = 2.5e7;     // Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
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

    mod_code = 0;
}


void Chemistry::assign_CH()
{
    // Constants
    rated_temp = 25.;  // Temperature at NOM_UNIT_CAP, deg C (25)
    coul_eff = 0.9976; // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (0.9976 for sres=1.6)
    dqdt = 0.01;       // Change of charge with temperature, fraction/deg C (0.01 from literature)
    dv_min_abs = 0.06; // Absolute value of +/- hysteresis limit, V (0.06)
    dvoc = -0.1;       // Baked-in table bias
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
    // CHINS has 11. vb shutoff point
    vb_off = 11.;         // Shutoff point in Mon, V (11.)
    vb_down = 10.6;       // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (10.6)
    vb_down_sim = 10.5;   // Shutoff point in Sim, V (10.5)
    vb_rising = 11.3;     // Shutoff point when off, V (11.3)
    vb_rising_sim = 10.75;// Shutoff point in Sim when off, V (10.75)
    v_sat = 13.85;        // Saturation threshold at temperature, deg C (13.85)

    // VOC_SOC table
    assign_voc_soc(N_S, M_T, X_SOC, Y_T, T_VOC);

    // Min SOC table
    assign_soc_min(N_N, X_SOC_MIN, T_SOC_MIN);

    // Hys table
    assign_hys(N_H, M_H, X_DV, Y_SOC, T_R, T_S, T_DV_MAX, T_DV_MIN);
}


// Workhorse assignment function for Hysteresis
void Chemistry::assign_hys(const int _n_h, const int _m_h, float *x, float *y, float *t, float *s, float *tx, float *tn)
{
    hys_T_ = new TableInterp2D(_n_h, _m_h, x, y, t);
    hys_Tn_ = new TableInterp1D(_m_h, y, tn);
    hys_Ts_ = new TableInterp2D(_n_h, _m_h, x, y, s);
    hys_Tx_ = new TableInterp1D(_m_h, y, tx);
}

// Workhorse assignment function for VOC_SOC
void Chemistry::assign_voc_soc(const int _n_s, const int _m_t, float *x, float *y, float *t)
{
    voc_T_ = new TableInterp2D(_n_s, _m_t, x, y, t);
}

// Workhorse assignment function for SOC_MIN
void Chemistry::assign_soc_min(const int _n_n, float *x, float *t)
{
    soc_min_T_ = new TableInterp1D(_n_n, x, t);
}

// Battery type model translate to plain English for display
String Chemistry::decode(const uint8_t mod)
{
    String result;
    if (mod == 0)
        result = "Battleborn";
    else if (mod == 1 || mod == 2)
        result = "CHINS";
    else
    {
        result = "unknown";
        Serial.printf("C::decode:  unknown mod %d. 'h' (Xm)\n", mod);
    }
    return (result);
}

// lookup_voc
float Chemistry::lookup_voc(const float soc, const float temp_c)
{
    return voc_T_->interp(soc, temp_c) + dvoc;
}

// Pretty print
void Chemistry::pretty_print(void)
{
#ifndef SOFT_DEPLOY_PHOTON
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
#else
     Serial.printf("Chemistry: silent DEPLOY\n");
#endif
}
