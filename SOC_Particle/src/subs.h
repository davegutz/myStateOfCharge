//
// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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
#include "Cloud.h"
#include "talk/chitchat.h"
#include "parameters.h"
#include "command.h"
#include "Sync.h"

// Sensors
#include "Sensors.h"
#include "serial.h"

// Display
#include "Adafruit/Adafruit_GFX.h"
#include "Adafruit/Adafruit_SSD1306.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern PublishPars pp;  // For publishing
extern CommandPars cp;  // Various parameters to be static at system level


// Pins
struct Pins
{
  uint16_t pin_1_wire;  // 1-wire Plenum temperature sensor
  uint16_t status_led;  // On-board led
  uint16_t Vb_pin;      // Battery voltage, e.g. Battleborn, CHINS
  uint16_t Vcn_pin;     // No Amp (n) common voltage
  uint16_t Von_pin;     // No Amp (n) output voltage
  uint16_t Vcm_pin;     // Amp (m) common voltage
  uint16_t Vom_pin;     // Amp (m) output voltage
  uint16_t Vh3v3_pin;   // 3.3V voltage
  uint16_t VTb_pin;     // Tb 2wire measurement voltage
  boolean using_opAmp;// Using differential hardware amp
  boolean using_hv3v3;  // Using differential hardware amp
  boolean using_VTb;    // Using I2C port for 2wire temperature measurement (RTD)
  Pins(void) {}
  Pins(uint16_t pin_1_wire, uint16_t status_led, uint16_t Vb_pin, uint16_t Vcn_pin, uint16_t Von_pin, uint16_t Vcm_pin, uint16_t Vom_pin)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vb_pin = Vb_pin;
    this->Vcn_pin = Vcn_pin;
    this->Von_pin = Von_pin;
    this->Vcm_pin = Vcm_pin;
    this->Vom_pin = Vom_pin;
    this->using_opAmp = false;
    this->using_hv3v3 = false;
  }
  Pins(uint16_t pin_1_wire, uint16_t status_led, uint16_t Vb_pin, uint16_t Von_pin, uint16_t Vom_pin)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vb_pin = Vb_pin;
    this->Von_pin = Von_pin;
    this->Vom_pin = Vom_pin;
    this->using_opAmp = true;
    this->using_hv3v3 = false;
  }
  Pins(uint16_t pin_1_wire, uint16_t status_led, uint16_t Vb_pin, uint16_t Von_pin, uint16_t Vom_pin, uint16_t Vh3v3_pin)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vb_pin = Vb_pin;
    this->Von_pin = Von_pin;
    this->Vom_pin = Vom_pin;
    this->Vh3v3_pin = Vh3v3_pin;
    this->using_opAmp = true;
    this->using_hv3v3 = true;
  }
  Pins(uint16_t pin_1_wire, uint16_t status_led, uint16_t Vb_pin, uint16_t Von_pin, uint16_t Vom_pin, uint16_t Vh3v3_pin, uint16_t VTb_pin, boolean using_2wire)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vb_pin = Vb_pin;
    this->Von_pin = Von_pin;
    this->Vom_pin = Vom_pin;
    this->Vh3v3_pin = Vh3v3_pin;
    this->using_opAmp = true;
    this->using_hv3v3 = true;
    this->VTb_pin = VTb_pin;
    this->using_VTb = using_2wire;
  }
};


// Headers
void harvest_temp_change(const float temp_c, BatteryMonitor *Mon, BatterySim *Sim);
void initialize_all(BatteryMonitor *Mon, Sensors *Sen, const float soc_in, const boolean use_soc_in);
void load_ib_vb(const boolean reset, const boolean reset_temp, Sensors *Sen, Pins *myPins, BatteryMonitor *Mon);
void monitor(const boolean reset, const boolean reset_temp, const unsigned long long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen);
void oled_display(Adafruit_SSD1306 *display, Sensors *Sen, BatteryMonitor *Mon);
void oled_display(Sensors *Sen, BatteryMonitor *Mon);
void sense_synth_select(const boolean reset, const boolean reset_temp, const unsigned long long now, const unsigned long long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen);
void sync_time(unsigned long long now, unsigned long long *last_sync, unsigned long long *millis_flip);
String time_long_2_str(const time_t current_time, char *tempStr);

#endif
