/***************************************************
  A simple dynamic filter library

  Class code for embedded application.

  07-Jan-2015   Dave Gutz   Created
  30-Sep-2016   Dave Gutz   LeadLagTustin
  23-Nov-2016   Dave Gutz   LeadLagExp
 ****************************************************/
#include "myFilters.h"
#include "math.h"

//#include <Arduino.h> //needed for Serial.println

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif


// class Debounce
// constructors
Debounce::Debounce()
    : nz_(1) {}
Debounce::  Debounce(const bool icValue, const int updates)
    : nz_(fmax(updates-1, 1))
{
  past_ = new bool[nz_];
  for (int i=0; i<nz_; i++) past_[i] = icValue;
}
Debounce::~Debounce() {}
// operators
// functions
bool Debounce::calculate(const bool in)
{
  bool past = past_[nz_-1];
  bool fail = false;
  for ( int i=0; i<nz_; i++ ) if ( in!=past_[i] ) fail = true;
  for ( int i=nz_-1; i>0; i-- ) past_[i] = past_[i-1];
  past_[0] = in;
  bool out = in;
  if ( fail ) out = past;
  return ( out );
}
bool Debounce::calculate(const bool in, const int RESET)
{
  if ( RESET ) for (int i=0; i<nz_; i++) past_[i] = in;
  return ( Debounce::calculate(in) );
}


// class DetectRise
// constructors
DetectRise::DetectRise()
    : past_(0) {}
DetectRise::~DetectRise() {}
// operators
// functions
bool DetectRise::calculate(const double in)
{
  bool out = false;
  if ( in > past_ ) out = true;
  past_ = in;
  return ( out );
}
bool DetectRise::calculate(const bool in)
{
  return ( DetectRise::calculate(double(in)) ); 
}
bool DetectRise::calculate(const int in)
{
  return ( DetectRise::calculate(double(in)) ); 
}


// class TFDelay
// constructors
TFDelay::TFDelay()
    : timer_(0), nt_(0), nf_(0), T_(1) {}
TFDelay::TFDelay(const bool in, const double Tt, const double Tf, const double T)
    : timer_(0), nt_(int(fmax(round(Tt/T)+1,0))), nf_(int(fmax(round(Tf/T+1),0))), T_(T)
{
  if ( Tt == 0 ) nt_ = 0;
  if ( Tf == 0 ) nf_ = 0;
  if ( in ) timer_ = nf_;
  else timer_ = -nt_;
}
TFDelay::~TFDelay() {}
// operators
// functions
double TFDelay::calculate(const bool in)
{
  if ( timer_ >= 0 )
  {
    if ( in ) timer_ = nf_;
    else
    {
      timer_--;
      if ( timer_<0 ) timer_ = -nt_;
    }
  }
  else
  {
    if ( !in ) timer_ = -nt_;
    else
    {
      timer_++;
      if ( timer_>=0 ) timer_=nf_;
    }
  }
  // Serial.print("in=");Serial.print(in);Serial.print(", timer=");Serial.print(timer_);Serial.print(", nt_=");Serial.print(nt_);
  // Serial.print(", nf_="); Serial.print(nf_);Serial.print(", return=");Serial.println(timer_>=0);
  return ( timer_> 0 );
}
double TFDelay::calculate(const bool in, const int RESET)
{
  bool out;
  if (RESET>0)
  {
      if ( in ) timer_ = nf_;
      else timer_ = -nt_;
      out = in;
  }
  else
  {
    out = TFDelay::calculate(in);
  }
  return ( out );
}
double TFDelay::calculate(const bool in, const double Tt, const double Tf)
{
  nt_ = int(fmax(round(Tt/T_),0));
  nf_ = int(fmax(round(Tf/T_),0));
  return(TFDelay::calculate(in));
}
double TFDelay::calculate(const bool in, const double Tt, const double Tf, const double T)
{
  T_ = T;
  nt_ = int(fmax(round(Tt/T_),0));
  nf_ = int(fmax(round(Tf/T_),0));
  return(TFDelay::calculate(in));
}
double TFDelay::calculate(const bool in, const double Tt, const double Tf, const int RESET)
{
  if (RESET>0)
  {
    if ( in ) timer_ = nf_;
    else timer_ = -nt_;
  }
  return(TFDelay::calculate(in, Tt, Tf));
}
double TFDelay::calculate(const bool in, const double Tt, const double Tf, const double T, const int RESET)
{
  if (RESET>0)
  {
    if ( in ) timer_ = nf_;
    else timer_ = -nt_;
  }
  return(TFDelay::calculate(in, Tt, Tf, T));
}


