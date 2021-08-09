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

#include "myRoom.h"
#include "myFilters.h"
#include <OneWire.h>
#include <DS18.h>
#include "myInsolation.h"

enum Mode {POT, WEB, AUTO, SCHD}; // To keep track of mode


// Pins
struct Pins
{
  byte pin_1_wire;     // 1-wire Plenum temperature sensor
  pin_t pwm_pin;       // Power the PWM transistor base via 300k resistor
  byte status_led;     // On-board led
  byte tach_sense;     // Sense ECMF speed
  byte pot_trim;       // Trim Pot
  byte pot_control;    // Control Pot
  Pins(void) {}
  Pins(byte pin_1_wire, pin_t pwm_pin, byte status_led, byte tach_sense, byte pot_trim, byte pot_control)
  {
    this->pin_1_wire = pin_1_wire;
    this->pwm_pin = pwm_pin;
    this->status_led = status_led;
    this->tach_sense = tach_sense;
    this->pot_trim = pot_trim;
    this->pot_control = pot_control;
  }
};


// Sensors
struct Sensors
{
  double Ta;          // Sensed ambient room temp, F
  double Tp;          // Sensed plenum temp, F
  double Ta_obs;      // Modeled air temp, F
  double Ta_filt;     // Filtered, sensed ambient room temp, F
  double OAT;         // Outside air temperature, F
  int I2C_status;
  double pcnt_pot;    // Potentiometer read, % of 0-10v
  double pcnt_tach;   // Tach read, % of 0-10v
  double last_Tp;     // For testing of change in value for shutdown function
  double T;
  double potValue;
  double hum;
  bool webHold;
  bool lastHold;      // Web toggled permanent and acknowledged
  bool held;          // Status of webHold command in Photon
  int potDmd;         // Pot value, deg F
  Mode controlMode;   // Present control mode
  double qduct;       // heat from duct airflow, BTU/hr
  double mdot;        // air flow in duct, lbm/hr
  double mdot_lag;    // lagged air flow in duct, lbm/hr
  Sensors(void) {}
  Sensors(double Ta, double Tp, double Ta_obs, double Ta_filt,
    double OAT, int I2C_status, double pcnt_pot, double pcnt_tach, double last_Tp,
    double T, double potValue, double hum, bool webHold, bool lastHold, bool held,
    int potDmd, Mode controlMode, double qduct, double mdot, double mdot_lag)
  {
    this->Ta = Ta;
    this->Tp = Tp;
    this->Ta_obs = Ta_obs;
    this->Ta_filt = Ta_filt;
    this->OAT = OAT;
    this->I2C_status = I2C_status;
    this->pcnt_pot = pcnt_pot;
    this->pcnt_tach = pcnt_tach;
    this->last_Tp = last_Tp;
    this->T = T;
    this->potValue = potValue;
    this->hum = hum;
    this->webHold = webHold;
    this->lastHold = lastHold;
    this->held = held;
    this->potDmd = potDmd;
    this->controlMode = controlMode;
    this->qduct = qduct;
    this->mdot = mdot;
    this->mdot_lag = mdot_lag;
  }
};

// Control
struct Control
{
  double cmd;
  double T;
  uint32_t duty;        // PWM duty cycle, 255-0 counts for 0-100% on ECMF-C
  double cmd_o;         // Observer PWM duty cycle output
  double set;
  double webDmd;        // Web sched, F
  double heat_o;        // Observer heat modification, Btu/hr
  double lastChangedWebDmd; // Remebered webDmd, F
  Control(void) {}
  Control(double cmd, double T, uint32_t duty, double cmd_o, double set, double webDmd,
    double heat_o, double lastChangedWebDmd)
  {
    this->cmd = cmd;
    this->T = T;
    this->duty = duty;
    this->cmd_o = cmd_o;
    this->set = set;
    this->webDmd = webDmd;
    this->heat_o = heat_o;
    this->lastChangedWebDmd = lastChangedWebDmd;
  }
};
 

// Weather data structure
struct WeatherData
{
  String locationStr;
  String weatherStr;
  String tempStr;
  String windStr;
  String visStr;
};


// Publishing
struct Publish
{
  uint32_t now;
  String unit;
  String hmString;
  double controlTime;
  double set;
  double Tp;
  double Ta;
  double cmd;
  double T;
  double OAT;
  double Ta_obs;
  int I2C_status;
  double err;
  double prop;
  double integ;
  double cont;
  double pcnt_pot;
  uint32_t duty;
  double Ta_filt;
  double solar_heat;
  double heat_o;
  double hum;
  int numTimeouts;
  bool held;
  double potDmd;
  double lastChangedWebDmd;
  bool webHold;
  double webDmd;
  WeatherData weatherData;
  double qduct;
  double mdot;
  double mdot_lag;
};


void publish_particle(unsigned long now);
void serial_print_inputs(unsigned long now, double T);
void serial_print(double cmd);
boolean load(int reset, double T, Sensors *sen, Control *con, DuctTherm *duct, RoomTherm *room,
      General2_Pole* TaSenseFilt, Insolation *sun_wall, DS18 *sensor_plenum, Pins *myPins);
uint32_t pwm_write(uint32_t duty, Pins *myPins);
void saveTemperature(const int set, const int webDmd, const int held, const int addr, double Ta_obs);
String tryExtractString(String str, const char* start, const char* end);
double  decimalTime(unsigned long *currentTime, char* tempStr);
int setSaveDisplayTemp(double t, Sensors *sen, Control *con);
void print_serial_header(void);

#endif
