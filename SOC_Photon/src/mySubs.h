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
#include "myLibrary/injection.h"
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
  double Voc;             // Model open circuit voltage based on TODO, V
  double Vbatt_filt;      // Filtered, sensed battery voltage, V
  double Tbatt;           // Sensed battery temp, F
  double Tbatt_filt;      // Filtered, sensed battery temp, F
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
  double curr_bias_amp;   // Signal injection bias for amplified current input, A
  double curr_bias_noamp; // Signal injection bias for non-amplified current input, A
  double curr_bias;       // Signal injection bias for selected current input, A
  boolean saturated;      // Battery saturation status based on Temp and VOC
  Sensors(void) {}
  Sensors(double Vbatt, double Vbatt_filt, double Tbatt, double Tbatt_filt,
          int16_t Vshunt_noamp_int, double Vshunt, double Vshunt_filt,
          int16_t Vshunt_amp_int, double Vshunt_amp, double Vshunt_amp_filt,
          int I2C_status, double T, double T_temp, boolean bare_ads_noamp, boolean bare_ads_amp)
  {
    this->Vbatt = Vbatt;
    this->Vbatt_filt = Vbatt_filt;
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
    this->curr_bias_noamp = 0.0;
    this->curr_bias_amp = 0.0;
  }
};


class BatteryModel: public Battery
{
public:
  BatteryModel();
  BatteryModel(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim);
  ~BatteryModel();
  // operators
  // functions
  double calculate(const double temp_C, const double soc_frac, const double curr_in, const double dt,
    const double q_capacity, const double q_cap);
  uint32_t calc_inj_duty(const unsigned long now, const uint8_t type, const double amp, const double freq);
  double count_coulombs(const double dt, const double temp_c, const double charge_curr, const double t_last);
  boolean saturated() { return model_saturated_; };
protected:
  SinInj *Sin_inj_;     // Class to create sine waves
  SqInj *Sq_inj_;       // Class to create square waves
  TriInj *Tri_inj_;     // Class to create triangle waves
  uint32_t duty_;       // Calculated duty cycle for D2 driver to ADC cards (0-255).  Bias on rp.offset
  double sat_ib_max_;   // Current cutback to be applied to modeled ib output, A
  double sat_ib_null_;  // Maximum cutback current for voc=vsat, A
  double sat_cutback_gain_; // Gain to retard ib when voc exceeds vsat, dimensionless
  boolean model_saturated_; // Indicator of maximal cutback, T = cutback = saturated
  double ib_sat_;       // Threshold to declare saturation.  This regeneratively slows down charging so if too small takes too long, A
};


// Headers
void manage_wifi(unsigned long now, Wifi *wifi);
void serial_print(unsigned long now, double T);
void load(const boolean reset_free, Sensors *Sen, Pins *myPins,
    Adafruit_ADS1015 *ads_amp, Adafruit_ADS1015 *ads_noamp, const unsigned long now,
    SlidingDeadband *SdVbatt);
void load_temp(Sensors *Sen, DS18 *SensorTbatt, SlidingDeadband *SdTbatt);
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFilt, General2_Pole* IshuntSenseFilt);
void filter_temp(int reset, Sensors *Sen, General2_Pole* TbattSenseFilt);
String tryExtractString(String str, const char* start, const char* end);
double  decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);
void print_serial_header(void);
void myDisplay(Adafruit_SSD1306 *display, Sensors *Sen);
uint32_t pwm_write(uint32_t duty, Pins *myPins);
String time_long_2_str(const unsigned long current_time, char *tempStr);
void create_print_string(char *buffer, Publish *pubList);
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);

// Talk Declarations
void talk(Battery *MyBatt, BatteryModel *MyBattModel);
void talkH(Battery *batt, BatteryModel *batt_model); // Help

#endif