// class SRLatch
// constructors
SRLatch::SRLatch()
    : state_(false) {}
SRLatch::SRLatch(const bool icValue)
    : state_(icValue){}
SRLatch::~SRLatch() {}
// operators
// functions
bool SRLatch::calculate(const bool S, const bool R)
{
  if ( R ) state_ = false;   // Reset overrides Set
  else if ( S ) state_ = true;
  return (state_);
}


// class Delay
// constructors
Delay::Delay()
    : nz_(0) {}
Delay::Delay(const double in, const int nz)
    : nz_(fmax(nz, 1))
{
  past_ = new double[nz_];
  for ( int i=0; i<nz_; i++ ) past_[i] = in;
}
Delay::~Delay() {}
// operators
// functions
double Delay::calculate(const double in)
{
  double out = past_[nz_-1];
  for (int i=nz_-1; i>0; i--) past_[i] = past_[i-1];
  past_[0] = in;
  return (out);
}
double Delay::calculate(const double in, const int RESET)
{
  if (RESET>0)
  {
      for (int i=0; i<nz_; i++) past_[i] = in;
      return(in);
  }
  else
  {
    return(Delay::calculate(in));
  }
}


// class RateLimit
// constructors
RateLimit::RateLimit()
    : past_(0), jmax_(0), jmin_(0), T_(1) {}
RateLimit::RateLimit(const double I, const double T)
    : past_(I), jmax_(0), jmin_(0), T_(T) {}
RateLimit::RateLimit(const double I, const double T, const double Rmax, const double Rmin)
    : past_(I), jmax_(fabs(Rmax*T)), jmin_(-fabs(Rmin*T)), T_(T) {}
RateLimit::~RateLimit() {}
// operators
// functions
double RateLimit::calculate(const double in)
{
  double out = fmax( fmin( in, past_+jmax_), past_+jmin_);
  past_ = in;
  return (out);
}
double RateLimit::calculate(const double in, const int RESET)
{
  if (RESET>0)
  {
    past_ = in;
  }
  double out = RateLimit::calculate(in);
  return(out);
}
double RateLimit::calculate(const double in, const double Rmax, const double Rmin)
{
  jmax_ = fabs(Rmax*T_);
  jmin_ = -fabs(Rmin*T_);
  double out = RateLimit::calculate(in);
  return(out);
}
double RateLimit::calculate(const double in, const double Rmax, const double Rmin, const int RESET)
{
  if (RESET>0)
  {
    past_ = in;
  }
  double out = RateLimit::calculate(in, Rmax, Rmin);
  return(out);
}


// class SlidingDeadband
// constructors
SlidingDeadband::SlidingDeadband()
    : z_(0), hdb_(0) {}
SlidingDeadband::SlidingDeadband(const double hdb)
    : z_(0), hdb_(hdb) {}
SlidingDeadband::~SlidingDeadband() {}
// operators
// functions
double SlidingDeadband::update(const double in)
{
  z_ = fmax( fmin( z_, in+hdb_), in-hdb_);
  return (z_);
}
double SlidingDeadband::update(const double in, const int RESET)
{
  if (RESET>0)
  {
    z_ = in;
  }
  double out = SlidingDeadband::update(in);
  return(out);
}


