/***************************************************
  A simple dynamic filter library

  Class code for embedded application.

  07-Jan-2015   Dave Gutz   Created
  30-Sep-2016   Dave Gutz   LeadLagTustin
  23-Nov-2016   Dave Gutz   LeadLagExp
  09-Feb-2021   Dave Gutz   RateLagExp, LagExp, General2_Pole
 ****************************************************/

#ifndef _myFilters_H
#define _myFilters_H

#include "application.h" // Should not be needed if file ino or Arduino
#include <math.h>

#define DEAD(X, HDB)  ( max(X-HDB, 0) + min(X+HDB, 0) )


class Debounce
{
public:
  Debounce();
  Debounce(const bool icValue, const int updates);
  ~Debounce();
  // operators
  // functions
  bool calculate(const bool in);
  bool calculate(const bool in, const int RESET);
protected:
  int nz_;     // Number of past consequetive states to agree with input to pass debounce
  bool *past_; // Array(nz_-1) of past inputs
};


class DetectRise
{
public:
  DetectRise();
  ~DetectRise();
  // operators
  // functions
  bool calculate(const bool in);
  bool calculate(const int in);
  bool calculate(const double in);
protected:
  double past_;
};


class SRLatch
{
public:
  SRLatch();
  SRLatch(const bool icValue);
  ~SRLatch();
  // operators
  // functions
  bool calculate(const bool S, const bool R);
protected:
  bool state_;
};


class Delay
{
public:
  Delay();
  Delay(const double in, const int nz);
  ~Delay();
  // operators
  // functions
  double calculate(const double in);
  double calculate(const double in, const int RESET);
protected:
  double *past_;
  int nz_;
};


class TFDelay
{
public:
  TFDelay();
  TFDelay(const bool in, const double Tt, const double Tf, const double T);
  ~TFDelay();
  // operators
  // functions
  double calculate(const bool in);
  double calculate(const bool in, const int RESET);
  double calculate(const bool in, const double Tt, const double Tf);
  double calculate(const bool in, const double Tt, const double Tf, const double T);
  double calculate(const bool in, const double Tt, const double Tf, const int RESET);
  double calculate(const bool in, const double Tt, const double Tf, const double T, const int RESET);
protected:
  int timer_;
  int nt_;
  int nf_;
  double T_;
};


class RateLimit
{
public:
  RateLimit();
  RateLimit(const double in, const double T);
  RateLimit(const double in, const double T, const double Rmax, const double Rmin);
  ~RateLimit();
  // operators
  // functions
  double calculate(const double in);
  double calculate(const double in, const int RESET);
  double calculate(const double in, const double Rmax, const double Rmin);
  double calculate(const double in, const double Rmax, const double Rmin, const int RESET);
protected:
  double past_;
  double jmax_;   // Max rate limit, units of in/update
  double jmin_;   // Min rate limit, units of in/update (<0)
  double T_;      // Update rate, sec
};


// ************************** 1-Pole Filters ***********************************************
class DiscreteFilter
{
public:
  DiscreteFilter();
  DiscreteFilter(const double T, const double tau, const double min, const double max);
  virtual ~DiscreteFilter();
  // operators
  // functions
  virtual double calculate(double in, int RESET);
  virtual void assignCoeff(double tau);
  virtual void rateState(double in);
  virtual double rateStateCalc(double in);
  virtual double state(void);
protected:
  double max_;
  double min_;
  double rate_;
  double T_;
  double tau_;
};


// Tustin rate-lag rate calculator, non-pre-warped, no limits, fixed update rate
class LeadLagTustin : public DiscreteFilter
{
public:
  LeadLagTustin();
  LeadLagTustin(const double T, const double tld, const double tau, const double min, const double max);
  //  LeadLagTustin(const LeadLagTustin & RLT);
  ~LeadLagTustin();
  //operators
  //functions
  virtual double calculate(const double in, const int RESET);
  virtual double calculate(const double in, const int RESET, const double T);
  virtual double calculate(double in, int RESET, const double T, const double tau, const double tld);
  virtual void assignCoeff(const double tld, const double tau, const double T);
  virtual double rateStateCalc(const double in);
  virtual double rateStateCalc(const double in, const double T);
  virtual double state(void);
protected:
  double a_;
  double b_;
  double state_;
  double tld_;
};


