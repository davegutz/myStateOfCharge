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

// BattleBorn 100 Ah, 12v LiFePO4
#define RATED_BATT_CAP        100.      // Nominal battery bank capacity, Ah (100)
#define RATED_TEMP            25.       // Temperature at RATED_BATT_CAP, deg C
// >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
#define BATT_V_SAT            13.85     // Normal battery cell saturation for SOC=99.7, V (13.85)
#define DQDT                  0.01      // Change of charge with temperature, fraction/deg C (0.01 from literature)
#define TCHARGE_DISPLAY_DEADBAND 0.1    // Inside this +/- deadband, charge time is displayed '---', A
#define Q_CAP_RATED           (RATED_BATT_CAP*3600)   // Rated capacity at t_rated_, saved for future scaling, C
#define T_RLIM                0.017    // Temperature sensor rate limit to minimize jumps in Coulomb counting, deg C/s (0.017 allows 1 deg for 1 minute)
const double sat_cutback_gain = 10; // Multiplier on saturation anti-windup
const double vb_dc_dc = 13.5;   // DC-DC charger estimated voltage, V

// Latest table from data
// See VOC_SOC data.xls.    T=40 values are only a notion.   Need data for it.
const double low_voc = 10.; // Voltage threshold for BMS to turn off battery
const double low_t = 0.;    // Minimum temperature for valid saturation check, because BMS shuts off battery low.
                            // Heater should keep >4, too
const unsigned int m_t = 4;
const double y_t[m_t] =  { 5., 11.1, 20., 40. };
const unsigned int n_s = 16;
const double x_soc[n_s] =     { 0.00,  0.05,  0.10,  0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.98,  1.00};
const double t_voc[m_t*n_s] = { 4.00,  4.00,  6.00,  9.50,  11.70, 12.30, 12.50, 12.65, 12.82, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 14.95,
                                4.00,  4.00,  8.00,  11.60, 12.40, 12.60, 12.70, 12.80, 12.92, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 14.96,
                                4.00,  8.00,  12.20, 12.68, 12.73, 12.79, 12.81, 12.89, 13.00, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 15.00,
                                4.08,  8.08,  12.28, 12.76, 12.81, 12.87, 12.89, 12.97, 13.08, 13.12, 13.17, 13.22, 13.29, 13.33, 13.35, 15.08};
const unsigned int n_n = 4;
const double x_soc_min[n_n] = { 5.,   11.1,  20.,  40. };
const double t_soc_min[n_n] = { 0.14, 0.12,  0.08, 0.07};
const double mxeps_bb = 1-1e-6;      // Level of soc that indicates mathematically saturated (threshold is lower for robustness)

// Hysteresis constants
const double hys_cap = 3.6e6;
const unsigned int n_h = 9;
const unsigned int m_h = 3;
const double x_dv[n_h]    = { -0.09, -0.07, -0.05, -0.03, 0.00, 0.03, 0.05, 0.07, 0.09};
const double y_soc[m_h]   = { 0.0,   0.5,    1.0};
const double t_r[m_h*n_h] = { 1e-7, 0.0064, 0.0050, 0.0036, 0.0015, 0.0024, 0.0030, 0.0046, 1e-7,
                              1e-7, 1e-7,   0.0050, 0.0036, 0.0015, 0.0024, 0.0030,   1e-7, 1e-7,
                              1e-7, 1e-7,     1e-7, 0.0036, 0.0015, 0.0024, 1e-7,     1e-7, 1e-7};

