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
struct Sensors;
#define t_float float

#define TCHARGE_DISPLAY_DEADBAND  0.1 // Inside this +/- deadband, charge time is displayed '---', A
#define T_RLIM          0.017     // Temperature sensor rate limit to minimize jumps in Coulomb counting, deg C/s (0.017 allows 1 deg for 1 minute)
const double vb_dc_dc = 13.5;     // DC-DC charger estimated voltage, V
#define EKF_CONV        1.5e-3    // EKF tracking error indicating convergence, V (1.5e-3)
#define EKF_T_CONV      30.       // EKF set convergence test time, sec (30.)
#define EKF_T_RESET (EKF_T_CONV/2.) // EKF reset test time, sec ('up 1, down 2')
#define EKF_Q_SD        0.001     // Standard deviation of EKF process uncertainty, V
#define EKF_R_SD        0.1       // Standard deviation of EKF state uncertainty, fraction (0-1)
#define EKF_NOM_DT      0.1       // EKF nominal update time, s (initialization; actual value varies) 
#define DF2             0.70      // Threshold to resest Coulomb Counter if different from ekf, fraction (0.20)
#define DF1             0.02      // Weighted selection lower transition drift, fraction (0.02)
#define TAU_Y_FILT      5.        // EKF y-filter time constant, sec (5.)
#define MIN_Y_FILT      -0.5      // EKF y-filter minimum, V (-0.5)
#define MAX_Y_FILT      0.5       // EKF y-filter maximum, V (0.5)
#define WN_Y_FILT       0.1       // EKF y-filter-2 natural frequency, r/s (0.1)
#define ZETA_Y_FILT     0.9       // EKF y-fiter-2 damping factor (0.9)
#define TMAX_FILT       3.        // Maximum y-filter-2 sample time, s (3.)
#define SOLV_ERR        1e-6      // EKF initialization solver error bound, V
#define SOLV_MAX_COUNTS 10        // EKF initialization solver max iters
#define SOLV_MAX_STEP   0.2       // EKF initialization solver max step size of soc, fraction
#define RANDLES_T_MAX   0.5       // Maximum update time of Randles state space model to avoid aliasing and instability
const double mxeps = 1-1e-6;      // Level of soc that indicates mathematically saturated (threshold is lower for robustness)


// BattleBorn 100 Ah, 12v LiFePO4
// See VOC_SOC data.xls.    T=40 values are only a notion.   Need data for it.
// >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
const uint8_t m_t_bb  = 4;    // Number temperature breakpoints for voc table
const uint8_t n_s_bb  = 16;   // Number soc breakpoints for voc table
const float y_t_bb[m_t_bb] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 40. }; 
const float x_soc_bb[n_s_bb] =      //soc breakpoints for voc table
        { 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.98,  1.00};
const float t_voc_bb[m_t_bb*n_s_bb] = // r(soc, dv) table
        { 4.00,  4.00,  6.00,  9.50,  11.70, 12.30, 12.50, 12.65, 12.82, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 14.95,
          4.00,  4.00,  8.00,  11.60, 12.40, 12.60, 12.70, 12.80, 12.92, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 14.96,
          4.00,  8.00,  12.20, 12.68, 12.73, 12.79, 12.81, 12.89, 13.00, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 15.00,
          4.08,  8.08,  12.28, 12.76, 12.81, 12.87, 12.89, 12.97, 13.08, 13.12, 13.17, 13.22, 13.29, 13.33, 13.35, 15.08};
const uint8_t n_n_bb = 4;   // Number of temperature breakpoints for x_soc_min table
const float x_soc_min_bb[n_n_bb] =  { 5.,   11.1,  20.,  40.};  // Temperature breakpoints for soc_min table
const float t_soc_min_bb[n_n_bb] =  { 0.14, 0.12,  0.08, 0.07}; // soc_min(t)
// Hysteresis
const uint8_t m_h_bb  = 3;          // Number of soc breakpoints in r(soc, dv) table t_r
const uint8_t n_h_bb  = 9;          // Number of dv breakpoints in r(dv) table t_r
const float x_dv_bb[n_h_bb] =       // dv breakpoints for r(soc, dv) table t_r
        { -0.09,-0.07, -0.05,   -0.03,  0.00,   0.03,   0.05,   0.07,   0.09};
const float y_soc_bb[m_h_bb] =      // soc breakpoints for r(soc, dv) table t_r
        { 0.0,  0.5,    1.0};