// **************************** First Order Filters *************************************
// class DiscreteFilter
// constructors
DiscreteFilter::DiscreteFilter()
    : max_(1e32), min_(-1e32), rate_(0.0), T_(1.0), tau_(0.0) {}
DiscreteFilter::DiscreteFilter(const double T, const double tau, const double min, const double max)
    : max_(max), min_(min), rate_(0.0), T_(T), tau_(tau) {}
DiscreteFilter::~DiscreteFilter() {}
// operators
// functions
double DiscreteFilter::calculate(double input, int RESET)
{
  if (RESET > 0)
  {
    rate_ = 0.0;
  }
  return (rate_);
}
void DiscreteFilter::rateState(double in) {}
double DiscreteFilter::rateStateCalc(double in) { return (0); }
void DiscreteFilter::assignCoeff(double tau) {}
double DiscreteFilter::state(void) { return (0); }


// Tustin rate-lag rate calculator, non-pre-warped, no limits, fixed update rate
// constructors
RateLagTustin::RateLagTustin() : DiscreteFilter() {}
RateLagTustin::RateLagTustin(const double T, const double tau, const double min, const double max)
    : DiscreteFilter(T, tau, min, max)
{
  RateLagTustin::assignCoeff(tau);
}
RateLagTustin::~RateLagTustin() {}
// operators
// functions
double RateLagTustin::calculate(double in, int RESET)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  RateLagTustin::rateState(in);
  return (rate_);
}
void RateLagTustin::rateState(double in)
{
  rate_ = fmax(fmin(a_ * (in - state_), max_), min_);
  state_ = in * (1.0 - b_) + state_ * b_;
}
void RateLagTustin::assignCoeff(double tau)
{
  tau_ = tau;
  a_ = 2.0 / (2.0 * tau_ + T_);
  b_ = (2.0 * tau_ - T_) / (2.0 * tau_ + T_);
}
double RateLagTustin::state(void) { return (state_); };


// Tustin lead-lag alculator, non-pre-warped, no limits, fixed update rate
// constructors
LeadLagTustin::LeadLagTustin() : DiscreteFilter() {}
LeadLagTustin::LeadLagTustin(const double T, const double tld, const double tau, const double min, const double max)
    : DiscreteFilter(T, tau, min, max)
{
  LeadLagTustin::assignCoeff(tld, tau, T);
}
LeadLagTustin::~LeadLagTustin() {}
// operators
// functions
double LeadLagTustin::calculate(double in, int RESET)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  double out = LeadLagTustin::rateStateCalc(in);
  return (out);
}
double LeadLagTustin::calculate(double in, int RESET, const double T, const double tau, const double tld)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  LeadLagTustin::assignCoeff(tld, tau, T);
  double out = LeadLagTustin::rateStateCalc(in, T);
  return (out);
}
double LeadLagTustin::calculate(double in, int RESET, const double T)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  double out = LeadLagTustin::rateStateCalc(in, T);
  return (out);
}
double LeadLagTustin::rateStateCalc(const double in)
{
  double out = rate_ + state_;
  rate_ = fmax(fmin(b_ * (in - state_), max_), min_);
  state_ = in * (1.0 - a_) + state_ * a_;
  return (out);
}
double LeadLagTustin::rateStateCalc(const double in, const double T)
{
  assignCoeff(tld_, tau_, T);
  double out = rateStateCalc(in);
  return (out);
}
void LeadLagTustin::assignCoeff(const double tld, const double tau, const double T)
{
  T_ = T;
  tld_ = tld;
  tau_ = tau;
  a_ = (2.0 * tau - T_) / (2.0 * tau_ + T_);
  b_ = (2.0 * tld_ + T_) / (2.0 * tau_ + T_);
}
double LeadLagTustin::state(void) { return (state_); };


