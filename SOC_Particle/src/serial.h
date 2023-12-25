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

#ifndef _SERIAL_H
#define _SERIAL_H

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

// Display
#include "Adafruit/Adafruit_GFX.h"
#include "Adafruit/Adafruit_SSD1306.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern PublishPars pp;  // For publishing
extern CommandPars cp;  // Various parameters to be static at system level

// Headers
void create_rapid_string(Publish *pubList, Sensors *Sen, BatteryMonitor *Mon);
void delay_no_block(const unsigned long long int interval);
String finish_request(const String in_str);
String get_cmd(String *source);
boolean is_finished(const char in_char);
void print_all_header(void);
void print_rapid_data(const boolean reset, Sensors *Sen, BatteryMonitor *Mon);
void print_serial_header(void);
void print_serial_sim_header(void);
void print_signal_sel_header(void);
void print_serial_ekf_header(void);
void rapid_print(Sensors *Sen, BatteryMonitor *Mon);
void wait_on_user_input(Adafruit_SSD1306 *display);
void wait_on_user_input();

#endif
