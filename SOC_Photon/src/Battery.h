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

#include "myLibrary/myTables.h"
#include "myLibrary/EKF_1x1.h"
#include "myLibrary/StateSpace.h"
#include "Coulombs.h"
#include "myLibrary/injection.h"
#include "myLibrary/myFilters.h"
#include "constants.h"
class Sensors;
#define t_float float

#define RATED_TEMP      25.       // Temperature at RATED_BATT_CAP, deg C
#define TCHARGE_DISPLAY_DEADBAND  0.1 // Inside this +/- deadband, charge time is displayed '---', A
#define T_RLIM          0.017     // Temperature sensor rate limit to minimize jumps in Coulomb counting, deg C/s (0.017 allows 1 deg for 1 minute)
const float vb_dc_dc = 13.5;      // DC-DC charger estimated voltage, V (13.5 < v_sat = 13.85)
#define EKF_CONV        1.5e-3    // EKF tracking error indicating convergence, V (1.5e-3)
#define EKF_T_CONV      30.       // EKF set convergence test time, sec (30.)
const float EKF_T_RESET = (EKF_T_CONV/2.); // EKF reset retest time, sec ('up 1, down 2')
#define EKF_Q_SD_NORM   0.00005   // Standard deviation of normal EKF process uncertainty, V (0.00005)
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
#define SOLV_MAX_COUNTS 10        // EKF initialization solver max iters (10)
#define SOLV_MAX_STEP   0.2       // EKF initialization solver max step size of soc, fraction (0.2)
#define RANDLES_T_MAX   0.31      // Maximum update time of Randles state space model to avoid aliasing and instability (0.31 allows DP3)
const double MXEPS = 1-1e-6;      // Level of soc that indicates mathematically saturated (threshold is lower for robustness) (1-1e-6)
#define HYS_SCALE       1.0       // Scalar on hysteresis (1.0)
#define HYS_SOC_MIN_MARG 0.15     // Add to soc_min to set thr for detecting low endpoint condition for reset of hysteresis (0.15)
#define HYS_IB_THR      1.0       // Ignore reset if opposite situation exists, A (1.0)
#define HYS_DV_MIN      0.2       // Minimum value of hysteresis reset, V (0.2)

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
// Hysteresis: reservoir model of battery electrical hysteresis
// Use variable resistor and capacitor to create hysteresis from an RC circuit
class Hysteresis
{
public:
  Hysteresis();
  Hysteresis(const double cap, Chemistry chem, float *rp_hys_scale);
  ~Hysteresis();
  // operators
  // functions
  void apply_scale(const float sclr) { *rp_hys_scale_ = max(sclr, 0.); };
  double calculate(const double ib, const double soc);
  float dv_max(const float soc) { return hys_Tx_->interp(soc); };
  float dv_min(const float soc) { return hys_Tn_->interp(soc); };
  void init(const double dv_init);
  double look_hys(const double dv, const double soc);
  void pretty_print();
  double update(const double dt, const boolean init_high, const boolean init_low, const float e_wrap);
  double ioc() { return ioc_; };
  double dv_hys() { return dv_hys_; };
  void dv_hys(const float st) { dv_hys_ = max(min(st, dv_max(soc_)), dv_min(soc_)); };
  float scale() { return *rp_hys_scale_; };
protected:
  boolean disabled_;    // Hysteresis disabled by low scale input < 1e-5, T=disabled
  double cap_;          // Capacitance, Farads
  double res_;          // Variable resistance value, ohms
  double soc_;          // State of charge input, dimensionless
  double ib_;           // Current in, A
  double ioc_;          // Current out, A
  double dv_hys_;       // State, voc_-voc_stat_, V
  double dv_dot_;       // Calculated voltage rate, V/s
  double tau_;          // Null time constant, sec
  TableInterp2D *hys_T_;// dv-soc 2-D table, V
  TableInterp1D *hys_Tx_;// soc 1-D table, V_max
  TableInterp1D *hys_Tn_;// soc 1-D table, V_min
  float *rp_hys_scale_; // Scalar on output of update
};


