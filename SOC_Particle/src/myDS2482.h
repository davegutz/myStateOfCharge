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


class TestClass
{
public:
	TestClass(int addr);
	virtual ~TestClass();

	void setup();
	void loop();
	void check();

protected:
	DS2482 ds;
	DS2482DeviceListStatic<10> deviceList;
};

TestClass::TestClass(int addr) : ds(Wire, addr) {}

TestClass::~TestClass(){}

void TestClass::setup()
{
	ds.setup();
	DS2482DeviceReset::run(ds, [](DS2482DeviceReset&, int status)
    {
		Serial.printf("deviceReset=%d\n", status);
	});
}

void TestClass::loop()
{
	ds.loop();
}

void TestClass::check()
{
	DS2482SearchBusCommand::run(ds, deviceList, [this](DS2482SearchBusCommand &obj, int status)
    {
		if (status != DS2482Command::RESULT_DONE)
        {
			Serial.printf("DS2482SearchBusCommand status=%d\n", status);
			return;
		}

		if (deviceList.getDeviceCount() == 0)
        {
			Serial.printf("no devices\n");
			return;
		}

		DS2482GetTemperatureForListCommand::run(ds, obj.getDeviceList(), [this](DS2482GetTemperatureForListCommand&, int status, DS2482DeviceList &deviceList)
        {
			if (status != DS2482Command::RESULT_DONE)
            {
				Serial.printf("DS2482GetTemperatureForListCommand status=%d\n", status);
				return;
			}

			for(size_t ii = 0; ii < deviceList.getDeviceCount(); ii++)
            {
				Serial.printf("%s valid=%d C=%f F=%f\n",
						deviceList.getAddressByIndex(ii).toString().c_str(),
						deviceList.getDeviceByIndex(ii).getValid(),
						deviceList.getDeviceByIndex(ii).getTemperatureC(),
						deviceList.getDeviceByIndex(ii).getTemperatureF());
			}
		});
	});
}

#endif