#define EKF_CONV        1e-3      // EKF tracking error indicating convergence, V (1e-3)
#define EKF_T_CONV      30.       // EKF set convergence test time, sec (30.)
#define EKF_T_RESET (EKF_T_CONV/2.) // EKF reset test time, sec ('up 1, down 2')
#define EKF_Q_SD        0.001     // Standard deviation of EKF process uncertainty, V
#define EKF_R_SD        0.1       // Standard deviation of EKF state uncertainty, fraction (0-1)
#define EKF_NOM_DT      0.1       // EKF nominal update time, s (initialization; actual value varies)
#define DF2             0.05      // Threshold to resest Coulomb Counter if different from ekf, fraction (0.05)
#define DF1             0.02      // Weighted selection lower transition drift, fraction
#define TAU_Y_FILT      5.        // EKF y-filter time constant, sec (5.)
#define MIN_Y_FILT      -0.5      // EKF y-filter minimum, V (-0.5)
#define MAX_Y_FILT      0.5       // EKF y-filter maximum, V (0.5)
#define BATT_DV         0.01      // Adjustment to compensate for tables generated without considering hys, V
#define BATT_R_0        0.003     // Randles R0, ohms
#define BATT_R_CT       0.0016    // Randles charge transfer resistance, ohms
#define BATT_R_DIFF     0.0077    // Randles diffusion resistance, ohms
#define BATT_TAU_CT     0.2       // Randles charge transfer time constant, s (=1/Rct/Cct)
#define BATT_TAU_DIFF   83.       // Randles diffusion time constant, s (=1/Rdif/Cdif)
#define BATT_TAU_SD     1.8e7     // Equivalent model for EKF.  Creating an anchor.   So large it's just a pass through, sec
#define BATT_R_SD       70.       // Equivalent model for EKF reference.  Parasitic equivalent, ohms
#define SOLV_ERR        1e-6      // EKF initialization solver error bound, V
#define SOLV_MAX_COUNTS 10        // EKF initialization solver max iters
#define SOLV_MAX_STEP   0.2       // EKF initialization solver max step size of soc, fraction
#define BATT_DVOC_DT    0.004     // Change of VOC with operating temperature in range 0 - 50 C (0.004 for Vb on 2022-02-18) V/deg C
const double batt_r_ss = BATT_R_0 + BATT_R_CT + BATT_R_DIFF; // Steady state equivalent battery resistance, for solver, Ohms

// Hysteresis: reservoir model of battery electrical hysteresis
// Use variable resistor and capacitor to create hysteresis from an RC circuit
class Hysteresis
{
public:
  Hysteresis();
  Hysteresis(const double cap, const double direx);
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
  Battery(double *rp_delta_q, double *rp_t_last, const double hys_direx);
  ~Battery();
  // operators
  // functions
  double calc_soc_voc(const double soc, const double temp_c, double *dv_dsoc);
  double calc_soc_voc_slope(double soc, double temp_c);
  double calc_vsat(void);
  virtual double calculate(const double temp_C, const double soc_frac, double curr_in, const double dt, const boolean dc_dc_on);
  void Dv(const double dv) { dv_ = dv; };
  double dv_dsoc() { return (dv_dsoc_); };
  double Dv() { return (dv_); };
  double hys_scale() { return (hys_->scale()*hys_->direx()); };
  void hys_scale(const double scale) { hys_->apply_scale(scale); };
  void init_battery(void);
  void init_hys(const double hys) { hys_->init(hys); };
  double ib() { return (ib_); };
  double ioc() { return (ioc_); };
  virtual void pretty_print();
  void pretty_print_ss();
  double voc() { return (voc_); };
  double voc_soc(const double soc, const double temp_c);
  void Sr(const double sr) { sr_ = sr; Randles_->insert_D(0, 0, -r0_*sr_); };
  double Sr() { return (sr_); };
  double temp_c() { return (temp_c_); };
  double vb() { return (vb_); };
  double vdyn() { return (vdyn_); };
  double vsat() { return (vsat_); };
protected:
  double voc_;      // Static model open circuit voltage, V
  double vdyn_;     // Sim current induced back emf, V
  double vb_;       // Battery terminal voltage, V
  double ib_;       // Battery terminal current, A
  double dv_dsoc_;  // Derivative scaled, V/fraction
  double sr_;       // Resistance scalar
  double nom_vsat_; // Nominal saturation threshold at 25C, V
  double vsat_;     // Saturation threshold at temperature, V
  double dv_;       // Table hard-coded adjustment, compensates for data collection errors (hysteresis), V
  double dvoc_dt_;  // Change of VOC with temperature, V/deg C
  double dt_;       // Update time, s
  double r0_;       // Randles R0, ohms
  double tau_ct_;   // Randles charge transfer time constant, s (=1/Rct/Cct)
  double rct_;      // Randles charge transfer resistance, ohms
  double tau_dif_;  // Randles diffusion time constant, s (=1/Rdif/Cdif)
  double r_dif_;    // Randles diffusion resistance, ohms
  double tau_sd_;   // Time constant of ideal battery capacitor model, input current A, output volts=soc (0-1)
  double r_sd_;     // Trickle discharge of ideal battery capacitor model, ohms
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
};


