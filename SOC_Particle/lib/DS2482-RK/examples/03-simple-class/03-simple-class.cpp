#include "DS2482-RK.h"

SerialLogHandler logHandler;

const unsigned long CHECK_PERIOD = 60000;
unsigned long lastCheck = 5000 - CHECK_PERIOD;

class Ds2482Class {
public:
	Ds2482Class(int addr);
	virtual ~Ds2482Class();

	void setup();
	void loop();
	void check();

protected:
	DS2482 ds;
	DS2482DeviceListStatic<10> deviceList;
};

Ds2482Class::Ds2482Class(int addr) : ds(Wire, addr) {

}

Ds2482Class::~Ds2482Class() {

}

void Ds2482Class::setup() {
	ds.setup();

	DS2482DeviceReset::run(ds, [](DS2482DeviceReset&, int status) {
		Serial.printlnf("deviceReset=%d", status);
	});

}

void Ds2482Class::loop() {
	ds.loop();
}

void Ds2482Class::check() {
	DS2482SearchBusCommand::run(ds, deviceList, [this](DS2482SearchBusCommand &obj, int status) {

		if (status != DS2482Command::RESULT_DONE) {
			Serial.printlnf("DS2482SearchBusCommand status=%d", status);
			return;
		}

		if (deviceList.getDeviceCount() == 0) {
			Serial.println("no devices");
			return;
		}

		DS2482GetTemperatureForListCommand::run(ds, obj.getDeviceList(), [this](DS2482GetTemperatureForListCommand&, int status, DS2482DeviceList &deviceList) {
			if (status != DS2482Command::RESULT_DONE) {
				Serial.printlnf("DS2482GetTemperatureForListCommand status=%d", status);
				return;
			}

			Serial.printlnf("got temperatures!");

			for(size_t ii = 0; ii < deviceList.getDeviceCount(); ii++) {
				Serial.printlnf("%s valid=%d C=%f F=%f",
						deviceList.getAddressByIndex(ii).toString().c_str(),
						deviceList.getDeviceByIndex(ii).getValid(),
						deviceList.getDeviceByIndex(ii).getTemperatureC(),
						deviceList.getDeviceByIndex(ii).getTemperatureF());
			}

		});
	});
}

Ds2482Class Ds2482(0);

void setup() {
	Serial.begin(9600);
	Ds2482.setup();
}



void loop() {

	Ds2482.loop();

	if (millis() - lastCheck >= CHECK_PERIOD) {
		lastCheck = millis();

		Ds2482.check();
	}
}


