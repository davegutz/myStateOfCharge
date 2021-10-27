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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "mySubs.h"
#include "myCloud.h"
#include "constants.h"
#include <math.h>

extern const int8_t debug;
extern Publish pubList;
extern char buffer[256];
extern boolean enable_wifi;

// Assignments
void assignPubList(Publish* pubList, const unsigned long now, const String unit, const String hmString,
  const double controlTime, struct Sensors* sen, const int numTimeouts)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hmString =hmString;
  pubList->controlTime = controlTime;
  pubList->Vbatt = sen->Vbatt;
  pubList->Tbatt = sen->Tbatt;
  pubList->Vshunt_01 = sen->Vshunt_01;
  pubList->Ishunt_01 = sen->Ishunt_01;
  pubList->Wshunt = sen->Wshunt;
  pubList->Vshunt_amp_01 = sen->Vshunt_amp_01;
  pubList->Ishunt_amp_01 = sen->Ishunt_amp_01;
  pubList->Wshunt_amp = sen->Wshunt_amp;
  pubList->numTimeouts = numTimeouts;
  pubList->socu_solved = -9;
  pubList->socu_free = -9;
  pubList->T = sen->T;
}
