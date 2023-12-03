// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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
// 3-Dec-2023  Dave Gutz   Create from  https://community.particle.io/t/ds2482-i2c-to-1-wire-library/40447

#ifndef _MY_DS2482_H
#define _MY_DS2482_H

#include "DS2482-RK.h"

#define MAX_DS2482 5

class MyDs2482_Class
{
public:
	MyDs2482_Class(int addr);
	virtual ~MyDs2482_Class();
    boolean ready() { return ready_; }
    float tempC(const int i) { return tempC_[i]; }
	void setup();
	void loop();
	void check();

protected:
	DS2482 ds_;
	DS2482DeviceListStatic<10> deviceList_;
    float tempC_[MAX_DS2482];
    boolean ready_;
};


#endif
