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
extern PublishPars pp;            // For publishing

// class Battery
// constructors
Battery::Battery() {}
Battery::Battery(double *rp_delta_q, float *rp_t_last, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code, float *rp_hys_scale)
    : Coulombs(rp_delta_q, rp_t_last, (RATED_BATT_CAP*3600), RATED_TEMP, T_RLIM, rp_mod_code, COULOMBIC_EFF),
    sr_(1), rp_nP_(rp_nP), rp_nS_(rp_nS)
{
    nom_vsat_   = chem_.v_sat - HDB_VBATT;   // Center in hysteresis
    hys_ = new Hysteresis(chem_.hys_cap, chem_, rp_hys_scale);
}
Battery::~Battery() {}
// operators
// functions

// Placeholder; not used.  May write this base version if needed using BatterySim::calculate()
// as a starting point but use the base class Randles formulation and re-arrange the i/o for that model.
double Battery::calculate(const float temp_C, const double q, const double curr_in, const double dt,
    const boolean dc_dc_on) { return 0.;}

/* calc_soc_voc:  VOC-OCV model
    INPUTS:
        soc         Fraction of saturation charge (q_capacity_) available (0-1) 
        temp_c      Battery temperature, deg C
    OUTPUTS:
        dv_dsoc     Derivative scaled, V/fraction
        voc_stat    Static model open circuit voltage from table (reference), V
*/
double Battery::calc_soc_voc(const double soc, const float temp_c, double *dv_dsoc)
{
    double voc_soc_stat;  // return value
    *dv_dsoc = calc_soc_voc_slope(soc, temp_c);
    voc_soc_stat = chem_.voc_T_->interp(soc, temp_c) + chem_.dvoc;
    return (voc_soc_stat);
}

/* calc_soc_voc_slope:  Derivative model read from tables
    INPUTS:
        soc         Fraction of saturation charge (q_capacity_) available (0-1) 
        temp_c      Battery temperature, deg C
    OUTPUTS:
        dv_dsoc     Derivative scaled, V/fraction
*/
double Battery::calc_soc_voc_slope(const double soc, const float temp_c)
{
    double dv_dsoc;  // return value
    if ( soc > 0.5 )
        dv_dsoc = (chem_.voc_T_->interp(soc, temp_c) - chem_.voc_T_->interp(soc-0.01, temp_c)) / 0.01;
    else
        dv_dsoc = (chem_.voc_T_->interp(soc+0.01, temp_c) - chem_.voc_T_->interp(soc, temp_c)) / 0.01;
    return (dv_dsoc);
}

/* calc_vsat: Saturated voltage calculation
    INPUTS:
        nom_vsat_   Nominal saturation threshold at 25C, V
        temp_c_     Battery temperature, deg C
        dvoc_dt     Change of VOC with operating temperature in range 0 - 50 C V/deg C
    OUTPUTS:
        vsat        Saturation threshold at temperature, deg C
*/  
double_t Battery::calc_vsat(void)
{
    return ( nom_vsat_ + (temp_c_-25.)*chem_.dvoc_dt );
}

// Initialize
// Works in 12 V batteryunits.   Scales up/down to number of series/parallel batteries on output/input.
void Battery::init_battery(const boolean reset, Sensors *Sen)
{
    if ( !reset ) return;
    vb_ = Sen->Vb / (*rp_nS_);
    ib_ = Sen->Ib / (*rp_nP_);
    ib_ = max(min(ib_, IMAX_NUM), -IMAX_NUM);  // Overflow protection when ib_ past value used
    if ( isnan(vb_) ) vb_ = 13.;    // reset overflow
    if ( isnan(ib_) ) ib_ = 0.;     // reset overflow
    double u[2] = {ib_, vb_};
    // if ( rp.debug==8 || rp.debug==7 ) Serial.printf("init_battery: Randles to %7.3f, %7.3f\n", ib_, vb_);
    Randles_->init_state_space(u);
    // if ( rp.debug==8 || rp.debug==7 ) Randles_->pretty_print();
    init_hys(0.0);
}

