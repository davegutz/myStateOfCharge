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
#define t_float float

// Use known saturation instances to adjust current sensor for zero net energy.
class Tweak
{
public:
  Tweak();
  Tweak(const String name, const double max_change, const double max_tweak, const double time_to_wait,
    float *rp_delta_q_cinf, float *rp_delta_q_dinf, float *rp_tweak_sclr, double coul_eff);
  ~Tweak();
  // operators
  // functions
  void adjust(unsigned long now);
  double coul_eff() { return ( coul_eff_ ); };
  void coul_eff(const double coul_eff) { coul_eff_ = coul_eff; };
  void delta_q_cinf(const double delta_q_cinf) { *rp_delta_q_cinf_ = delta_q_cinf; };
  double delta_q_cinf() { return ( *rp_delta_q_cinf_ ); };
  void delta_q_dinf(const double delta_q_dinf) { *rp_delta_q_dinf_ = delta_q_dinf; };
  double delta_q_dinf() { return ( *rp_delta_q_dinf_ ); };
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
  void save_new_sat(unsigned long int now);
  double tweak_sclr() { return ( *rp_tweak_sclr_ ); };
  void tweak_sclr(const double sclr) { *rp_tweak_sclr_ = sclr; };
protected:
  String name_;
  double max_change_;           // Maximum allowed change to calibration scalar # 'XN/MC'
  double max_tweak_;            // Maximum allowed calibration scalar # 'XN/Mx'
  boolean sat_;                 // Indication that battery is saturated, T=saturated
  unsigned long int time_sat_past_; // Time at last declared saturation, ms
  double time_to_wait_;         // Time specified to wait before engaging delta_q_max declaration, ms
  float *rp_delta_q_cinf_;      // Charge infinity at past update # 'XN/Mi', Coulombs
  float *rp_delta_q_dinf_;      // Discharge infinity at past update # 'XN/Mi', Coulombs
  float *rp_tweak_sclr_;        // Scalar on Coulombic efficiency 'N/Mk'
  double delta_hrs_;            // Time since last allowed saturation # 'XN/Mz', hr
  double coul_eff_;             // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
};

#endif