const float t_r_bb[m_h_bb*n_h_bb] = // r(soc, dv) table
        { 1e-7, 0.0064, 0.0050, 0.0036, 0.0015, 0.0024, 0.0030, 0.0046, 1e-7,
          1e-7, 1e-7,   0.0050, 0.0036, 0.0015, 0.0024, 0.0030,   1e-7, 1e-7,
          1e-7, 1e-7,   1e-7,   0.0036, 0.0015, 0.0024, 1e-7,     1e-7, 1e-7};


// LION 100 Ah, 12v LiFePO4
// shifted Battleborn because don't have real data yet; test structure of program
const uint8_t m_t_li  = 4;    // Number temperature breakpoints for voc table
const uint8_t n_s_li  = 16;   // Number soc breakpoints for voc table
const float y_t_li[m_t_li] =  //Temperature breakpoints for voc table
        { 5., 11.1, 20., 40. }; 
const float x_soc_li[n_s_li] =      //soc breakpoints for voc table
        { 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.98,  1.00};
const float t_voc_li[m_t_li*n_s_li] = // r(soc, dv) table
        { 3.00,  3.00,  5.00,  8.50,  10.70, 11.30, 11.50, 11.65, 11.82, 11.91, 11.98, 12.05, 12.11, 12.17, 12.22, 13.95,
          3.00,  3.00,  7.00,  10.60, 11.40, 11.60, 11.70, 11.80, 11.92, 12.01, 12.06, 12.11, 12.17, 12.20, 12.23, 13.96,
          3.00,  7.00,  11.20, 11.68, 11.73, 11.79, 11.81, 11.89, 12.00, 12.04, 12.09, 12.14, 12.21, 12.25, 12.27, 14.00,
          3.08,  7.08,  11.28, 11.76, 11.81, 11.87, 11.89, 11.97, 12.08, 12.12, 12.17, 12.22, 12.29, 12.33, 12.35, 14.08};
const uint8_t n_n_li = 4;   // Number of temperature breakpoints for x_soc_min table
const float x_soc_min_li[n_n_li] =  { 5.,   11.1,  20.,  40.};  // Temperature breakpoints for soc_min table
const float t_soc_min_li[n_n_li] =  { 0.28, 0.24,  0.16, 0.14}; // soc_min(t)
// Hysteresis
const uint8_t m_h_li  = 3;          // Number of soc breakpoints in r(soc, dv) table t_r
const uint8_t n_h_li  = 9;          // Number of dv breakpoints in r(dv) table t_r
const float x_dv_li[n_h_li] =       // dv breakpoints for r(soc, dv) table t_r
        { -0.09,-0.07, -0.05,   -0.03,  0.00,   0.03,   0.05,   0.07,   0.09};
const float y_soc_li[m_h_li] =      // soc breakpoints for r(soc, dv) table t_r
        { 0.0,  0.5,    1.0};
const float t_r_li[m_h_li*n_h_li] = // r(soc, dv) table
        { 1e-7, 0.0064, 0.0050, 0.0036, 0.0015, 0.0024, 0.0030, 0.0046, 1e-7,
          1e-7, 1e-7,   0.0050, 0.0036, 0.0015, 0.0024, 0.0030,   1e-7, 1e-7,
          1e-7, 1e-7,   1e-7,   0.0036, 0.0015, 0.0024, 1e-7,     1e-7, 1e-7};

// Hysteresis: reservoir model of battery electrical hysteresis
// Use variable resistor and capacitor to create hysteresis from an RC circuit
class Hysteresis
{
public:
  Hysteresis();
  Hysteresis(const double cap, const double direx, Chemistry chem);
  ~Hysteresis();
  // operators
  // functions
  void apply_scale(const double scale);
  double calculate(const double ib, const double voc_stat, const double soc);
  void init(const double dv_init);
  double look_hys(const double dv, const double soc);
  void pretty_print();
  double update(const double dt);
  double ioc() { return (ioc_); };
  double dv_hys() { return (dv_hys_); };
  double scale();
  double direx() { return (direx_); };
protected:
  boolean disabled_;    // Hysteresis disabled by low scale input < 1e-5, T=disabled
  double cap_;          // Capacitance, Farads
  double res_;          // Variable resistance value, ohms
  double soc_;          // State of charge input, dimensionless
  double ib_;           // Current in, A
  double ioc_;          // Current out, A
  double voc_in_;       // Voltage input, V
  double voc_out_;      // Voltage output, V
  double dv_hys_;       // Delta voltage state, V
  double dv_dot_;       // Calculated voltage rate, V/s
  double tau_;          // Null time constant, sec
  double direx_;        // Direction scalar
  TableInterp2D *hys_T_;   // dv-soc 2-D table, V
  double cap_init_;     // Initial capacitance specification, Farads
};


