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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "Battery.h"
#include "retained.h"
#include <math.h>
#include "constants.h"
#include "command.h"
#include "mySubs.h"
extern RetainedPars rp; // Various parameters to be static at system level
extern CommandPars cp;


// class Battery
// constructors
Battery::Battery()
    :voc_(0), vdyn_(0), vb_(0), ib_(0), num_cells_(4), dv_dsoc_(0), sr_(1), vsat_(13.7),
    dv_(0), dvoc_dt_(0){}
Battery::Battery(double *rp_delta_q, double *rp_t_last, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim, const double hys_direx)
    : Coulombs(rp_delta_q, rp_t_last, q_cap_rated, t_rated, t_rlim), voc_(0), vdyn_(0), vb_(0), ib_(0), num_cells_(num_cells), dv_dsoc_(0),
    sr_(1.), nom_vsat_(batt_vsat), dv_(0.01), dvoc_dt_(dvoc_dt),  // 0.01 to compensate for tables generated without considering hys
    r0_(0.003), tau_ct_(0.2), rct_(0.0016), tau_dif_(83.), r_dif_(0.0077),
    tau_sd_(1.8e7), r_sd_(70.), ioc_(0)
{
    // Battery characteristic tables
    voc_T_ = new TableInterp2D(n_s, m_t, x_soc, y_t, t_voc);

    // Randles dynamic model for EKF, forward version based on sensor inputs {ib, vb} --> {voc}, ioc=ib
    // Resistance values add up to same resistance loss as matched to installed battery
    //   i.e.  (r0_ + rct_ + rdif_) = (r1 + r2)*num_cells
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    double c_ct = tau_ct_ / rct_;
    double c_dif = tau_dif_ / r_dif_;
    int rand_n = 2; // Rows A and B
    int rand_p = 2; // Col B    
    int rand_q = 1; // Row C and D
    rand_A_ = new double[rand_n*rand_n];
    rand_A_[0] = -1./tau_ct_;
    rand_A_[1] = 0.;
    rand_A_[2] = 0.;
    rand_A_[3] = -1/tau_dif_;
    rand_B_ = new double [rand_n*rand_p];
    rand_B_[0] = 1./c_ct;
    rand_B_[1] = 0.;
    rand_B_[2] = 1./c_dif;
    rand_B_[3] = 0.;
    rand_C_ = new double [rand_q*rand_n];
    rand_C_[0] = -1.;
    rand_C_[1] = -1.;
    rand_D_ = new double [rand_q*rand_p];
    rand_D_[0] = -r0_;
    rand_D_[1] = 1.;
    Randles_ = new StateSpace(rand_A_, rand_B_, rand_C_, rand_D_, rand_n, rand_p, rand_q);
    hys_ = new Hysteresis(hys_cap, hys_direx);
}
Battery::~Battery() {}
// operators
// functions

// SOC-OCV curve fit method per Zhang, et al.   May write this base version if needed using BatteryModel::calculate()
// as a starting point but use the base class Randles formulation and re-arrange the i/o for that model.
double Battery::calculate(const double temp_C, const double q, const double curr_in, const double dt,
    const boolean dc_dc_on) { return 0.;}

// VOC-OCV model
double Battery::calc_soc_voc(const double soc, const double temp_c, double *dv_dsoc)
{
    double voc;  // return value
    *dv_dsoc = calc_soc_voc_slope(soc, temp_c);
    voc = voc_T_->interp(soc, temp_c) + dv_;
    return (voc);
}

// Derivative model
double Battery::calc_soc_voc_slope(const double soc, const double temp_c)
{
    double dv_dsoc;  // return value
    if ( soc > 0.5 )
        dv_dsoc = (voc_T_->interp(soc, temp_c) - voc_T_->interp(soc-0.01, temp_c)) / 0.01;
    else
        dv_dsoc = (voc_T_->interp(soc+0.01, temp_c) - voc_T_->interp(soc, temp_c)) / 0.01;
    return (dv_dsoc);
}

// Initialize
void Battery::init_battery(void)
{
    Randles_->init_state_space();
    init_hys(0.0);
}