// Tustin rate-lag rate calculator, non-pre-warped, no limits, fixed update rate
class LeadLagExp : public DiscreteFilter
{
public:
  LeadLagExp();
  LeadLagExp(const double T, const double tld, const double tau, const double min, const double max);
  //  LeadLagExp(const LeadLagExp & RLT);
  ~LeadLagExp();
  //operators
  //functions
  virtual double calculate(const double in, const int RESET);
  virtual double calculate(const double in, const int RESET, const double T);
  virtual double calculate(double in, int RESET, const double T, const double tau, const double tld);
  virtual void assignCoeff(const double tld, const double tau, const double T);
  virtual double rateStateCalc(const double in);
  virtual double rateStateCalc(const double in, const double T);
  virtual double state(void);
protected:
  double a_;
  double b_;
  double state_;
  double instate_;
  double tld_;
};


// Tustin rate-lag rate calculator, non-pre-warped, no limits, fixed update rate
class RateLagTustin : public DiscreteFilter
{
public:
  RateLagTustin();
  RateLagTustin(const double T, const double tau, const double min, const double max);
  //  RateLagTustin(const RateLagTustin & RLT);
  ~RateLagTustin();
  //operators
  //functions
  virtual double calculate(double in, int RESET);
  virtual void assignCoeff(double tau);
  virtual void rateState(double in);
  virtual double state(void);
protected:
  double a_;
  double b_;
  double state_;
};


// Exponential rate-lag rate calculator
class RateLagExp : public DiscreteFilter
{
public:
  RateLagExp();
  RateLagExp(const double T, const double tau, const double min, const double max);
  ~RateLagExp();
  //operators
  //functions
  virtual double calculate(double in, int RESET);
  virtual double calculate(double in, int RESET, const double T);
  virtual void assignCoeff(double tau);
  virtual void rateState(double in);
  virtual void rateState(double in, const double T);
  virtual double state(void);
  double a() { return (a_); };
  double b() { return (b_); };
  double c() { return (c_); };
  double lstate() { return (lstate_); };
  double rstate() { return (rstate_); };
protected:
  double a_;
  double b_;
  double c_;
  double lstate_; // lag state
  double rstate_; // rate state
};


// Tustin lag calculator
class LagTustin : public DiscreteFilter
{
public:
  LagTustin();
  LagTustin(const double T, const double tau, const double min, const double max);
  //  LagTustin(const LagTustin & RLT);
  ~LagTustin();
  //operators
  //functions
  virtual double calculate(double in, int RESET);
  virtual double calculate(double in, int RESET, const double T);
  virtual void assignCoeff(double tau);
  virtual void calcState(double in);
  virtual void calcState(double in, const double T);
  virtual double state(void);
  double a() { return (a_); };
  double b() { return (b_); };
  double rate() { return (rate_); };
protected:
  double a_;
  double b_;
  double rate_;
  double state_;
};


// Exponential lag calculator
class LagExp : public DiscreteFilter
{
public:
  LagExp();
  LagExp(const double T, const double tau, const double min, const double max);
  ~LagExp();
  //operators
  //functions
  virtual double calculate(double in, int RESET);
  virtual double calculate(double in, int RESET, const double T);
  virtual void assignCoeff(double tau);
  virtual void rateState(double in);
  virtual void rateState(double in, const double T);
  virtual double state(void);
  double a() { return (a_); };
  double b() { return (b_); };
  double c() { return (c_); };
  double rate() { return (rate_); };
  double lstate() { return (lstate_); };
  double rstate() { return (rstate_); };
protected:
  double a_;
  double b_;
  double c_;
  double rate_;
  double lstate_;
  double rstate_;
};


// *********************** Integrators *******************************************
class DiscreteIntegrator
{
public:
  DiscreteIntegrator();
  DiscreteIntegrator(const double T, const double min, const double max, const double a, const double b, const double c);
  virtual ~DiscreteIntegrator();
  // operators
  // functions
  virtual double calculate(double in, int RESET, double init_value);
  virtual double calculate(double in, double T, int RESET, double init_value);
  virtual void newState(double newState);
  virtual double state() { return lstate_; };
  virtual bool lim() { return lim_; };
protected:
  double a_;
  double b_;
  double c_;
  bool lim_;
  double max_;
  double min_;
  double lstate_;
  double rstate_;
  double T_;
};


// AB2_Integrator
class AB2_Integrator : public DiscreteIntegrator
{
public:
  AB2_Integrator();
  AB2_Integrator(const double T, const double min, const double max);
  ~AB2_Integrator();
  //operators
  //functions
protected:
};


