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
#include "myLibrary/EKF_1x1.h"
#include <math.h>
#include "command.h"
extern CommandPars cp;
extern RetainedPars rp; // Various parameters to be static at system level

// struct CoulombCounter
CoulombCounter::CoulombCounter(){}

// functions
void CoulombCounter::prime(const double nom_q_cap_, const double t_rated_, const double init_q, const double init_t_c, const double s_cap)
{
    nom_q_cap = nom_q_cap_;
    t_rated = t_rated_;
    q = init_q;
    t_sat = init_t_c;
    q_cap = nom_q_cap * s_cap;
    q_sat = calculate_saturation_charge(t_sat, q_cap);
    q_capacity = calculate_capacity(init_t_c);
    soc = q / q_capacity;
    SOC = q / nom_q_cap * 100.;
    Serial.printf("re-primed CoulombCounter\n");
}

// Scale size of battery and adjust as needed to preserve delta_q.  t_sat_ unchanged.
// Goal is to scale battery and see no change in delta_q on screen of 
// test comparisons.   The rationale for this is that the battery is frequently saturated which
// resets all the model parameters.   This happens daily.   Then both the model and the battery
// are discharged by the same current so the delta_q will be the same.
void CoulombCounter::apply_cap_scale(const double scale)
{
  q_cap = scale * nom_q_cap;
  q = SOC / 100. * q_cap;
  q_sat = calculate_saturation_charge(t_sat, q_cap);
  q_capacity = q_sat;
  soc = q / q_capacity;
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void CoulombCounter::apply_soc(const double soc_)
{
  soc = soc_;
  q = soc*q_capacity;
  delta_q = q - q_capacity;
  SOC = q / q_cap * 100.;
  resetting = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  delta_q preserved
void CoulombCounter::apply_SOC(const double SOC_)
{
  SOC = SOC_;
  q = SOC / 100. * q_cap;
  delta_q = q - q_capacity;
  soc = q / q_capacity;
  resetting = true;     // momentarily turn off saturation check
}

// Memory set, adjust book-keeping as needed.  q_cap_ etc presesrved
void CoulombCounter::apply_delta_q(const double delta_q_)
{
  delta_q = delta_q_;
  q = q_capacity + delta_q;
  soc = q / q_capacity;
  SOC = q / q_cap * 100.;
  resetting = true;
}

// Capacity
double CoulombCounter::calculate_capacity(const double temp_c_)
{
    return( q_sat * (1-DQDT*(temp_c_ - t_sat)) );
}

// Saturation charge
double CoulombCounter::calculate_saturation_charge(const double t_sat_, const double q_cap_)
{
    return( q_cap_ * ((t_sat_ - t_rated)*DQDT + 1.) );
}

// Count coulombs based on true=actual capacity
double CoulombCounter::count_coulombs(const double dt_, const double temp_c_, const double charge_curr_, const boolean sat_, const double t_last_)
{
    /* Count coulombs based on true=actual capacity
    Inputs:
        dt_             Integration step, s
        temp_c_         Battery temperature, deg C
        charge_curr_    Charge, A
        sat_            Indicator that battery is saturated (VOC>threshold(temp)), T/F
        tlast_          Past value of battery temperature used for rate limit memory, deg C
    */
    double d_delta_q_ = charge_curr_ * dt_;
    t_last = t_last_;

    // Rate limit temperature
    double temp_lim = t_last + max(min( (temp_c_-t_last), t_rat*dt_), -t_rat*dt_);

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    // TODO:   should we just use q_sat all the time in soc calculation?  (Memory behavior causes problems with saturation
    // detection).
    if ( false )    // TODO:  BatteryModel needs to use something different than Battery.  TODO:  add CoulombCounter to Battery and separate BatteryModel
    // if ( sat_ )
    {
        if ( d_delta_q_ > 0 )
        {
            d_delta_q_ = 0.;
            if ( !resetting ) delta_q = 0.;
            else resetting = false;     // one pass flag.  Saturation debounce should reset next pass
        }
        t_sat = temp_c_;  // TODO:  not used
        q_sat = ((t_sat - t_rated)*DQDT + 1.)*q_cap; // TODO:  not used
    }

    // Integration
    q_capacity = q_cap*(1. + DQDT*(temp_lim - t_rated));
    delta_q = max(min(delta_q + d_delta_q_ - DQDT*q_capacity*(temp_lim-t_last), 1.1*(q_cap - q_capacity)), -q_capacity);
    q = q_capacity + delta_q;

    // Normalize
    soc = q / q_capacity;
    SOC = q / nom_q_cap * 100;

    if ( rp.debug==76 )
        Serial.printf("CoulombCounter::count_coulombs:,  dt,voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, tsat,q_capacity,soc,SOC,       %7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%9.1f,%7.3f,%7.3f,\n",
                    dt_,cp.pubList.VOC,  sat_voc(temp_c_), temp_lim, sat_, charge_curr_, d_delta_q_, delta_q, q, t_sat, q_capacity, soc, SOC);
    if ( rp.debug==-76 )
        Serial.printf("voc, v_sat, sat, temp_lim, charge_curr, d_d_q, d_q, q_sat, tsat,q_capacity,soc, SOC,          \n%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%9.1f,%7.3f,%7.3f,\n",
                    cp.pubList.VOC,  sat_voc(temp_c_), temp_lim, sat_, charge_curr_, d_delta_q_, delta_q, q, t_sat, q_capacity, soc, SOC);

    // Save and return
    t_last = temp_lim;
    return ( soc );
}

// Load states from retained memory
void CoulombCounter::load(const double delta_q_, const double t_sat_, const double q_sat_, const double t_last_)
{
    delta_q = delta_q_;
    t_sat = t_sat_;
    q_sat = q_sat_;
    t_last = t_last_;
}

// Update states to be saved in retained memory
void CoulombCounter::update(double *delta_q_, double *t_sat_, double *q_sat_, double *t_last_)
{
    *delta_q_ = delta_q;
    *t_sat_ = t_sat;
    *q_sat_ = q_sat;
    *t_last_ = t_last;
}


// class Battery
// constructors
Battery::Battery()
    : b_(0), a_(0), c_(0), m_(0), n_(0), d_(0), nz_(1), q_(nom_q_cap), r1_(0), r2_(0), c2_(0), voc_(0),
    vdyn_(0), vb_(0), ib_(0), num_cells_(4), dv_dsoc_(0), tcharge_(24), sr_(1), vsat_(13.7),
    sat_(false), dv_(0), dvoc_dt_(0) {Q_ = 0.; R_ = 0.;}
Battery::Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt)
    : b_(0), a_(0), c_(0), m_(m), n_(n), d_(d), nz_(nz), q_(nom_q_cap), r1_(r1), r2_(r2), c2_(r2c2/r2_),
    voc_(0), vdyn_(0), vb_(0), ib_(0), num_cells_(num_cells), dv_dsoc_(0), tcharge_(24.),
    sr_(1.), nom_vsat_(batt_vsat), sat_(false), dv_(0), dvoc_dt_(dvoc_dt),
    r0_(0.003), tau_ct_(0.2), rct_(0.0016), tau_dif_(83.), r_dif_(0.0077),
    tau_sd_(1.8e7), r_sd_(70.)
{

    // Battery characteristic tables
    B_T_ = new TableInterp1Dclip(nz_, x_tab, b_tab);
    A_T_ = new TableInterp1Dclip(nz_, x_tab, a_tab);
    C_T_ = new TableInterp1Dclip(nz_, x_tab, c_tab);

    // EKF
    this->Q_ = 0.001*0.001;
    this->R_ = 0.1*0.1;

    // Randles dynamic model for EKF, forward version based on sensor inputs {ib, vb} --> {voc}, ioc=ib
    // Resistance values add up to same resistance loss as matched to installed battery
    //   i.e.  (r0_ + rct_ + rdif_) = (r1 + r2)*num_cells
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    double c_ct = tau_ct_ / rct_;
    double c_dif = tau_ct_ / rct_;
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

}
Battery::~Battery() {}
// operators
// functions


// VOC-OCV model
void Battery::calc_soc_voc_coeff(double soc, double tc, double *b, double *a, double *c, double *log_soc, double *exp_n_soc, double *pow_log_soc)
{
    *b = B_T_ ->interp(tc);
    *a = A_T_ ->interp(tc);
    *c = C_T_ ->interp(tc);

    *log_soc = log(soc);
    *exp_n_soc = exp(n_*(soc-1));
    *pow_log_soc = pow(-*log_soc, m_);
}
double Battery::calc_soc_voc(double soc_lim, double *dv_dsoc, double b, double a, double c, double log_soc, double exp_n_soc, double pow_log_soc)
{
    double voc;  // return value
    *dv_dsoc = calc_h_jacobian(soc_lim, b, c, log_soc, exp_n_soc, pow_log_soc);
    voc = double(num_cells_) * ( a + b*pow_log_soc + c*soc_lim + d_*exp_n_soc );
    // d2v_dsoc2_ = double(num_cells_) * ( b*m_/soc_/soc_*pow_log_soc/log_soc*((m_-1.)/log_soc - 1.) + d_*n_*n_*exp_n_soc );
    return (voc);
}

double Battery::calc_h_jacobian(double soc_lim, double b, double c, double log_soc, double exp_n_soc, double pow_log_soc)
{
    double dv_dsoc = double(num_cells_) * ( b*m_/soc_lim*pow_log_soc/log_soc + c + d_*n_*exp_n_soc );
    return (dv_dsoc);
}

// SOC-OCV curve fit method per Zhang, et al.   May write this base version if needed using BatteryModel::calculate()
// as a starting point but use the base class Randles formulation and re-arrange the i/o for that model.
double Battery::calculate(const double temp_C, const double q, const double curr_in, const double dt) { return 0.;}

// SOC-OCV curve fit method per Zhang, et al modified by ekf
double Battery::calculate_ekf(const double temp_c, const double vb, const double ib, const double dt, const boolean saturated)
{
    double b, a, c, log_soc, exp_n_soc, pow_log_soc;
    temp_c_ = temp_c;

    // VOC-OCV model
    calc_soc_voc_coeff(soc_ekf_, temp_c_, &b, &a, &c, &log_soc, &exp_n_soc, &pow_log_soc);

    // Dynamic emf
    vb_ = vb;
    ib_ = ib;
    double u[2] = {ib, vb};
    Randles_->calc_x_dot(u);
    Randles_->update(dt);
    voc_dyn_ = Randles_->y(0);
    vdyn_ = vb_ - voc_dyn_;
    voc_ = voc_dyn_;

    // EKF 1x1
    predict_ekf(ib);      // u = ib
    update_ekf(voc_dyn_, 0., 1., dt);   // z = voc_dyn
    soc_ekf_ = x_ekf();   // x = Vsoc (0-1 ideal capacitor voltage) proxy for soc

    if ( rp.debug==34 )
        Serial.printf("dt,ib,voc_dyn,vdyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf,   %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f,\n",
            dt, ib, voc_dyn_, vdyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_);
    if ( rp.debug==-34 )
        Serial.printf("dt,ib,voc_dyn,vdyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf,  \n%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f,\n",
            dt, ib, voc_dyn_, vdyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_);
    if ( rp.debug==37 )
        Serial.printf("ib,vb,voc_dyn(z_),  K_,y_,SOC_ekf,   %7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f,\n",
            ib, vb, voc_dyn_,     K_, y_, soc_ekf_);
    if ( rp.debug==-37 )
        Serial.printf("ib,vb*10-110,voc_dyn(z_)*10-110,  K_,y_,SOC_ekf-90,   \n%7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f,\n",
            ib, vb*10-110, voc_dyn_*10-110,     K_, y_, soc_ekf_*100-90);

    // Charge time if used ekf
    if ( ib_ > 0.1 )  tcharge_ekf_ = min(RATED_BATT_CAP / ib_ * (1. - soc_ekf_), 24.);
    else if ( ib_ < -0.1 ) tcharge_ekf_ = max(RATED_BATT_CAP / ib_ * soc_ekf_, -24.);
    else if ( ib_ >= 0. ) tcharge_ekf_ = 24.*(1. - soc_ekf_);
    else tcharge_ekf_ = -24.*soc_ekf_;

    return ( soc_ekf_ );
}

// Charge time calculation
double Battery::calculate_charge_time(const double temp_c, const double charge_curr,
    const double delta_q, const double t_sat, const double q_sat, const double soc)
{
    double q_capacity = calculate_capacity(temp_c, t_sat, q_sat);
    if ( charge_curr > TCHARGE_DISPLAY_DEADBAND )  tcharge_ = min( -delta_q / ib_ / 3600., 24.);
    else if ( charge_curr < -TCHARGE_DISPLAY_DEADBAND ) tcharge_ = max( (q_capacity + delta_q) / ib_ / 3600., -24.);
    else if ( charge_curr >= 0. ) tcharge_ = 24.;
    else tcharge_ = -24.;

    amp_hrs_remaining_ = (q_capacity + delta_q) / 3600.;
    if ( soc > 0. )
        amp_hrs_remaining_ekf_ = amp_hrs_remaining_ * soc_ekf_ / soc;
    else
        amp_hrs_remaining_ekf_ = 0.;

    return( tcharge_ );
}

// EKF model for predict
void Battery::ekf_model_predict(double *Fx, double *Bu)
{
    // Process model
    *Fx = exp(-dt_ / tau_sd_);
    *Bu = (1. - *Fx) * r_sd_;
}

// EKF model for update
void Battery::ekf_model_update(double *hx, double *H)
{
    // Measurement function hx(x), x=soc ideal capacitor
    double b, a, c, log_soc, exp_n_soc, pow_log_soc;
    double dv_dsoc;
    double x_lim = max(min(x_, mxeps_bb), mneps_bb);
    calc_soc_voc_coeff(x_lim, temp_c_,  &b, &a, &c, &log_soc, &exp_n_soc, &pow_log_soc);
    *hx = calc_soc_voc(x_lim, &dv_dsoc, b, a, c, log_soc, exp_n_soc, pow_log_soc);

    // Jacodian of measurement function
    *H = dv_dsoc;
}

// Init EKF
void Battery::init_soc_ekf(const double soc)
{
    soc_ekf_ = soc;
    q_sat_ = soc * nom_q_cap;
    init_ekf(soc_ekf_, 0.0);
    if ( rp.debug==-34 )
    {
        Serial.printf("init_soc_ekf:  soc_ekf_, x_ekf_ = %7.3f, %7.3f,\n", soc_ekf_, x_ekf());
    }
}

/* C <- A * B */
void mulmat(double * a, double * b, double * c, int arows, int acols, int bcols)
{
    int i, j,l;

    for(i=0; i<arows; ++i)
        for(j=0; j<bcols; ++j) {
            c[i*bcols+j] = 0;
            for(l=0; l<acols; ++l)
                c[i*bcols+j] += a[i*acols+l] * b[l*bcols+j];
        }
}
void mulvec(double * a, double * x, double * y, int m, int n)
{
    int i, j;

    for(i=0; i<m; ++i) {
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
boolean is_sat(const double temp_c, const double voc)
{
    double vsat = sat_voc(temp_c);
    return ( voc >= vsat );
}

// Capacity
double calculate_capacity(const double temp_c, const double t_sat, const double q_sat)
{
    return( q_sat * (1-DQDT*(temp_c - t_sat)) );
}

// Saturation charge
double calculate_saturation_charge(const double t_sat, const double q_cap)
{
    return( q_cap * ((t_sat - 25.)*DQDT + 1.) );
}
