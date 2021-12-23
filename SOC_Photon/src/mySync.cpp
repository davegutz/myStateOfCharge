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
//
// 17-Feb-2021  Dave Gutz   Create
#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "mySync.h"
#include "retained.h"
extern RetainedPars rp;

// Constructors
Sync::Sync()
  : delay_(0), last_(0UL), now_(0UL), stat_(false), updateDiff_(0), updateTime_(0)
{}
Sync::Sync(unsigned long delay)
    : delay_(delay), last_(0UL), now_(0UL), stat_(false), updateDiff_(0), updateTime_(0)
{}

// Check and count 
boolean Sync::update(boolean reset, unsigned long now, boolean andCheck)
{
  now_ = now;
  updateDiff_ = now_ - last_;
  stat_ = reset || ((updateDiff_>=delay_) && andCheck);
  if ( stat_ )
  {
    last_ = now_;
    updateTime_ = double(updateDiff_)/1000.;
  }
  return( stat_ );
}
boolean Sync::update(unsigned long now, boolean reset, boolean andCheck)
{
  now_ = now;
  updateDiff_ = now_ - last_;
  stat_ = ((updateDiff_>=delay_) || reset) && andCheck;
  if ( stat_ )
  {
    last_ = now_;
    updateTime_ = double(updateDiff_)/1000.;
  }
  return( stat_ );
}
boolean Sync::update(unsigned long now, boolean reset)
{
  now_ = now;
  updateDiff_ = now_ - last_;
  stat_ = (updateDiff_>=delay_) || reset;
  if ( stat_ )
  {
    last_ = now_;
    updateTime_ = double(updateDiff_)/1000.;
  }
  if ( rp.debug==-13 ) Serial.printf("reset,now,last,updateDiff,updateTime,delay,stat, %d, %ld, %ld, %ld,%7.3f, %ld, %d,\n",
            reset, now_, last_, updateDiff_, updateTime_, delay_, int(stat_));
  return( stat_ );
}
boolean Sync::updateN(unsigned long now, boolean reset, boolean orCheck)
{
  now_ = now;
  updateDiff_ = now_ - last_;
  stat_ = reset || ((stat_ && (updateDiff_<delay_)) || orCheck);
  if ( stat_ )
  {
    last_ = now_;
    updateTime_ = double(updateDiff_)/1000.;
  }
  return( stat_ );
}
