/*
	SerialRAM.cpp
	Very simple class to interface with Microchip's 4K/16K I2C Serial EERAM (47L04, 47C04, 47L16 and 47C16) chips

	This example code is licensed under CC BY 4.0.
	Please see https://creativecommons.org/licenses/by/4.0/

	modified 6th August 2017
	by Tony Pottier
	https://idyl.io

*/

#include <stdint.h>
#include <Wire.h>
#include "SerialRAM.h"


///<summary>
///	Initialize the RAM chip with the given A0 and A1 values.
///	<param name="A0">A0 value (logic 0 or 1) of the RAM chip you want to address. Default value 0.</param>
///	<param name="A1">A1 value (logic 0 or 1) of the RAM chip you want to address. Default value 0.</param>
///</summary>
void SerialRAM::begin(const uint8_t a0, const uint8_t a1) {
	//build mask
	uint8_t mask = (a0 << 1) | (a1);
	mask <<= 1;

	//save registers addresses
	this->SRAM_REGISTER = 0x50 | mask;
	this->CONTROL_REGISTER = 0x18 | mask;

	//Arduino I2C lib
	Wire1.begin();
}


///<summary>
///	Write the given byte "value" at the 16 bit address "address".
///		47x16 chips valid addresses range from 0x0000 to 0x07FF
///		47x04 chips valid addresses range from 0x0000 to 0x01FF
///		<param name="address">16 bit address</param>
///		<param name="value">value (byte) to be written</param>
///		<returns>0:success, 1:data too long to fit in transmit buffer, 2 : received NACK on transmit of address, 3 : received NACK on transmit of data, 4 : other error </returns>
///</summary>
uint8_t SerialRAM::write(const uint16_t address, const uint8_t value) {
	address16b a;
	a.a16 = address;
	Wire1.beginTransmission(this->SRAM_REGISTER);
	Wire1.write(a.a8[1]);
	Wire1.write(a.a8[0]);
	Wire1.write(value);
	return Wire1.endTransmission();
}

///<summary>
///	Read the byte "value" located at the 16 bit address "address".
///		47x16 chips valid addresses range from 0x0000 to 0x07FF
///		47x04 chips valid addresses range from 0x0000 to 0x01FF
///		<param name="address">16 bit address</param>
///		<returns>value (byte) read at the address</returns>
///</summary>
uint8_t SerialRAM::read(const uint16_t address) {
	uint8_t buffer;
	address16b a;
	a.a16 = address;

	
	Wire1.beginTransmission(this->SRAM_REGISTER);
	Wire1.write(a.a8[1]);
	Wire1.write(a.a8[0]);
	Wire1.endTransmission();

	Wire1.requestFrom(this->SRAM_REGISTER, 1);
	buffer = Wire1.read();
	Wire1.endTransmission();

	return buffer;
}

uint8_t SerialRAM::readControlRegister() {
	uint8_t buffer = 0x80;

	Wire1.beginTransmission(this->CONTROL_REGISTER);
	Wire1.write(0x00); //status register
	Wire1.endTransmission();

	Wire1.requestFrom(this->CONTROL_REGISTER, 1);
	buffer = Wire1.read();
	Wire1.endTransmission();

	return buffer;
}

///<summary>
///	De/Activate the "AutoStore" to EEPROM functionnality of the RAM when power is lost.
///		<param name="value">Set to true to activate, false otherwise</param>
///</summary>
void SerialRAM::setAutoStore(const bool value)
{
	uint8_t buffer = this->readControlRegister();
	buffer = value ? buffer|0x02 : buffer&0xfd;
	Wire1.beginTransmission(this->CONTROL_REGISTER);
	Wire1.write(0x00); //status register
	Wire1.write(buffer);
	Wire1.endTransmission();
}

///<summary>
///	De/Activate the "AutoStore" to EEPROM functionnality of the RAM when power is lost.
///		<returns>true of auto store is active</return>
///</summary>
bool SerialRAM::getAutoStore()
{
	uint8_t buffer = this->readControlRegister();
	return buffer & 0x02;
}

///<summary>
///	Write the array of bytes "values" at the 16 bit address "address".
///		47x16 chips valid addresses range from 0x0000 to 0x07FF
///		47x04 chips valid addresses range from 0x0000 to 0x01FF
///		<param name="address">16 bit address</param>
///		<param name="values">values (bytes) to be written</param>
///		<returns>0:success, 1:data too long to fit in transmit buffer, 2 : received NACK on transmit of address, 3 : received NACK on transmit of data, 4 : other error </returns>
///</summary>
uint8_t SerialRAM::write(const uint16_t address, const uint8_t* values, const uint16_t size)
{
	for ( int i=0; i<size; i++) Serial.printf("%X ", values[i]);
	 Serial.printf("\n");

	address16b a;
	a.a16 = address;
	uint8_t result;
	Wire1.beginTransmission(this->SRAM_REGISTER);
	Wire1.write(a.a8[1]);
	Wire1.write(a.a8[0]);
	uint8_t writer = Wire1.write(values, size); Serial.printf("writer %d\n", writer);
	result = Wire1.endTransmission();
	Serial.printf("result=%d:", result);
	return result;
}

///<summary>
///	Read "size" number of bytes into "values" array located at the 16 bit address "address".
///		Make sure values is big enough to contain all data or a segfault will occur.
///		<param name="address">16 bit startign address of the data</param>
///		<param name="values">array to be used to store the data</param>
///		<param name="size">number of bytes to retrieve</param>
///</summary>
void SerialRAM::read(const uint16_t address, uint8_t * values, const uint16_t size)
{
	address16b a;
	a.a16 = address;

	Wire1.beginTransmission(this->SRAM_REGISTER);
	Wire1.write(a.a8[1]);
	Wire1.write(a.a8[0]);
	Wire1.endTransmission();

	Wire1.requestFrom(this->SRAM_REGISTER, size);
	for (uint16_t i = 0; i < size; i++) {
		values[i] = Wire1.read();
	}
	Wire1.endTransmission();
}