// Battery Class
class Battery : public Coulombs
{
public:
  Battery();
  Battery(double *rp_delta_q, float *rp_t_last, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code, float *rp_hys_scale);
  ~Battery();
  // operators
  // functions
  virtual void assign_randles(void) { Serial.printf("ERROR:  Battery::assign_randles called\n"); };
  boolean bms_off() { return bms_off_; };
  double calc_soc_voc(const double soc, const float temp_c, double *dv_dsoc);
  double calc_soc_voc_slope(double soc, float temp_c);
  double calc_vsat(void);
  virtual double calculate(const float temp_C, const double soc_frac, double curr_in, const double dt, const boolean dc_dc_on);
  String decode(const uint8_t mod);
  void ds_voc_soc(const float _ds) { ds_voc_soc_ = _ds; };
  float ds_voc_soc() { return ds_voc_soc_; };
  void Dv(const double _dv) { chem_.dvoc = _dv; };
  double Dv() { return chem_.dvoc; };
  double dv_dsoc() { return dv_dsoc_; };
  double dv_dyn() { return dv_dyn_; };
  double dV_dyn() { return dv_dyn_*(*rp_nS_); };
  void dv_voc_soc(const float _dv) { dv_voc_soc_ = _dv; };
  float dv_voc_soc() { return dv_voc_soc_; };
  uint8_t encode(const String mod_str);
  void hys_pretty_print () { hys_->pretty_print(); };
  double hys_scale() { return hys_->scale(); };
  void hys_scale(const double scale) { hys_->apply_scale(scale); };
  double hys_state() { return hys_->dv_hys(); };
  void hys_state(const double st) { hys_->dv_hys(st); };
  void init_battery(const boolean reset, Sensors *Sen);
  void init_hys(const double hys) { hys_->init(hys); };
  double ib() { return ib_; };            // Battery terminal current, A
  double Ib() { return ib_*(*rp_nP_); };  // Battery bank current, A
  double ioc() { return ioc_; };
  double nP() { return *rp_nP_; };
  void nP(const double np) { *rp_nP_ = np; };
  double nS() { return *rp_nS_; };
  void nS(const double ns) { *rp_nS_ = ns; };
  virtual void pretty_print();
  void pretty_print_ss();
  void print_signal(const boolean print) { print_now_ = print; };
  void Sr(const double sr) { sr_ = sr; Randles_->insert_D(0, 0, -chem_.r_0*sr_); };
  double Sr() { return sr_; };
  float temp_c() { return temp_c_; };    // Battery temperature, deg C
  double Tb() { return temp_c_; };        // Battery bank temperature, deg C
  double vb() { return vb_; };            // Battery terminal voltage, V
  double Vb() { return vb_*(*rp_nS_); };  // Battery bank voltage, V
  double voc() { return voc_; };
  double Voc() { return voc_*(*rp_nS_); };
  double voc_stat() { return voc_stat_; };
  double Voc_stat() { return voc_stat_*(*rp_nS_); };
  double voc_soc_tab(const double soc, const float temp_c);
  double vsat() { return vsat_; };
  double Vsat() { return vsat_*(*rp_nS_); };
protected:
  double voc_;      // Static model open circuit voltage, V
  double dv_dyn_;   // ib-induced back emf, V
  double dv_hys_;   // Hysteresis state, voc-voc_out, V
  double vb_;       // Battery terminal voltage, V
  double ib_;       // Battery terminal current, A
  double dv_dsoc_;  // Derivative scaled, V/fraction
  double sr_;       // Resistance scalar
  double nom_vsat_; // Nominal saturation threshold at 25C, V
  double vsat_;     // Saturation threshold at temperature, V
  double dt_;       // Update time, s
  // EKF declarations
  StateSpace *Randles_; // Randles model {ib, vb} --> {voc}, ioc=ib for Battery version
                        // Randles model {ib, voc} --> {vb}, ioc=ib for BatterySim version
  double *rand_A_;  // Randles model A
  double *rand_B_;  // Randles model B
  double *rand_C_;  // Randles model C
  double *rand_D_;  // Randles model D
  float temp_c_;    // Battery temperature, deg C
  boolean bms_off_; // Indicator that battery management system is off, T = off preventing current flow
  Hysteresis *hys_;
  double ioc_;      // Current into charge portion of battery, A
  double voc_stat_; // Static, table lookup value of voc before applying hysteresis, V
  float *rp_nP_;    // Number of parallel batteries in bank, e.g. '2P1S'
  float *rp_nS_;    // Number of series batteries in bank, e.g. '2P1S'
  boolean print_now_; // Print command
  float ds_voc_soc_;    // VOC(SOC) delta soc on input
  float dv_voc_soc_;    // VOC(SOC) delta voc on output
};


