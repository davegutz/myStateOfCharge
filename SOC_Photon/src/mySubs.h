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
#include "mySync.h"

// Sensors
#include "mySensors.h"

// Display
#include <Adafruit/Adafruit_GFX.h>
#include <Adafruit/Adafruit_SSD1306.h>

extern RetainedPars rp; // Various parameters to be static at system level
extern PublishPars pp;  // For publishing
extern CommandPars cp;  // Various parameters to be static at system level


// Pins
struct Pins
{
  byte pin_1_wire;  // 1-wire Plenum temperature sensor
  byte status_led;  // On-board led
  byte Vb_pin;      // Battery voltage
  pin_t pwm_pin;    // External signal injection
  Pins(void) {}
  Pins(byte pin_1_wire, byte status_led, byte Vb_pin, pin_t pwm_pin)
  {
    this->pin_1_wire = pin_1_wire;
    this->status_led = status_led;
    this->Vb_pin = Vb_pin;
    this->pwm_pin = pwm_pin;
  }
};


// Headers
void create_print_string(Publish *pubList);
void create_tweak_string(Publish *pubList, Sensors *Sen, BatteryMonitor *Mon);
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip);
void finish_request(void);
void get_string(String *source);
void load_ib_vb(const boolean reset, const unsigned long now, Sensors *Sen, Pins *myPins, BatteryMonitor *Mon);
void manage_wifi(unsigned long now, Wifi *wifi);
void monitor(const boolean reset, const boolean reset_temp, const unsigned long now,
  TFDelay *Is_sat_delay, BatteryMonitor *Mon, Sensors *Sen);
void oled_display(Adafruit_SSD1306 *display, Sensors *Sen);
void print_serial_header(void);
void print_serial_sim_header(void);
void print_signal_sel_header(void);
void sense_synth_select(const boolean reset, const boolean reset_temp, const unsigned long now, const unsigned long elapsed,
  Pins *myPins, BatteryMonitor *Mon, Sensors *Sen);
void serial_print(unsigned long now, double T);
void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip);
String time_long_2_str(const unsigned long current_time, char *tempStr);
String tryExtractString(String str, const char* start, const char* end);
void tweak_on_new_desat(Sensors *Sen, unsigned long int now);
void tweak_print(Sensors *Sen, BatteryMonitor *Mon);

#endif