// Print
void Battery::pretty_print(void)
{
    Serial.printf("Battery:\n");
    Serial.printf("  temp_c_ =    %7.3f;  //  Battery temperature, deg C\n", temp_c_);
    Serial.printf("  num_cells_ =       %d;  //  Battery number of cells\n", num_cells_);
    Serial.printf("  dvoc_dt_ =%10.6f;  //  Change of VOC with temperature, V/deg C\n", dvoc_dt_);
    Serial.printf("  r0_ =     %10.6f;  //  Randles R0, ohms\n", r0_);
    Serial.printf("  rct_ =    %10.6f;  //  Randles charge transfer resistance, ohms\n", rct_);
    Serial.printf("  tau_ct =  %10.6f;  //  Randles charge transfer time constant, s (=1/Rct/Cct)\n", tau_ct_);
    Serial.printf("  r_dif_ =  %10.6f;  //  Randles diffusion resistance, ohms\n", r_dif_);
    Serial.printf("  tau_dif_ =%10.6f;  //  Randles diffusion time constant, s (=1/Rdif/Cdif)\n", tau_dif_);
    Serial.printf("  r_sd_ =   %10.6f;  //  Trickle discharge of ideal battery capacitor model, ohms\n", r_sd_);
    Serial.printf("  tau_sd_ = %10.1f;  //  Time constant of ideal battery capacitor model, input current A, output volts=soc (0-1)\n", tau_sd_);
    Serial.printf("  bms_off_ =         %d;  // Calculated indication that the BMS has turned off charge current, T=off\n", bms_off_);
    Serial.printf("  dv_dsoc_= %10.6f;  // Derivative scaled, V/fraction\n", dv_dsoc_);
    Serial.printf("  ib_ =        %7.3f;  // Battery terminal current, A\n", ib_);
    Serial.printf("  vb_ =        %7.3f;  // Battery terminal voltage, V\n", vb_);
    Serial.printf("  voc_ =       %7.3f;  // Static model open circuit voltage, V\n", voc_);
    Serial.printf("  vsat_ =      %7.3f;  // Saturation threshold at temperature, V\n", vsat_);
    Serial.printf("  vdyn_ =      %7.3f;  // Sim current induced back emf, V\n", vdyn_);
    Serial.printf("  sr_ =        %7.3f;  // Resistance scalar\n", sr_);
    Serial.printf("  dv_ =        %7.3f;  // Table hard-coded adjustment, compensates for data collection errors (hysteresis), V\n", dv_);
    Serial.printf("  dt_ =        %7.3f;  // Update time, s\n", dt_);
}

// Print State Space
void Battery::pretty_print_ss(void)
{
    Randles_->pretty_print();
    Serial.printf("  ::"); hys_->pretty_print();
}

