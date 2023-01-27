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


#ifndef BATTERY_H_
#define BATTERY_H_

#include "myLibrary/myTables.h"
#include "myLibrary/EKF_1x1.h"
#include "myLibrary/StateSpace.h"
#include "Coulombs.h"
#include "myLibrary/injection.h"
#include "myLibrary/myFilters.h"
#include "constants.h"
#include "myLibrary/iterate.h"
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
const float MXEPS = 1-1e-6;      // Level of soc that indicates mathematically saturated (threshold is lower for robustness) (1-1e-6)
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

// CHINS 100 Ah, 12v LiFePO4.  Initial data from manual
const uint8_t M_T_CH  = 4;    // Number temperature breakpoints for voc table
const uint8_t N_S_CH  = 18;   // Number soc breakpoints for voc table
const float Y_T_CH[M_T_CH] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 40. }; 
const float X_SOC_CH[N_S_CH] =      //soc breakpoints for voc table
        { -0.15, 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99, 0.995, 1.00};
const float T_VOC_CH[M_T_CH*N_S_CH] = // r(soc, dv) table
        { 4.00, 4.00,  4.00,  4.00,  10.,   11.,   13.09, 13.15, 13.20, 13.26, 13.28, 13.31, 13.34, 13.37, 13.40, 13.42, 13.60, 14.45,
          4.00, 4.00,  4.00,  9.50,  13.01, 13.05, 13.09, 13.15, 13.20, 13.26, 13.28, 13.31, 13.34, 13.37, 13.40, 13.42, 13.60, 14.45,
          4.00, 4.00,  10.00, 12.97, 13.01, 13.05, 13.09, 13.15, 13.20, 13.26, 13.28, 13.31, 13.34, 13.37, 13.40, 13.42, 13.60, 14.45,
          4.00, 4.00,  11.00, 12.97, 13.01, 13.05, 13.09, 13.15, 13.20, 13.26, 13.28, 13.31, 13.34, 13.37, 13.40, 13.42, 13.60, 14.45};
const uint8_t N_N_CH = 4;   // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_CH[N_N_CH] =  { 5.,   11.1,  20.,  40.};  // Temperature breakpoints for soc_min table
const float T_SOC_MIN_CH[N_N_CH] =  { 0.10, 0.07,  0.05, 0.03}; // soc_min(t)
// Hysteresis
const uint8_t M_H_CH  = 3;          // Number of soc breakpoints in r(soc, dv) table t_r
const uint8_t N_H_CH  = 7;          // Number of dv breakpoints in r(dv) table t_r
const float X_DV_CH[N_H_CH] =       // dv breakpoints for r(soc, dv) table t_r
        { -0.7,  -0.5,  -0.3,  0.0,   0.15,  0.3,   0.7 };
const float Y_SOC_CH[M_H_CH] =      // soc breakpoints for r(soc, dv) table t_r
        { 0.0,  0.5,   1.0};
const float T_R_CH[M_H_CH*N_H_CH] = // r(soc, dv) table.    // DAG 9/29/2022 tune to match hist data
        { 0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
          0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
          0.016, 0.016, 0.016, 0.005, 0.010, 0.010, 0.010};
const float T_DV_MAX_CH[M_H_CH] =   // dv_max(soc) table.  Pulled values from insp of T_R_CH where flattens
        {0.7, 0.3, 0.15};
const float T_DV_MIN_CH[M_H_CH] =   // dv_max(soc) table.  Pulled values from insp of T_R_CH where flattens
        {-0.7, -0.5, -0.3};


// SP spare to reserve memory, perhaps for LION
const uint8_t M_T_SP  = 4;    // Number temperature breakpoints for voc table
const uint8_t N_S_SP  = 18;   // Number soc breakpoints for voc table
const float Y_T_SP[M_T_SP] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 40. }; 
const float X_SOC_SP[N_S_SP] =      //soc breakpoints for voc table
        { -0.15, 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99, 0.995, 1.00};
