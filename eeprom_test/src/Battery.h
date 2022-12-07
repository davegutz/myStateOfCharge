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


#ifndef BATTERY_H_
#define BATTERY_H_

#include "constants.h"
class Sensors;
#define t_float float

#define RATED_TEMP      25.       // Temperature at RATED_BATT_CAP, deg C (25)
#define TCHARGE_DISPLAY_DEADBAND  0.1 // Inside this +/- deadband, charge time is displayed '---', A
#define T_RLIM          0.017     // Temperature sensor rate limit to minimize jumps in Coulomb counting, deg C/s (0.017 allows 1 deg for 1 minute)
const float VB_DC_DC = 13.5;      // DC-DC charger estimated voltage, V (13.5 < v_sat = 13.85)
#define EKF_CONV        1.5e-3    // EKF tracking error indicating convergence, V (1.5e-3)
#define EKF_T_CONV      30.       // EKF set convergence test time, sec (30.)
const float EKF_T_RESET = (EKF_T_CONV/2.); // EKF reset retest time, sec ('up 1, down 2')
#define EKF_Q_SD_NORM   0.0015    // Standard deviation of normal EKF process uncertainty, V (0.0015)
#define EKF_R_SD_NORM   0.5       // Standard deviation of normal EKF state uncertainty, fraction (0-1) (0.5)
#define EKF_NOM_DT      0.1       // EKF nominal update time, s (initialization; actual value varies) 
#define DF2             1.2       // Threshold to resest Coulomb Counter if different from ekf, fraction (0.20)
#define TAU_Y_FILT      5.        // EKF y-filter time constant, sec (5.)
#define MIN_Y_FILT      -0.5      // EKF y-filter minimum, V (-0.5)
#define MAX_Y_FILT      0.5       // EKF y-filter maximum, V (0.5)
#define WN_Y_FILT       0.1       // EKF y-filter-2 natural frequency, r/s (0.1)
#define ZETA_Y_FILT     0.9       // EKF y-fiter-2 damping factor (0.9)
#define TMAX_FILT       3.        // Maximum y-filter-2 sample time, s (3.)
#define SOLV_ERR        1e-6      // EKF initialization solver error bound, V (1e-6)
#define SOLV_MAX_COUNTS 30        // EKF initialization solver max iters (30)
#define SOLV_SUCC_COUNTS 6        // EKF initialization solver iters to switch from successive approximation to Newton-Rapheson (6)
#define SOLV_MAX_STEP   0.2       // EKF initialization solver max step size of soc, fraction (0.2)
#define HYS_INIT_COUNTS 30        // Maximum initialization iterations hysteresis (50)
#define HYS_INIT_TOL    1e-8      // Initialization tolerance hysteresis (1e-8)
#define RANDLES_T_MAX   0.31      // Maximum update time of Randles state space model to avoid aliasing and instability (0.31 allows DP3)
const double MXEPS = 1-1e-6;      // Level of soc that indicates mathematically saturated (threshold is lower for robustness) (1-1e-6)
#define HYS_SCALE       1.0       // Scalar on hysteresis (1.0)
#define HYS_SOC_MIN_MARG 0.15     // Add to soc_min to set thr for detecting low endpoint condition for reset of hysteresis (0.15)
#define HYS_IB_THR      1.0       // Ignore reset if opposite situation exists, A (1.0)
#define HYS_DV_MIN      0.2       // Minimum value of hysteresis reset, V (0.2)
#define V_BATT_OFF      10.       // Shutoff point in Mon, V (10.)
#define V_BATT_DOWN     9.8       // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
#define V_BATT_RISING   10.3      // Shutoff point when off, V (10.3)
#define V_BATT_DOWN_SIM   9.5     // Shutoff point in Sim, V (9.5)
#define V_BATT_RISING_SIM 9.75    // Shutoff point in Sim when off, V (9.75)


// BattleBorn 100 Ah, 12v LiFePO4
// See VOC_SOC data.xls.    T=40 values are only a notion.   Need data for it.
// >13.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
const uint8_t M_T_BB  = 5;    // Number temperature breakpoints for voc table
const uint8_t N_S_BB  = 18;   // Number soc breakpoints for voc table
const float Y_T_BB[M_T_BB] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 30., 40. }; 
const float X_SOC_BB[N_S_BB] =      //soc breakpoints for voc table
        { -0.15, 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99, 0.995, 1.00};
const float T_VOC_BB[M_T_BB*N_S_BB] = // r(soc, dv) table
        { 4.00, 4.00,   4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
          4.00, 4.00,   4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
          4.00, 4.00,   10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
          4.00, 4.00,   12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
          4.00, 4.00,   4.00,  4.00,  10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19, 13.20, 13.72, 14.50};