// Print
void Battery::pretty_print(void)
{
    Serial.printf("Battery:\n");
    Serial.printf("  temp_c=%7.3f; dg C\n", temp_c_);
    Serial.printf(" *rp_delt_q=%10.1f;  C\n", *rp_delta_q_);
    Serial.printf(" *rp_t_last=%10.1f; dg C\n", *rp_t_last_);
    Serial.printf("  dvoc_dt=%10.6f; V/dg C\n", chem_.dvoc_dt);
    Serial.printf("  r_0=%10.6f;  ohm\n", chem_.r_0);
    Serial.printf("  r_ct = %10.6f;  ohm\n", chem_.r_ct);
    Serial.printf("  tau_ct=%10.6f;  s (=1/Rct/Cct)\n", chem_.tau_ct);
    Serial.printf("  r_diff=%10.6f;  ohm\n", chem_.r_diff);
    Serial.printf("  tau_diff =%10.6f;  s (=1/Rdif/Cdif)\n", chem_.tau_diff);
    Serial.printf("  r_sd = %10.6f;  ohm\n", chem_.r_sd);
    Serial.printf("  tau_sd=%10.1f;  s\n", chem_.tau_sd);
    Serial.printf("  bms_off=%d;\n", bms_off_);
    Serial.printf("  dv_dsoc=%10.6f; V/frac\n", dv_dsoc_);
    Serial.printf("  ib=%7.3f; A\n", ib_);
    Serial.printf("  Ib=%7.3f; Bank, A\n", ib_*(*rp_nP_));
    Serial.printf("  vb=%7.3f; V\n", vb_);
    Serial.printf("  Vb=%7.3f; Bank, V\n", vb_*(*rp_nS_));
    Serial.printf("  voc=%7.3f; V\n", voc_);
    Serial.printf("  voc_stat=%7.3f; V\n", voc_stat_);
    Serial.printf("  vsat=%7.3f; V\n", vsat_);
    Serial.printf("  dv_dyn= %7.3f; V\n", dv_dyn_);
    Serial.printf("  sr=%7.3f; sclr\n", sr_);
    Serial.printf("  dt=%7.3f; s\n", dt_);
    Serial.printf(" *rp_nP=%5.2f; P bank, e.g. '2P1S'\n", *rp_nP_);
    Serial.printf(" *rp_nS=%5.2f; S bank, e.g. '2P1S'\n", *rp_nS_);
}

// Print State Space
void Battery::pretty_print_ss(void)
{
    Randles_->pretty_print();
    Serial.printf("  ::"); hys_->pretty_print();
}