// EKF model for update
double Battery::voc_soc(const double soc, const double temp_c)
{
    double voc;     // return value
    double dv_dsoc;
    voc = calc_soc_voc(soc, temp_c, &dv_dsoc);
    return ( voc );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery monitor class
BatteryMonitor::BatteryMonitor(): Battery()
{
    Q_ = 0.;
    R_ = 0.;
}
BatteryMonitor::BatteryMonitor(double *rp_delta_q, double *rp_t_last, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim, const double hys_direx, const double hdb_vbatt) :
    Battery(rp_delta_q, rp_t_last, num_cells, r1, r2, r2c2, batt_vsat, dvoc_dt, q_cap_rated, t_rated, t_rlim, hys_direx), tcharge_(24), voc_filt_(batt_vsat)
{

    // EKF
    this->Q_ = 0.001*0.001;
    this->R_ = 0.1*0.1;
    SdVbatt_ = new SlidingDeadband(hdb_vbatt);
}
BatteryMonitor::~BatteryMonitor() {}

// operators
// functions



/* BatteryModel::calculate_ekf:  SOC-OCV curve fit solved by ekf
        Inputs:
        Sen->Tbatt_filt Tb filtered for noise, past value of temp_c_, deg C
        Sen->Vbatt      Battery terminal voltage, V
        Sen->Ishunt     Shunt current Ib, A
        Sen->T          Update time, sec
        q_capacity_     Saturation charge at temperature, C
        q_cap_rated_scaled_   Applied rated capacity at t_rated_, after scaling, C
        RATED_BATT_CAP  Nominal battery bank capacity (assume actual not known), Ah
    Outputs:
        vsat_           Saturation threshold at temperature, V
        voc_dyn_        VOC estimated from Vb and RC model, V
        voc_            Static model open circuit voltage, V
        vdyn_           Sim current induced back emf, V
        voc_filt_       Filtered open circuit voltage for saturation detect, V
        ioc_            Best estimate of IOC charge current after hysteresis, A
        bms_off_        Calculated indication that the BMS has turned off charge current, T=off
        voc_stat_       Static model open circuit voltage from table (reference), V\n
        Tb              Tb, deg C
        ib_             Battery terminal current, A
        vb_             Battery terminal voltage, V
        rp.duty         (0-255) for D2 hardware injection when rp.modeling and proper wire connections made
        soc_ekf_        Solved state of charge, fraction
        q_ekf_          Filtered charge calculated by ekf, C
        SOC_ekf_ (return)     Solved state of charge, percent
        tcharge_ekf_    Solved charging time to full from ekf, hr
*/
double BatteryMonitor::calculate_ekf(Sensors *Sen)
{
    // Inputs
    temp_c_ = Sen->Tbatt_filt;
    vsat_ = calc_vsat(temp_c_);
    dt_ =  min(Sen->T, F_MAX_T);

    // Dynamic emf
    vb_ = Sen->Vbatt;
    ib_ = Sen->Ishunt;
    double u[2] = {ib_, vb_};
    Randles_->calc_x_dot(u);
    Randles_->update(dt_);
    if ( rp.debug==35 )
    {
        Serial.printf("BatteryMonitor::calculate_ekf:"); Randles_->pretty_print();
    }
    voc_dyn_ = Randles_->y(0);
    vdyn_ = vb_ - voc_dyn_;
    voc_stat_ = voc_soc(soc_, Sen->Tbatt_filt);
    // Hysteresis model
    hys_->calculate(ib_, voc_dyn_, soc_);
    voc_ = hys_->update(dt_);
    voc_filt_ = SdVbatt_->update(voc_);
    ioc_ = hys_->ioc();
    bms_off_ = temp_c_ <= low_t;    // KISS
    if ( bms_off_ )
    {
        ib_ = 0.;
        voc_ = voc_stat_;
        voc_filt_ = voc_stat_;
        vdyn_ = voc_stat_;
        voc_dyn_ = 0.;
    }

    // EKF 1x1
    predict_ekf(ib_);           // u = ib
    update_ekf(voc_, 0., 1.);   // z = voc, voc_filtered = hx
    soc_ekf_ = x_ekf();         // x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
    q_ekf_ = soc_ekf_ * q_capacity_;
    SOC_ekf_ = q_ekf_ / q_cap_rated_scaled_ * 100.;

    if ( rp.debug==34 )
        Serial.printf("dt,ib,voc_dyn,voc,voc_filt,vdyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf,   %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f,\n",
            dt_, ib_, voc_dyn_, voc_, voc_filt_, vdyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_);
    if ( rp.debug==-34 )
        Serial.printf("dt,ib,voc_dyn,voc,voc_filt,vdyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf,  \n%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f,\n",
            dt_, ib_, voc_dyn_, voc_, voc_filt_, vdyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_);
    if ( rp.debug==37 )
        Serial.printf("ib,vb,voc_dyn,voc(z_),  K_,y_,SOC_ekf,   %7.3f,%7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f,\n",
            ib_, vb_, voc_dyn_, voc_,     K_, y_, soc_ekf_);
    if ( rp.debug==-37 )
        Serial.printf("ib,vb*10-110,voc_dyn_,voc(z_)*10-110,  K_,y_,SOC_ekf-90,   \n%7.3f,%7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f,\n",
            ib_, vb_*10-110, voc_dyn_*10-110, voc_*10-110,     K_, y_, soc_ekf_*100-90);

    // Charge time if used ekf 
    if ( ib_ > 0.1 )  tcharge_ekf_ = min(RATED_BATT_CAP / ib_ * (1. - soc_ekf_), 24.);
    else if ( ib_ < -0.1 ) tcharge_ekf_ = max(RATED_BATT_CAP / ib_ * soc_ekf_, -24.);
    else if ( ib_ >= 0. ) tcharge_ekf_ = 24.*(1. - soc_ekf_);
    else tcharge_ekf_ = -24.*soc_ekf_;

    return ( soc_ekf_ );
}

// Charge time calculation
double BatteryMonitor::calculate_charge_time(const double q, const double q_capacity, const double charge_curr, const double soc)
{
    double delta_q = q - q_capacity;
    if ( charge_curr > TCHARGE_DISPLAY_DEADBAND )  tcharge_ = min( -delta_q / charge_curr / 3600., 24.);
    else if ( charge_curr < -TCHARGE_DISPLAY_DEADBAND ) tcharge_ = max( max(q_capacity + delta_q - q_min_, 0.) / charge_curr / 3600., -24.);
    else if ( charge_curr >= 0. ) tcharge_ = 24.;
    else tcharge_ = -24.;

    amp_hrs_remaining_ = max(q_capacity - q_min_ + delta_q, 0.) / 3600.;
    if ( soc > 0. )
        amp_hrs_remaining_ekf_ = amp_hrs_remaining_ * (soc_ekf_ - soc_min_) / max(soc - soc_min_, 1e-8);
    else
        amp_hrs_remaining_ekf_ = 0.;

    return( tcharge_ );
}

// EKF model for predict
void BatteryMonitor::ekf_model_predict(double *Fx, double *Bu)
{
    // Process model
    *Fx = exp(-dt_ / tau_sd_);
    *Bu = (1. - *Fx) * r_sd_;
}

// EKF model for update
// EKF model for update
void BatteryMonitor::ekf_model_update(double *hx, double *H)
{
    // Measurement function hx(x), x=soc ideal capacitor
    double x_lim = max(min(x_, 1.0), 0.0);
    *hx = Battery::calc_soc_voc(x_lim, temp_c_, &dv_dsoc_);

    // Jacodian of measurement function
    *H = dv_dsoc_;
}

// Init EKF
void BatteryMonitor::init_soc_ekf(const double soc)
{
    soc_ekf_ = soc;
    init_ekf(soc_ekf_, 0.0);
    q_ekf_ = soc_ekf_ * q_capacity_;
    SOC_ekf_ = q_ekf_ / q_cap_rated_scaled_ * 100.;
    if ( rp.debug==-34 )
    {
        Serial.printf("init_soc_ekf:  soc, soc_ekf_, x_ekf_ = %7.3f,%7.3f, %7.3f,\n", soc, soc_ekf_, x_ekf());
    }
}

// Print
void BatteryMonitor::pretty_print(void)
{
    Serial.printf("BatteryMonitor::");
    this->Battery::pretty_print();
    Serial.printf(" BatteryMonitor::BatteryMonitor:\n");
    Serial.printf("  voc_filt_ =               %7.3f;  // Filtered open circuit voltage for saturation detect, V\n", voc_filt_);
    Serial.printf("  voc_stat_ =               %7.3f;  // Static model open circuit voltage from table (reference), V\n", voc_stat_);
    Serial.printf("  q_ekf =                %10.1f;  // Filtered charge calculated by ekf, C\n", q_ekf_);
    Serial.printf("  tcharge =                   %5.1f;  // Counted charging time to full, hr\n", tcharge_);
    Serial.printf("  tcharge_ekf =               %5.1f;  // Solved charging time to full from ekf, hr\n", tcharge_ekf_);
    Serial.printf("  soc_ekf =                 %7.3f;  // Solved state of charge, fraction\n", soc_ekf_);
    Serial.printf("  SOC_ekf_ =                  %5.1f;  // Solved state of charge, percent\n", SOC_ekf_);
    Serial.printf("  amp_hrs_remaining =       %7.3f;  // Discharge amp*time left if drain to q=0, A-h\n", amp_hrs_remaining_);
    Serial.printf("  amp_hrs_remaining_ekf_ =  %7.3f;  // Discharge amp*time left if drain to q_ekf=0, A-h\n", amp_hrs_remaining_ekf_);
}
/*
INPUTS:
    Sen->Vbatt      
    Sen->Ishunt
OUTPUTS:
    Mon->soc_ekf
*/
// Steady state voc(soc) solver for initialization of ekf state.  Expects Sen->Tbatt_filt to be in reset mode
boolean BatteryMonitor::solve_ekf(Sensors *Sen)
{
    // Solver, steady
    #define SOLV_ERR 1e-6
    #define SOLV_MAX_COUNTS 10
    #define SOLV_MAX_STEP 0.2
    const double meps = 1-1e-6;
    double voc = Sen->Vbatt - Sen->Ishunt*batt_r_ss;
    int8_t count = 0;
    static double soc_solved = 1.0;
    double dv_dsoc;
    double vb_solved = calc_soc_voc(soc_solved, Sen->Tbatt_filt, &dv_dsoc);
    double err = voc - vb_solved;
    while( abs(err)>SOLV_ERR && count++<SOLV_MAX_COUNTS )
    {
        soc_solved = max(min(soc_solved + max(min( err/dv_dsoc, SOLV_MAX_STEP), -SOLV_MAX_STEP), meps), 1e-6);
        vb_solved = calc_soc_voc(soc_solved, Sen->Tbatt_filt, &dv_dsoc);
        err = voc - vb_solved;
        if ( rp.debug==6 ) Serial.printf("solve_ekf:Tbatt_f,ib,count,soc_s,voc,voc_m_s,err, %7.3f,%7.3f,  %d,%7.3f,%7.3f,%7.3f,%10.6f,\n",
            Sen->Tbatt_filt, Sen->Ishunt, count, soc_solved, voc, vb_solved, err);
    }
    if ( rp.debug==7 ) Serial.printf("solve_ekf:Tbatt_f,ib,count,soc_s,voc,voc_m_s,err, %7.3f,%7.3f,  %d,%7.3f,%7.3f,%7.3f,%10.6f,\n",
    Sen->Tbatt_filt, Sen->Ishunt, count, soc_solved, voc, vb_solved, err);
    init_soc_ekf(soc_solved);
    return ( count<SOLV_MAX_COUNTS );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery model class for reference use mainly in jumpered hardware testing
BatteryModel::BatteryModel() : Battery() {}
BatteryModel::BatteryModel(double *rp_delta_q, double *rp_t_last, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim, const double hys_direx, 
    double *rp_s_cap_model) :
    Battery(rp_delta_q, rp_t_last, num_cells, r1, r2, r2c2, batt_vsat, dvoc_dt, q_cap_rated, t_rated, t_rlim, hys_direx), q_(q_cap_rated),
    rp_s_cap_model_(rp_s_cap_model)
{
    // Randles dynamic model for EKF
    // Resistance values add up to same resistance loss as matched to installed battery
    //   i.e.  (r0_ + rct_ + rdif_) = (r1 + r2)*num_cells
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    double c_ct = tau_ct_ / rct_;
    double c_dif = tau_dif_ / r_dif_;
    int rand_n = 2; // Rows A and B
    int rand_p = 2; // Col B    
    int rand_q = 1; // Row C and D
    rand_A_ = new double[rand_n*rand_n];
    rand_A_[0] = -1./tau_ct_;
    rand_A_[1] = 0.;
    rand_A_[2] = 0.;
    rand_A_[3] = -1/tau_dif_;
    rand_B_ = new double [rand_n*rand_p];
    rand_B_[0] = 1./c_ct;
    rand_B_[1] = 0.;
    rand_B_[2] = 1./c_dif;
    rand_B_[3] = 0.;
    rand_C_ = new double [rand_q*rand_n];
    rand_C_[0] = 1.;
    rand_C_[1] = 1.;
    rand_D_ = new double [rand_q*rand_p];
    rand_D_[0] = r0_;
    rand_D_[1] = 1.;
    Randles_ = new StateSpace(rand_A_, rand_B_, rand_C_, rand_D_, rand_n, rand_p, rand_q);
    Sin_inj_ = new SinInj();
    Sq_inj_ = new SqInj();
    Tri_inj_ = new TriInj();
    Cos_inj_ = new CosInj();
    sat_ib_null_ = 0.;          // Current cutback value for soc=1, A
    sat_cutback_gain_ = 1000.;  // Gain to retard ib when soc approaches 1, dimensionless
    model_saturated_ = false;
    ib_sat_ = 0.5;              // deadzone for cutback actuation, A
}

// calculate:  SOC-OCV table with a Battery Management System (BMS) and hysteresis.
// Makes a good reference model. Intervenes in sensor path to provide Mon with inputs.
//
//  Inputs:
//    Sen->Tbatt_filt Simulated Tb filtered for noise, past value of temp_c_, deg C
//    Sen->Ishunt     Proposed simulated Ib, A
//    Sen->T          Update time, sec
//
//  States:
//    soc_            State of Charge, fraction
//
//  Outputs:
//    Tb              Simulated Tb, deg C
//    Ib              Simulated over-ridden by saturation Ib, A
//    Vb (return)     Simulated Vb, V
//    rp.duty         (0-255) for D2 hardware injection when rp.modeling and proper wire connections made
double BatteryModel::calculate(Sensors *Sen, const boolean dc_dc_on)
{
    const double temp_C = Sen->Tbatt_filt;
    double curr_in = Sen->Ishunt;
    const double dt = min(Sen->T, F_MAX_T);

    dt_ = dt;
    temp_c_ = temp_C;

    double soc_lim = max(min(soc_, 1.0), 0.0);
    double SOC = soc_ * q_capacity_ / q_cap_rated_scaled_ * 100;

    // VOC-OCV model
    voc_stat_ = calc_soc_voc(soc_, temp_C, &dv_dsoc_);
    voc_stat_ = min(voc_stat_ + (soc_ - soc_lim) * dv_dsoc_, max_voc);  // slightly beyond but don't windup
    bms_off_ = ( temp_c_ <= low_t ) || ( voc_stat_ < low_voc );
    if ( bms_off_ ) curr_in = 0.;

    // Dynamic emf
    // Hysteresis model
    hys_->calculate(curr_in, voc_stat_, soc_);
    voc_ = hys_->update(dt);
    ioc_ = hys_->ioc();
    // Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
    double u[2] = {ib_, voc_};  // past value ib_
    Randles_->calc_x_dot(u);
    Randles_->update(dt);
    vb_ = Randles_->y(0);
    vdyn_ = vb_ - voc_;
    // Special cases override
    if ( bms_off_ )
    {
        vdyn_ = voc_stat_;
        voc_ = voc_stat_;
    }
    if ( bms_off_ && dc_dc_on )
    {
        vb_ = vb_dc_dc;
    }

    // Saturation logic, both full and empty
    vsat_ = nom_vsat_ + (temp_C-25.)*dvoc_dt_;
    sat_ib_max_ = sat_ib_null_ + (1. - soc_) * sat_cutback_gain_ * rp.cutback_gain_scalar;
    if ( rp.tweak_test ) sat_ib_max_ = curr_in; // Disable cutback when doing tweak_test test
    ib_ = min(curr_in, sat_ib_max_);
    if ( (q_ <= 0.) && (curr_in < 0.) ) ib_ = 0.;  //  empty
    model_cutback_ = (voc_stat_ > vsat_) && (ib_ == sat_ib_max_);
    model_saturated_ = (voc_stat_ > vsat_) && (ib_ < ib_sat_) && (ib_ == sat_ib_max_);
    Coulombs::sat_ = model_saturated_;
    if ( rp.debug==79 ) Serial.printf("temp_C, dvoc_dt, vsat_, voc, q_capacity, sat_ib_max, ib,=   %7.3f,%7.3f,%7.3f,%7.3f, %10.1f, %7.3f, %7.3f,\n",
        temp_C, dvoc_dt_, vsat_, voc_, q_capacity_, sat_ib_max_, ib_);

    if ( rp.debug==78 ) Serial.printf("BatteryModel::calculate:,  dt,tempC,curr,soc_,voc,,vdyn,v,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
     dt,temp_C, ib_, soc_, voc_, vdyn_, vb_);
    if ( rp.debug==-78 ) Serial.printf("SOC/10,soc*10,voc,vsat,curr_in,sat_ib_max_,ib,sat,\n%7.3f, %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,\n", 
      SOC/10, soc_*10, voc_, vsat_, curr_in, sat_ib_max_, ib_, model_saturated_);
    
    if ( rp.debug==76 ) Serial.printf("BatteryModel::calculate:,  soc=%7.3f, temp_c=%7.3f, ib=%7.3f, voc_stat=%7.3f, voc=%7.3f, vsat=%7.3f, model_saturated=%d, bms_off=%d, dc_dc_on=%d, vb_dc_dc=%7.3f, vb=%7.3f\n",
        soc_, temp_C, ib_, voc_stat_, voc_, vsat_, model_saturated_, bms_off_, dc_dc_on, vb_dc_dc, vb_);

    return ( vb_ );
}

// Injection model, calculate duty
uint32_t BatteryModel::calc_inj_duty(const unsigned long now, const uint8_t type, const double amp, const double freq)
{
  double t;
  if ( rp.tweak_test )
  {
      if ( now<2*TEMP_INIT_DELAY )
      {
          duty_ = 0UL;
          rp.inj_soft_bias = 0.;
          return(duty_);
      }
      else
          t = (now - 2*TEMP_INIT_DELAY)/1e3;
  }
  else t = now/1e3;
  double sin_bias = 0.;
  double square_bias = 0.;
  double tri_bias = 0.;
  double inj_bias = 0.;
  double bias = 0.;
  double cos_bias = 0.;
  // Calculate injection amounts from user inputs (talk).
  // One-sided because PWM voltage >0.  rp.inj_soft_bias applied elsewhere
  switch ( type )
  {
    case ( 0 ):   // Nothing
      break;
    case ( 1 ):   // Sine wave
      sin_bias = Sin_inj_->signal(amp, freq, t, 0.0);
      break;
    case ( 2 ):   // Square wave
      square_bias = Sq_inj_->signal(amp, freq, t, 0.0);
      break;
    case ( 3 ):   // Triangle wave
      tri_bias = Tri_inj_->signal(amp, freq, t, 0.0);
    case ( 4 ): case ( 5 ): // Software biases only
      break;
    case ( 6 ):   // Positve bias
      bias = amp;
      break;
    case ( 8 ):   // Cosine wave
      cos_bias = Cos_inj_->signal(amp, freq, t, 0.0);
      break;
    default:
      break;
  }
  inj_bias = sin_bias + square_bias + tri_bias + bias + cos_bias;
  if ( rp.tweak_test )   // Use inj_soft_bias path, bypassing PWM that has limited hardware range
  {
    duty_ = 0UL;
    rp.inj_soft_bias = inj_bias - rp.amp;
  }
  else
    duty_ = min(uint32_t(inj_bias / bias_gain), uint32_t(255.));

  if ( rp.debug==-41 ) Serial.printf("type,amp,freq,sin,square,tri,bias,inj,duty,tnow,off=%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,   %ld,  %7.3f, %7.3f,\n",
            type, amp, freq, sin_bias, square_bias, tri_bias, bias, inj_bias, duty_, t, rp.inj_soft_bias);

  return ( duty_ );
}

/* BatteryModel::count_coulombs: Count coulombs based on assumed model true=actual capacity
Inputs:
    Sen->T          Integration step, s
    Sen->Tbatt      Battery temperature, deg C
    Sen->Ishunt     Charge, A
    t_last          Past value of battery temperature used for rate limit memory, deg C
Outputs:
    q_capacity_     Saturation charge at temperature, C
    *rp_delta_q_    Charge change since saturated, C
    *rp_t_last_     Updated value of battery temperature used for rate limit memory, deg C
    resetting_      Sticky flag for initialization, T=reset
    soc_            Fraction of saturation charge (q_capacity_) available (0-1) 
    SOC_            Fraction of rated capacity available (0 - ~1.2).   For comparison to other batteries
    soc_min_        Estimated soc where battery BMS will shutoff current, fraction
    q_min_          Estimated charge at low voltage shutdown, C\
*/
double BatteryModel::count_coulombs(Sensors *Sen, const boolean reset, const double t_last)
{
    double d_delta_q = Sen->Ishunt * Sen->T;

    // Rate limit temperature
    double temp_lim = max(min(Sen->Tbatt, t_last + t_rlim_*Sen->T), t_last - t_rlim_*Sen->T);
    if ( reset ) temp_lim = Sen->Tbatt;
    if ( reset ) *rp_t_last_ = Sen->Tbatt;

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    if ( model_saturated_ )
    {
        if ( reset ) *rp_delta_q_ = 0.;  // Sim is truth.   Saturate it then restart it to reset charge
    }
    resetting_ = false;     // one pass flag.  Saturation debounce should reset next pass

    // Integration
    q_capacity_ = calculate_capacity(temp_lim);
    if ( !reset ) *rp_delta_q_ = max(min(*rp_delta_q_ + d_delta_q - DQDT*q_capacity_*(temp_lim-*rp_t_last_), 0. ), -q_capacity_);
    q_ = q_capacity_ + *rp_delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    soc_min_ = soc_min_T_->interp(temp_lim);
    q_min_ = soc_min_ * q_capacity_;
    SOC_ = q_ / q_cap_rated_scaled_ * 100;

    if ( rp.debug==97 )
        Serial.printf("BatteryModel::cc,  dt,voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc,SOC,    %7.3f,%7.3f,%7.3f,%7.3f,  %d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%10.6f,%5.1f,\n",
                    Sen->T,cp.pubList.voc,  sat_voc(Sen->Tbatt), temp_lim, model_saturated_, Sen->Ishunt, d_delta_q, *rp_delta_q_, q_, q_capacity_, soc_, SOC_);
    if ( rp.debug==-97 )
        Serial.printf("voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc, SOC,        \n%7.3f,%7.3f,%7.3f,  %d,%7.3f,%10.6f,%9.1f,%9.1f,%9.1f,%10.6f,%5.1f,\n",
                    cp.pubList.voc,  sat_voc(Sen->Tbatt), temp_lim, model_saturated_, Sen->Ishunt, d_delta_q, *rp_delta_q_, q_, q_capacity_, soc_, SOC_);

    // Save and return
    *rp_t_last_ = temp_lim;
    return ( soc_ );
}

// Load states from retained memory
void BatteryModel::load()
{
    apply_cap_scale(*rp_s_cap_model_);
}

// Print
void BatteryModel::pretty_print(void)
{
    Serial.printf("BatteryModel::");
    this->Battery::pretty_print();
    Serial.printf(" BatteryModel::BatteryModel:\n");
    Serial.printf("  sat_ib_max_ =       %7.3f;  // Current cutback to be applied to modeled ib output, A\n", sat_ib_max_);
    Serial.printf("  sat_ib_null_ =      %7.3f;  // Current cutback value for voc=vsat, A\n", sat_ib_null_);
    Serial.printf("  sat_cutback_gain_ = %7.1f;  // Gain to retard ib when voc exceeds vsat, dimensionless\n", sat_cutback_gain_);
    Serial.printf("  model_cutback_ =          %d;  // Gain to retard ib when voc exceeds vsat, dimensionless\n", model_cutback_);
    Serial.printf("  model_saturated_ =        %d;  // Modeled current being limited on saturation cutback, T = cutback limited\n", model_saturated_);
    Serial.printf("  ib_sat_ =           %7.3f;  // Indicator of maximal cutback, T = cutback saturated\n", ib_sat_);
    Serial.printf("  s_cap_ =            %7.3f;  // Rated capacity scalar\n", s_cap_);
}


Hysteresis::Hysteresis()
: disabled_(false), res_(0), soc_(0), ib_(0), ioc_(0), voc_in_(0), voc_out_(0), dv_hys_(0), dv_dot_(0), tau_(0),
   direx_(0), cap_init_(0){};
Hysteresis::Hysteresis(const double cap, const double direx)
: disabled_(false), res_(0), soc_(0), ib_(0), ioc_(0), voc_in_(0), voc_out_(0), dv_hys_(0), dv_dot_(0), tau_(0),
   direx_(direx), cap_init_(cap)
{
    // Characteristic table
    hys_T_ = new TableInterp2D(n_h, m_h, x_dv, y_soc, t_r);

    // Disabled logic
    disabled_ = rp.hys_scale < 1e-5;

    // Capacitance logic
    if ( disabled_ ) cap_ = cap;
    else cap_ = cap / rp.hys_scale;    // maintain time constant = R*C
}

// Apply scale
void Hysteresis::apply_scale(const double scale)
{
    rp.hys_scale = max(scale, 1e-6);
    disabled_ = rp.hys_scale < 1e-5;
    if ( disabled_ )
    {
        cap_ = cap_init_;
        init(0.);
    }
    else cap_ = cap_init_ / rp.hys_scale;    // maintain time constant = R*C
}

// Calculate
double Hysteresis::calculate(const double ib, const double voc_stat, const double soc)
{
    ib_ = ib;
    voc_in_ = voc_stat;
    soc_ = soc;

    // Calculate
    if ( disabled_ )
    {
        res_ = 0.;
        ioc_ = ib;
        dv_dot_ = 0.;
    }
    else
    {
        res_ = look_hys(dv_hys_, soc_);
        ioc_ = dv_hys_ / res_;
        dv_dot_ = (ib_ - dv_hys_/res_) / cap_;  // Capacitor ode
    }
    return ( dv_dot_ );
}

// Initialize
void Hysteresis::init(const double dv_init)
{
    dv_hys_ = dv_init;
}

// Table lookup
double Hysteresis::look_hys(const double dv, const double soc)
{
    double res;         // return value
    if ( disabled_ )
        res = 0.;
    else
        res = hys_T_->interp(dv/rp.hys_scale, soc) * rp.hys_scale;
    return res;
}

// Print
void Hysteresis::pretty_print()
{
    Serial.printf("Hysteresis:\n");
    Serial.printf("  res_ =         %6.4f;  // Null resistance, Ohms\n", res_);
    Serial.printf("  cap_init_ =%10.1f;  // Capacitance, Farads\n", cap_init_);
    Serial.printf("  cap_ =     %10.1f;  // Capacitance, Farads\n", cap_);
    double res = look_hys(0., 0.8);
    Serial.printf("  tau_ =     %10.1f;  // Null time constant, sec\n", res*cap_);
    Serial.printf("  ib_ =         %7.3f;  // Current in, A\n", ib_);
    Serial.printf("  ioc_ =        %7.3f;  // Current out, A\n", ioc_);
    Serial.printf("  voc_in_ =     %7.3f;  // Voltage input, V\n", voc_in_);
    Serial.printf("  voc_out_ =    %7.3f;  // Voltage output, V\n", voc_out_);
    Serial.printf("  soc_ =        %7.3f;  // State of charge input, dimensionless\n", soc_);
    Serial.printf("  res_ =        %7.3f;  // Variable resistance value, ohms\n", res_);
    Serial.printf("  dv_dot_ =     %7.3f;  // Calculated voltage rate, V/s\n", dv_dot_);
    Serial.printf("  dv_hys_ =     %7.3f;  // Delta voltage state, V\n", dv_hys_);
    Serial.printf("  disabled_ =         %d;  // Hysteresis disabled by low scale input < 1e-5, T=disabled\n", disabled_);
    Serial.printf("  direx_  =          %2.0f;  // If hysteresis hooked up backwards, -1.=reversed\n", direx_);
    Serial.printf("  rp.hys_scale=  %6.2f;  // Scalar on hysteresis, dimensionless\n", rp.hys_scale);
}

// Scale
double Hysteresis::scale()
{
    return rp.hys_scale*direx_;
}

// Dynamic update
double Hysteresis::update(const double dt)
{
    dv_hys_ += dv_dot_ * dt;
    voc_out_ = voc_in_ + dv_hys_*direx_;
    return voc_out_;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* C <- A * B */
void mulmat(double * a, double * b, double * c, int arows, int acols, int bcols)
{
    int i, j,l;

    for(i=0; i<arows; ++i)
        for(j=0; j<bcols; ++j)
        {
            c[i*bcols+j] = 0;
            for(l=0; l<acols; ++l)
                c[i*bcols+j] += a[i*acols+l] * b[l*bcols+j];
        }
}
void mulvec(double * a, double * x, double * y, int m, int n)
{
    int i, j;

    for(i=0; i<m; ++i)
    {
        y[i] = 0;
        for(j=0; j<n; ++j)
            y[i] += x[j] * a[i*n+j];
    }
}

/* Calculate saturation voltage
    Inputs:
        temp_c  Battery temperature, deg C
        batt_vsat   Battery nominal saturation voltage, constant from Battery.h, V
        dvoc_dt Battery saturation sensitivity with temperature, V/deg C
    Outputs:
        sat_voc Battery saturation open circuit voltage, V
*/
double sat_voc(const double temp_c)
{
    return ( batt_vsat + (temp_c-25.)*dvoc_dt );
}

/* Calculate saturation status
    Inputs:
        temp_c  Battery temperature, deg C
        voc     Battery open circuit voltage, V
    Outputs:
        is_saturated Battery saturation status, T/F
*/
boolean is_sat(const double temp_c, const double voc, const double soc)
{
    double vsat = sat_voc(temp_c);
    return ( temp_c > low_t && (voc >= vsat || soc >= mxeps_bb) );
}

// Saturated voltage calculation
double calc_vsat(const double temp_c)
{
    return ( sat_voc(temp_c) );
}