const uint8_t N_N_BB = 5;   // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_BB[N_N_BB] =  { 5.,   11.1,  20.,  30.,  40.};  // Temperature breakpoints for soc_min table
const float T_SOC_MIN_BB[N_N_BB] =  { 0.10, 0.07,  0.05, 0.00, 0.20}; // soc_min(t).  At 40C BMS shuts off at 12V
// Hysteresis
const uint8_t M_H_BB  = 3;          // Number of soc breakpoints in r(soc, dv) table t_r
const uint8_t N_H_BB  = 7;          // Number of dv breakpoints in r(dv) table t_r
const float X_DV_BB[N_H_BB] =       // dv breakpoints for r(soc, dv) table t_r. // DAG 6/13/2022 tune x10 to match data
        { -0.7,  -0.5,  -0.3,  0.0,   0.15,  0.3,   0.7};
const float Y_SOC_BB[M_H_BB] =      // soc breakpoints for r(soc, dv) table t_r
        { 0.0,  0.5,   1.0};
const float T_R_BB[M_H_BB*N_H_BB] = // r(soc, dv) table.    // DAG 9/29/2022 tune to match hist data
        { 0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
          0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
          0.016, 0.016, 0.016, 0.005, 0.010, 0.010, 0.010};
const float T_DV_MAX_BB[M_H_BB] =   // dv_max(soc) table.  Pulled values from insp of T_R_BB where flattens
        {0.7, 0.3, 0.15};
const float T_DV_MIN_BB[M_H_BB] =   // dv_max(soc) table.  Pulled values from insp of T_R_BB where flattens
        {-0.7, -0.5, -0.3};

// LION 100 Ah, 12v LiFePO4.  "LION" placeholder.  Data fabricated.   Useful to test weird shapes T=40 (Dt15)
// shifted Battleborn because don't have real data yet; test structure of program
const uint8_t M_T_LI  = 4;    // Number temperature breakpoints for voc table
const uint8_t N_S_LI  = 18;   // Number soc breakpoints for voc table
const float Y_T_LI[M_T_LI] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 40. }; 
const float X_SOC_LI[N_S_LI] =      //soc breakpoints for voc table
        { -0.15, 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99, 0.995, 1.00};
const float T_VOC_LI[M_T_LI*N_S_LI] = // r(soc, dv) table
        { 4.00, 4.00,  4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
          4.00, 4.00,  4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.2,  13.23, 13.60, 14.46,
          4.00, 4.00,  10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
          4.00, 4.00,  11.00, 13.60, 13.77, 13.85, 13.89, 13.95, 13.99, 14.03, 14.04, 13.80, 13.54, 13.21, 13.25, 13.27, 14.72, 15.50};
const uint8_t N_N_LI = 4;   // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_LI[N_N_LI] =  { 5.,   11.1,  20.,  40.};  // Temperature breakpoints for soc_min table
const float T_SOC_MIN_LI[N_N_LI] =  { 0.10, 0.07,  0.05, 0.03}; // soc_min(t)
// Hysteresis
const uint8_t M_H_LI  = 3;          // Number of soc breakpoints in r(soc, dv) table t_r
const uint8_t N_H_LI  = 7;          // Number of dv breakpoints in r(dv) table t_r
const float X_DV_LI[N_H_LI] =       // dv breakpoints for r(soc, dv) table t_r
        { -0.7,  -0.5,  -0.3,  0.0,   0.15,  0.3,   0.7 };
const float Y_SOC_LI[M_H_LI] =      // soc breakpoints for r(soc, dv) table t_r
        { 0.0,  0.5,   1.0};
const float T_R_LI[M_H_LI*N_H_LI] = // r(soc, dv) table.    // DAG 9/29/2022 tune to match hist data
        { 0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
          0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
          0.016, 0.016, 0.016, 0.005, 0.010, 0.010, 0.010};
const float T_DV_MAX_LI[M_H_LI] =   // dv_max(soc) table.  Pulled values from insp of T_R_LI where flattens
        {0.7, 0.3, 0.15};
const float T_DV_MIN_LI[M_H_LI] =   // dv_max(soc) table.  Pulled values from insp of T_R_LI where flattens
        {-0.7, -0.5, -0.3};


// LION control EKF curve that is monotonic increasing
const uint8_t M_T_LIE  = 4;    // Number temperature breakpoints for voc table
const uint8_t N_S_LIE  = 18;   // Number soc breakpoints for voc table
const float Y_T_LIE[M_T_LIE] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 40. }; 
const float X_SOC_LIE[N_S_LIE] =      //soc breakpoints for voc table
        { -0.15, 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99, 0.995, 1.00};
const float T_VOC_LIE[M_T_LIE*N_S_LIE] = // r(soc, dv) table
        { 4.00, 4.00,  4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
          4.00, 4.00,  4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.2,  13.23, 13.60, 14.46,
          4.00, 4.00,  10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
          4.00, 4.00,  10.50, 13.10, 13.27, 13.31, 13.44, 13.46, 13.40, 13.44, 13.48, 13.52, 13.56, 13.60, 13.64, 13.68, 14.22, 15.00};
const uint8_t N_N_LIE = 4;   // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_LIE[N_N_LIE] =  { 5.,   11.1,  20.,  40.};  // Temperature breakpoints for soc_min table
const float T_SOC_MIN_LIE[N_N_LIE] =  { 0.10, 0.07,  0.05, 0.0}; // soc_min(t)


#endif