// EKF model for update
double Battery::voc_soc_tab(const double soc, const float temp_c)
{
    double voc_stat;     // return value
    double dv_dsoc;
    voc_stat = calc_soc_voc(soc, temp_c, &dv_dsoc);
    return ( voc_stat );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery monitor class
BatteryMonitor::BatteryMonitor(): Battery() {}
BatteryMonitor::BatteryMonitor(double *rp_delta_q, float *rp_t_last, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code, float *rp_hys_scale):
    Battery(rp_delta_q, rp_t_last, rp_nP, rp_nS, rp_mod_code, rp_hys_scale)
{
    voc_filt_ = chem_.v_sat-HDB_VBATT;
    // EKF
    this->Q_ = EKF_Q_SD_NORM*EKF_Q_SD_NORM;
    this->R_ = EKF_R_SD_NORM*EKF_R_SD_NORM;
    // Randles dynamic model for EKF, forward version based on sensor inputs {ib, vb} --> {voc}, ioc=ib
    // Resistance values add up to same resistance loss as matched to installed battery
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    int rand_n = 2; // Rows A and B
    int rand_p = 2; // Col B    
    int rand_q = 1; // Row C and D
    rand_A_ = new double [rand_n*rand_n];
    rand_B_ = new double [rand_n*rand_p];
    rand_C_ = new double [rand_q*rand_n];
    rand_D_ = new double [rand_q*rand_p];
    assign_randles();
    Randles_ = new StateSpace(rand_A_, rand_B_, rand_C_, rand_D_, rand_n, rand_p, rand_q);
    SdVb_ = new SlidingDeadband(HDB_VBATT);  // Noise filter
    EKF_converged = new TFDelay(false, EKF_T_CONV, EKF_T_RESET, EKF_NOM_DT); // Convergence test debounce.  Initializes false
}
BatteryMonitor::~BatteryMonitor() {}

// operators
// functions

// BatteryMonitor::assign_randles:    Assign constants from battery chemistry to arrays for state space model
void BatteryMonitor::assign_randles(void)
{
    rand_A_[0] = -1./chem_.tau_ct;
    rand_A_[1] = 0.;
    rand_A_[2] = 0.;
    rand_A_[3] = -1/chem_.tau_diff;
    rand_B_[0] = 1./(chem_.tau_ct / chem_.r_ct);
    rand_B_[1] = 0.;
    rand_B_[2] = 1./(chem_.tau_diff / chem_.r_diff);
    rand_B_[3] = 0.;
    rand_C_[0] = -1.;
    rand_C_[1] = -1.;
    rand_D_[0] = -chem_.r_0;
    rand_D_[1] = 1.;
}

/* BatteryMonitor::calculate:  SOC-OCV curve fit solved by ekf.   Works in 12 V
   battery units.  Scales up/down to number of series/parallel batteries on output/input.
        Inputs:
        Sen->Tb_filt    Tb filtered for noise, past value of temp_c_, deg C
        Sen->Vb         Battery terminal voltage, V
        Sen->Ib         Shunt current Ib, A
        Sen->T          Update time, sec
        q_capacity_     Saturation charge at temperature, C
        q_cap_rated_scaled_   Applied rated capacity at t_rated_, after scaling, C
        RATED_BATT_CAP  Nominal battery bank capacity (assume actual not known), Ah
    Outputs:
        vsat_           Saturation threshold at temperature, V
        voc_            Charging voltage estimated from Vb and RC model, V
        dv_dyn_         ib-induced back emf, V
        voc_filt_       Filtered open circuit voltage for saturation detect, V
        ioc_            Best estimate of IOC charge current after hysteresis, A
        bms_off_        Calculated indication that the BMS has turned off charge current, T=off
        voc_stat_       Static model open circuit voltage from table (reference), V\n
        Tb              Tb, deg C
        ib_             Battery terminal current, A
        vb_             Battery terminal voltage, V
        rp.inj_bias  Used to inject fake shunt current
        soc_ekf_        Solved state of charge, fraction
        q_ekf_          Filtered charge calculated by ekf, C
        soc_ekf_ (return)     Solved state of charge, fraction
        tcharge_ekf_    Solved charging time to full from ekf, hr
        y_filt_         Filtered EKF y residual value, V

                <---ib        ______________         <---ib
                 voc          |             |
       -----------+-----------|   RANDLES   |----------+-----------+ vb
       |                      |_____________|
       |
    ___|___
    |      |
    | HYS  |   |              HYS stores charge ib-ioc
    |______|   |
       |       v  ioc ~= ib
       +   voc_stat
       |
    ___|____  voc_soc
    |  +    |   ^
    | Batt  |   |
    |  -    |   |             Batt stores charge ioc
    |_______|
        |
        |                      Total charge storage ib-ioc + ioc = ib
        _
        -
        gnd
*/ 
double BatteryMonitor::calculate(Sensors *Sen, const boolean reset)
{
    // Inputs
    temp_c_ = Sen->Tb_filt;
    vsat_ = calc_vsat();
    dt_ =  min(Sen->T, F_MAX_T);
    double T_rate = T_RLim->calculate(temp_c_, T_RLIM, T_RLIM, reset, Sen->T);

    // Dynamic emf
    vb_ = Sen->Vb / (*rp_nS_);
    ib_ = Sen->Ib / (*rp_nP_);
    ib_ = max(min(ib_, IMAX_NUM), -IMAX_NUM);  // Overflow protection when ib_ past value used
    double u[2] = {ib_, vb_};
    Randles_->calc_x_dot(u);
    if ( dt_<=RANDLES_T_MAX )
    {
        Randles_->update(dt_);
        voc_ = Randles_->y(0);
    }
    else    // aliased, unstable if T<0.5  TODO:  consider deleting Randles model (hardware filters)
        voc_ = vb_ - chem_.r_ss * ib_;
    // if ( rp.debug==35 )
    // {
    //     Serial.printf("BatteryMonitor::calculate:"); Randles_->pretty_print();
    // }
    dv_dyn_ = vb_ - voc_;
    // Hysteresis model
    hys_->calculate(ib_, soc_);
    dv_hys_ = hys_->update(dt_);
    voc_soc_ = voc_soc_tab(soc_, Sen->Tb_filt);
    voc_stat_ = voc_ - dv_hys_;
    voc_filt_ = SdVb_->update(voc_);
    ioc_ = hys_->ioc();
    bms_off_ = temp_c_ <= chem_.low_t || ( vb_<VBATT_MIN && !Sen->Flt->vb_fa() );    // KISS
    if ( bms_off_ )
    {
        voc_stat_ = voc_soc_;
        ib_ = 0.;
        voc_ = voc_stat_;
        dv_dyn_ = 0.;
        voc_filt_ = voc_stat_;
    }
    if ( Sen->Flt->vb_fa() )
    {
        voc_ = voc_stat_ = voc_filt_ = voc_soc_;
        dv_dyn_ = dv_hys_ = 0;
    }

    // EKF 1x1
    double ddq_dt = ib_;
    if ( ddq_dt>0. && !rp.tweak_test() ) ddq_dt *= coul_eff_ * Sen->sclr_coul_eff;
    ddq_dt -= chem_.dqdt * q_capacity_ * T_rate;
    predict_ekf(ddq_dt);       // u = d(dq)/dt
    update_ekf(voc_stat_, 0., 1.);  // z = voc_stat, estimated = voc_filtered = hx, predicted = est past
    soc_ekf_ = x_ekf();             // x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
    q_ekf_ = soc_ekf_ * q_capacity_;
    delta_q_ekf_ = q_ekf_ - q_capacity_;

    // Normalize
    soc_ = q_ / q_capacity_;

    y_filt_ = y_filt->calculate(y_, min(Sen->T, EKF_T_RESET));
    if ( rp.debug==7 )
        Serial.printf("calculate:Tb_f,ib,count,soc_ekf,vb,voc,voc_m_s,dv_dyn,dv_hys,err, %7.3f,%7.3f,  %d,%8.4f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%10.6f,\n",
            Sen->Tb_filt, Sen->Ib, 0, soc_ekf_, vb_, voc_, hx_, dv_dyn_, dv_hys_, y_);

    // EKF convergence.  Audio industry found that detection of quietness requires no more than
    // second order filter of the signal.   Anything more is 'gilding the lily'
    boolean conv = abs(y_filt_)<EKF_CONV && !cp.soft_reset;  // Initialize false
    EKF_converged->calculate(conv, EKF_T_CONV, EKF_T_RESET, min(Sen->T, EKF_T_RESET), cp.soft_reset);

    // if ( rp.debug==34 || rp.debug==7 )
    //     Serial.printf("BatteryMonitor:dt,ib,voc_stat_tab,voc_stat,voc,voc_filt,dv_dyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf, y_ekf_f, soc, conv,  %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f,%7.4f, %7.4f,  %d,\n",
    //         dt_, ib_, voc_soc_, voc_stat_, voc_, voc_filt_, dv_dyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_, y_filt_, soc_, converged_ekf());
    if ( rp.debug==37 )
        Serial.printf("BatteryMonitor:ib,vb,voc_stat,voc(z_),  K_,y_,soc_ekf, y_ekf_f, conv,  %7.3f,%7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f,%7.4f,  %d,\n",
            ib_, vb_, voc_stat_, voc_,     K_, y_, soc_ekf_, y_filt_, converged_ekf());

    // Charge time if used ekf 
    if ( ib_ > 0.1 )
        tcharge_ekf_ = min(RATED_BATT_CAP / ib_ * (1. - soc_ekf_), 24.);
    else if ( ib_ < -0.1 )
        tcharge_ekf_ = max(RATED_BATT_CAP / ib_ * soc_ekf_, -24.);
    else if ( ib_ >= 0. )
        tcharge_ekf_ = 24.*(1. - soc_ekf_);
    else 
        tcharge_ekf_ = -24.*soc_ekf_;

    return ( soc_ekf_ );
}

// Charge time calculation
double BatteryMonitor::calc_charge_time(const double q, const double q_capacity, const double charge_curr, const double soc)
{
    double delta_q = q - q_capacity;
    if ( charge_curr > TCHARGE_DISPLAY_DEADBAND )
        tcharge_ = min( -delta_q / charge_curr / 3600., 24.);
    else if ( charge_curr < -TCHARGE_DISPLAY_DEADBAND )
        tcharge_ = max( (q_capacity + delta_q - q_min_) / charge_curr / 3600., -24.);
    else if ( charge_curr >= 0. )
        tcharge_ = 24.;
    else
        tcharge_ = -24.;

    double amp_hrs_remaining = (q_capacity - q_min_ + delta_q) / 3600.;
    if ( soc > soc_min_)
    {
        amp_hrs_remaining_ekf_ = amp_hrs_remaining * (soc_ekf_ - soc_min_) / (soc - soc_min_);
        amp_hrs_remaining_soc_ = amp_hrs_remaining * (soc_ - soc_min_) / (soc - soc_min_);
    }
    else if ( soc < soc_min_)
    {
        amp_hrs_remaining_ekf_ = amp_hrs_remaining * (soc_ekf_ - soc_min_) / (soc - soc_min_);
        amp_hrs_remaining_soc_ = amp_hrs_remaining * (soc_ - soc_min_) / (soc - soc_min_);
    }
    else
    {
        amp_hrs_remaining_ekf_ = 0;
        amp_hrs_remaining_soc_ = 0;
    }

    return( tcharge_ );
}

// EKF model for predict
void BatteryMonitor::ekf_predict(double *Fx, double *Bu)
{
    // Process model
    *Fx = exp(-dt_ / chem_.tau_sd);
    *Bu = (1. - *Fx) * chem_.r_sd;
}

// EKF model for update
void BatteryMonitor::ekf_update(double *hx, double *H)
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
}

