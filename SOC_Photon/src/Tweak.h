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

#ifndef _MY_TWEAK_H
#define _MY_TWEAK_H
#define t_float double

// Use known saturation instances to adjust current sensor for zero net energy.
class Tweak
{
public:
  Tweak();
  Tweak(const String name, const double gain, const double max_change, const double max_tweak, const double time_to_wait,
    t_float *rp_delta_q_inf, t_float *rp_tweak_bias);
  ~Tweak();
  // operators
  // functions
  void adjust(unsigned long now);
  void delta_q_inf(const double delta_q_inf) { *rp_delta_q_inf_ = delta_q_inf; };
  double delta_q_inf() { return ( *rp_delta_q_inf_ ); };
  void delta_q_sat_past(const double new_delta_q_sat_past) { delta_q_sat_past_ = new_delta_q_sat_past; };
  double delta_q_sat_past() { return( delta_q_sat_past_ ); };
  double delta_q_sat_present() { return( delta_q_sat_present_ ); };
  void delta_q_sat_present(const double new_delta_q_sat_present) { delta_q_sat_present_ = new_delta_q_sat_present; };
  double gain() { return( gain_ ); };
  void gain(const double new_gain) { gain_ = new_gain; };
  double max_change() { return( max_change_ ); };
  void max_change(const double new_max) { max_change_ = abs(new_max); };
  double max_tweak() { return( max_tweak_ ); };
  void max_tweak(const double new_max_tweak) { max_tweak_ = max(new_max_tweak, 0.); };
  boolean new_desat(const double curr_in, const double T,  const boolean is_sat, unsigned long int now);
  void pretty_print();
  void reset();
  double time_sat_past() { return( double(millis()-time_sat_past_)/3600000. ); };
  void time_sat_past(const double new_time) { time_sat_past_ = millis()-(unsigned long int)(new_time*3600000.); };
  double time_to_wait() { return( time_to_wait_); };
  void time_to_wait(const double new_time) { time_to_wait_ = new_time; };
  double tweak_bias() { return( *rp_tweak_bias_ ); };
  void tweak_bias(const double bias) { *rp_tweak_bias_ = bias; };
  void save_new_sat(unsigned long int now);
protected:
  String name_;
  double gain_;                 // Current correction to be made for charge error, A/Coulomb/day
  double max_change_;           // Maximum allowed change to calibration adjustment, A
  double max_tweak_;            // Maximum allowed calibration adjustment, A\n
  double delta_q_sat_present_;  // TODO:  finish these comments and populate nom.txt too
  double delta_q_sat_past_;
  boolean sat_;
  double delta_q_max_;          // Running tab since last de-saturation of potential new delta_q_sat
  unsigned long int time_sat_past_;
  double time_to_wait_;
  t_float *rp_delta_q_inf_;
  t_float *rp_tweak_bias_;
  double delta_hrs_;
};

#endif
