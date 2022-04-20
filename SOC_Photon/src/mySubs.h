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
#include "Tweak.h"

// Temp sensor
#include <hardware/OneWire.h>
#include <hardware/DS18.h>

// AD
#include <Adafruit/Adafruit_ADS1X15.h>

// Display
#include <Adafruit/Adafruit_GFX.h>
#include <Adafruit/Adafruit_SSD1306.h>

extern RetainedPars rp; // Various parameters to be static at system level
extern CommandPars cp;  // Various parameters to be static at system level


// ADS1015-based shunt
class Shunt: public Tweak, Adafruit_ADS1015
{
public:
  Shunt();
  Shunt(const String name, const uint8_t port, double *rp_delta_q_inf, double *rp_tweak_bias, double *cp_curr_bias, 
    const double v2a_s);
  ~Shunt();
  // operators
  // functions
  boolean bare() { return ( bare_ ); };
  double ishunt() { return ( ishunt_ ); };
  double ishunt_cal() { return ( ishunt_cal_ ); };
  void load();
  double v2a_s() { return ( v2a_s_ ); };
  double vshunt() { return ( vshunt_ ); };
  int16_t vshunt_int() { return ( vshunt_int_ ); };
  int16_t vshunt_int_0() { return ( vshunt_int_0_ ); };
  int16_t vshunt_int_1() { return ( vshunt_int_1_ ); };
protected:
  String name_;         // For print statements, multiple instances
  uint8_t port_;        // Octal I2C port used by Acafruit_ADS1015
  boolean bare_;        // If ADS to be ignored
  double *cp_curr_bias_;// Global bias, A
  double v2a_s_;        // Selected shunt conversion gain, A/V
  int16_t vshunt_int_;  // Sensed shunt voltage, count
  int16_t vshunt_int_0_;// Interim conversion, count
  int16_t vshunt_int_1_;// Interim conversion, count
  double vshunt_;       // Sensed shunt voltage, V
  double vshunt_filt_;  // Filtered, sensed shunt voltage, V
  double ishunt_cal_;   // Sensed, calibrated ADC, A
  double ishunt_;       // Selected calibrated, shunt current, A
  double ishunt_filt_;  // Filtered, calibrated sensed shunt current for observer, A
};


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
  double Vbatt_model;     // Sim coefficient model battery voltage based on filtered current, V
  double Tbatt;           // Sensed battery temp, C
  double Tbatt_filt;      // Filtered, sensed battery temp, C
  double Vshunt_amp;      // Sensed shunt voltage, V
  double Vshunt;          // Sensed shunt voltage, V
  double Vshunt_filt;     // Filtered, sensed shunt voltage, V
  double shunt_v2a_s;     // Selected shunt conversion gain, A/V
  double Ishunt;          // Selected calibrated, shunt current, A
  double Ishunt_filt;     // Filtered, calibrated sensed shunt current for observer, A
  double Wshunt;          // Sensed shunt power, W
  double Wcharge;         // Charge power, W
  int I2C_status;
  double T;               // Update time, s
  double T_filt;          // Filter update time, s
  double T_temp;          // Temperature update time, s
  boolean saturated;      // Battery saturation status based on Temp and VOC
  Shunt *ShuntAmp;        // Shunt amplified
  Shunt *ShuntNoAmp;      // Shunt non-amplified
  Sensors(void) {}
  Sensors(int I2C_status, double T, double T_temp)
  {
    this->I2C_status = I2C_status;
    this->T = T;
    this->T_filt = T;
    this->T_temp = T_temp;
    this->ShuntAmp = new Shunt("Amp", 0x49, &rp.delta_q_inf_amp, &rp.tweak_bias_amp, &cp.curr_bias_amp, shunt_amp_v2a_s);
    this->ShuntNoAmp = new Shunt("No Amp", 0x48, &rp.delta_q_inf_noamp, &rp.tweak_bias_noamp, &cp.curr_bias_noamp, shunt_noamp_v2a_s);
  }
};


// Headers
void manage_wifi(unsigned long now, Wifi *wifi);
void serial_print(unsigned long now, double T);
void load(const boolean reset_free, const unsigned long now, Sensors *Sen, Pins *myPins);
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
void tweak(Sensors *Sen, unsigned long int now);

#endif