const float T_VOC_SP[M_T_SP*N_S_SP] = // r(soc, dv) table
        { 4.00, 4.00,  4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
          4.00, 4.00,  4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.2,  13.23, 13.60, 14.46,
          4.00, 4.00,  10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
          4.00, 4.00,  10.50, 13.10, 13.27, 13.31, 13.44, 13.46, 13.40, 13.44, 13.48, 13.52, 13.56, 13.60, 13.64, 13.68, 14.22, 15.00};
const uint8_t N_N_SP = 4;   // Number of temperature breakpoints for x_soc_min table
const float X_SOC_MIN_SP[N_N_SP] =  { 5.,   11.1,  20.,  40.};  // Temperature breakpoints for soc_min table
const float T_SOC_MIN_SP[N_N_SP] =  { 0.10, 0.07,  0.05, 0.0}; // soc_min(t)
// Hysteresis: reservoir model of battery electrical hysteresis
// Use variable resistor and capacitor to create hysteresis from an RC circuit
class Hysteresis
{
public:
  Hysteresis();
  Hysteresis(const float cap, Chemistry chem);
  ~Hysteresis();
  // operators
  // functions
  float calculate(const float ib, const float soc);
  float dv_max(const float soc) { return hys_Tx_->interp(soc); };
  float dv_min(const float soc) { return hys_Tn_->interp(soc); };
  void init(const float dv_init);
  float look_hys(const float dv, const float soc);
  void pretty_print();
  float update(const double dt, const boolean init_high, const boolean init_low, const float e_wrap, const boolean reset);
  float ioc() { return ioc_; };
  float dv_hys() { return dv_hys_; };
  void dv_hys(const float st) { dv_hys_ = max(min(st, dv_max(soc_)), dv_min(soc_)); };
//   float scale() { return sp.hys_scale(); };
protected:
  boolean disabled_;    // Hysteresis disabled by low scale input < 1e-5, T=disabled
  float cap_;          // Capacitance, Farads
  float res_;          // Variable resistance value, ohms
  float soc_;          // State of charge input, dimensionless
  float ib_;           // Current in, A
  float ioc_;          // Current out, A
  float dv_hys_;       // State, voc_-voc_stat_, V
  float dv_dot_;       // Calculated voltage rate, V/s
  float tau_;          // Null time constant, sec
  TableInterp2D *hys_T_;// dv-soc 2-D table, V
  TableInterp1D *hys_Tx_;// soc 1-D table, V_max
  TableInterp1D *hys_Tn_;// soc 1-D table, V_min
};


