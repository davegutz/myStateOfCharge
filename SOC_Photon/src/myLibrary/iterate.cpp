/***************************************************
  A simple dynamic filter library

  Class code for embedded application.

  07-Jan-2015   Dave Gutz   Created
  30-Sep-2016   Dave Gutz   LeadLagTustin
  23-Nov-2016   Dave Gutz   LeadLagExp
 ****************************************************/
#include "iterate.h"
#include "math.h"

// class Iterator
// constructors
Iterator::Iterator() {}
Iterator::  Iterator(const String desc)
    :count_(0), desc_(desc), de_(0), des_(0), dx_(0), e_(0), ep_(0), limited_(false), x_(0), xmax_(0), xmin_(0), xp_(0)
{}
Iterator::~Iterator() {}
// operators
// functions
void Iterator::init(const double xmax, const double xmin, const double eInit)
{
    xmax_ = xmax;
    xmin_ = xmin;
    e_ = eInit;
    ep_ = eInit;
    xp_ = xmax;
    x_ = xmin;   // Do min and max first
    dx_ = x_ - xp_;
    de_ = e_ - ep_;
    count_ = 0;
}

// Generic iteration calculation, method of successive approximations for
// success_count then Newton-Rapheson as needed - works with iterateInit.
// Inputs:  e_
// Outputs:  x_
double Iterator::iterate(const boolean verbose, const uint16_t success_count, const boolean en_no_soln)
{
    de_ = e_ - ep_;
    des_  = sgn(de_)*max(abs(de_), 1e-16);
    dx_   = x_ - xp_;
    if ( verbose )
    {
        Serial.printf("%s(%d): xmin%12.8f x%12.8f xmax%12.8f e%12.8f  des%12.8f dx%12.8f de%12.8f\n", desc_.c_str(), count_, xmin_, x_, xmax_, e_, des_, dx_, de_);
    }

    // Check min max sign change
    boolean no_soln = false;
    if ( count_ == 2 )
    {
        if ( e_*ep_ >= 0  && en_no_soln ) // No solution possible
        {
            no_soln = true;
            if ( abs(ep_) < abs(e_) )
                x_   = xp_;
            ep_  = e_;
            limited_ = false;
            if ( verbose )
                Serial.printf("%s:No soln\n", desc_.c_str());  // Leaving x at most likely limit value and recalculating...
            return ( e_ );
        }
        else
            no_soln = false;
    }
    if ( count_==3 && no_soln )  // Stop after recalc and no_soln
    {
        e_ = 0;
        return ( e_ );
    }
    xp_ = x_;
    ep_ = e_;
    if ( count_ == 1 )
        x_ = xmax_;  // Do min and max first
    else
    {
        if ( count_ > success_count )
        {
            x_ = max(min(x_ - e_/des_*dx_, xmax_),xmin_);
            if ( e_ > 0 )
                xmax_ = xp_;
            else
                xmin_ = xp_;
        }
        else
        {
            if ( e_ > 0 )
            {
                xmax_ = xp_;
                x_ = (xmin_ + x_)/2;
            }
            else
            {
                xmin_ = xp_;
                x_ = (xmax_ + x_)/2;
            }
        }
        if ( x_==xmax_ || x_==xmin_ )
            limited_ = 0;
        else
            limited_ = 1;
    }
    return ( e_ );
}