/* is_sat:  Calculate saturation status
    Inputs:
        temp_c      Battery temperature, deg C
        voc_filt    Filtered battery charging voltage, V
    Outputs:
        is_saturated    Battery saturation status, T/F
*/
boolean BatteryMonitor::is_sat(const boolean reset)
{
    if ( reset)
        return ( temp_c_ > chem_.low_t && voc_filt_ >= vsat_ );
    else
        return ( temp_c_ > chem_.low_t && (voc_filt_ >= vsat_ || soc_ >= MXEPS) );
}

// Print
void BatteryMonitor::pretty_print(void)
{
    Serial.printf("BM::");
    this->Battery::pretty_print();
    Serial.printf(" BM::BM:\n");
    Serial.printf("  ah_rem_ekf_=%7.3f; A-h\n", amp_hrs_remaining_ekf_);
    Serial.printf("  ah_rem_soc_=%7.3f; A-h\n", amp_hrs_remaining_soc_);
    Serial.printf("  EKF_conv=%d;\n", converged_ekf());
    Serial.printf("  q_ekf=%10.1f; C\n", q_ekf_);
    Serial.printf("  soc_ekf=%8.4f; frac\n", soc_ekf_);
    Serial.printf("  tcharge=%5.1f; hr\n", tcharge_);
    Serial.printf("  tcharge_ekf=%5.1f; hr\n", tcharge_ekf_);
    Serial.printf("  voc_filt=%7.3f; V\n", voc_filt_);
    Serial.printf("  voc_stat=%7.3f; V\n", voc_stat_);
    Serial.printf("  y_filt=%7.3f; Res EKF, V\n", y_filt_);
}