// Exponential lead-lag calculator, non-pre-warped, no limits, fixed update rate
// http://www.mathpages.com/home/kmath198/2-2/2-2.htm
// constructors
LeadLagExp::LeadLagExp() : DiscreteFilter() {}
LeadLagExp::LeadLagExp(const double T, const double tld, const double tau, const double min, const double max)
    : DiscreteFilter(T, tau, min, max)
{
  LeadLagExp::assignCoeff(tld, tau, T);
}
LeadLagExp::~LeadLagExp() {}
// operators
// functions
double LeadLagExp::calculate(double in, int RESET)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  double out = LeadLagExp::rateStateCalc(in);
  return (out);
}
double LeadLagExp::calculate(double in, int RESET, const double T, const double tau, const double tld)
{
  if (RESET > 0)
  {
    instate_ = in;
    state_ = in;
  }
  LeadLagExp::assignCoeff(tld, tau, T);
  double out = LeadLagExp::rateStateCalc(in, T);
  return (out);
}
double LeadLagExp::calculate(double in, int RESET, const double T)
{
  if (RESET > 0)
  {
    instate_ = in;
    state_ = in;
  }
  double out = LeadLagExp::rateStateCalc(in, T);
  return (out);
}
double LeadLagExp::rateStateCalc(const double in)
{
  rate_ = fmax(fmin(b_ * (in - instate_), max_), min_);
  state_ += (a_ * (instate_ - state_) + rate_);
  instate_ = in;
  return (state_);
}
double LeadLagExp::rateStateCalc(const double in, const double T)
{
  assignCoeff(tld_, tau_, T);
  double out = rateStateCalc(in);
  return (out);
}
void LeadLagExp::assignCoeff(const double tld, const double tau, const double T)
{
  T_   = fmax(T, 1e-9);
  tld_ = fmax(tld, 0.0);
  tau_ = fmax(tau, 0.0);
  if (tau_ > 0.)  a_ = 1.0 - exp(-T_ / tau_);
  else            a_ = 1.0;
  b_ = 1.0 + a_ * (tld_ - tau_) / T_;
}
double LeadLagExp::state(void) { return (state_); };


// Exponential rate-lag rate calculator, non-pre-warped, no limits, fixed update rate
// constructors
RateLagExp::RateLagExp() : DiscreteFilter() {}
RateLagExp::RateLagExp(const double T, const double tau, const double min, const double max)
    : DiscreteFilter(T, tau, min, max)
{
  RateLagExp::assignCoeff(tau);
}
RateLagExp::~RateLagExp() {}
// operators
// functions
double RateLagExp::calculate(double in, int RESET)
{
  if (RESET > 0)
  {
    lstate_ = in;
    rstate_ = in;
  }
  RateLagExp::rateState(in);
  return (rate_);
}
double RateLagExp::calculate(double in, int RESET, const double T)
{
  if (RESET > 0)
  {
    lstate_ = in;
    rstate_ = in;
  }
  RateLagExp::rateState(in, T);
  return (rate_);
}
void RateLagExp::rateState(double in)
{
  rate_ = fmax(fmin(c_ * (a_ * rstate_ + b_ * in - lstate_), max_), min_);
  rstate_ = in;
  lstate_ += T_ * rate_;
}
void RateLagExp::rateState(double in, const double T)
{
  T_ = T;
  assignCoeff(tau_);
  rateState(in);
}
void RateLagExp::assignCoeff(double tau)
{
  double eTt = exp(-T_ / tau_);
  a_ = tau_ / T_ - eTt / (1 - eTt);
  b_ = 1.0 / (1 - eTt) - tau_ / T_;
  c_ = (1.0 - eTt) / T_;
}
double RateLagExp::state(void) { return (lstate_); };


