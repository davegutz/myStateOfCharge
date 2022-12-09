/*
	SerialRAM.h
	Very simple class to interface with Microchip's 4K/16K I2C Serial EERAM (47L04, 47C04, 47L16 and 47C16) chips

	This example code is licensed under CC BY 4.0.
	Please see https://creativecommons.org/licenses/by/4.0/

	modified 6th August 2017
	by Tony Pottier
	https://idyl.io

*/

#ifndef _SerialRAM_h
#define _SerialRAM_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


typedef union {
	uint16_t a16;
	uint8_t a8[2];
}address16b;

class SerialRAM {
private:
	int8_t SRAM_REGISTER;
	int8_t CONTROL_REGISTER;
	uint8_t readControlRegister();

public:
	
	void begin(const uint8_t a0, const uint8_t a1);
	uint8_t write(const uint16_t address, const uint8_t value);
	uint8_t read(const uint16_t address);
	void setAutoStore(const bool value);
	bool getAutoStore();
	
	uint8_t write(const uint16_t address, const uint8_t* values, const uint16_t size);
	void read(const uint16_t address, uint8_t* values, const uint16_t size);

	//Functionality to 'get' and 'put' objects to and from EERAM
	// https://github.com/sparkfun/SparkFun_External_EEPROM_Arduino_Library/blob/master/src/SparkFun_External_EEPROM.h
	template <typename T>
	T &get(uint16_t idx, T &t)
	{
		uint8_t *ptr = (uint8_t *)&t;
		read(idx, ptr, sizeof(T)); //Address, data, sizeOfData
		return t;
	}

	template <typename T>
	const T &put(uint16_t idx, const T &t) //Address, data
	{
		const uint8_t *ptr = (const uint8_t *)&t;
		write(idx, ptr, sizeof(T)); //Address, data, sizeOfData
		return t;
	}

};




#endif

