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

#include "mySync.h"

// Constructors
Sync::Sync()
  : delay_(0), last_(0UL), now_(0UL), stat_(false), updateTime_(0)
{}
Sync::Sync(unsigned long delay)
    : delay_(delay), last_(0UL), now_(0UL), stat_(false), updateTime_(0)
{}

// Check and count 
bool Sync::update(bool reset, unsigned long now, bool andCheck)
{
  now_ = now;
  updateTime_ = now_ - last_;
  stat_ = reset || ((updateTime_>=delay_) && andCheck);
  if ( stat_ ) last_ = now_;
  return( stat_ );
}
bool Sync::update(unsigned long now, bool reset, bool andCheck)
{
  now_ = now;
  updateTime_ = now_ - last_;
  stat_ = ((updateTime_>=delay_) || reset) && andCheck;
  if ( stat_ ) last_ = now_;
  return( stat_ );
}
bool Sync::update(unsigned long now, bool reset)
{
  now_ = now;
  updateTime_ = now_ - last_;
  stat_ = (updateTime_>=delay_) || reset;
  if ( stat_ ) last_ = now_;
  return( stat_ );
}
bool Sync::updateN(unsigned long now, bool reset, bool orCheck)
{
  now_ = now;
  updateTime_ = now_ - last_;
  stat_ = reset || ((stat_ && (updateTime_<delay_)) || orCheck);
  if ( !stat_ ) last_ = now_;
  return( stat_ );
}