// Tustin lag calculator, non-pre-warped, no limits, fixed update rate
// constructors
LagTustin::LagTustin() : DiscreteFilter() {}
LagTustin::LagTustin(const double T, const double tau, const double min, const double max)
    : DiscreteFilter(T, tau, min, max)
{
  LagTustin::assignCoeff(tau);
}
LagTustin::~LagTustin() {}
// operators
// functions
double LagTustin::calculate(double in, int RESET)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  LagTustin::calcState(in);
  return (state_);
}
double LagTustin::calculate(double in, int RESET, const double T)
{
  if (RESET > 0)
  {
    state_ = in;
  }
  LagTustin::calcState(in, T);
  return (state_);
}
void LagTustin::calcState(double in)
{
  rate_ = fmax(fmin(a_ * (in - state_), max_), min_);
  state_ =fmax(fmin(in * (1.0 - b_) + state_ * b_, max_), min_);  // dag 12/22/2020
}
void LagTustin::calcState(double in, const double T)
{
  T_ = T;
  assignCoeff(tau_);
  calcState(in);
}
void LagTustin::assignCoeff(double tau)
{
  tau_ = tau;
  a_ = 2.0 / (2.0 * tau_ + T_);
  b_ = (2.0 * tau_ - T_) / (2.0 * tau_ + T_);
}
double LagTustin::state(void) { return (state_); };


// Exp lag calculator variable update rate and limits
// constructors
LagExp::LagExp() : DiscreteFilter() {}
LagExp::LagExp(const double T, const double tau, const double min, const double max)
    : DiscreteFilter(T, tau, min, max)
{
  LagExp::assignCoeff(tau);
}
LagExp::~LagExp() {}
// operators
// functions
double LagExp::calculate(double in, int RESET)
{
  if (RESET > 0)
  {
    lstate_ = in;
    rstate_ = in;
    rate_ = 0;
  }
  LagExp::rateState(in);
  return (rate_);
}
double LagExp::calculate(double in, int RESET, const double T)
{
  if (RESET > 0)
  {
    lstate_ = in;
    rstate_ = in;
  }
  LagExp::rateState(in, T);
  return (lstate_);
}
void LagExp::rateState(double in)
{
  rate_ = c_ * (a_ * rstate_ + b_ * in - lstate_);
  rstate_ = in;
  lstate_ = fmax(fmin(lstate_ + T_ * rate_, max_), min_);
}
void LagExp::rateState(double in, const double T)
{
  T_ = T;
  assignCoeff(tau_);
  rateState(in);
}
void LagExp::assignCoeff(double tau)
{
  double eTt = exp(-T_ / tau_);
  double meTt = 1 - eTt;
  a_ = tau_ / T_ - eTt / meTt;
  b_ = 1.0 / meTt - tau_ / T_;
  c_ = meTt / T_;
}
double LagExp::state(void) { return (lstate_); };


// ***************************** Integrators ******************************************
// class DiscreteIntegrator
// constructors
DiscreteIntegrator::DiscreteIntegrator()
  : max_(1e32), min_(-1e32), T_(1.0){}
DiscreteIntegrator::DiscreteIntegrator(const double T, const double min, const double max, 
  const double a, const double b, const double c)
  : a_(a), b_(b), c_(c), lim_(false), max_(max), min_(min), lstate_(0), rstate_(0), T_(T) {}
DiscreteIntegrator::~DiscreteIntegrator() {}
// operators
// functions
void DiscreteIntegrator::newState(double newState)
{
  lstate_ = max(min(newState, max_), min_);
  rstate_ = 0.0;
}
double DiscreteIntegrator::calculate(double in, int RESET, double init_value)
{
  if (RESET > 0)
  {
    lstate_ = init_value;  rstate_ = 0.0;
  }
  else
  {
    lstate_ += (a_*in + b_*rstate_)*T_/c_;
  }
  if ( lstate_<min_ )
  {
    lstate_ = min_;  lim_ = true;  rstate_ = 0.0;
  }
  else if ( lstate_>max_ )
  {
    lstate_ = max_;  lim_ = true;  rstate_ = 0.0;
  }
  else
  {
    lim_ = false;  rstate_ = in;
  }
  return (lstate_);
}
double DiscreteIntegrator::calculate(double in, double T, int RESET, double init_value)
{
  T_ = T;
  if (RESET > 0)
  {
    lstate_ = init_value;  rstate_ = 0.0;
  }
  else
  {
    lstate_ += (a_*in + b_*rstate_)*T_/c_;
  }
  if ( lstate_<min_ )
  {
    lstate_ = min_;  lim_ = true;  rstate_ = 0.0;
  }
  else if ( lstate_>max_ )
  {
    lstate_ = max_;  lim_ = true;  rstate_ = 0.0;
  }
  else
  {
    lim_ = false;  rstate_ = in;
  }
  return (lstate_);
}