// Reset Coulomb Counter to EKF under restricted conditions especially new boot no history of saturation
void BatteryMonitor::regauge(const float temp_c)
{
    if ( converged_ekf() && abs(soc_ekf_-soc_)>DF2 )
    {
        Serial.printf("Resetting Coulomb Counter Monitor from %7.3f to EKF=%7.3f...", soc_, soc_ekf_);
        apply_soc(soc_ekf_, temp_c);
        Serial.printf("confirmed %7.3f\n", soc_);
    }
}

/* Steady state voc(soc) solver for initialization of ekf state.  Expects Sen->Tb_filt to be in reset mode
    INPUTS:
        Sen->Vb      
        Sen->Ib
    OUTPUTS:
        Mon->soc_ekf
*/
boolean BatteryMonitor::solve_ekf(const boolean reset, Sensors *Sen)
{
    // Average dynamic inputs through the initialization period before apply EKF
    static float Tb_avg = Sen->Tb_filt;
    static float Vb_avg = Sen->Vb;
    static float Ib_avg = Sen->Ib;
    static uint16_t n_avg = 0;
    if ( !reset )
    {
        Tb_avg = Sen->Tb_filt;
        Vb_avg = Sen->Vb;
        Ib_avg = Sen->Ib;
        n_avg = 0;
        return ( true );
    }
    else
    {
        n_avg++;
        Tb_avg = (Tb_avg*float(n_avg-1) + Sen->Tb_filt) / float(n_avg);
        Vb_avg = (Vb_avg*float(n_avg-1) + Sen->Vb) / float(n_avg);
        Ib_avg = (Ib_avg*float(n_avg-1) + Sen->Ib) / float(n_avg);
    }

    // Solver, steady
    const double meps = 1-1e-6;
    double vb = Vb_avg/(*rp_nS_);
    double dv_dyn = Ib_avg/(*rp_nP_)*chem_.r_ss;
    double voc =  vb - dv_dyn;
    double dv_hys_ = 0.;
    int8_t count = 0;
    static double soc_solved = 1.0;
    double dv_dsoc;
    double voc_solved = calc_soc_voc(soc_solved, Tb_avg, &dv_dsoc);
    double err = voc - voc_solved;
    while( abs(err)>SOLV_ERR && count++<SOLV_MAX_COUNTS )
    {
        soc_solved = max(min(soc_solved + max(min( err/dv_dsoc, SOLV_MAX_STEP), -SOLV_MAX_STEP), meps), 1e-6);
        voc_solved = calc_soc_voc(soc_solved, Tb_avg, &dv_dsoc);
        err = voc - voc_solved;
    }
    init_soc_ekf(soc_solved);
    if ( rp.debug==7 )
            Serial.printf("solve    :n_avg, Tb_avg,Vb_avg,Ib_avg,  count,soc_s,vb_avg,voc,voc_m_s,dv_dyn,dv_hys,err, %d, %7.3f,%7.3f,%7.3f,  %d,%8.4f,%7.3f,%7.3f,%7.3f,%7.3f,%10.6f,\n",
            n_avg, Tb_avg, Vb_avg, Ib_avg, count, soc_solved, voc, voc_solved, dv_dyn, dv_hys_, err);
    return ( count<SOLV_MAX_COUNTS );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery model class for reference use mainly in jumpered hardware testing
BatterySim::BatterySim() : Battery() {}
BatterySim::BatterySim(double *rp_delta_q, float *rp_t_last, float *rp_s_cap_model, float *rp_nP, float *rp_nS, uint8_t *rp_mod_code, float *rp_hys_scale) :
    Battery(rp_delta_q, rp_t_last, rp_nP, rp_nS, rp_mod_code, rp_hys_scale), q_(RATED_BATT_CAP*3600.), rp_s_cap_model_(rp_s_cap_model)
{
    // Randles dynamic model for EKF
    // Resistance values add up to same resistance loss as matched to installed battery
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    Sin_inj_ = new SinInj();
    Sq_inj_ = new SqInj();
    Tri_inj_ = new TriInj();
    Cos_inj_ = new CosInj();
    sat_ib_null_ = 0.;          // Current cutback value for soc=1, A
    sat_cutback_gain_ = 1000.;  // Gain to retard ib when soc approaches 1, dimensionless
    model_saturated_ = false;
    ib_sat_ = 0.5;              // deadzone for cutback actuation, A
    // Randles dynamic model for EKF, backward version based on model {ib, voc} --> {vb}, ioc=ib
    // Resistance values add up to same resistance loss as matched to installed battery
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    int rand_n = 2; // Rows A and B
    int rand_p = 2; // Col B    
    int rand_q = 1; // Row C and D
    rand_A_ = new double [rand_n*rand_n];
    rand_B_ = new double [rand_n*rand_p];
    rand_C_ = new double [rand_q*rand_n];
    rand_D_ = new double [rand_q*rand_p];
    assign_randles();
    Randles_ = new StateSpace(rand_A_, rand_B_, rand_C_, rand_D_, rand_n, rand_p, rand_q);
}

// BatterySim::assign_randles:    Assign constants from battery chemistry to arrays for state space model
void BatterySim::assign_randles(void)
{
    rand_A_[0] = -1./chem_.tau_ct;
    rand_A_[1] = 0.;
    rand_A_[2] = 0.;
    rand_A_[3] = -1/chem_.tau_diff;
    rand_B_[0] = 1./(chem_.tau_ct / chem_.r_ct);
    rand_B_[1] = 0.;
    rand_B_[2] = 1./(chem_.tau_diff / chem_.r_diff);
    rand_B_[3] = 0.;
    rand_C_[0] = 1.;
    rand_C_[1] = 1.;
    rand_D_[0] = chem_.r_0;
    rand_D_[1] = 1.;
}

/* BatterySim::calculate:  Sim SOC-OCV table with a Battery Management System (BMS) and hysteresis.
// Makes a good reference model. Intervenes in sensor path to provide Mon with inputs
// when running simulations.  Never used for anything during normal operation.  Models
// for monitoring in normal operation are in the BatteryMonitor object.   Works in 12 V battery
// units.   Scales up/down to number of series/parallel batteries on output/input.
//
//  Inputs:
//    Sen->Tb_filt      Filtered battery bank temp, C
//    Sen->Ib_model_in  Battery bank current input to model, A
//    ib_fut_(past)     Past future value of limited current, A
//    Sen->T            Update time, sec
//
//  States:
//    soc_              State of Charge, fraction
//
//  Outputs:
//    temp_c_           Simulated Tb, deg C
//    ib_fut_           Simulated over-ridden by saturation, A
//    vb_               Simulated Vb, V
//    rp.inj_bias       Used to inject fake shunt current, A

                <---ib        ______________         <---ib
                 voc          |             |
       -----------+-----------|   RANDLES   |----------+-----------+ vb
       |                      |_____________|
       |
    ___|___
    |      |
    | HYS  |   |              HYS stores charge ib-ioc
    |______|   |
       |       v  ioc ~= ib
       +   voc_stat
       |
    ___|____ voc_soc
    |  +    |   ^
    | Batt  |   |
    |  -    |   |              Batt stores charge ioc
    |_______|
        |
        |                      Total charge storage ib-ioc + ioc = ib
        _
        -
        gnd

*/
double BatterySim::calculate(Sensors *Sen, const boolean dc_dc_on, const boolean reset)
{
    temp_c_ = Sen->Tb_filt;
    double curr_in = Sen->Ib_model_in;
    const double dt = min(Sen->T, F_MAX_T);
    ib_in_ = curr_in;
    if ( reset ) ib_fut_ = ib_in_;
    ib_ = max(min(ib_fut_, IMAX_NUM), -IMAX_NUM);  //  Past value ib_.  Overflow protection when ib_ past value used

    dt_ = dt;
    vsat_ = calc_vsat();

    double soc_lim = max(min(soc_, 1.0), -0.2);  // slightly beyond

    // VOC-OCV model
    voc_stat_ = calc_soc_voc(soc_, temp_c_, &dv_dsoc_);
    voc_stat_ = min(voc_stat_ + (soc_ - soc_lim) * dv_dsoc_, vsat_*1.2);  // slightly beyond but don't windup
    bms_off_ = (( temp_c_ <= chem_.low_t ) || ( voc_stat_ < chem_.low_voc )) && !rp.tweak_test();
    if ( bms_off_ && rp.mod_ib() ) ib_in_ = 0.;  // keep running when real world.  Input ib represents bms

    // Dynamic emf
    // Hysteresis model
    hys_->calculate(ib_in_, soc_);
    dv_hys_ = hys_->update(dt);
    voc_ = voc_stat_ + dv_hys_;
    ioc_ = hys_->ioc();
    // Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
    double u[2] = {ib_, voc_};
    Randles_->calc_x_dot(u);
    if ( dt_<=RANDLES_T_MAX )
    {
        Randles_->update(dt);
        vb_ = Randles_->y(0);
    }
    else    // aliased, unstable if T<0.5.  TODO:  consider deleting Randles model (hardware filters)
        vb_ = voc_ + chem_.r_ss * ib_;
    dv_dyn_ = vb_ - voc_;
    // Special cases override
    if ( bms_off_ )
    {
        dv_dyn_ = 0.;
        voc_ = voc_stat_;
    }
    if ( bms_off_ && dc_dc_on )
    {
        vb_ = vb_dc_dc;
    }

    // Saturation logic, both full and empty
    sat_ib_max_ = sat_ib_null_ + (1. - soc_) * sat_cutback_gain_ * rp.cutback_gain_scalar;
    if ( rp.tweak_test() || !rp.modeling ) sat_ib_max_ = ib_in_;   // Disable cutback when real world or when doing tweak_test test
    ib_fut_ = min(ib_in_/(*rp_nP_), sat_ib_max_);      // the feedback of ib_
    if ( (q_ <= 0.) && (ib_in_ < 0.) ) ib_fut_ = 0.;   //  empty
    model_cutback_ = (voc_stat_ > vsat_) && (ib_fut_ == sat_ib_max_);
    model_saturated_ = model_cutback_ && (ib_fut_ < ib_sat_);
    Coulombs::sat_ = model_saturated_;

    // if ( rp.debug==75 ) Serial.printf("BatterySim::calculate: temp_c_, soc_, voc_stat_, low_voc,=  %7.3f, %10.6f, %9.5f, %7.3f,\n",
    //     temp_c_, soc_, voc_stat_, chem_.low_voc);

    // if ( rp.debug==79 ) Serial.printf("temp_c_, dvoc_dt, vsat_, voc, q_capacity, sat_ib_max, ib_fut, ib,=   %7.3f,%7.3f,%7.3f,%7.3f, %10.1f, %7.3f, %7.3f, %7.3f,\n",
    //     temp_c_, chem_.dvoc_dt, vsat_, voc_, q_capacity_, sat_ib_max_, ib_fut_, ib_);

    if ( rp.debug==78 || rp.debug==7 ) Serial.printf("BatterySim::calculate:,  dt,tempC,curr,soc_,voc,,dv_dyn,vb,%7.3f,%7.3f,%7.3f,%8.4f,%7.3f,%7.3f,%7.3f,\n",
     dt,temp_c_, ib_, soc_, voc_, dv_dyn_, vb_);
    
    if ( rp.debug==76 ) Serial.printf("BatterySim::calculate:,  soc=%8.4f, temp_c_=%7.3f, ib_in=%7.3f,ib=%7.3f, voc_stat=%7.3f, voc=%7.3f, vsat=%7.3f, model_saturated=%d, bms_off=%d, dc_dc_on=%d, vb_dc_dc=%7.3f, vb=%7.3f\n",
        soc_, temp_c_, ib_in_, ib_, voc_stat_, voc_, vsat_, model_saturated_, bms_off_, dc_dc_on, vb_dc_dc, vb_);

    return ( vb_*(*rp_nS_) );
}

// Injection model, calculate inj bias based on time since boot
float BatterySim::calc_inj(const unsigned long now, const uint8_t type, const double amp, const double freq)
{
    // Return if time 0
    if ( now== 0UL )
    {
        duty_ = 0UL;
        rp.inj_bias = 0.;
        return(duty_);
    }

    // Injection.  time shifted by 1UL
    double t = (now-1UL)/1e3;
    double sin_bias = 0.;
    double square_bias = 0.;
    double tri_bias = 0.;
    double inj_bias = 0.;
    double bias = 0.;
    double cos_bias = 0.;
    // Calculate injection amounts from user inputs (talk).
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
    if ( type!=3 ) rp.inj_bias = inj_bias - rp.amp;
    else rp.inj_bias = inj_bias;

    return ( rp.inj_bias );
}

/* BatterySim::count_coulombs: Count coulombs based on assumed model true=actual capacity.
    Uses Tb instead of Tb_filt to be most like hardware and provide independence from application.
Inputs:
    model_saturated Indicator of maximal cutback, T = cutback saturated
    Sen->T          Integration step, s
    Sen->Tb         Battery bank temperature, deg C
    Sen->Ib         Selected battery bank current, A
    t_last          Past value of battery temperature used for rate limit memory, deg C
    coul_eff_       Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
States:
    *rp_delta_q_    Charge change since saturated, C
    *rp_t_last_     Updated value of battery temperature used for rate limit memory, deg C
    soc_            Fraction of saturation charge (q_capacity_) available (0-1) 
Outputs:
    q_capacity_     Saturation charge at temperature, C
    resetting_      Sticky flag for initialization, T=reset
    soc_min_        Estimated soc where battery BMS will shutoff current, fraction
    q_min_          Estimated charge at low voltage shutdown, C\
*/
double BatterySim::count_coulombs(Sensors *Sen, const boolean reset, BatteryMonitor *Mon) 
{
    // float charge_curr = Sen->Ib / (*rp_nP_); TODO:  re-run Xp10 with this change
    float charge_curr = ib_in_;
    double d_delta_q = charge_curr * Sen->T;
    if ( charge_curr>0. ) d_delta_q *= coul_eff_;

    // Rate limit temperature
    if ( reset ) *rp_t_last_ = Sen->Tb;
    double temp_lim = max(min(Sen->Tb, *rp_t_last_ + T_RLIM*Sen->T), *rp_t_last_ - T_RLIM*Sen->T);
    
    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status
    // But if not modeling in real world, set to Monitor when Monitor saturated and reset to EKF otherwise
    static boolean reset_past = reset;   // needed because model called first in reset path; need to pick up latest
    if ( !rp.mod_vb() )  // Real world
    {
        if ( Mon->sat() ) apply_delta_q(Mon->delta_q());
        else if ( reset_past ) apply_delta_q(Mon->delta_q_ekf());  // Solution to boot up unsaturated
    }
    else if ( model_saturated_ )  // Modeling
    {
        if ( reset ) *rp_delta_q_ = 0.;
    }
    reset_past = reset;
    resetting_ = false;     // one pass flag

    // Integration can go to -10%
    q_capacity_ = calculate_capacity(temp_lim);
    if ( !reset )
    {
        *rp_delta_q_ += d_delta_q - chem_.dqdt*q_capacity_*(temp_lim-*rp_t_last_);
        *rp_delta_q_ = max(min(*rp_delta_q_, 0.), -q_capacity_*1.2);
    }
    q_ = q_capacity_ + *rp_delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    soc_min_ = chem_.soc_min_T_->interp(temp_lim);
    q_min_ = soc_min_ * q_capacity_;

    if ( rp.debug==26 && cp.publishS ) // print_serial_sim
    {
        double cTime;
        if ( rp.tweak_test() ) cTime = double(Sen->now)/1000.;
        else cTime = Sen->control_time;
        sprintf(cp.buffer, "unit_sim, %13.3f, %d, %7.5f,%7.5f, %7.5f,%7.5f,%7.5f,%7.5f, %7.3f,%7.3f,%7.3f,  %d,  %9.1f,  %8.5f, %d, %c",
            cTime, rp.sim_mod,  Sen->Tb, temp_lim, vsat_, voc_stat_, dv_dyn_, vb_, ib_, ib_in_, ioc_, model_saturated_, *rp_delta_q_, soc_, reset,'\0');
        Serial.println(cp.buffer);
    }

    // Save and return
    *rp_t_last_ = temp_lim;
    return ( soc_ );
}

// Load states from retained memory
void BatterySim::load()
{
    apply_cap_scale(*rp_s_cap_model_);
}

// Print
void BatterySim::pretty_print(void)
{
    Serial.printf("BS::");
    this->Battery::pretty_print();
    Serial.printf(" BS::BS:\n");
    Serial.printf("  sat_ib_max=%7.3f; A\n", sat_ib_max_);
    Serial.printf("  sat_ib_null=%7.3f; A\n", sat_ib_null_);
    Serial.printf("  sat_cutback_gain=%7.1f; voc>vsat\n", sat_cutback_gain_);
    Serial.printf("  model_cutback=%d;\n", model_cutback_);
    Serial.printf("  model_saturated=%d;\n", model_saturated_);
    Serial.printf("  ib=%7.3f; A\n", ib_);
    Serial.printf("  ib_in=%7.3f; Saved, A\n", ib_in_);
    Serial.printf("  ib_fut=%7.3f; A\n", ib_fut_);
    Serial.printf("  ib_sat=%7.3f;\n", ib_sat_);
    Serial.printf(" *rp_s_cap_model=%7.3f; Slr\n", *rp_s_cap_model_);
}


Hysteresis::Hysteresis()
: disabled_(false), cap_(0), res_(0), soc_(0), ib_(0), ioc_(0), dv_hys_(0), dv_dot_(0), tau_(0){};
Hysteresis::Hysteresis(const double cap, Chemistry chem, float *rp_hys_scale)
: disabled_(false), cap_(cap), res_(0), soc_(0), ib_(0), ioc_(0), dv_hys_(0), dv_dot_(0), tau_(0), rp_hys_scale_(rp_hys_scale)
{
    // Characteristic table
    hys_T_ = new TableInterp2D(chem.n_h, chem.m_h, chem.x_dv, chem.y_soc, chem.t_r);

}

// Calculate
double Hysteresis::calculate(const double ib, const double soc)
{
    ib_ = ib;
    soc_ = soc;

    // Disabled logic
    disabled_ = *rp_hys_scale_ < 1e-5;

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
        res = hys_T_->interp(dv, soc);
    return res;
}

// Print
void Hysteresis::pretty_print()
{
    Serial.printf("Hysteresis:\n");
    Serial.printf("  res=%6.4f; Null, Ohm\n", res_);
    Serial.printf("  cap=%10.1f; F\n", cap_);
    double res = look_hys(0., 0.8);
    Serial.printf("  tau=%10.1f; Null, s\n", res*cap_);
    Serial.printf("  ib=%7.3f; In, A\n", ib_);
    Serial.printf("  ioc=%7.3f; Out, A\n", ioc_);
    Serial.printf("  soc=%8.4f; Input\n", soc_);
    Serial.printf("  res=%7.3f; Var, ohm\n", res_);
    Serial.printf("  dv_dot=%7.3f; V/s\n", dv_dot_);
    Serial.printf("  dv_hys=%7.3f; Delta state, V\n", dv_hys_);
    Serial.printf("  disabled=%d; input < 1e-5\n", disabled_);
    Serial.printf(" *rp_hys_scale=%6.2f; Slr\n", *rp_hys_scale_);
    Serial.printf("  r(soc, dv):\n");
    hys_T_->pretty_print();
}

// Dynamic update
double Hysteresis::update(const double dt)
{
    dv_hys_ += dv_dot_ * dt;
    return (dv_hys_ * (*rp_hys_scale_)); // Scale on output only.   Don't retain it for feedback to ode
}