// Battery Class
class Battery : public Coulombs
{
public:
  Battery();
  Battery(double *rp_delta_q, float *rp_t_last, const double hys_direx, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code);
  ~Battery();
  // operators
  // functions
  virtual void assign_rand(void) { Serial.printf("ERROR:  Battery::assign_rand called\n"); };
  double calc_soc_voc(const double soc, const double temp_c, double *dv_dsoc);
  double calc_soc_voc_slope(double soc, double temp_c);
  double calc_vsat(void);
  virtual double calculate(const double temp_C, const double soc_frac, double curr_in, const double dt, const boolean dc_dc_on);
  String decode(const uint8_t mod);
  void Dv(const double dv) { dv_ = dv; };
  double dv_dsoc() { return (dv_dsoc_); };
  double Dv() { return (dv_); };
  uint8_t encode(const String mod_str);
  double hys_scale() { return (hys_->scale()*hys_->direx()); };
  void hys_scale(const double scale) { hys_->apply_scale(scale); };
  void init_battery(Sensors *Sen);
  void init_hys(const double hys) { hys_->init(hys); };
  double ib() { return (ib_); };            // Battery terminal current, A
  double Ib() { return (ib_*(*rp_nP_)); };  // Battery bank current, A
  double ioc() { return (ioc_); };
  double nP() { return (*rp_nP_); };
  void nP(const double np) { *rp_nP_ = np; };
  double nS() { return (*rp_nS_); };
  void nS(const double ns) { *rp_nS_ = ns; };
  virtual void pretty_print();
  void pretty_print_ss();
  double voc() { return (voc_); };
  double Voc() { return (voc_*(*rp_nS_)); };
  double voc_soc(const double soc, const double temp_c);
  void Sr(const double sr) { sr_ = sr; Randles_->insert_D(0, 0, -chem_.r_0*sr_); };
  double Sr() { return (sr_); };
  double temp_c() { return (temp_c_); };    // Battery temperature, deg C
  double Tb() { return (temp_c_); };        // Battery bank temperature, deg C
  double vb() { return (vb_); };            // Battery terminal voltage, V
  double Vb() { return (vb_*(*rp_nS_)); };  // Battery bank voltage, V
  double vdyn() { return (vdyn_); };
  double Vdyn() { return (vdyn_*(*rp_nS_)); };
  double vsat() { return (vsat_); };
  double Vsat() { return (vsat_*(*rp_nS_)); };
protected:
  double voc_;      // Static model open circuit voltage, V
  double vdyn_;     // Current-induced back emf, V
  double vb_;       // Battery terminal voltage, V
  double ib_;       // Battery terminal current, A
  double dv_dsoc_;  // Derivative scaled, V/fraction
  double sr_;       // Resistance scalar
  double nom_vsat_; // Nominal saturation threshold at 25C, V
  double vsat_;     // Saturation threshold at temperature, V
  double dv_;       // Table hard-coded adjustment, compensates for data collection errors (hysteresis), V
  double dt_;       // Update time, s
  // EKF declarations
  StateSpace *Randles_; // Randles model {ib, vb} --> {voc}, ioc=ib for Battery version
                        // Randles model {ib, voc} --> {vb}, ioc=ib for BatteryModel version
  double *rand_A_;  // Randles model A
  double *rand_B_;  // Randles model B
  double *rand_C_;  // Randles model C
  double *rand_D_;  // Randles model D
  double temp_c_;   // Battery temperature, deg C
  boolean bms_off_; // Indicator that battery management system is off, T = off preventing current flow
  TableInterp2D *voc_T_;   // SOC-VOC 2-D table, V
  Hysteresis *hys_;
  double ioc_;      // Current into charge portion of battery, A
  double voc_stat_; // Static, table lookup value of voc before applying hysteresis, V
  float *rp_nP_;    // Number of parallel batteries in bank, e.g. '2P1S'
  float *rp_nS_;    // Number of series batteries in bank, e.g. '2P1S'
};


