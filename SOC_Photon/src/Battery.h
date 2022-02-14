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

// BattleBorn 100 Ah, 12v LiFePO4
#define NOM_SYS_VOLT          12.       // Nominal system output, V, at which the reported amps are used (12)
#define RATED_BATT_CAP        100.      // Nominal battery bank capacity, Ah (100).   Accounts for internal losses.  This is 
                                        // what gets delivered, e.g. Wshunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C currents
                                        // or 20-40 A for a 100 Ah battery
#define RATED_TEMP            25.       // Temperature at RATED_BATT_CAP, deg C
#define BATT_DVOC_DT          0.0068    // Change of VOC with operating temperature in range 0 - 50 C (0.0068 on 2022-01-22) V/deg C
// >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C
#define BATT_V_SAT            3.4625    // Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
#define BATT_R1               0.00126   // Battery Randels static resistance, Ohms (0.00126) for 3v cell matches transients
#define BATT_R2               0.00168   // Battery Randels dynamic resistance, Ohms (0.00168) for 3v cell matches transients
#define BATT_R2C2             100.      // Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                                        // test so using with R2 and then multiplying by 4 for total result is valid,
                                        // though probably not for an individual cell
#define DQDT                  0.01      // Change of charge with temperature, fraction/deg C
                                        // DQDT from literature.   0.01 / deg C is commonly used.
#define TCHARGE_DISPLAY_DEADBAND 0.1    // Inside this +/- deadband, charge time is displayed '---', A
const double max_voc = 1.2*NOM_SYS_VOLT;// Prevent windup of battery model, V
const int batt_num_cells = NOM_SYS_VOLT/3;  // Number of standard 3 volt LiFePO4 cells
const double batt_vsat = double(batt_num_cells)*double(BATT_V_SAT);  // Total bank saturation for 0.997=soc, V
const double batt_vmax = (14.3/4)*double(batt_num_cells); // Observed max voltage of 14.3 V at 25C for 12V prototype bank, V
const double batt_r1 = double(BATT_R1);     // Randels static resistance per cell, Ohms
const double batt_r2 = double(BATT_R2);     // Randels dynamic resistance per cell, Ohms
const double batt_r2c2 = double(BATT_R2C2);// Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                              // test so using with R2 and then multiplying by 4 for total result is valid,
                              // though probably not for an individual cell
const double batt_c2 = double(BATT_R2C2)/batt_r2;
const double nom_q_cap = RATED_BATT_CAP * 3600;   // Nominal battery capacity, C
const double q_cap_rated = RATED_BATT_CAP * 3600;   // Nominal battery capacity, C;
const double t_rlim = 0.017;    // Temperature sensor rate limit to minimize jumps in Coulomb counting, deg C/s
const double dvoc_dt = BATT_DVOC_DT * double(batt_num_cells);
const double sat_cutback_gain = 10; // Multiplier on saturation anti-windup
const double vb_dc_dc = 13.5;   // DC-DC charger estimated voltage, V

// Latest table from data
// See Model Fit 202201 tab of BattleBorn Rev1.xls
const double low_voc = 10.; // Voltage threshold for BMS to turn off battery
const double low_t = 8.;    // Minimum temperature for valid saturation check, because BMS shuts off battery low.
                            // Heater should keep >8, too
const unsigned int m_t = 3;
const double y_t[m_t] =  { 5., 20., 40. };
const unsigned int n_s = 14;
const double x_soc[n_s] =     { 0.00,  0.10,  0.20,  0.23,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.98,  1.00};
const double t_voc[m_t*n_s] = { 4.00,  4.00,  10.5,  12.00, 12.43, 12.65, 12.82, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59,
                                9.38,  12.18, 12.83, 12.86, 12.90, 12.99, 13.18, 13.21, 13.28, 13.38, 13.45, 13.49, 13.57, 13.92,
                                9.86,  12.66, 13.31, 13.35, 13.39, 13.47, 13.66, 13.69, 13.76, 13.86, 13.93, 13.97, 14.05, 14.40};
const unsigned int n_n = 5;
const double x_soc_min[n_n] = { 10.,  20.,  40. };
const double t_soc_min[n_n] = { 0.20, 0.05, 0.00};
const double mxeps_bb = 1-1e-6;      // Level of soc that indicates saturated

