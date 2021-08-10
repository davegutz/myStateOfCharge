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


#ifndef _MY_SUBS_H
#define _MY_SUBS_H

#include "myFilters.h"
#include <OneWire.h>
#include <DS18.h>
#include <Adafruit_ADS1X15.h>


// Pins
struct Pins
{
  byte pin_1_wire;     // 1-wire Plenum temperature sensor
  byte status_led;     // On-board led
  byte Vbatt_pin;      // Battery voltage
  Pins(void) {}
  Pins(byte pin_1_wire, byte status_led, byte Vbatt_pin)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vbatt_pin = Vbatt_pin;
  }
};


// Sensors
struct Sensors
{
  double Vbatt;           // Sensed battery voltage, V
  double Vbatt_filt;      // Filtered, sensed battery voltage, V
  double Tbatt;           // Sensed battery temp, F
  double Tbatt_filt;      // Filtered, sensed battery temp, F
  int16_t Vshunt_int;     // Sensed shunt voltage, count
  double Vshunt;          // Sensed shunt voltage, V
  double Vshunt_filt;     // Filtered, sensed shunt voltage, V
  int I2C_status;
  double T;
  Sensors(void) {}
  Sensors(double Vbatt, double Vbatt_filt, double Tbatt, double Tbatt_filt,
          int16_t Vshunt_int, double Vshunt, double Vshunt_filt,
          int I2C_status, double T)
  {
    this->Vbatt = Vbatt;
    this->Vbatt_filt = Vbatt_filt;
    this->Tbatt = Tbatt;
    this->Tbatt_filt = Tbatt_filt;
    this->Vshunt_int = Vshunt_int;
    this->Vshunt = Vshunt;
    this->Vshunt_filt = Vshunt_filt;
    this->I2C_status = I2C_status;
    this->T = T;
  }
};


// Publishing
struct Publish
{
  uint32_t now;
  String unit;
  String hmString;
  double controlTime;
  double Vbatt;
  double Tbatt;
  double Vshunt;
  double T;
  int I2C_status;
  double Vbatt_filt;
  double Tbatt_filt;
  double Vshunt_filt;
  int numTimeouts;
};


void publish_particle(unsigned long now);
void serial_print_inputs(unsigned long now, double T);
void serial_print(void);
boolean load(int reset, double T, Sensors *sen, DS18 *sensor_tbatt, General2_Pole* VbattSenseFilt, 
    General2_Pole* TbattSenseFilt, General2_Pole* VshuntSenseFilt, Pins *myPins, Adafruit_ADS1015 *ads);
String tryExtractString(String str, const char* start, const char* end);
double  decimalTime(unsigned long *currentTime, char* tempStr);
void print_serial_header(void);

#endif
