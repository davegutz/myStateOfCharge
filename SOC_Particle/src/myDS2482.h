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
// 3-Dec-2023  Dave Gutz   Create

#ifndef _MY_DS2482_H
#define _MY_DS2482_H
#include "DS2482-RK.h"

#define MAX_DS2482 5

class Ds2482Class
{
public:
	Ds2482Class(int addr);
	virtual ~Ds2482Class();
    float tempC(const int i) { return tempC_[i]; }
	void setup();
	void loop();
	void check();

protected:
	DS2482 ds_;
	DS2482DeviceListStatic<10> deviceList_;
    float tempC_[MAX_DS2482];
};

Ds2482Class::Ds2482Class(int addr) : ds_(Wire, addr)
{
    for ( int ii=0; ii<5; ii++ ) tempC_[ii] = 0.;
}

Ds2482Class::~Ds2482Class(){}

void Ds2482Class::setup()
{
	ds_.setup();
	DS2482DeviceReset::run(ds_, [](DS2482DeviceReset&, int status)
    {
		Log.info("Ds2482DeviceReset::status %d", status);
	});
}

void Ds2482Class::loop()
{
	ds_.loop();
}

void Ds2482Class::check()
{
	DS2482SearchBusCommand::run(ds_, deviceList_, [this](DS2482SearchBusCommand &obj, int status)
    {
		if (status != DS2482Command::RESULT_DONE)
        {
			Serial.printf("DS2482SearchBusCommand status=%d\n", status);
			return;
		}

		if (deviceList_.getDeviceCount() == 0)
        {
			Serial.printf("no devices\n");
			return;
		}

		DS2482GetTemperatureForListCommand::run(ds_, obj.getDeviceList(), [this](DS2482GetTemperatureForListCommand&, int status, DS2482DeviceList &deviceList_)
        {
			if (status != DS2482Command::RESULT_DONE)
            {
				Serial.printf("DS2482GetTemperatureForListCommand status=%d\n", status);
				return;
			}

			for(size_t ii = 0; ii < min((unsigned int)MAX_DS2482, deviceList_.getDeviceCount()); ii++)
            {
                tempC_[ii] = deviceList_.getDeviceByIndex(ii).getTemperatureC();
			}
		});
	});
}

#endif
