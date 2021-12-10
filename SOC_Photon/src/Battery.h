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

#include "myTables.h"
#include "EKF_1x1.h"
#include "StateSpace.h"

class Battery: public EKF_1x1
{
public:
  Battery();
  Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt);
  ~Battery();
  // operators
  // functions
  void Dv(const double dv) { dv_ = dv; };
  void Sr(const double sr) { sr_ = sr; };
  void calc_soc_voc_coeff(double soc, double tc, double *b, double *a, double *c, double *log_soc,
                          double *exp_n_soc, double *pow_log_soc);
  double calc_h_jacobian(double soc_lim, double b, double c, double log_soc, double exp_n_soc, double pow_log_soc);
  double calc_voc_ocv(double soc_lim, double *dv_dsoc, double b, double a, double c, double log_soc, double exp_n_soc, double pow_log_soc);
  double calculate(const double temp_C, const double socu_frac, const double curr_in, const double dt);
  double calculate_ekf(const double temp_c, const double vb, const double ib, const double dt);
  void init_soc_ekf(const double socu_free);
  double num_cells() { return (num_cells_); };
  double socs() { return (socs_); };
  double socu() { return (socu_); };
  double voc() { return (voc_); };
  double voc_dyn() { return (voc_dyn_); };
  double vdyn() { return (vdyn_); };
  double v() { return (v_); };
  double tcharge() { return (tcharge_); };
  double dv_dsocs() { return (dv_dsocs_); };
  double dv_dsocu() { return (dv_dsocu_); };
  double Dv() { return (dv_); };
  double Sr() { return (sr_); };
  boolean sat() { return (sat_); };
  boolean sat(const double v, const double i) { return (v-i*(r1_+r2_)*num_cells_>= vsat_); };
protected:
  TableInterp1Dclip *B_T_;  // Battery coeff
  TableInterp1Dclip *A_T_;  // Battery coeff
  TableInterp1Dclip *C_T_;  // Battery coeff
  double b_;        // Battery coeff
  double a_;        // Battery coeff
  double c_;        // Battery coeff
  double m_;        // Battery coeff
  double n_;        // Battery coeff
  double d_;        // Battery coeff
  unsigned int nz_; // Number of breakpoints
  double socs_;     // State of charge scaled 0-1
  double socu_;     // State of charge may extend <0
  double r1_;       // Randels resistance, Ohms per cell
  double r2_;       // Randels resistance, Ohms per cell
  double c2_;       // Randels capacitance, Farads per cell
  double voc_;      // Static model open circuit voltage, V
  double vdyn_;     // Model current induced back emf, V
  double v_;        // Total model voltage, V
  double curr_in_;  // Current into battery, A
  int num_cells_;   // Number of cells
  double dv_dsocs_;  // Derivative scaled, V/fraction
  double dv_dsocu_;  // Derivative unscaled, V/fraction
  double tcharge_;  // Charging time to 100%, hr
  double pow_in_;   // Charging power, w
  double sr_;       // Resistance scalar
  double nom_vsat_; // Nominal saturation threshold at 25C
  double vsat_;     // Saturation threshold at temperature
  boolean sat_;     // Saturation status
  double dv_;       // Adjustment, V
  double dvoc_dt_;  // Change of VOC with temperature, V/deg C
  double dt_;       // Update time
  double r0_;       // Randles R0, ohms
  double tau_ct_;   // Randles charge transfer time constant, s (=1/Rct/Cct)
  double rct_;      // Randles charge transfer resistance, ohms
  double tau_dif_;  // Randles diffusion time constant, s (=1/Rdif/Cdif)
  double r_dif_;    // Randles diffusion resistance, ohms
  double tau_sd_;   // Time constant of ideal battery capacitor model, input current A, output volts=soc (0-1)
  double r_sd_;     // Trickle discharge of ideal battery capacitor model, ohms
  // EKF declarations
  StateSpace *Randles_;  // Randles model
  double *rand_A_;
  double *rand_B_;
  double *rand_C_;
  double *rand_D_;
  int8_t rand_n_, rand_p_, rand_q_;
  double temp_c_;   // Battery temperature, C
  double soc_ekf_;  // Filtered state of charge from ekf (0-1)
  double pow_in_ekf_;   // Charging power from ekf, w
  double tcharge_ekf_;  // Charging time to 100% from ekf, hr
  double voc_dyn_;  // Charging voltage, V
  void ekf_model_predict(double *Fx, double *Bu);
  void ekf_model_update(double *hx, double *H);
};

