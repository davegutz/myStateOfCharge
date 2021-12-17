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

// class Battery
// constructors
Battery::Battery()
    : b_(0), a_(0), c_(0), m_(0), n_(0), d_(0), nz_(1), socs_(1.0), socu_(1.0), r1_(0), r2_(0), c2_(0), voc_(0),
    vdyn_(0), vb_(0), ib_(0), num_cells_(4), dv_dsocs_(0), dv_dsocu_(0), tcharge_(24), pow_in_(0.0), sr_(1), vsat_(13.7),
    sat_(false), dv_(0), dvoc_dt_(0) {Q_ = 0.; R_ = 0.;}
Battery::Battery(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt)
    : b_(0), a_(0), c_(0), m_(m), n_(n), d_(d), nz_(nz), socs_(1.0), socu_(1.0), r1_(r1), r2_(r2), c2_(r2c2/r2_),
    voc_(0), vdyn_(0), vb_(0), ib_(0), num_cells_(num_cells), dv_dsocs_(0), dv_dsocu_(0), tcharge_(24.), pow_in_(0.0),
    sr_(1.), nom_vsat_(batt_vsat), sat_(false), dv_(0), dvoc_dt_(dvoc_dt),
    r0_(0.003), tau_ct_(0.2), rct_(0.0016), tau_dif_(83.), r_dif_(0.0077),
    tau_sd_(1.8e7), r_sd_(70.), dQdT_(0.01)
{
    // dQdT from literature.   0.01 / deg C is commonly used.

    // Battery characteristic tables
    B_T_ = new TableInterp1Dclip(nz_, x_tab, b_tab);
    A_T_ = new TableInterp1Dclip(nz_, x_tab, a_tab);
    C_T_ = new TableInterp1Dclip(nz_, x_tab, c_tab);

    // EKF
    this->Q_ = 0.001*0.001;
    this->R_ = 0.1*0.1;

    // Randles dynamic model for EKF
    // Resistance values add up to same resistance loss as matched to installed battery
    //   i.e.  (r0_ + rct_ + rdif_) = (r1 + r2)*num_cells
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    double c_ct = tau_ct_ / rct_;
    double c_dif = tau_ct_ / rct_;
    rand_n_ = 2;     // TODO:   don't need these
    rand_p_ = 2;
    rand_q_ = 1;
    rand_A_ = new double[rand_n_*rand_n_];
    rand_A_[0] = -1./tau_ct_;
    rand_A_[1] = 0.;
    rand_A_[2] = 0.;
    rand_A_[3] = -1/tau_dif_;
    rand_B_ = new double [rand_n_*rand_p_];
    rand_B_[0] = 1./c_ct;
    rand_B_[1] = 0.;
    rand_B_[2] = 1./c_dif;
    rand_B_[3] = 0.;
    rand_C_ = new double [rand_q_*rand_n_];
    rand_C_[0] = -1.;
    rand_C_[1] = -1.;
    rand_D_ = new double [rand_q_*rand_p_];
    rand_D_[0] = -r0_;
    rand_D_[1] = 1.;
    rand_Cinv_ = new double [rand_q_*rand_n_];
    rand_Cinv_[0] = 1.;
    rand_Cinv_[1] = 1.;
    rand_Dinv_ = new double [rand_q_*rand_p_];
    rand_Dinv_[0] = r0_;
    rand_Dinv_[1] = 1.;
    Randles_ = new StateSpace(rand_A_, rand_B_, rand_C_, rand_D_, rand_n_, rand_p_, rand_q_);
    RandlesInv_ = new StateSpace(rand_A_, rand_B_, rand_Cinv_, rand_Dinv_, rand_n_, rand_p_, rand_q_);

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
double Battery::calc_voc_ocv(double soc_lim, double *dv_dsoc, double b, double a, double c, double log_soc, double exp_n_soc, double pow_log_soc)
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

// SOC-OCV curve fit method per Zhang, et al.
double Battery::calculate(const double temp_C, const double socu_frac, const double curr_in, const double dt)
{
    dt_ = dt;

    socu_ = socu_frac;
    socs_ = 1-(1-socu_)*cu_bb/cs_bb;
    double socs_lim = max(min(socs_, mxeps_bb), mneps_bb);
    ib_ = curr_in;

    // VOC-OCV model
    double log_socs, exp_n_socs, pow_log_socs;
    calc_soc_voc_coeff(socs_lim, temp_C, &b_, &a_, &c_, &log_socs, &exp_n_socs, &pow_log_socs);
    voc_ = calc_voc_ocv(socs_lim, &dv_dsocs_, b_, a_, c_, log_socs, exp_n_socs, pow_log_socs)
             + (socs_ - socs_lim) * dv_dsocs_;  // slightly beyond
    voc_ +=  dv_;  // Experimentally varied
    dv_dsocu_ = dv_dsocs_ * cu_bb / cs_bb;

    // Dynamic emf  TODO:   aren't these the same?
    if ( ib_ < 0. ) vdyn_ = double(num_cells_) * ib_*(r1_ + r2_)*sr_;
    else vdyn_ = double(num_cells_) * ib_*(r1_ + r2_)*sr_;

    // Summarize   TODO: get rid of the global defines here because they differ from one battery to another
    vb_ = voc_ + vdyn_;
    pow_in_ = vb_*ib_ - ib_*ib_*(r1_+r2_)*sr_*num_cells_;  // Internal resistance of battery is a loss
    if ( pow_in_>1. )  tcharge_ = min(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * (1.-socs_lim), 24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
    else if ( pow_in_<-1. ) tcharge_ = max(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * socs_lim, -24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
    else if ( pow_in_>=0. ) tcharge_ = 24.*(1.-socs_lim);
    else tcharge_ = -24.*socs_lim;
    vsat_ = nom_vsat_ + (temp_C-25.)*dvoc_dt_;
    sat_ = voc_ >= vsat_;

    if ( rp.debug==-8 ) Serial.printf("calculate:  SOCU_in,v,curr,pow,tcharge,vsat,voc,sat= %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,\n", 
      socu_frac, vb_, ib_, pow_in_, tcharge_, vsat_, voc_, sat_);

    if ( rp.debug==-9 )Serial.printf("calculate:  tempC,tempF,curr,a,b,c,d,n,m,r,soc,logsoc,expnsoc,powlogsoc,voc,vdyn,v,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
     temp_C, temp_C*9./5.+32., ib_, a_, b_, c_, d_, n_, m_, (r1_+r2_)*sr_ , socs_, log_socs, exp_n_socs, pow_log_socs, voc_, vdyn_,vb_);

    return ( vb_ );
}

// SOC-OCV curve fit method per Zhang, et al modified by ekf
double Battery::calculate_ekf(const double temp_c, const double vb, const double ib, const double dt, const boolean saturated)
{
    // Save temperature for callbacks
    temp_c_ = temp_c;

    // VOC-OCV model
    double b, a, c, log_soc, exp_n_soc, pow_log_soc;
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
    predict_ekf(ib);            // u = ib
    update_ekf(voc_dyn_, 0., 1., dt);   // z = voc_dyn
    soc_ekf_ = x_ekf();         // x = Vsoc (0-1 ideal capacitor voltage)

    // Coulomb counter  TODO:  move to main and use rp.delta_soc
    coulomb_counter_avail(dt, saturated);

    if ( rp.debug==-34 )
    {
        Serial.printf("dt,ib,voc_dyn,vdyn,vb,   u,Fx,Bu,P,   z_,S_,K_,y_,soc_ekf, soc_avail= %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,     %7.3f,%7.3f,%7.4f,%7.4f,       %7.3f,%7.4f,%7.4f,%7.4f,%7.4f, %7.4f,\n",
            dt, ib, voc_dyn_, vdyn_, vb_,     u_, Fx_, Bu_, P_,    z_, S_, K_, y_, soc_ekf_, soc_avail_);
    }
    if ( rp.debug==-37 )
    {
        Serial.printf("ib,vb*10-110,voc_dyn(z_)*10-110,  K_,y_,SOC_ekf-90, SOC_avail-90\n");
        Serial.printf("%7.3f,%7.3f,%7.3f,      %7.4f,%7.4f,%7.4f, %7.4f,\n",
            ib, vb*10-110, voc_dyn_*10-110,     K_, y_, soc_ekf_*100-90, soc_avail_*100-90);
    }

    // Summarize
    pow_in_ekf_ = vb*ib - ib*ib*(r1_+r2_)*sr_*num_cells_;  // Internal resistance of battery is a loss
    if ( pow_in_ekf_>1. )  tcharge_ = min(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * (1.-soc_ekf_), 24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
    else if ( pow_in_ekf_<-1. ) tcharge_ekf_ = max(NOM_BATT_CAP /pow_in_ekf_*NOM_SYS_VOLT * soc_ekf_, -24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
    else if ( pow_in_ekf_>=0. ) tcharge_ekf_ = 24.*(1.-soc_ekf_);
    else tcharge_ = -24.*soc_ekf_;
    // vsat_ = nom_vsat_ + (temp_c-25.)*dvoc_dt_;
    // sat_ = voc_ >= vsat_;

    if ( rp.debug==-9 )Serial.printf("tempc=%7.3f", temp_c);
    
    return ( soc_ekf_ );
}

// SOC-OCV curve fit method per Zhang, et al.   Makes a good reference model
double Battery::calculate_model(const double temp_C, const double socu_frac, const double curr_in, const double dt)
{
    dt_ = dt;

    socu_ = socu_frac;
    socs_ = 1-(1-socu_)*cu_bb/cs_bb;
    double socs_lim = max(min(socs_, mxeps_bb), mneps_bb);
    ib_ = curr_in;

    // VOC-OCV model
    double log_socs, exp_n_socs, pow_log_socs;
    calc_soc_voc_coeff(socs_lim, temp_C, &b_, &a_, &c_, &log_socs, &exp_n_socs, &pow_log_socs);
    voc_ = calc_voc_ocv(socs_lim, &dv_dsocs_, b_, a_, c_, log_socs, exp_n_socs, pow_log_socs)
             + (socs_ - socs_lim) * dv_dsocs_;  // slightly beyond
    voc_ +=  dv_;  // Experimentally varied
    // dv_dsocu_ = dv_dsocs_ * cu_bb / cs_bb;

    // Dynamic emf
    // vdyn_ = double(num_cells_) * ib_*(r1_ + r2_)*sr_;
    double u[2] = {ib_, voc_};
    RandlesInv_->calc_x_dot(u);
    RandlesInv_->update(dt);
    vb_ = max(RandlesInv_->y(0), 5.);
    vdyn_ = vb_ - voc_;


    // Summarize   TODO: get rid of the global defines here because they differ from one battery to another
    pow_in_ = vb_*ib_ - ib_*ib_*(r1_+r2_)*sr_*num_cells_;  // Internal resistance of battery is a loss
    // if ( pow_in_>1. )  tcharge_ = min(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * (1.-socs_lim), 24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
    // else if ( pow_in_<-1. ) tcharge_ = max(NOM_BATT_CAP /pow_in_*NOM_SYS_VOLT * socs_lim, -24.);  // NOM_BATT_CAP is defined at NOM_SYS_VOLT
    // else if ( pow_in_>=0. ) tcharge_ = 24.*(1.-socs_lim);
    // else tcharge_ = -24.*socs_lim;
    vsat_ = nom_vsat_ + (temp_C-25.)*dvoc_dt_;
    // sat_ = voc_ >= vsat_;

    if ( rp.debug==-38 ) Serial.printf("calculate_ model:  SOCU_in,v,curr,pow,vsat,voc= %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
      socu_frac, vb_, ib_, pow_in_, vsat_, voc_);

    if ( rp.debug==-39 )Serial.printf("calculate_model:  tempC,tempF,curr,a,b,c,d,n,m,r,soc,logsoc,expnsoc,powlogsoc,voc,vdyn,v,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
     temp_C, temp_C*9./5.+32., ib_, a_, b_, c_, d_, n_, m_, (r1_+r2_)*sr_ , socs_, log_socs, exp_n_socs, pow_log_socs, voc_, vdyn_, vb_);

    return ( vb_ );
}

/* Count coulombs base on true=actual capacity  TODO:  move to main
    Internal resistance of battery is a loss
    Inputs:
        ioc     Charge current, A
        voc_dyn Charge voltage calculated from dynamics, V
        vb      Battery terminal voltage, V
        i_r_dif Current in diffusion process, A
        i_r_ct  Current in charge transfer process, A
        sr      Experimental scalar
    Outputs:
        soc_avail   State of charge, fraction (0-1.5)
        # soc_norm    Normalized state of charge, fraction (0-1)
        v_sat   Charge voltage at saturation, V
        sat     Battery is saturated, T/F
*/
double Battery::coulomb_counter_avail(const double dt, const boolean saturated)
{
    double delta_delta_soc = pow_in_ekf_/NOM_SYS_VOLT*dt/3600./TRUE_BATT_CAP;
    rp.delta_soc = max(min(rp.delta_soc + delta_delta_soc, 1.5), -1.5);
    if ( saturated )
    {
        rp.delta_soc = 0.;
        rp.t_sat = temp_c_;
        rp.soc_sat = (rp.t_sat - 25.)*dQdT_ + 1.;
    }
    soc_avail_ = max(min(rp.soc_sat*(1. - dQdT_*(temp_c_ - rp.t_sat)) + rp.delta_soc, 1.5), 0.);
    if ( rp.debug==-36 )
    {
        Serial.printf("coulomb_counter_avail:  sat, pow_in_ekf, delta_delta_soc, delta_soc, soc_sat, tsat,-->,soc_avail=     %d,%7.3f,%10.6f,%10.6f,%7.3f,%7.3f,-->,%7.3f,\n",
                    saturated, pow_in_ekf_, delta_delta_soc, rp.delta_soc, rp.soc_sat, rp.t_sat, soc_avail_);
    }
    return ( soc_avail_ );
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
    *hx = calc_voc_ocv(x_lim, &dv_dsoc, b, a, c, log_soc, exp_n_soc, pow_log_soc);

    // Jacodian of measurement function
    *H = dv_dsoc;
}

// Init EKF
void Battery::init_soc_ekf(const double socu_free_in)
{
    soc_ekf_ = 1-(1-socu_free_in)*cu_bb/cs_bb;
    qsat_ = socu_free_in*TRUE_BATT_CAP;
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