// Battery Class
class Battery : public Coulombs
{
public:
  Battery();
  Battery(double *sp_delta_q, float *sp_t_last, uint8_t *sp_mod_code);
  ~Battery();
  // operators
  // functions
  virtual void assign_randles(void) { Serial.printf("ERROR:  Battery::assign_randles called\n"); };
  boolean bms_off() { return bms_off_; };
  float calc_soc_voc(const float soc, const float temp_c, float *dv_dsoc);
  float calc_soc_voc_slope(float soc, float temp_c);
  float calc_vsat(void);
  virtual float calculate(const float temp_C, const float soc_frac, float curr_in, const double dt, const boolean dc_dc_on);
  float C_rate() { return ib_ / RATED_BATT_CAP; }
  String decode(const uint8_t mod);
  float dqdt() { return chem_.dqdt; };
  void ds_voc_soc(const float _ds) { ds_voc_soc_ = _ds; };
  float ds_voc_soc() { return ds_voc_soc_; };
  void Dv(const float _dv) { chem_.dvoc = _dv; };
  float Dv() { return chem_.dvoc; };
  float dv_dsoc() { return dv_dsoc_; };
  float dv_dyn() { return dv_dyn_; };
  void dv_voc_soc(const float _dv) { dv_voc_soc_ = _dv; };
  float dv_voc_soc() { return dv_voc_soc_; };
  uint8_t encode(const String mod_str);
  void hys_pretty_print () { hys_->pretty_print(); };
  float hys_state() { return hys_->dv_hys(); };
  void hys_state(const float st) { hys_->dv_hys(st); };
  void init_hys(const float hys) { hys_->init(hys); };
  float ib() { return ib_; };            // Battery terminal current, A
  float ioc() { return ioc_; };
  virtual void pretty_print();
  void pretty_print_ss();
  void print_signal(const boolean print) { print_now_ = print; };
  void Sr(const float sr) { sr_ = sr; Randles_->insert_D(0, 0, -chem_.r_0*sr_); };
  float Sr() { return sr_; };
  float temp_c() { return temp_c_; };    // Battery temperature, deg C
  float Tb() { return temp_c_; };        // Battery bank temperature, deg C
  float vb() { return vb_; };            // Battery terminal voltage, V
  float voc() { return voc_; };
  float voc_stat() { return voc_stat_; };
  float voc_soc_tab(const float soc, const float temp_c);
  float vsat() { return vsat_; };
protected:
  boolean bms_off_; // Indicator that battery management system is off, T = off preventing current flow
  float ds_voc_soc_;    // VOC(SOC) delta soc on input
  float dt_;       // Update time, s
  float dv_dsoc_;  // Derivative scaled, V/fraction
  float dv_dyn_;   // ib-induced back emf, V
  float dv_hys_;   // Hysteresis state, voc-voc_out, V
  float dv_voc_soc_;    // VOC(SOC) delta voc on output
  float ib_;       // Battery terminal current, A
  float ioc_;      // Current into charge portion of battery, A
  float nom_vsat_; // Nominal saturation threshold at 25C, V
  boolean print_now_; // Print command
  float sr_;       // Resistance scalar
  float temp_c_;    // Battery temperature, deg C
  float vb_;       // Battery terminal voltage, V
  float voc_;      // Static model open circuit voltage, V
  float voc_stat_; // Static, table lookup value of voc before applying hysteresis, V
  float vsat_;     // Saturation threshold at temperature, V
  // EKF declarations
  StateSpace *Randles_; // Randles model {ib, vb} --> {voc}, ioc=ib for Battery version
                        // Randles model {ib, voc} --> {vb}, ioc=ib for BatterySim version
  double *rand_A_;  // Randles model A
  double *rand_B_;  // Randles model B
  double *rand_C_;  // Randles model C
  double *rand_D_;  // Randles model D
  Hysteresis *hys_;
};


// BatteryMonitor: extend Battery to use as monitor object
class BatteryMonitor: public Battery, public EKF_1x1
{
public:
  BatteryMonitor();
  ~BatteryMonitor();
  // operators
  // functions
  float amp_hrs_remaining_ekf() { return amp_hrs_remaining_ekf_; };
  float amp_hrs_remaining_soc() { return amp_hrs_remaining_soc_; };
  virtual void assign_randles(void);
  float calc_charge_time(const double q, const float q_capacity, const float charge_curr, const float soc);
  float calculate(Sensors *Sen, const boolean reset);
  boolean converged_ekf() { return EKF_converged->state(); };
  double delta_q_ekf() { return delta_q_ekf_; };
  float hx() { return hx_; };
  float ib_charge() { return ib_charge_; };
  void init_battery_mon(const boolean reset, Sensors *Sen);
  void init_soc_ekf(const float soc);
  boolean is_sat(const boolean reset);
  float K_ekf() { return K_; };
  void pretty_print(Sensors *Sen);
  void regauge(const float temp_c);
  float r_sd () { return chem_.r_sd; };
  float r_ss () { return chem_.r_ss; };
  float soc_ekf() { return soc_ekf_; };
  boolean solve_ekf(const boolean reset, const boolean reset_temp, Sensors *Sen);
  float tcharge() { return tcharge_; };
  float dv_dyn() { return dv_dyn_; };
  float voc_filt() { return voc_filt_; };
  float voc_soc() { return voc_soc_; };
  double y_ekf() { return y_; };
  double y_ekf_filt() { return y_filt_; };
  double delta_q_ekf_;         // Charge deficit represented by charge calculated by ekf, C
protected:
  General2_Pole *y_filt = new General2_Pole(2.0, WN_Y_FILT, ZETA_Y_FILT, MIN_Y_FILT, MAX_Y_FILT);  // actual update time provided run time
  SlidingDeadband *SdVb_;  // Sliding deadband filter for Vb
  TFDelay *EKF_converged;     // Time persistence
  RateLimit *T_RLim = new RateLimit();
  Iterator *ice_;       // Iteration control for EKF solver
  float amp_hrs_remaining_ekf_;  // Discharge amp*time left if drain to q_ekf=0, A-h
  float amp_hrs_remaining_soc_;  // Discharge amp*time left if drain soc_ to 0, A-h
  double dt_eframe_;    // Update time for EKF major frame
  uint8_t eframe_;      // Counter to run EKF slower than Coulomb Counter and Randles models
  float ib_charge_;  // Current input avaiable for charging, A
  double q_ekf_;        // Filtered charge calculated by ekf, C
  float soc_ekf_;      // Filtered state of charge from ekf (0-1)
  float tcharge_;      // Counted charging time to 100%, hr
  float tcharge_ekf_;  // Solved charging time to 100% from ekf, hr
  float voc_filt_;     // Filtered, static model open circuit voltage, V
  float voc_soc_;  // Raw table lookup of voc, V
  float y_filt_;       // Filtered EKF y value, V
  void ekf_predict(double *Fx, double *Bu);
  void ekf_update(double *hx, double *H);
};