// AB-2 Integrator future-predictor
// constructors
AB2_Integrator::AB2_Integrator() : DiscreteIntegrator() {}
AB2_Integrator::AB2_Integrator(const double T, const double min, const double max)
    : DiscreteIntegrator(T, min, max, 3.0, -1.0, 2.0)
{}
AB2_Integrator::~AB2_Integrator() {}
// operators
// functions

// Tustin Integrator updater
// constructors
TustinIntegrator::TustinIntegrator() : DiscreteIntegrator() {}
TustinIntegrator::TustinIntegrator(const double T, const double min, const double max)
    : DiscreteIntegrator(T, min, max, 1.0, 1.0, 2.0)
{}
TustinIntegrator::~TustinIntegrator() {}
// operators
// functions


// ************************ 2-Pole Filters *************************************************************


DiscreteFilter2::DiscreteFilter2()
  : max_(0), min_(0), omega_n_(0), T_(0), zeta_(0) {}
DiscreteFilter2::DiscreteFilter2(const double T, const double omega_n, const double zeta, const double min, const double max)
  : max_(max), min_(min), omega_n_(omega_n), T_(T), zeta_(zeta)
   {
   }
DiscreteFilter2::~DiscreteFilter2() {}
// functions
double DiscreteFilter2::calculate(const double in, const int RESET) {return (0.0);}
void DiscreteFilter2::assignCoeff(const double T) {}
void DiscreteFilter2::rateState(const double in, const int RESET) {}
void DiscreteFilter2::rateStateCalc(const double in, const double T, const int RESET) {}


// General 2-Pole filter variable update rate and limits, poor aliasing characteristics
// constructors
General2_Pole::General2_Pole() : DiscreteFilter2() {}
General2_Pole::General2_Pole(const double T, const double omega_n, const double zeta, const double min, const double max)
    : DiscreteFilter2(T, omega_n, zeta, min, max)
{
  AB2_ = new AB2_Integrator(T, -1e12, 1e12);
  Tustin_ = new TustinIntegrator(T, min, max);
  a_ = 2 * zeta_ * omega_n_;
  b_ = omega_n_ * omega_n_;
  General2_Pole::assignCoeff(T);
}
General2_Pole::~General2_Pole() {}
// operators
// functions
double General2_Pole::calculate(double in, int RESET)
{
  General2_Pole::rateState(in, RESET);
  return (Tustin_->state());
}
void General2_Pole::assignCoeff(const double T)
{
  T_ = T;
}
double General2_Pole::calculate(double in, int RESET, const double T)
{
  General2_Pole::assignCoeff(T);
  General2_Pole::rateStateCalc(in, T, RESET);
  return (Tustin_->state());
}
void General2_Pole::rateState(double in, int RESET)
{
  double accel;
  if ( RESET>0 )
  {
    accel = 0;
  }
  else
  {
    accel = b_*(in - Tustin_->state()) - a_*AB2_->state();
  }
  Tustin_->calculate(AB2_->calculate(accel, T_, RESET, 0), T_, RESET, in);
  if ( Tustin_->lim() )
  {
    AB2_->newState(0);
  }
}
void General2_Pole::rateStateCalc(double in, const double T, const int RESET)
{
  assignCoeff(T);
  rateState(in, RESET);
}
