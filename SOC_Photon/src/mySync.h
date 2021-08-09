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

#ifndef _MY_SYNC_H
#define _MY_SYNC_H

// Duct Model Class
class Sync
{
public:
  // Constructors
  Sync(void);
  Sync(unsigned long delay);
  // Functions
  bool update(bool reset, unsigned long now, bool andCheck);
  bool update(unsigned long now, bool reset, bool andCheck);
  bool update(unsigned long now, bool reset);
  bool updateN(unsigned long now, bool reset, bool orCheck);
  unsigned long delay() { return(delay_); };
  unsigned long last() { return(last_); };
  bool stat() { return(stat_); };
  unsigned long updateTime() { return(updateTime_); };
private:
  unsigned long delay_;
  unsigned long last_;
  bool stat_;
  unsigned long updateTime_;
};

#endif