// BatterySim: extend Battery to use as model object
class BatterySim: public Battery
{
public:
  BatterySim();
  ~BatterySim();
  // operators
  // functions
  virtual void assign_randles(void);
  float calculate(Sensors *Sen, const boolean dc_dc_on, const boolean reset);
  float calc_inj(const unsigned long now, const uint8_t type, const float amp, const double freq);
  float count_coulombs(Sensors *Sen, const boolean reset, BatteryMonitor *Mon, const boolean initializing_all);
  boolean cutback() { return model_cutback_; };
  double delta_q() { return *sp_delta_q_; };
  unsigned long int dt(void) { return sample_time_ - sample_time_z_; };
  float ib_charge() { return ib_charge_; };
  float ib_fut() { return ib_fut_; };
  void init_battery_sim(const boolean reset, Sensors *Sen);
  void pretty_print(void);
  unsigned long int sample_time(void) { return sample_time_; };
  boolean saturated() { return model_saturated_; };
  float t_last() { return *sp_t_last_; };
  float voc() { return voc_; };
  float voc_stat() { return voc_stat_; };
protected:
  SinInj *Sin_inj_;         // Class to create sine waves
  SqInj *Sq_inj_;           // Class to create square waves
  TriInj *Tri_inj_;         // Class to create triangle waves
  CosInj *Cos_inj_;         // Class to create cosine waves
  uint32_t duty_;           // Used in Test Mode to inject Fake shunt current (0 - uint32_t(255))
  float ib_charge_;        // Current input avaiable for charging, A
  float ib_fut_;           // Future value of limited current, A
  float ib_in_;            // Saved value of current input, A
  float ib_sat_;           // Threshold to declare saturation.  This regeneratively slows down charging so if too small takes too long, A
  boolean model_cutback_;   // Indicate that modeled current being limited on saturation cutback, T = cutback limited
  boolean model_saturated_; // Indicator of maximal cutback, T = cutback saturated
  double q_;                // Charge, C
  unsigned long int sample_time_;       // Exact moment of hardware signal generation, ms
  unsigned long int sample_time_z_;     // Exact moment of past hardware signal generation, ms
  float sat_cutback_gain_; // Gain to retard ib when voc exceeds vsat, dimensionless
  float sat_ib_max_;       // Current cutback to be applied to modeled ib output, A
  float sat_ib_null_;      // Current cutback value for voc=vsat, A
};


// Methods

#endif