// BatteryMonitor: extend Battery to use as monitor object
class BatteryMonitor: public Battery, public EKF_1x1
{
public:
  BatteryMonitor();
  BatteryMonitor(double *rp_delta_q, float *rp_t_last, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code);
  ~BatteryMonitor();
  // operators
  // functions
  double amp_hrs_remaining_ekf() { return (amp_hrs_remaining_ekf_); };
  double amp_hrs_remaining_wt() { return (amp_hrs_remaining_wt_); };
  double Amp_hrs_remaining_ekf() { return (amp_hrs_remaining_ekf_*(*rp_nP_)*(*rp_nS_)); };
  double Amp_hrs_remaining_wt() { return (amp_hrs_remaining_wt_*(*rp_nP_)*(*rp_nS_)); };
  virtual void assign_rand(void);
  double calc_charge_time(const double q, const double q_capacity, const double charge_curr, const double soc);
  double calculate(Sensors *Sen);
  boolean converged_ekf() { return(EKF_converged->state()); };
  double hx() { return (hx_); };
  double Hx() { return (hx_*(*rp_nS_)); };
  void init_soc_ekf(const double soc);
  boolean is_sat(void);
  double K_ekf() { return (K_); };
  void pretty_print(void);
  void regauge(const double temp_c);
  void select();
  double soc_ekf() { return (soc_ekf_); };
  double soc_wt() { return soc_wt_; };
  boolean solve_ekf(Sensors *Sen);
  double tcharge() { return (tcharge_); };
  double voc() { return (voc_); };
  double voc_dyn() { return (voc_dyn_); };
  double Voc_dyn() { return (voc_dyn_*(*rp_nS_)); };
  double voc_filt() { return (voc_filt_); };
  double Voc_filt() { return (voc_filt_*(*rp_nS_)); };
  double voc_stat() { return (voc_stat_); };
  double Voc_stat() { return (voc_stat_*(*rp_nS_)); };
  double y_ekf() { return (y_); };
  double y_ekf_filt() { return (y_filt_); };
protected:
  double voc_stat_;     // Sim voc from soc-voc table, V
  double tcharge_ekf_;  // Solved charging time to 100% from ekf, hr
  double voc_dyn_;      // Charging voltage, V
  double soc_ekf_;      // Filtered state of charge from ekf (0-1)
  double tcharge_;      // Counted charging time to 100%, hr
  double q_ekf_;        // Filtered charge calculated by ekf, C
  double voc_filt_;     // Filtered, static model open circuit voltage, V
  double soc_wt_;       // Weighted selection of ekf state of charge and coulomb counter (0-1)
  double amp_hrs_remaining_ekf_;  // Discharge amp*time left if drain to q_ekf=0, A-h
  double amp_hrs_remaining_wt_;   // Discharge amp*time left if drain soc_wt_ to 0, A-h
  double y_filt_;       // Filtered EKF y value, V
  // LagTustin *y_filt = new LagTustin(0.1, TAU_Y_FILT, MIN_Y_FILT, MAX_Y_FILT);  // actual update time provided run time
  General2_Pole *y_filt = new General2_Pole(0.1, WN_Y_FILT, ZETA_Y_FILT, MIN_Y_FILT, MAX_Y_FILT);  // actual update time provided run time
  SlidingDeadband *SdVbatt_;  // Sliding deadband filter for Vbatt
  TFDelay *EKF_converged = new TFDelay();   // Time persistence
  void ekf_predict(double *Fx, double *Bu);
  void ekf_update(double *hx, double *H);
};


// BatteryModel: extend Battery to use as model object
class BatteryModel: public Battery
{
public:
  BatteryModel();
  BatteryModel(double *rp_delta_q, float *rp_t_last, float *rp_s_cap_model, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code);
  ~BatteryModel();
  // operators
  // functions
  virtual void assign_rand(void);
  double calculate(Sensors *Sen, const boolean dc_dc_on);
  uint32_t calc_inj_duty(const unsigned long now, const uint8_t type, const double amp, const double freq);
  double count_coulombs(Sensors *Sen, const boolean reset, const double t_last);
  void load();
  void pretty_print(void);
  boolean cutback() { return model_cutback_; };
  boolean saturated() { return model_saturated_; };
  double voc() { return (voc_); };
  double voc_stat() { return (voc_stat_); };
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
  double s_cap_;            // Rated capacity scalar
  double *rp_delta_q_model_;// Charge change since saturated, C
  float *rp_t_last_model_;  // Battery temperature past value for rate limit memory, deg C
  float *rp_s_cap_model_;   // Rated capacity scalar
};


// Methods

#endif