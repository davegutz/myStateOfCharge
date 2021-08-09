/***************************************************
  A simple dynamic filter library

  Class code for embedded application.

  07-Jan-2015   Dave Gutz   Created
  30-Sep-2016   Dave Gutz   LeadLagTustin
  23-Nov-2016   Dave Gutz   LeadLagExp
 ****************************************************/

#ifndef _myFilters_H
#define _myFilters_H

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
  //RateLagExp(const RateLagExp & RLT);
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

#endif

