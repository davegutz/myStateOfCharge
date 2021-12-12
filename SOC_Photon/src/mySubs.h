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

// Definition of structure to be saved in SRAM
struct RetainedPars
{
  double curr_bias = 0;     // Calibrate current sensor, A 
  double curr_amp_bias = 0; // Calibrate amp current sensor, A 
  double socu_free = 0.5;   // Coulomb Counter state
  double vbatt_bias = 0;    // Calibrate Vbatt, V
};            

extern RetainedPars rp; // Various parameters to be static at system level

// Sensors
struct Sensors
{
  double Vbatt;           // Sensed battery voltage, V
  double Vbatt_solved;    // Solved coefficient model battery voltage, V
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
  double Wcharge;          // Charge power, W
  int16_t Vshunt_amp_int;     // Sensed shunt voltage, count
  double Vshunt_amp;          // Sensed shunt voltage, V
  double Vshunt_amp_filt;     // Filtered, sensed shunt voltage, V
  double Vshunt_amp_filt_obs; // Filtered, sensed shunt voltage for  observer, V
  double Ishunt_amp;          // Sensed shunt current, A
  double Ishunt_amp_filt;     // Filtered, sensed shunt current, A
  double Ishunt_amp_filt_obs; // Filtered, sensed shunt current for observer, A
  double Wshunt_amp;          // Sensed shunt power, W
  double Wshunt_amp_filt;     // Filtered, sensed shunt power, W
  double Wcharge_amp;           // Charge power, W
  int I2C_status;
  double T;               // Update time, s
  double T_filt;          // Filter update time, s
  double T_temp;          // Temperature update time, s
  bool bare_ads;          // If no ADS detected
  bool bare_ads_amp;      // If no ADS detected
  Sensors(void) {}
  Sensors(double Vbatt, double Vbatt_filt, double Tbatt, double Tbatt_filt,
          int16_t Vshunt_int, double Vshunt, double Vshunt_filt,
          int16_t Vshunt_amp_int, double Vshunt_amp, double Vshunt_amp_filt,
          int I2C_status, double T, double T_temp, bool bare_ads, bool bare_ads_amp)
  {
    this->Vbatt = Vbatt;
    this->Vbatt_filt = Vbatt_filt;
    this->Vbatt_filt_obs = Vbatt_filt;
    this->Vbatt_solved = Vbatt;
    this->Tbatt = Tbatt;
    this->Tbatt_filt = Tbatt_filt;
    this->Vshunt_int = Vshunt_int;
    this->Vshunt = Vshunt;
    this->Vshunt_filt = Vshunt_filt;
    this->Ishunt = Vshunt * SHUNT_V2A_S + double(SHUNT_V2A_A) + rp.curr_bias;
    this->Ishunt_filt = Vshunt_filt * SHUNT_V2A_S + SHUNT_V2A_A + rp.curr_bias;
    this->Wshunt = Vshunt * Ishunt;
    this->Wcharge = Vshunt * Ishunt;
    this->Wshunt_filt = Vshunt_filt * Ishunt_filt;
    this->Vshunt_amp_int = Vshunt_amp_int;
    this->Vshunt_amp = Vshunt_amp;
    this->Vshunt_amp_filt = Vshunt_amp_filt;
    this->Ishunt_amp = Vshunt_amp * SHUNT_AMP_V2A_S + double(SHUNT_AMP_V2A_A) + rp.curr_amp_bias;
    this->Ishunt_amp_filt = Vshunt_amp_filt * SHUNT_AMP_V2A_S + SHUNT_AMP_V2A_A + rp.curr_amp_bias;
    this->Wshunt_amp = Vshunt_amp * Ishunt_amp;
    this->Wshunt_amp_filt = Vshunt_amp_filt * Ishunt_amp_filt;
    this->Wcharge_amp = Vshunt_amp * Ishunt_amp;
    this->I2C_status = I2C_status;
    this->T = T;
    this->T_filt = T;
    this->T_temp = T_temp;
    this->bare_ads = bare_ads;
    this->bare_ads_amp = bare_ads_amp;
  }
};


// Headers
void manage_wifi(unsigned long now, Wifi *wifi);
void serial_print(unsigned long now, double T);
void load(const bool reset_free, Sensors *Sen, Pins *myPins,
    Adafruit_ADS1015 *ads, Adafruit_ADS1015 *ads_amp, const unsigned long now,
    SlidingDeadband *SdIshunt, SlidingDeadband *SdIshunt_amp, SlidingDeadband *SdVbatt);
void load_temp(Sensors *Sen, DS18 *SensorTbatt, SlidingDeadband *SdTbatt);
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFiltObs,
  General2_Pole* VshuntSenseFiltObs,  General2_Pole* VshuntAmpSenseFiltObs, 
  General2_Pole* VbattSenseFilt,  General2_Pole* VshuntSenseFilt,  General2_Pole* VshuntAmpSenseFilt);
void filter_temp(int reset, Sensors *Sen, General2_Pole* TbattSenseFilt);
String tryExtractString(String str, const char* start, const char* end);
double  decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);
void print_serial_header(void);
void myDisplay(Adafruit_SSD1306 *display);
String time_long_2_str(const unsigned long current_time, char *tempStr);
void create_print_string(char *buffer, Publish *pubList);
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);

// Talk Declarations
void talk(bool *stepping, double *step_val, bool *vectoring, int8_t *vec_num,
  Battery *MyBattSolved, Battery *MyBattFree);
void talkT(bool *stepping, double *step_val, bool *vectoring, int8_t *vec_num);  // Transient inputs
void talkH(double *step_val, int8_t *vec_num, Battery *batt_solved); // Help

#endif
