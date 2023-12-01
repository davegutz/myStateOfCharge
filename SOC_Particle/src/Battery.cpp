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
#include "Battery.h"
#include "parameters.h"
#include <math.h>
#include "constants.h"
#include "command.h"
#include "mySubs.h"
extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern CommandPars cp;  // Various parameters to be static at system level
extern PublishPars pp;  // For publishing

// class Battery
// constructors
Battery::Battery() {}
Battery::Battery(double *sp_delta_q, float *sp_t_last, uint8_t *sp_mod_code, const float d_voc_soc)
    : Coulombs(sp_delta_q, sp_t_last, (NOM_UNIT_CAP*3600), T_RLIM, sp_mod_code, COULOMBIC_EFF_SCALE), bms_charging_(false),
	bms_off_(false), ds_voc_soc_(0), dt_(0.1), dv_dsoc_(0.3), dv_dyn_(0.), dv_hys_(0.),
    ib_(0.), ibs_(0.), ioc_(0.), print_now_(false), sr_(1.), temp_c_(NOMINAL_TB), vb_(NOMINAL_VB),
    voc_(NOMINAL_VB), voc_stat_(NOMINAL_VB), voltage_low_(false), vsat_(NOMINAL_VB)
{
    nom_vsat_   = chem_.v_sat - HDB_VB;   // Center in hysteresis
    ChargeTransfer_ = new LagExp(EKF_NOM_DT, chem_.tau_ct, -NOM_UNIT_CAP, NOM_UNIT_CAP);  // Update time and time constant changed on the fly
}
Battery::~Battery() {}
// operators
// functions

// Placeholder; not used
float Battery::calculate(const float temp_C, const float soc_frac, float curr_in, const double dt, const boolean dc_dc_on)
{
    return 0.;
}

/* calc_soc_voc:  VOC-OCV model
    INPUTS:
        soc         Fraction of saturation charge (q_capacity_) available (0-1) 
        temp_c      Battery temperature, deg C
    OUTPUTS:
        dv_dsoc     Derivative scaled, V/fraction
        voc         Static model open circuit voltage from table (reference), V
*/
float Battery::calc_soc_voc(const float soc, const float temp_c, float *dv_dsoc)
{
    float voc;  // return value
    *dv_dsoc = calc_soc_voc_slope(soc + ds_voc_soc_, temp_c);
    voc = chem_.voc_T_->interp(soc + ds_voc_soc_, temp_c);
    return voc;
}

