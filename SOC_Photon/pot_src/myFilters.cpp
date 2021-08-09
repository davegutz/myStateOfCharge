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

extern int verbose;


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
//RateLagTustin::RateLagTustin(const RateLagTustin & RLT)
//: DiscreteFilter(RLT.T_, RLT.tau_, RLT.min_, RLT.max_){}
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
//LeadLagTustin::LeadLagTustin(const LeadLagTustin & RLT)
//: DiscreteFilter(RLT.T_, RLT.tau_, RLT.min_, RLT.max_){}
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
//LeadLagExp::LeadLagExp(const LeadLagExp & RLT)
//: DiscreteFilter(RLT.T_, RLT.tau_, RLT.min_, RLT.max_){}
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
//RateLagExp::RateLagExp(const RateLagExp & RLT)
//: DiscreteFilter(RLT.T_, RLT.tau_, RLT.min_, RLT.max_){}
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
//LagTustin::LagTustin(const LagTustin & RLT)
//: DiscreteFilter(RLT.T_, RLT.tau_, RLT.min_, RLT.max_){}
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