// BattleBorn 100 Ah, 12v LiFePO4
#define NOM_SYS_VOLT          12        // Nominal system output, V, at which the reported amps are used (12)
#define NOM_BATT_CAP          100       // Nominal battery bank capacity, Ah (100).   Accounts for internal losses.  This is 
                                        // what gets delivered, e.g. Wshunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C currents
                                        // or 20-40 A for a 100 Ah battery
#define BATT_DVOC_DT          0.001875  // Change of VOC with operating temperature in range 0 - 50 C (0.0075) V/deg C
// >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 60-95 F
#define BATT_V_SAT            3.4625    // Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
#define BATT_R1               0.00126   // Battery Randels static resistance, Ohms (0.00126) for 3v cell matches transients
#define BATT_R2               0.00168   // Battery Randels dynamic resistance, Ohms (0.00168) for 3v cell matches transients
#define BATT_R2C2             100       // Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                                        // test so using with R2 and then multiplying by 4 for total result is valid,
                                        // though probably not for an individual cell
const int batt_num_cells = NOM_SYS_VOLT/3;  // Number of standard 3 volt LiFePO4 cells
const double batt_vsat = double(batt_num_cells)*double(BATT_V_SAT);  // Total bank saturation for 0.997=soc, V
const double batt_vmax = (14.3/4)*double(batt_num_cells); // Observed max voltage of 14.3 V at 25C for 12V prototype bank, V
const double batt_r1 = double(BATT_R1);     // Randels static resistance per cell, Ohms
const double batt_r2 = double(BATT_R2);     // Randels dynamic resistance per cell, Ohms
const double batt_r2c2 = double(BATT_R2C2);// Battery Randels dynamic term, Ohms-Farads (100).   Value of 100 probably derived from a 4 cell
                              // test so using with R2 and then multiplying by 4 for total result is valid,
                              // though probably not for an individual cell
const double batt_c2 = double(BATT_R2C2)/batt_r2;

// Battery model LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang etal.pdf'
// SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope constraint
// >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity
// > NOM_BATT_CAP so equations error when soc<=0 to match data.
const unsigned int nz_bb = 3;
const double m_bb = 0.478;
static const double t_bb[nz_bb] = {0.,	    25.,    50.};
static const double b_bb[nz_bb] = {-0.836,  -0.836, -0.836};
static const double a_bb[nz_bb] = {3.999,   4.046,  4.093};
static const double c_bb[nz_bb] = {-1.181,  -1.181, -1.181};
const double d_bb = 0.707;
const double n_bb = 0.4;
const double cu_bb = NOM_BATT_CAP;  // Assumed capacity, Ah
const double cs_bb = 102.;          // Data fit to this capacity to avoid math 0, Ah
const double mxeps_bb = 1-1e-6;     // Numerical maximum of coefficient model with scaled socs.
const double mneps_bb = 1e-6;       // Numerical minimum of coefficient model without scaled socs.
const double mxepu_bb = 1-1e-6;     // Numerical maximum of coefficient model with scaled socs.
const double mnepu_bb = 1 - (1-1e-6)*cs_bb/cu_bb;  // Numerical minimum of coefficient model without scaled socs.
const double dvoc_dt = BATT_DVOC_DT * double(batt_num_cells);

// Charge test profiles
#define NUM_VEC           1   // Number of vectors defined here
static const unsigned int n_v1 = 10;
static const double t_min_v1[n_v1] =  {0,     0.2,   0.2001, 1.4,   1.4001, 2.0999, 2.0,    3.1999, 3.2,    3.6};
static const double v_v1[n_v1] =      {13.95, 13.95, 13.95,  13.0,  13.0,   13.0,   13.0,   13.95,  13.95,  13.95}; // Saturation 13.7
static const double i_v1[n_v1] =      {0.,    0.,    -500.,  -500., 0.,     0.,     160.,   160.,   0.,     0.};
static const double T_v1[n_v1] =      {77.,   77.,   77.,    77.,   77.,    77.,    77.,    77.,    77.,    77.};
static TableInterp1Dclip  *V_T1 = new TableInterp1Dclip(n_v1, t_min_v1, v_v1);
static TableInterp1Dclip  *I_T1 = new TableInterp1Dclip(n_v1, t_min_v1, i_v1);
static TableInterp1Dclip  *T_T1 = new TableInterp1Dclip(n_v1, t_min_v1, T_v1);

// Methods
void mulmat(double * a, double * b, double * c, int arows, int acols, int bcols);
void mulvec(double * a, double * x, double * y, int m, int n);

#endif