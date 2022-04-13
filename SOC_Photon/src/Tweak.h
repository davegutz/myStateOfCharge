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

// Use known saturation instances to adjust current sensor for zero net energy.
class Tweak
{
public:
  Tweak();
  Tweak(const double gain, const double max_change, const double max_tweak, const unsigned long int time_to_wait);
  ~Tweak();
  // operators
  // functions
  double adjust(const double Di);
  void adjust_max(const double new_max_tweak) { max_tweak_ = max(new_max_tweak, 0.); };
  double max_tweak() { return( max_tweak_ ); };
  void pretty_print();
  void reset();
  boolean update(const double delta_q_inf, const boolean is_sat, unsigned long int now);
  void save_new_sat(unsigned long int now);
protected:
  double gain_;
  double max_change_;
  double max_tweak_;
  double delta_q_inf_past_;
  double delta_q_sat_present_;
  double delta_q_sat_past_;
  boolean sat_;
  double delta_q_max_;          // Running tab since last de-saturation of potential new delta_q_sat
  unsigned long int time_sat_past_;
  unsigned long int time_to_wait_;
};

#endif
