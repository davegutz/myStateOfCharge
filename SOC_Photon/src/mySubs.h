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
#include "Battery.h"
#include "constants.h"
#include "myCloud.h"

// Temp sensor
#include <OneWire.h>
#include <DS18.h>

// AD
#include <Adafruit_ADS1X15.h>

// Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
  double Vbatt_filt_obs;  // Filtered, sensed battery voltage for observer, V
  double Tbatt;           // Sensed battery temp, F
  double Tbatt_filt;      // Filtered, sensed battery temp, F
  int16_t Vshunt_int;     // Sensed shunt voltage, count
  double Vshunt;          // Sensed shunt voltage, V
  double Vshunt_filt;     // Filtered, sensed shunt voltage, V
  double Vshunt_filt_obs; // Filtered, sensed shunt voltage for  observer, V
  double Ishunt;          // Sensed shunt current, A
  double Ishunt_filt;     // Filtered, sensed shunt current, A
  double Ishunt_filt_obs; // Filtered, sensed shunt current for observer, A
  double Wshunt;          // Sensed shunt power, W
  double Wshunt_filt;     // Filtered, sensed shunt power, W
  double Wbatt;          // Battery power, W
  int I2C_status;
  double T;
  bool bare_ads;          // If no ADS detected
  double Vbatt_model;         // Modeled battery voltage including steady current draw, V
  double Vbatt_model_filt;    // Filtered modeled battery voltage including steady current draw, V
  double Vbatt_model_tracked; // Tracked modeled battery voltage including steady current draw, V
  Sensors(void) {}
  Sensors(double Vbatt, double Vbatt_filt, double Tbatt, double Tbatt_filt,
          int16_t Vshunt_int, double Vshunt, double Vshunt_filt,
          int I2C_status, double T, bool bare_ads)
  {
    this->Vbatt = Vbatt;
    this->Vbatt_filt = Vbatt_filt;
    this->Vbatt_filt_obs = Vbatt_filt;
    this->Tbatt = Tbatt;
    this->Tbatt_filt = Tbatt_filt;
    this->Vshunt_int = Vshunt_int;
    this->Vshunt = Vshunt;
    this->Vshunt_filt = Vshunt_filt;
    this->Ishunt = Vshunt * SHUNT_V2A_S + double(SHUNT_V2A_A);
    this->Ishunt_filt = Vshunt_filt * SHUNT_V2A_S + SHUNT_V2A_A;
    this->Wshunt = Vshunt * Ishunt;
    this->Wshunt_filt = Vshunt_filt * Ishunt_filt;
    this->Wbatt = Vshunt * Ishunt;
    this->I2C_status = I2C_status;
    this->T = T;
    this->bare_ads = bare_ads;
    this->Vbatt_model = 0;
    this->Vbatt_model_filt = 0;
    this->Vbatt_model_tracked = 0;
  }
};


// Headers
void manage_wifi(unsigned long now, Wifi *wifi);
void serial_print_inputs(unsigned long now, double T);
void serial_print(void);
void load(Sensors *sen, DS18 *sensor_tbatt, Pins *myPins, Adafruit_ADS1015 *ads);
void filter(int reset, Sensors *sen, General2_Pole* VbattSenseFiltObs, General2_Pole* VshuntSenseFiltObs, 
  General2_Pole* VbattSenseFilt,  General2_Pole* TbattSenseFilt, General2_Pole* VshuntSenseFilt);
String tryExtractString(String str, const char* start, const char* end);
double  decimalTime(unsigned long *currentTime, char* tempStr);
void print_serial_header(void);
void myDisplay(Adafruit_SSD1306 *display);

// Talk Declarations
void talk(bool *stepping, PID* pid, double *stepVal, bool *vectoring, int8_t *vec_num);
void talkT(bool *stepping, double *stepVal, bool *vectoring, int8_t *vec_num);  // Transient inputs
void talkH(PID *pid, double *stepVal, int8_t *vec_num); // Help
#endif
