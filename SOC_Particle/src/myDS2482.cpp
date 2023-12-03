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

#include "myDS2482.h"

MyDs2482_Class::MyDs2482_Class(int addr) : ds_(Wire, addr), ready_(false)
{
    for ( int ii=0; ii<5; ii++ ) tempC_[ii] = 0.;
}

MyDs2482_Class::~MyDs2482_Class(){}

void MyDs2482_Class::setup()
{
	ds_.setup();
	DS2482DeviceReset::run(ds_, [](DS2482DeviceReset&, int status)
    {
		Log.info("Ds2482DeviceReset::status %d", status);
	});
}

void MyDs2482_Class::loop()
{
	ds_.loop();
}

void MyDs2482_Class::check()
{
	DS2482SearchBusCommand::run(ds_, deviceList_, [this](DS2482SearchBusCommand &obj, int status)
    {
		if (status != DS2482Command::RESULT_DONE)
        {
			Serial.printf("DS2482SearchBusCommand status=%d\n", status);
            ready_ = false;
			return;
		}

		if (deviceList_.getDeviceCount() == 0)
        {
			Serial.printf("no devices\n");
            ready_ = false;
			return;
		}

		DS2482GetTemperatureForListCommand::run(ds_, obj.getDeviceList(), [this](DS2482GetTemperatureForListCommand&, int status, DS2482DeviceList &deviceList_)
        {
			if (status != DS2482Command::RESULT_DONE)
            {
				Serial.printf("DS2482GetTemperatureForListCommand status=%d\n", status);
                ready_ = false;
				return;
			}

            ready_ = true;
			for(size_t ii = 0; ii < min((unsigned int)MAX_DS2482, deviceList_.getDeviceCount()); ii++)
            {
                tempC_[ii] = deviceList_.getDeviceByIndex(ii).getTemperatureC();
			}
		});
	});
}
