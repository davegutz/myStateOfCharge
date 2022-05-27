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
extern PublishPars pp;            // For publishing
extern CommandPars cp;  // Various parameters to be static at system level


// ADS1015-based shunt
class Shunt: public Tweak, Adafruit_ADS1015
{
public:
  Shunt();
  Shunt(const String name, const uint8_t port, float *rp_delta_q_inf, float *rp_tweak_bias, float *cp_ibatt_bias, 
    const float v2a_s);
  ~Shunt();
  // operators
  // functions
  boolean bare() { return ( bare_ ); };
  float cp_ibatt_bias() { return ( *cp_ibatt_bias_ ); };
  float ishunt_cal() { return ( ishunt_cal_ ); };
  void load();
  void pretty_print();
  float v2a_s() { return ( v2a_s_ ); };
  float vshunt() { return ( vshunt_ ); };
  int16_t vshunt_int() { return ( vshunt_int_ ); };
  int16_t vshunt_int_0() { return ( vshunt_int_0_ ); };
  int16_t vshunt_int_1() { return ( vshunt_int_1_ ); };
protected:
  String name_;         // For print statements, multiple instances
  uint8_t port_;        // Octal I2C port used by Acafruit_ADS1015
  boolean bare_;        // If ADS to be ignored
  float *cp_ibatt_bias_; // Global bias, A
  float v2a_s_;        // Selected shunt conversion gain, A/V
  int16_t vshunt_int_;  // Sensed shunt voltage, count
  int16_t vshunt_int_0_;// Interim conversion, count
  int16_t vshunt_int_1_;// Interim conversion, count
  float vshunt_;       // Sensed shunt voltage, V
  float ishunt_cal_;   // Sensed, calibrated ADC, A
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
  float Vbatt;            // Selected battery bank voltage, V
  float Vbatt_hdwe;       // Sensed battery bank voltage, V
  float Vbatt_model;      // Modeled battery bank voltage, V
  float Tbatt;            // Selected battery bank temp, C
  float Tbatt_filt;       // Selected filtered battery bank temp, C
  float Tbatt_hdwe;       // Sensed battery temp, C
  float Tbatt_hdwe_filt;  // Filtered, sensed battery temp, C
  float Tbatt_model;      // Temperature used for battery bank temp in model, C
  float Tbatt_model_filt; // Filtered, modeled battery bank temp, C
  float Vshunt;           // Sensed shunt voltage, V
  float Ibatt;            // Selected battery bank current, A
  float Ibatt_hdwe;       // Sensed battery bank current, A
  float Ibatt_model;      // Modeled battery bank current, A
  float Ibatt_model_in;   // Battery bank current input to model (modified by cutback), A
  float Wbatt;            // Sensed battery bank power, use to compare to other shunts, W
  unsigned long int now;  // Time at sample, ms
  double T;               // Update time, s
  double T_filt;          // Filter update time, s
  double T_temp;          // Temperature update time, s
  boolean saturated;      // Battery saturation status based on Temp and VOC
  Shunt *ShuntAmp;        // Ib sense amplified
  Shunt *ShuntNoAmp;      // Ib sense non-amplified
  DS18* SensorTbatt;      // Tb sense
  General2_Pole* TbattSenseFilt;  // Linear filter for Tb. There are 1 Hz AAFs in hardware for Vb and Ib
  SlidingDeadband *SdTbatt;       // Non-linear filter for Tb
  BatteryModel *Sim;      //  used to model Vb and Ib.   Use Talk 'Xp?' to toggle model on/off. 
  unsigned long int start_inj;  // Start of calculated injection, ms
  unsigned long int stop_inj;   // Stop of calculated injection, ms
  unsigned long int wait_inj;   // Wait before start injection, ms
  float cycles_inj;             // Number of injection cycles
    Sensors(void) {}
  Sensors(double T, double T_temp, byte pin_1_wire)
  {
    this->T = T;
    this->T_filt = T;
    this->T_temp = T_temp;
    this->ShuntAmp = new Shunt("Amp", 0x49, &rp.delta_q_inf_amp, &rp.tweak_bias_amp, &cp.ibatt_bias_amp, shunt_amp_v2a_s);
    if ( rp.debug>102 )
    {
      Serial.printf("After new Shunt('Amp'):\n");
      this->ShuntAmp->pretty_print();
    }
    this->ShuntNoAmp = new Shunt("No Amp", 0x48, &rp.delta_q_inf_noamp, &rp.tweak_bias_noamp, &cp.ibatt_bias_noamp, shunt_noamp_v2a_s);
    if ( rp.debug>102 )
    {
      Serial.printf("After new Shunt('No Amp'):\n");
      this->ShuntNoAmp->pretty_print();
    }
    this->SensorTbatt = new DS18(pin_1_wire, TEMP_PARASITIC, TEMP_DELAY);
    this->TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);
    this->SdTbatt = new SlidingDeadband(HDB_TBATT);
    this->Sim = new BatteryModel(&rp.delta_q_model, &rp.t_last_model, &rp.s_cap_model, &rp.nP, &rp.nS, &rp.sim_mod);
    this->start_inj = 0UL;
    this->stop_inj = 0UL;
    this->cycles_inj = 0.;
  }
};

// Headers
void create_print_string(char *buffer, Publish *pubList);
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);
void filter_temp(const int reset, const float t_rlim, Sensors *Sen, const float tbatt_bias, float *tbatt_bias_last);
void load(const boolean reset_free, const unsigned long now, Sensors *Sen, Pins *myPins);
void load_temp(Sensors *Sen);
void manage_wifi(unsigned long now, Wifi *wifi);
void  monitor(const int reset, const boolean reset_temp, const unsigned long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen);
void oled_display(Adafruit_SSD1306 *display, Sensors *Sen);
void print_serial_header(void);
uint32_t pwm_write(uint32_t duty, Pins *myPins);
void sense_synth_select(const int reset, const boolean reset_temp, const unsigned long now, const unsigned long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen);
void serial_print(unsigned long now, double T);
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);
String time_long_2_str(const unsigned long current_time, char *tempStr);
String tryExtractString(String str, const char* start, const char* end);
void tweak_on_new_desat(Sensors *Sen, unsigned long int now);

#endif