// BatteryMonitor: extend Battery to use as monitor object
class BatteryMonitor: public Battery, public EKF_1x1
{
public:
  BatteryMonitor();
  BatteryMonitor(double *rp_delta_q, float *rp_t_last, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code, float *rp_hys_scale);
  ~BatteryMonitor();
  // operators
  // functions
  double amp_hrs_remaining_ekf() { return amp_hrs_remaining_ekf_; };
  double amp_hrs_remaining_soc() { return amp_hrs_remaining_soc_; };
  double Amp_hrs_remaining_ekf() { return amp_hrs_remaining_ekf_*(*rp_nP_)*(*rp_nS_); };
  double Amp_hrs_remaining_soc() { return amp_hrs_remaining_soc_*(*rp_nP_)*(*rp_nS_); };
  virtual void assign_randles(void);
  double calc_charge_time(const double q, const double q_capacity, const double charge_curr, const double soc);
  double calculate(Sensors *Sen, const boolean reset);
  boolean converged_ekf() { return EKF_converged->state(); };
  double delta_q_ekf() { return delta_q_ekf_; };
  double hx() { return hx_; };
  double Hx() { return hx_*(*rp_nS_); };
  void init_soc_ekf(const double soc);
  boolean is_sat(const boolean reset);
  double K_ekf() { return K_; };
  void pretty_print(Sensors *Sen);
  void regauge(const float temp_c);
  float r_sd () { return chem_.r_sd; };
  float r_ss () { return chem_.r_ss; };
  double soc_ekf() { return soc_ekf_; };
  boolean solve_ekf(const boolean reset, Sensors *Sen);
  double tcharge() { return tcharge_; };
  double dv_dyn() { return dv_dyn_; };
  double dV_dyn() { return dv_dyn_*(*rp_nS_); };
  double voc_filt() { return voc_filt_; };
  double Voc_filt() { return voc_filt_*(*rp_nS_); };
  double voc_soc() { return voc_soc_; };
  double Voc_tab() { return voc_soc_*(*rp_nS_); };
  double y_ekf() { return y_; };
  double y_ekf_filt() { return y_filt_; };
  double delta_q_ekf_;         // Charge deficit represented by charge calculated by ekf, C
protected:
  double tcharge_ekf_;  // Solved charging time to 100% from ekf, hr
  double soc_ekf_;      // Filtered state of charge from ekf (0-1)
  double tcharge_;      // Counted charging time to 100%, hr
  double q_ekf_;        // Filtered charge calculated by ekf, C
  double voc_filt_;     // Filtered, static model open circuit voltage, V
  double soc_;          // Weighted selection of ekf state of charge and coulomb counter (0-1)
  double amp_hrs_remaining_ekf_;  // Discharge amp*time left if drain to q_ekf=0, A-h
  double amp_hrs_remaining_soc_;  // Discharge amp*time left if drain soc_ to 0, A-h
  double y_filt_;       // Filtered EKF y value, V
  General2_Pole *y_filt = new General2_Pole(0.1, WN_Y_FILT, ZETA_Y_FILT, MIN_Y_FILT, MAX_Y_FILT);  // actual update time provided run time
  SlidingDeadband *SdVb_;  // Sliding deadband filter for Vb
  TFDelay *EKF_converged;     // Time persistence
  void ekf_predict(double *Fx, double *Bu);
  void ekf_update(double *hx, double *H);
  RateLimit *T_RLim = new RateLimit();
  float voc_soc_;  // Raw table lookup of voc, V
};


// BatterySim: extend Battery to use as model object
class BatterySim: public Battery
{
public:
  BatterySim();
  BatterySim(double *rp_delta_q, float *rp_t_last, float *rp_s_cap_model, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code, float *rp_hys_scale);
  ~BatterySim();
  // operators
  // functions
  virtual void assign_randles(void);
  double calculate(Sensors *Sen, const boolean dc_dc_on, const boolean reset);
  float calc_inj(const unsigned long now, const uint8_t type, const double amp, const double freq);
  double count_coulombs(Sensors *Sen, const boolean reset, BatteryMonitor *Mon);
  double delta_q() { return *rp_delta_q_; };
  float t_last() { return *rp_t_last_; };
  void load();
  void pretty_print(void);
  boolean cutback() { return model_cutback_; };
  boolean saturated() { return model_saturated_; };
  double voc() { return voc_; };
  double voc_stat() { return voc_stat_; };
protected:
  double q_;                // Charge, C
  SinInj *Sin_inj_;         // Class to create sine waves
  SqInj *Sq_inj_;           // Class to create square waves
  TriInj *Tri_inj_;         // Class to create triangle waves
  CosInj *Cos_inj_;         // Class to create sosine waves
  uint32_t duty_;           // Used in Test Mode to inject Fake shunt current (0 - uint32_t(255))
  double sat_ib_max_;       // Current cutback to be applied to modeled ib output, A
  double sat_ib_null_;      // Current cutback value for voc=vsat, A
  double sat_cutback_gain_; // Gain to retard ib when voc exceeds vsat, dimensionless
  boolean model_cutback_;   // Indicate that modeled current being limited on saturation cutback, T = cutback limited
  boolean model_saturated_; // Indicator of maximal cutback, T = cutback saturated
  double ib_sat_;           // Threshold to declare saturation.  This regeneratively slows down charging so if too small takes too long, A
  double *rp_delta_q_model_;// Charge change since saturated, C
  float *rp_t_last_model_;  // Battery temperature past value for rate limit memory, deg C
  float *rp_s_cap_model_;   // Rated capacity scalar
  double ib_fut_;           // Future value of limited current, A
  double ib_in_;            // Saved value of current input, A
};


// Methods

#endif