// Battery Class
class Battery : public Coulombs, public EKF_1x1
{
public:
  Battery();
  Battery(const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim);
  ~Battery();
  // operators
  // functions
  void Dv(const double dv) { dv_ = dv; };
  void Sr(const double sr) { sr_ = sr; Randles_->insert_D(0, 0, -r0_*sr_); };
  double calc_h_jacobian(double soc, double temp_c);
  double calc_soc_voc(const double soc, const double temp_c, double *dv_dsoc);
  virtual double calculate(const double temp_C, const double soc_frac, double curr_in, const double dt, const boolean dc_dc_on);
  double calculate_charge_time(const double q, const double q_capacity, const double charge_curr, const double soc);
  double calculate_ekf(const double temp_c, const double vb, const double ib, const double dt);
  void init_battery(void);
  void init_soc_ekf(const double soc);
  virtual void pretty_print();
  void pretty_print_ss();
  double soc_ekf() { return (soc_ekf_); };
  double SOC_ekf() { return (SOC_ekf_); };
  double voc() { return (voc_); };
  double vsat() { return (vsat_); };
  double voc_dyn() { return (voc_dyn_); };
  double voc_soc() { return (voc_stat_); };
  double vdyn() { return (vdyn_); };
  double vb() { return (vb_); };
  double ib() { return (ib_); };
  double temp_c() { return (temp_c_); };
  double tcharge() { return (tcharge_); };
  double dv_dsoc() { return (dv_dsoc_); };
  double Dv() { return (dv_); };
  double Sr() { return (sr_); };
  double K_ekf() { return (K_); };
  double y_ekf() { return (y_); };
  double amp_hrs_remaining() { return (amp_hrs_remaining_); };
  double amp_hrs_remaining_ekf() { return (amp_hrs_remaining_ekf_); };
  double voc_soc(const double soc, const double temp_c);
protected:
  double q_;        // Charge, C
  double voc_;      // Static model open circuit voltage, V
  double vdyn_;     // Model current induced back emf, V
  double vb_;        // Total model voltage, voltage at terminals, V
  double ib_;  // Current into battery, A
  int num_cells_;   // Number of cells
  double dv_dsoc_;  // Derivative scaled, V/fraction
  double tcharge_;  // Charging time to 100%, hr
  double sr_;       // Resistance scalar
  double nom_vsat_; // Nominal saturation threshold at 25C, V
  double vsat_;     // Saturation threshold at temperature, V
  double dv_;       // Adjustment, V
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
  double tcharge_ekf_;  // Charging time to 100% from ekf, hr
  double voc_dyn_;  // Charging voltage, V
  double soc_ekf_;  // Filtered state of charge from ekf (0-1)
  double SOC_ekf_;  // Filtered state of charge from ekf (0-100)
  double q_ekf_;    // Filtered charge calculated by ekf, C
  double amp_hrs_remaining_;  // Discharge amp*time left if drain to q=0, A-h
  double amp_hrs_remaining_ekf_;  // Discharge amp*time left if drain to q_ekf=0, A-h
  double voc_stat_; // Model voc from soc-voc table, V
  boolean bms_off_; // Indicator that battery management system is off, T = off preventing current flow
  TableInterp2D *voc_T_;   // SOC-VOC 2-D table, V
  void ekf_model_predict(double *Fx, double *Bu);
  void ekf_model_update(double *hx, double *H);
};


class BatteryModel: public Battery
{
public:
  BatteryModel();
  BatteryModel(const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim);
  ~BatteryModel();
  // operators
  // functions
  void apply_delta_q_t(const double delta_q, const double temp_c);
  double calculate(const double temp_C, const double soc_frac, double curr_in, const double dt,
    const double q_capacity, const double q_cap, const boolean dc_dc_on);
  uint32_t calc_inj_duty(const unsigned long now, const uint8_t type, const double amp, const double freq);
  double count_coulombs(const double dt, const boolean reset, const double temp_c, const double charge_curr, const double t_last);
  void load(const double delta_q, const double t_last, const double s_cap_model);
  void update(double *delta_q, double *t_last);
  void pretty_print(void);
  boolean cutback() { return model_cutback_; };
  boolean saturated() { return model_saturated_; };
protected:
  SinInj *Sin_inj_;     // Class to create sine waves
  SqInj *Sq_inj_;       // Class to create square waves
  TriInj *Tri_inj_;     // Class to create triangle waves
  uint32_t duty_;       // Calculated duty cycle for D2 driver to ADC cards (0-255).  Bias on rp.offset
  double sat_ib_max_;   // Current cutback to be applied to modeled ib output, A
  double sat_ib_null_;  // Current cutback value for voc=vsat, A
  double sat_cutback_gain_; // Gain to retard ib when voc exceeds vsat, dimensionless
  boolean model_cutback_;   // Indicate that modeled current being limited on saturation cutback, T = cutback limited
  boolean model_saturated_; // Indicator of maximal cutback, T = cutback saturated
  double ib_sat_;       // Threshold to declare saturation.  This regeneratively slows down charging so if too small takes too long, A
  double s_cap_;        // Rated capacity scalar
};


// Methods
void mulmat(double * a, double * b, double * c, int arows, int acols, int bcols);
void mulvec(double * a, double * x, double * y, int m, int n);
double sat_voc(const double temp_c);
double calc_vsat(const double temp_c);
boolean is_sat(const double temp_c, const double voc, const double soc);

#endif