/* calc_soc_voc_slope:  Derivative model read from tables
    INPUTS:
        soc         Fraction of saturation charge (q_capacity_) available (0-1) 
        temp_c      Battery temperature, deg C
    OUTPUTS:
        dv_dsoc     Derivative scaled, V/fraction
*/
float Battery::calc_soc_voc_slope(const float soc, const float temp_c)
{
    float dv_dsoc;  // return value
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
        rated_temp  Battery rated temperature, deg C
        dvoc_dt     Change of VOC with operating temperature in range 0 - 50 C V/deg C
    OUTPUTS:
        vsat        Saturation threshold at temperature, deg C
*/  
float_t Battery::calc_vsat(void)
{
    return ( nom_vsat_ + (temp_c_-chem_.rated_temp)*chem_.dvoc_dt );
}

// Print
void Battery::pretty_print(void)
{
#ifndef DEPLOY_PHOTON
    Serial.printf("Battery:\n");
    Serial.printf("  bms_charging %d\n", bms_charging_);
    Serial.printf("  bms_off %d\n", bms_off_);
    Serial.printf("  c_sd%9.3g, farad\n", chem_.c_sd);
    Serial.printf("  ds_voc_soc%10.6f, frac\n", ds_voc_soc_);
    Serial.printf("  dt%7.3f, s\n", dt_);
    Serial.printf("  dv_dsoc%10.6f, V/frac\n", dv_dsoc_);
    Serial.printf("  dv_dyn%7.3f, V\n", dv_dyn_);
    Serial.printf("  dvoc_dt%10.6f, V/dg C\n", chem_.dvoc_dt);
    Serial.printf("  ib%7.3f, A\n", ib_);
    Serial.printf("  r_0%10.6f, ohm\n", chem_.r_0);
    Serial.printf("  r_ct%10.6f, ohm\n", chem_.r_ct);
    Serial.printf("  r_sd%10.6f, ohm\n", chem_.r_sd);
    Serial.printf("  soc%8.4f\n", soc_);
    Serial.printf(" *sp_delt_q%10.1f, C\n", *sp_delta_q_);
    Serial.printf(" *sp_t_last%10.1f, dg C\n", *sp_t_last_);
    Serial.printf("  sr%7.3f, sclr\n", sr_);
    Serial.printf("  tau_ct%10.6f, s (=1/R/C)\n", chem_.tau_ct);
    Serial.printf("  tau_sd%9.3g, s\n", chem_.tau_sd);
    Serial.printf("  temp_c%7.3f, dg C\n", temp_c_);
    Serial.printf("  vb%7.3f, V\n", vb_);
    Serial.printf("  voc%7.3f, V\n", voc_);
    Serial.printf("  voc_stat%7.3f, V\n", voc_stat_);
    Serial.printf("  voltage_low %d, BMS will turn off\n", voltage_low_);
    Serial.printf("  vsat%7.3f, V\n", vsat_);
#else
     Serial.printf("Battery: silent DEPLOY\n");
#endif
}

// EKF model for update
float Battery::voc_soc_tab(const float soc, const float temp_c)
{
    float voc;     // return value
    float dv_dsoc;
    voc = calc_soc_voc(soc, temp_c, &dv_dsoc);
    return ( voc );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery monitor class
BatteryMonitor::BatteryMonitor():
    Battery(&sp.Delta_q_z, &sp.T_state_z, &sp.Mon_chm_z, VM),
	amp_hrs_remaining_ekf_(0.), amp_hrs_remaining_soc_(0.), dt_eframe_(0.1), eframe_(0), ib_charge_(0.), ib_past_(0.),
    q_ekf_(NOM_UNIT_CAP*3600.), soc_ekf_(1.0), tcharge_(0.), tcharge_ekf_(0.), voc_filt_(NOMINAL_VB), voc_soc_(NOMINAL_VB),
    y_filt_(0.)
{
    voc_filt_ = chem_.v_sat - HDB_VB;
    // EKF
    this->Q_ = EKF_Q_SD_NORM*EKF_Q_SD_NORM;
    this->R_ = EKF_R_SD_NORM*EKF_R_SD_NORM;
    SdVb_ = new SlidingDeadband(HDB_VB);  // Noise filter
    EKF_converged = new TFDelay(false, EKF_T_CONV, EKF_T_RESET, EKF_NOM_DT); // Convergence test debounce.  Initializes false
    ice_ = new Iterator("EKF solver");
}
BatteryMonitor::~BatteryMonitor() {}

// operators

// functions

/* BatteryMonitor::calculate:  SOC-OCV curve fit solved by ekf.   Works in 12 V
   battery units.  Scales up/down to number of series/parallel batteries on output/input.
        Inputs:
        Sen->Tb_filt    Measured Tb filtered for noise, past value of temp_c_, deg C
        Sen->Vb         Measured battery terminal voltage, V
        Sen->Ib         Measured shunt current Ib, A
        Sen->T          Update time, sec
        q_capacity_     Saturation charge at temperature, C
        q_cap_rated_scaled_   Applied rated capacity at rated_temp, after scaling, C
        NOM_UNIT_CAP    Nominal battery bank capacity (assume actual not known), Ah
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
        sp.inj_bias  Used to inject fake shunt current
        soc_ekf_        Solved state of charge, fraction
        q_ekf_          Filtered charge calculated by ekf, C
        soc_ekf_ (return)     Solved state of charge, fraction
        tcharge_ekf_    Solved charging time to full from ekf, hr
        y_filt_         Filtered EKF y residual value, V

                <--- ib      ______________         <--- ib
                 voc          |             |
       -----------+-----------|   ChargeTransfer   |----------+-----------+ vb
       |                      |_____________|
       |
    ___|___
    |      |   |
    | HYS  |   |              HYS stores charge ib-ioc.  dv_hys = voc - voc_stat,  Include tau_diff (diffusion)
    |______|   v
       |       ioc ~= ib
       +   voc_stat
       |
    ___|____  voc_soc
    |  +    |  
    | Batt  |   ^
    |  -    |   |             Batt stores charge ioc
    |_______|   |
        |
        |                      Total charge storage ib-ioc + ioc = ib
        _
        -
        gnd
*/ 
float BatteryMonitor::calculate(Sensors *Sen, const boolean reset_temp)
{
    // Inputs
    temp_c_ = Sen->Tb_filt;
    vsat_ = calc_vsat();
    dt_ =  Sen->T;
    float T_rate = T_RLim->calculate(temp_c_, T_RLIM, T_RLIM, reset_temp, Sen->T);
    vb_ = Sen->vb();
    ib_ = Sen->ib();
    ib_ = max(min(ib_, IMAX_NUM), -IMAX_NUM);  // Overflow protection when ib_ past value used

    // Table lookup
    voc_soc_ = voc_soc_tab(soc_, temp_c_) + sp.Dw_z;

    // Battery management system model
    if ( !bms_off_ )
        voltage_low_ = voc_stat_ < chem_.vb_down;
    else
        voltage_low_ = voc_stat_ < chem_.vb_rising;
    bms_charging_ = ib_ > IB_MIN_UP;
    bms_off_ = (temp_c_ <= chem_.low_t) || ( voltage_low_ && !Sen->Flt->vb_fa() && !sp.tweak_test() );    // KISS
    Sen->bms_off = bms_off_;
    ib_charge_ = ib_;
    if ( bms_off_ && !bms_charging_ )
        ib_charge_ = 0.;
    if ( bms_off_ && voltage_low_ )
        ib_ = 0.;
    if ( reset_temp ) ib_past_ = ib_;

    // Dynamic emf. vb_ is stale when running with model
    float ib_dyn;
    if (sp.mod_vb()) ib_dyn = ib_past_;
    else ib_dyn = ib_;
    voc_ = vb_ - (ChargeTransfer_->calculate(ib_dyn, reset_temp, chem_.tau_ct, dt_)*chem_.r_ct*sr_ + ib_dyn*chem_.r_0*sr_);
    if ( !cp.fake_faults )
    {
        if ( (bms_off_ && voltage_low_) ||  Sen->Flt->vb_fa())
        {
            voc_ = voc_stat_ = voc_filt_ = vb_;  // Keep high to avoid chatter with voc_stat_ used above in voltage_low
        }
    }
    dv_dyn_ = vb_ - voc_;

    // Hysteresis model
    dv_hys_ = 0.;  // disable hys g20230530a
    voc_stat_ = voc_ - dv_hys_;
    ioc_ = ib_dyn;
    

    // Reversionary model
    vb_model_rev_ = voc_soc_ + dv_dyn_ + dv_hys_;

    // EKF 1x1
    if ( eframe_ == 0 )
    {
        float ddq_dt = ib_;
        dt_eframe_ = dt_ * float(cp.eframe_mult);  // TODO:  this is noisy error if dt_ varies
        if ( ddq_dt>0. && !sp.tweak_test() ) ddq_dt *= coul_eff_;
        ddq_dt -= chem_.dqdt * q_capacity_ * T_rate;
        predict_ekf(ddq_dt);       // u = d(dq)/dt
        update_ekf(voc_stat_, 0., 1.);  // z = voc_stat, estimated = voc_filtered = hx, predicted = est past
        soc_ekf_ = x_ekf();             // x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
        q_ekf_ = soc_ekf_ * q_capacity_;
        delta_q_ekf_ = q_ekf_ - q_capacity_;
        y_filt_ = y_filt->calculate(y_, min(dt_eframe_, EKF_T_RESET));
        // EKF convergence.  Audio industry found that detection of quietness requires no more than
        // second order filter of the signal.   Anything more is 'gilding the lily'
        boolean conv = abs(y_filt_)<EKF_CONV && !cp.soft_reset;  // Initialize false
        EKF_converged->calculate(conv, EKF_T_CONV, EKF_T_RESET, min(dt_eframe_, EKF_T_RESET), cp.soft_reset);
    }
    eframe_++;
    if ( reset_temp || cp.soft_reset || eframe_ >= cp.eframe_mult ) eframe_ = 0;  // '>=' allows changing cp.eframe_mult on the fly
    if ( (sp.Debug_z==3 || sp.Debug_z==4) && cp.publishS ) EKF_1x1::serial_print(Sen->control_time, Sen->now, dt_eframe_);  // print EKF in Read frame

    // Filter
    voc_filt_ = SdVb_->update(voc_);   // used for saturation test

    // if ( sp.Debug_z==13 || sp.Debug_z==2 || sp.Debug_z==4 )
    //     Serial.printf("bms_off,soc,ib,vb,voc,voc_stat,voc_soc,dv_hys,dv_dyn,%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
    //     bms_off_, soc_, ib_, vb_, voc_, voc_stat_, voc_soc_, dv_hys_, dv_dyn_);

    #ifndef CONFIG_PHOTON
    if ( sp.Debug_z==34 || sp.Debug_z==7 )
        Serial.printf("BatteryMonitor:dt,ib,voc_stat_tab,voc_stat,voc,voc_filt,dv_dyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf, y_ekf_f, soc, conv,  %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f,%7.4f, %7.4f,  %d,\n",
            dt_, ib_, voc_soc_, voc_stat_, voc_, voc_filt_, dv_dyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_, y_filt_, soc_, converged_ekf());
    if ( sp.Debug_z==37 )
        Serial.printf("BatteryMonitor:ib,vb,voc_stat,voc(z_),  K_,y_,soc_ekf, y_ekf_f, conv,  %7.3f,%7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f,%7.4f,  %d,\n",
            ib_, vb_, voc_stat_, voc_,     K_, y_, soc_ekf_, y_filt_, converged_ekf());
    if ( sp.Debug_z==-24 ) Serial.printf("Mon:  ib%7.3f soc%8.4f reset_temp%d tau_ct%9.5f r_ct%7.3f r_0%7.3f dv_dyn%7.3f dv_hys%7.3f voc_soc%7.3f  voc_stat%7.3f voc%7.3f vb%7.3f ib _charge%7.3f ",
        ib_, soc_, reset_temp, chem_.tau_ct, chem_.r_ct, chem_.r_0, dv_dyn_, dv_hys_, voc_soc_, voc_stat_, voc_, vb_, ib_charge_);
    #endif

    // Charge time if used ekf 
    if ( ib_charge_ > 0.1 )
        tcharge_ekf_ = min(NOM_UNIT_CAP / ib_charge_ * (1. - soc_ekf_), 24.);
    else if ( ib_charge_ < -0.1 )
        tcharge_ekf_ = max(NOM_UNIT_CAP / ib_charge_ * soc_ekf_, -24.);
    else if ( ib_charge_ >= 0. )
        tcharge_ekf_ = 24.*(1. - soc_ekf_);
    else 
        tcharge_ekf_ = -24.*soc_ekf_;

    // Past value for synchronization with vb_, only when modeling    
    ib_past_ = ib_;

    return ( vb_model_rev_ );
}

// Charge time calculation
float BatteryMonitor::calc_charge_time(const double q, const float q_capacity, const float charge_curr, const float soc)
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

    float amp_hrs_remaining = (q_capacity - q_min_ + delta_q) / 3600.;
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
    // Process model  dt_eframe_<<chem_.tau_sd

    // Approximation to *Fx = exp(-dt_eframe_ / chem_.tau_sd);
    *Fx = 1. - dt_eframe_ / chem_.tau_sd;
    
    // Approximation to *Bu = (1. - *Fx) * chem_.r_sd;
    *Bu = dt_eframe_ / chem_.c_sd;
}

// EKF model for update
void BatteryMonitor::ekf_update(double *hx, double *H)
{
    // Measurement function hx(x), x=soc ideal capacitor
    float x_lim = max(min(x_, 1.0), 0.0);
    *hx = Battery::calc_soc_voc(x_lim, temp_c_, &dv_dsoc_) + sp.Dw_z;

    // Jacodian of measurement function
    *H = dv_dsoc_;
}

// Initialize
// Works in 12 V batteryunits.   Scales up/down to number of series/parallel batteries on output/input.
void BatteryMonitor::init_battery_mon(const boolean reset, Sensors *Sen)
{
    if ( !reset ) return;
    vb_ = Sen->vb();
    ib_ = Sen->ib();
    ib_ = max(min(ib_, IMAX_NUM), -IMAX_NUM);  // Overflow protection when ib_ past value used
    if ( isnan(vb_) ) vb_ = 13.;    // reset overflow
    if ( isnan(ib_) ) ib_ = 0.;     // reset overflow
    dv_dyn_ = ib_*chem_.r_ss*sr_;
    voc_ = vb_ - dv_dyn_;
    #ifdef DEBUG_INIT
        if ( sp.Debug_z==-1 )
            Serial.printf("mon: ib%7.3f vb%7.3f voc%7.3f\n", ib_, vb_, voc_);
    #endif
}

// Init EKF
void BatteryMonitor::init_soc_ekf(const float soc)
{
    soc_ekf_ = soc;
    init_ekf(soc_ekf_, 0.0);
    q_ekf_ = soc_ekf_ * q_capacity_;
    delta_q_ekf_ = q_ekf_ - q_capacity_;
}

/* is_sat:  Calculate saturation status
    Inputs:
        soc         State of charge
        temp_c      Battery temperature, deg C
        voc_filt    Filtered battery charging voltage, V
    State:
        sat_mem    Battery saturation status, T/F
*/
boolean BatteryMonitor::is_sat(const boolean reset)
{
    static boolean sat_mem;
    if ( reset)
        sat_mem = temp_c_ > chem_.low_t && (voc_filt_ >= vsat_);
    else
        sat_mem = temp_c_ > chem_.low_t && (voc_filt_ >= vsat_ || (soc_ >= MXEPS && !sat_mem) );
    return sat_mem;
}

// Print
void BatteryMonitor::pretty_print(Sensors *Sen)
{
#ifndef DEPLOY_PHOTON
    Serial.printf("BM::");
    this->Battery::pretty_print();
    Serial.printf(" BM::BM:\n");
    Serial.printf("  ah_ekf%7.3f A-h\n", amp_hrs_remaining_ekf_);
    Serial.printf("  ah_soc%7.3f A-h\n", amp_hrs_remaining_soc_);
    Serial.printf("  EKF_conv %d\n", converged_ekf());
    Serial.printf("  e_wrap%7.3f V\n", Sen->Flt->e_wrap());
    Serial.printf("  q_ekf%10.1f C\n", q_ekf_);
    Serial.printf("  soc_ekf%8.4f frac\n", soc_ekf_);
    Serial.printf("  tc%5.1f hr\n", tcharge_);
    Serial.printf("  tc_ekf%5.1f hr\n", tcharge_ekf_);
    Serial.printf("  voc_filt%7.3f V\n", voc_filt_);
    Serial.printf("  voc_soc%7.3f V\n", voc_soc_);
    Serial.printf("  voc_stat%7.3f V\n", voc_stat_);
    Serial.printf("  y_filt%7.3f Res EKF, V\n", y_filt_);
    Serial.printf(" *sp_s_cap_mon%7.3f Slr\n", sp.S_cap_mon_z);
    Serial.printf("  vb_model_rev%7.3f V\n", vb_model_rev_);
#else
     Serial.printf("BatteryMonitor: silent DEPLOY\n");
#endif
}

// Reset Coulomb Counter state to EKF under restricted conditions especially new boot no history of saturation
void BatteryMonitor::regauge(const float temp_c)
{
    if ( converged_ekf() && abs(soc_ekf_-soc_)>DF2 )
    {
        Serial.printf("CC Mon from%7.3f to EKF%7.3f...", soc_, soc_ekf_);

        apply_soc(soc_ekf_, temp_c);
        
        Serial.printf("conf %7.3f\n", soc_);
    }
}

/* Steady state voc(soc) solver for initialization of ekf state.  Expects Sen->Tb_filt to be in reset mode
    INPUTS:
        Sen->Vb      
        Sen->Ib
    OUTPUTS:
        Mon->soc_ekf
*/
boolean BatteryMonitor::solve_ekf(const boolean reset, const boolean reset_temp, Sensors *Sen)
{
    // Average dynamic inputs through the initialization period before apply EKF
    static float Tb_avg = Sen->Tb_filt;
    static float Vb_avg = Sen->Vb;
    static float Ib_avg = Sen->Ib;
    static uint16_t n_avg = 0;
    if ( reset )
    {
        Tb_avg = Sen->Tb_filt;
        Vb_avg = Sen->Vb;
        Ib_avg = Sen->Ib;
        n_avg = 0;
    }
    if ( reset_temp )  // The idea is to average the noisey inputs that happen over reset_temp time period
    {
        n_avg++;
        Tb_avg = (Tb_avg*float(n_avg-1) + Sen->Tb_filt) / float(n_avg);
        Vb_avg = (Vb_avg*float(n_avg-1) + Sen->Vb) / float(n_avg);
        Ib_avg = (Ib_avg*float(n_avg-1) + Sen->Ib) / float(n_avg);
    }
    else  // remember inputs in avg and return
    {
        Tb_avg = Sen->Tb_filt;
        Vb_avg = Sen->Vb;
        Ib_avg = Sen->Ib;
        n_avg = 0;
        return ( true );
    }

    // Solver
    static float soc_solved = 1.;
    float dv_dsoc;
    float voc_solved = calc_soc_voc(soc_solved, Tb_avg, &dv_dsoc) + sp.Dw_z;
    ice_->init(1., soc_min_, 2*SOLV_ERR);
    while ( abs(ice_->e())>SOLV_ERR && ice_->count()<SOLV_MAX_COUNTS && abs(ice_->dx())>0. )
    {
        ice_->increment();
        soc_solved = ice_->x();
        voc_solved = calc_soc_voc(soc_solved, Tb_avg, &dv_dsoc) + sp.Dw_z;
        ice_->e(voc_solved - voc_stat_);
        ice_->iterate(sp.Debug_z==-1 && reset_temp, SOLV_SUCC_COUNTS, false);
    }
    init_soc_ekf(soc_solved);

    #ifdef DEBUG_INIT
        if ( sp.Debug_z==-1 && reset_temp) Serial.printf("sek: Vb%7.3f Vba%7.3f voc_soc%7.3f voc_stat%7.3f voc_sol%7.3f cnt %d dx%8.4f e%10.6f soc_sol%8.4f\n",
            Sen->Vb, Vb_avg, voc_soc_, voc_stat_, voc_solved, ice_->count(), ice_->dx(), ice_->e(), soc_solved);
    #endif

    return ( ice_->count()<SOLV_MAX_COUNTS );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Battery model class for reference use mainly in regression testing
BatterySim::BatterySim() :
    Battery(&sp.Delta_q_model_z, &sp.T_state_model_z, &sp.Sim_chm_z, VS), duty_(0UL), hys_scale_(HYS_SCALE), ib_fut_(0.),
    ib_in_(0.), model_cutback_(true), q_(NOM_UNIT_CAP*3600.), sample_time_(0UL), sample_time_z_(0UL), sat_ib_max_(0.)
{
    // ChargeTransfer dynamic model for EKF
    // Resistance values add up to same resistance loss as matched to installed battery
    Sin_inj_ = new SinInj();
    Sq_inj_ = new SqInj();
    Tri_inj_ = new TriInj();
    Cos_inj_ = new CosInj();
    dv_voc_soc_ = 0.;           // Talk table bias, Dy, V
    sat_ib_null_ = 0.;          // Current cutback value for soc=1, A
    sat_cutback_gain_ = 1000.;  // Gain to retard ib when soc approaches 1, dimensionless
    model_saturated_ = false;
    ib_sat_ = 0.5;              // deadzone for cutback actuation, A
    ib_charge_ = 0.;
    hys_ = new Hysteresis(&chem_);
}
BatterySim::~BatterySim() {}

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
//    sat
//    bms_off
//
//  States:
//    soc_              State of Charge, fraction
//
//  Outputs:
//    temp_c_           Simulated Tb, deg C
//    ib_fut_           Simulated over-ridden by saturation, A
//    vb_               Simulated vb, V
//    sp.inj_bias       Used to inject fake shunt current, A

                <---ib        ______________         <---ib
                 voc          |             |
       -----------+-----------|   ChargeTransfer   |----------+-----------+ vb
       |                      |_____________|
       |
    ___|___
    |      |
    | HYS  |   |              HYS stores charge ib-ioc.  dv_hys = voc - voc_stat,  Include tau_diff (diffusion)
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
float BatterySim::calculate(Sensors *Sen, const boolean dc_dc_on, const boolean reset)
{
    // Inputs
    temp_c_ = Sen->Tb_filt;
    dt_ = Sen->T;
    ib_in_ = Sen->Ib_model_in / sp.nP_z;
    if ( reset ) ib_fut_ = ib_in_;
    ib_ = max(min(ib_fut_, IMAX_NUM), -IMAX_NUM);  //  Past value ib_.  Overflow protection when ib_ past value used
    vsat_ = calc_vsat();
    float soc_lim = max(min(soc_, 1.0), -0.2);  // slightly beyond

    // VOC-OCV model
    voc_stat_ = calc_soc_voc(soc_, temp_c_, &dv_dsoc_) + dv_voc_soc_;
    voc_stat_ = min(voc_stat_ + (soc_ - soc_lim) * dv_dsoc_, vsat_*1.2);  // slightly beyond sat but don't windup


    // Hysteresis model
    hys_->calculate(ib_in_, soc_, hys_scale_);
    boolean init_low = bms_off_ || ( soc_<(soc_min_+HYS_SOC_MIN_MARG) && ib_>HYS_IB_THR );
    dv_hys_ = hys_->update(dt_, sat_, init_low, 0.0, hys_scale_, reset);
    voc_ = voc_stat_ + dv_hys_;
    ioc_ = hys_->ioc();

    // Battery management system (bms).   I believe bms can see only vb but using this for a model causes
    // lots of chatter as it shuts off, restores vb due to loss of dynamic current, then repeats shutoff.
    // Using voc_ is not better because change in dv_hys_ causes the same effect.   So using nice quiet
    // voc_stat_ for ease of simulation, not accuracy.
    if ( reset ) vb_ = voc_stat_;
    if ( !bms_off_ )
        voltage_low_ = voc_stat_ < chem_.vb_down_sim;
    else
        voltage_low_ = voc_stat_ < chem_.vb_rising_sim;
    bms_charging_ = ib_in_ > IB_MIN_UP;
    bms_off_ = (temp_c_ <= chem_.low_t) || (voltage_low_ && !sp.tweak_test());
    float ib_charge_fut = ib_in_;  // Pass along current to charge unless bms_off
    if ( bms_off_ && sp.mod_ib() && !bms_charging_)
        ib_charge_fut = 0.;
    if ( bms_off_ && voltage_low_ )
        ib_ = 0.;

    // ChargeTransfer dynamic model for model, reverse version to generate sensor inputs
    vb_ = voc_ + (ChargeTransfer_->calculate(ib_, reset, chem_.tau_ct, dt_)*chem_.r_ct*sr_ + ib_*chem_.r_0*sr_);

    // Special cases override
    if ( bms_off_ )
    {
        vb_ = voc_;
    }
    if ( bms_off_ && dc_dc_on )
    {
        vb_ = VB_DC_DC;
    }
    dv_dyn_ = vb_ - voc_;

    // Saturation logic, both full and empty
    sat_ib_max_ = sat_ib_null_ + (1. - (soc_ + ds_voc_soc_) ) * sat_cutback_gain_ * sp.Cutback_gain_sclr_z;
    if ( sp.tweak_test() || !sp.mod_ib() ) sat_ib_max_ = ib_charge_fut;   // Disable cutback when real world or when doing tweak_test test
    ib_fut_ = min(ib_charge_fut, sat_ib_max_);      // the feedback of ib_
    // ib_charge_ = ib_charge_fut;  // Same time plane as volt calcs, added past value.  (This prevents sat logic from working)
    ib_charge_ = ib_fut_;  // Same time plane as volt calcs, added past value
    if ( (q_ <= 0.) && (ib_charge_ < 0.) && sp.mod_ib() ) ib_charge_ = 0.;   //  empty
    model_cutback_ = (voc_stat_ > vsat_) && (ib_fut_ == sat_ib_max_);
    model_saturated_ = model_cutback_ && (ib_fut_ < ib_sat_);
    Coulombs::sat_ = model_saturated_;
    
    #ifndef CONFIG_PHOTON

        if ( sp.Debug_z==75 ) Serial.printf("BatterySim::calculate: temp_c_ soc_ voc_stat_ low_voc =  %7.3f %10.6f %9.5f %7.3f\n",
            temp_c_, soc_, voc_stat_, chem_.low_voc);

        if ( sp.Debug_z==76 ) Serial.printf("BatterySim::calculate:,  soc=%8.4f, temp_c_=%7.3f, ib_in%7.3f ib%7.3f voc_stat%7.3f voc%7.3f vsat%7.3f model_saturated%d bms_off%d dc_dc_on%d VB_DC_DC%7.3f vb%7.3f\n",
            soc_, temp_c_, ib_in_, ib_, voc_stat_, voc_, vsat_, model_saturated_, bms_off_, dc_dc_on, VB_DC_DC, vb_);

        if ( sp.Debug_z==78 || sp.Debug_z==7 ) Serial.printf("BatterySim::calculate:,  dt_,tempC,curr,soc_,voc,dv_dyn,vb,%7.3f,%7.3f,%7.3f,%8.4f,%7.3f,%7.3f,%7.3f,\n",
        dt_,temp_c_, ib_, soc_, voc_, dv_dyn_, vb_);
 
        if ( sp.Debug_z==79 ) Serial.printf("reset, mod_ib, temp_c_, dvoc_dt, vsat_, voc, q_capacity, sat_ib_max, ib_fut, ib,=%d,%d,%7.3f,%7.3f,%7.3f,%7.3f, %10.1f, %7.3f, %7.3f, %7.3f,\n",
            reset, sp.mod_ib(), temp_c_, chem_.dvoc_dt, vsat_, voc_, q_capacity_, sat_ib_max_, ib_fut_, ib_);

    #endif

    return ( vb_ );
}

// Injection model, calculate inj bias based on time since boot
float BatterySim::calc_inj(const unsigned long now, const uint8_t type, const float amp, const double freq)
{

    // Sample at instant of signal injection
    sample_time_z_ = sample_time_;
    sample_time_ = millis();

    // Return if time 0
    if ( now== 0UL )
    {
        duty_ = 0UL;
        sp.put_Inj_bias(0.);
        return(duty_);
    }

    // Injection.  time shifted by 1UL
    double t = (now-1UL)/1e3;
    float inj_bias = 0.;
    // Calculate injection amounts from user inputs (talk).

    switch ( type )
    {
        case ( 0 ):   // Nothing
            inj_bias = sp.Inj_bias_z;
            break;
        case ( 1 ):   // Sine wave
            inj_bias = Sin_inj_->signal(amp, freq, t, 0.0) - sp.Amp_z;
            break;
        case ( 2 ):   // Square wave
            inj_bias = Sq_inj_->signal(amp, freq, t, 0.0) - sp.Amp_z;
            break;
        case ( 3 ):   // Triangle wave
            inj_bias = Tri_inj_->signal(amp, freq, t, 0.0);
            break;
        case ( 4 ): case ( 5 ): // Software biases only
            inj_bias = sp.Inj_bias_z - sp.Amp_z;
            break;
        case ( 6 ):   // Positve bias
            inj_bias = amp - sp.Amp_z;
            break;
        case ( 8 ):   // Cosine wave
            inj_bias = Cos_inj_->signal(amp, freq, t, 0.0) - sp.Amp_z;
            break;
        default:
            inj_bias = -sp.Amp_z;
            break;
    }
    sp.put_Inj_bias(inj_bias);
    return ( inj_bias );
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
    *sp_delta_q_    Charge change since saturated, C
    *sp_t_last_     Updated value of battery temperature used for rate limit memory, deg C
    soc_            Fraction of saturation charge (q_capacity_) available (0-1) 
Outputs:
    q_capacity_     Saturation charge at temperature, C
    resetting_      Sticky flag for initialization, T=reset
    soc_min_        Estimated soc where battery BMS will shutoff current, fraction
    q_min_          Estimated charge at low voltage shutdown, C\
*/
float BatterySim::count_coulombs(Sensors *Sen, const boolean reset_temp, BatteryMonitor *Mon, const boolean initializing_all) 
{
    float charge_curr = ib_charge_;
    double d_delta_q = charge_curr * Sen->T;
    if ( charge_curr>0. ) d_delta_q *= coul_eff_;

    // Rate limit temperature.  When modeling, initialize to no change
    if ( reset_temp && sp.mod_vb() )
    {
        *sp_t_last_ = Sen->Tb;
    }
    float temp_lim = max(min(Sen->Tb, *sp_t_last_ + T_RLIM*Sen->T), *sp_t_last_ - T_RLIM*Sen->T);
    
    // Saturation and re-init.   Goal is to set q_capacity and hold it so remember last saturation status
    // But if not modeling in real world, set to Monitor when Monitor saturated and reset_temp to EKF otherwise
    static boolean reset_temp_past = reset_temp;   // needed because model called first in reset_temp path; need to pick up latest
    if ( !sp.mod_vb() )  // Real world init to track Monitor
    {
        if ( Mon->sat() || reset_temp_past )
            apply_delta_q(Mon->delta_q());
    }
    else if ( model_saturated_ )  // Modeling initializes on reset_temp to Tb=RATED_TEMP
    {
        if ( reset_temp )
        {
            *sp_delta_q_ = 0.;
        }
    }
    reset_temp_past = reset_temp;
    resetting_ = false;     // one pass flag

    // Integration can go to -20%
    q_capacity_ = calculate_capacity(temp_lim);
    if ( !reset_temp )
    {
        *sp_delta_q_ += d_delta_q - chem_.dqdt*q_capacity_*(temp_lim-*sp_t_last_);
        *sp_delta_q_ = max(min(*sp_delta_q_, 0.), -q_capacity_*1.2);
    }
    // if ( sp.Debug_z==-24 )Serial.printf("Sim:  charge_curr%7.3f d_delta_q%10.6f delta_q%10.1f temp_lim%7.3f t_last%7.3f\n", charge_curr, d_delta_q, *sp_delta_q_, temp_lim, *sp_t_last_);
    q_ = q_capacity_ + *sp_delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    soc_min_ = chem_.soc_min_T_->interp(temp_lim);
    q_min_ = soc_min_ * q_capacity_;

    // print_serial_sim
    if ( (sp.Debug_z==2 || sp.Debug_z==3 || sp.Debug_z==4 )  && cp.publishS && !initializing_all)
    {
        double cTime;
        if ( sp.tweak_test() ) cTime = float(Sen->now)/1000.;
        else cTime = Sen->control_time;
        sprintf(cp.buffer, "unit_sim, %13.3f, %d, %7.0f, %d, %7.5f,%7.5f, %7.5f,%7.5f,%7.5f,%7.5f, %7.3f,%7.3f,%7.3f,%7.3f,  %d,  %9.1f,  %8.5f, %d, %c",
            cTime, sp.Sim_chm_z, q_cap_rated_scaled_, bms_off_, Sen->Tb, temp_lim, vsat_, voc_stat_, dv_dyn_, vb_, ib_, ib_in_, ib_charge_, ioc_, model_saturated_, *sp_delta_q_, soc_, reset_temp,'\0');
        Serial.printf("%s\n", cp.buffer);
    }

    // Save and return
    *sp_t_last_ = temp_lim;
    return ( soc_ );
}

// Initialize
// Works in 12 V batteryunits.   Scales up/down to number of series/parallel batteries on output/input.
void BatterySim::init_battery_sim(const boolean reset, Sensors *Sen)
{
    if ( !reset ) return;
    ib_ = Sen->ib_model_in();
    ib_ = max(min(ib_, IMAX_NUM), -IMAX_NUM);  // Overflow protection when ib_ past value used
    vb_ = Sen->vb();
    voc_ = vb_ - ib_*chem_.r_ss*sr_;
    if ( isnan(voc_) ) voc_ = 13.;    // reset overflow
    if ( isnan(ib_) ) ib_ = 0.;     // reset overflow
    dv_dyn_ = vb_ - voc_;
    ib_fut_ = ib_;
    init_hys(0.0);
    ibs_ = hys_->ibs();
    #ifdef DEBUG_INIT
        if ( sp.Debug_z==-1 )
        {
            Serial.printf("sim: ib%7.3f ibs%7.3f voc%7.3f vb%7.3f\n", ib_, ibs_, voc_, vb_);
        }
    #endif
}

// Print
void BatterySim::pretty_print(void)
{
#ifndef DEPLOY_PHOTON
    Serial.printf("BS::");
    this->Battery::pretty_print();
    Serial.printf(" BS::BS:\n");
    Serial.printf("  dv_hys%7.3f, V\n", hys_->dv_hys());
    Serial.printf("  hys_scale%7.3f,\n", hys_scale_);
    Serial.printf("  ib%7.3f, A\n", ib_);
    Serial.printf("  ib_fut%7.3f, A\n", ib_fut_);
    Serial.printf("  ib_in%7.3f, A\n", ib_in_);
    Serial.printf("  ib_sat%7.3f\n", ib_sat_);
    Serial.printf("  mod_cb %d\n", model_cutback_);
    Serial.printf("  mod_sat %d\n", model_saturated_);
    Serial.printf("  sat_cb_gn%7.1f\n", sat_cutback_gain_);
    Serial.printf("  sat_ib_max%7.3f, A\n", sat_ib_max_);
    Serial.printf("  sat_ib_null%7.3f, A\n", sat_ib_null_);
    Serial.printf(" *sp_s_cap_sim%7.3f Slr\n", sp.S_cap_sim_z);
    hys_->pretty_print();
#else
     Serial.printf("BatterySim: silent DEPLOY\n");
#endif
}
