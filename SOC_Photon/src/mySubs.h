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

#include "myLibrary/myFilters.h"
#include "Battery.h"
#include "constants.h"
#include "myCloud.h"
#include "myTalk.h"
#include "retained.h"
#include "command.h"

// Temp sensor
#include <hardware/OneWire.h>
#include <hardware/DS18.h>

// AD
#include <Adafruit/Adafruit_ADS1X15.h>

// Display
#include <Adafruit/Adafruit_GFX.h>
#include <Adafruit/Adafruit_SSD1306.h>

#include "retained.h"
extern RetainedPars rp; // Various parameters to be static at system level

// Pins
struct Pins
{
  byte pin_1_wire;  // 1-wire Plenum temperature sensor
  byte status_led;  // On-board led
  byte Vbatt_pin;   // Battery voltage
  pin_t pwm_pin;    // External signal injection
  Pins(void) {}
  Pins(byte pin_1_wire, byte status_led, byte Vbatt_pin, pin_t pwm_pin)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vbatt_pin = Vbatt_pin;
    this->pwm_pin = pwm_pin;
  }
};

// Sensors
struct Sensors
{
  double Vbatt;           // Sensed battery voltage, V
  double Vbatt_model;     // Model coefficient model battery voltage based on filtered current, V
  // double Vbatt_filt;      // Filtered, sensed battery voltage, V
  double Tbatt;           // Sensed battery temp, C
  double Tbatt_filt;      // Filtered, sensed battery temp, C
  int16_t Vshunt_amp_int; // Sensed shunt voltage, count
  int16_t Vshunt_noamp_int;// Sensed shunt voltage, count
  double Vshunt_amp;      // Sensed shunt voltage, V
  double Vshunt_noamp;    // Sensed shunt voltage, V
  double Vshunt;          // Sensed shunt voltage, V
  double Vshunt_filt;     // Filtered, sensed shunt voltage, V
  double shunt_v2a_s;     // Selected shunt conversion gain, A/V
  double Ishunt_amp_cal;  // Sensed, calibrated amplified ADC, A
  double Ishunt_noamp_cal;// Sensed, calibrated non-amplified ADC, A
  double Ishunt;          // Selected calibrated, shunt current, A
  double Ishunt_filt;     // Filtered, calibrated sensed shunt current for observer, A
  double Wshunt;          // Sensed shunt power, W
  double Wcharge;         // Charge power, W
  int I2C_status;
  double T;               // Update time, s
  double T_filt;          // Filter update time, s
  double T_temp;          // Temperature update time, s
  boolean bare_ads_amp;   // If no ADS detected
  boolean bare_ads_noamp; // If no ADS detected
  boolean saturated;      // Battery saturation status based on Temp and VOC
  Sensors(void) {}
  Sensors(double Vbatt, double Tbatt, double Tbatt_filt,  // TODO:  is this needed?
          int16_t Vshunt_noamp_int, double Vshunt, double Vshunt_filt,
          int16_t Vshunt_amp_int, double Vshunt_amp, double Vshunt_amp_filt,
          int I2C_status, double T, double T_temp, boolean bare_ads_noamp, boolean bare_ads_amp)
  {
    this->Vbatt = Vbatt;
    this->Tbatt = Tbatt;
    this->Tbatt_filt = Tbatt_filt;
    this->Vshunt_noamp_int = Vshunt_noamp_int;
    this->Vshunt = Vshunt;
    this->Vshunt_filt = Vshunt_filt;
    this->Ishunt = Vshunt * shunt_noamp_v2a_s + rp.curr_bias_noamp;
    this->Wshunt = Vshunt * Ishunt;
    this->Wcharge = Vshunt * Ishunt;
    this->Vshunt_amp_int = Vshunt_amp_int;
    this->I2C_status = I2C_status;
    this->T = T;
    this->T_filt = T;
    this->T_temp = T_temp;
    this->bare_ads_noamp = bare_ads_noamp;
    this->bare_ads_amp = bare_ads_amp;
  }
};


// Headers
void manage_wifi(unsigned long now, Wifi *wifi);
void serial_print(unsigned long now, double T);
void load(const boolean reset_free, Sensors *Sen, Pins *myPins,
    Adafruit_ADS1015 *ads_amp, Adafruit_ADS1015 *ads_noamp, const unsigned long now);
void load_temp(Sensors *Sen, DS18 *SensorTbatt, SlidingDeadband *SdTbatt);
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFilt, General2_Pole* IshuntSenseFilt);
void filter_temp(const int reset, const double t_rlim, Sensors *Sen, General2_Pole* TbattSenseFilt, const double t_bias, double *t_bias_last);
String tryExtractString(String str, const char* start, const char* end);
double  decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);
void print_serial_header(void);
void myDisplay(Adafruit_SSD1306 *display, Sensors *Sen);
uint32_t pwm_write(uint32_t duty, Pins *myPins);
String time_long_2_str(const unsigned long current_time, char *tempStr);
void create_print_string(char *buffer, Publish *pubList);
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);

#endif