// Tustin Integrator
class TustinIntegrator : public DiscreteIntegrator
{
public:
  TustinIntegrator();
  TustinIntegrator(const double T, const double min, const double max);
  ~TustinIntegrator();
  //operators
  //functions
protected:
};


// ************************************** 2-pole filters  *************************
class DiscreteFilter2
{
public:
  DiscreteFilter2();
  DiscreteFilter2(const double T, const double omega_n, const double zeta, const double min, const double max);
  virtual ~DiscreteFilter2();
  // operators
  // functions
  virtual double calculate(const double in, const int RESET);
  virtual void assignCoeff(const double T);
  virtual void rateState(const double in, const int RESET);
  virtual void rateStateCalc(const double in, const double T, const int RESET);
protected:
  double max_;
  double min_;
  double omega_n_;
  double T_;
  double zeta_;
};


// General 2-Pole for any value of z, aliases easily though
class General2_Pole : public DiscreteFilter2
{
public:
  General2_Pole();
  General2_Pole(const double T, const double omega_n, const double zeta, const double min, const double max);
  ~General2_Pole();
  //operators
  //functions
  virtual double calculate(const double in, const int RESET);
  virtual double calculate(const double in, const int RESET, const double T);
  virtual void assignCoeff(const double T);
  virtual void rateState(const double in, const int RESET);
  virtual void rateStateCalc(const double in, const double T, const int RESET);
protected:
  AB2_Integrator *AB2_;
  double a_;
  double b_;
  TustinIntegrator *Tustin_;
};

// PID
struct PID
{
  double G;     // Gain, r/s = %/F
  double tau;   // Lead, s
  double MAX;   // Integrator max limit, %
  double MIN;   // Integrator min limit, %
  double LLMAX; // Lead max, %
  double LLMIN; // Lead min, %
  double prop;  // Proportional output, %
  double integ; // Integrator output, %
  double DB;    // Half deadband width, deg F
  double err;   // Error, F
  double err_comp; // Compensated error, F
  double cont;  // Total control output, %
  double sd_;   // Derivative tlead lookup scalar
  double sg_;   // Integral lookup scalar
  double st_;   // Proportional lookup scalar
  double ad_;   // Derivative tlead adder
  double ag_;   // Integral lookup adder
  double at_;   // Proportional lookup adder
  double kick_th_; // Threshold to kick
  double kick_; // Kick scalar
  double Sd(void) { return(sd_);};  // Der Tld scalar
  double Sg(void) { return(sg_);};  // Gain scalar
  double St(void) { return(st_);};  // Prop Tld scalar
  double Ad(void) { return(ad_);}; 
  double Ag(void) { return(ag_);};
  double At(void) { return(at_);};
  void Sd(const double S) { sd_ = S; };
  void Ad(const double A) { ad_ = A; };
  void Sg(const double S) { sg_ = S; };
  void Ag(const double A) { ag_ = A; };
  void St(const double S) { st_ = S; };
  void At(const double A) { at_ = A; };
  PID(double G, double tau, double MAX, double MIN, double LLMAX, double LLMIN, double prop, double integ, double DB,
    double err, double err_comp, double cont, double kick_th, double kick)
  {
    this->G = G;
    this->tau = tau;
    this->MAX = MAX;
    this->MIN = MIN;
    this->LLMAX = LLMAX;
    this->LLMIN = LLMIN;
    this->prop = prop;
    this->integ = integ;
    this->DB = DB;
    this->err = err;
    this->err_comp = err_comp;
    this->cont = cont;
    this->sd_ = 1;
    this->ad_ = 0;
    this->sg_ = 1;
    this->ag_ = 0;
    this->st_ = 1;
    this->at_ = 0;
    this->kick_th_ = kick_th;
    this->kick_ = kick;
  }
  void update(bool reset, double ref, double fb, double updateTime, double init, double dyn_max, double dyn_min, bool kill_db)
  {
    err = ref - fb;
    double DB_loc = DB;
    if ( kill_db ) DB_loc = 0.;
    err_comp = DEAD(err, (DB_loc*sd_+ad_)) * (G*sg_+ag_);
    if ( fabs(err)>kick_th_ ) err_comp *= kick_;
    prop = max(min(err_comp * (tau*st_+at_), LLMAX), LLMIN);
    integ = max(min(integ + updateTime*err_comp, dyn_max-prop), dyn_min-prop);
    if ( reset ) integ = init;
    cont = max(min(integ + prop, dyn_max), dyn_min);
  }
};

#endif