// BatteryMonitor: extend Battery to use as monitor object
class BatteryMonitor: public Battery, public EKF_1x1
{
public:
  BatteryMonitor();
  BatteryMonitor(double *rp_delta_q, double *rp_t_last);
  ~BatteryMonitor();
  // operators
  // functions
  double amp_hrs_remaining_ekf() { return (amp_hrs_remaining_ekf_); };
  double amp_hrs_remaining_wt() { return (amp_hrs_remaining_wt_); };
  double calc_charge_time(const double q, const double q_capacity, const double charge_curr, const double soc);
  double calculate(Sensors *Sen);
  boolean converged_ekf() { return(EKF_converged->state()); };
  void init_soc_ekf(const double soc);
  boolean is_sat(void);
  double K_ekf() { return (K_); };
  void pretty_print(void);
  void regauge(const double temp_c);
  void select();
  double soc_ekf() { return (soc_ekf_); };
  double soc_wt() { return soc_wt_; };
  double SOC_ekf() { return (SOC_ekf_); };
  double SOC_wt() { return SOC_wt_; };
  boolean solve_ekf(Sensors *Sen);
  double tcharge() { return (tcharge_); };
  double voc() { return (voc_); };
  double voc_dyn() { return (voc_dyn_); };
  double voc_filt() { return (voc_filt_); };
  double voc_stat() { return (voc_stat_); };
  double y_ekf() { return (y_); };
  double y_ekf_filt() { return (y_filt_); };
protected:
  double voc_stat_; // Sim voc from soc-voc table, V
  double tcharge_ekf_;  // Solved charging time to 100% from ekf, hr
  double voc_dyn_;  // Charging voltage, V
  double soc_ekf_;  // Filtered state of charge from ekf (0-1)
  double SOC_ekf_;  // Filtered state of charge from ekf (0-100)
  double tcharge_;  // Counted charging time to 100%, hr
  double q_ekf_;    // Filtered charge calculated by ekf, C
  void ekf_predict(double *Fx, double *Bu);
  void ekf_update(double *hx, double *H);
  double voc_filt_; // Filtered, static model open circuit voltage, V
  SlidingDeadband *SdVbatt_;  // Sliding deadband filter for Vbatt
  TFDelay *EKF_converged = new TFDelay();   // Time persistence
  double soc_wt_;   // Weighted selection of ekf state of charge and coulomb counter (0-1)
  double SOC_wt_;   // Weighted selection of ekf state of charge and coulomb counter, percent
  double amp_hrs_remaining_ekf_;  // Discharge amp*time left if drain to q_ekf=0, A-h
  double amp_hrs_remaining_wt_; // Discharge amp*time left if drain soc_wt_ to 0, A-h
  LagTustin *y_filt = new LagTustin(0.1, TAU_Y_FILT, MIN_Y_FILT, MAX_Y_FILT);  // actual update time provided run time
  double y_filt_;   // Filtered EKF y value, V
};


// BatteryModel: extend Battery to use as model object
class BatteryModel: public Battery
{
public:
  BatteryModel();
  BatteryModel(double *rp_delta_q, double *rp_t_last, double *rp_s_cap_model);
  ~BatteryModel();
  // operators
  // functions
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
  double q_;        // Charge, C
  SinInj *Sin_inj_;     // Class to create sine waves
  SqInj *Sq_inj_;       // Class to create square waves
  TriInj *Tri_inj_;     // Class to create triangle waves
  CosInj *Cos_inj_;     // Class to create sosine waves
  uint32_t duty_;       // Calculated duty cycle for D2 driver to ADC cards (0-255).  Bias on rp.inj_soft_bias
  double sat_ib_max_;   // Current cutback to be applied to modeled ib output, A
  double sat_ib_null_;  // Current cutback value for voc=vsat, A
  double sat_cutback_gain_; // Gain to retard ib when voc exceeds vsat, dimensionless
  boolean model_cutback_;   // Indicate that modeled current being limited on saturation cutback, T = cutback limited
  boolean model_saturated_; // Indicator of maximal cutback, T = cutback saturated
  double ib_sat_;       // Threshold to declare saturation.  This regeneratively slows down charging so if too small takes too long, A
  double s_cap_;        // Rated capacity scalar
  double *rp_delta_q_model_;
  double *rp_t_last_model_;
  double *rp_s_cap_model_;
};


// Methods

#endif