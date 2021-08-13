/*  Heart rate and pulseox calculation Constants

18-Dec-2020 	DA Gutz 	Created from MAXIM code.
// Copyright (C) 2020 - Dave Gutz
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

*/

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#if (PLATFORM_ID == 6)
#define PHOTON
#else
#undef PHOTON
#endif


// Disable flags if needed for debugging, usually commented
//#define BARE                  // ****** see local_config.h  ****Don't change it here
#include "local_config.h"       // this is not in GitHub repository.  Normally empty file

// Test feature usually commented
//#define  FAKETIME                         // For simulating rapid time passing of schedule

// Constants always defined
#define TBATT_TEMPCAL     1       // Maxim 1-wire plenum temp sense calibrate (0), F
#define NOMVBATT          13.1    // Nominal battery voltage, V
#define NOMTBATT          72      // Nominal battery temp, F
#define NOMVSHUNTI        0       // Nominal shunt reading, integer
#define NOMVSHUNT         0       // Nominal shunt reading, V
#define ONE_DAY_MILLIS    86400000 // Number of milliseconds in one day (24*60*60*1000)
//#define CONTROL_DELAY     2000UL  // Control law wait, ms (5000)
#define PUBLISH_DELAY     10000UL // Time between cloud updates, ms (10000UL)
#define PUBLISH_PARTICLE_DELAY 2000UL // Particle cloud updates (2000UL)
#define READ_DELAY        1000UL  // Sensor read wait (5000, 100 for stress test), ms (1000UL)
#define READ_TBATT_DELAY  1000UL  // Sensor read wait (5000, 100 for stress test), ms (1000UL)
#define QUERY_DELAY       900000UL  // Web query wait (15000, 100 for stress test), ms (900000UL)
#define DISPLAY_DELAY     300UL   // Serial display scheduling frame time, ms (300UL)
#define SERIAL_DELAY      4000UL  // Serial print interval (5000UL)
#define STAT_RESERVE      200     // Space to reserve for status string publish (150)
#define GMT               -5      // Enter time different to zulu (does not respect DST)
#define USE_DST           1       // Whether to apply DST or not, 0 or 1
#define VBATT_SENSE_R_LO  4700    // Vbatt low sense resistor, ohm
#define VBATT_SENSE_R_HI  20000   // Vbatt high sense resistor, ohm
#define VBATT_S           1.017   // Vbatt sense scalar
#define VBATT_A           0.0     // Vbatt sense adder, V
#define PHOTON_ADC_COUNT  4096    // Photon ADC range, counts
#define PHOTON_ADC_VOLT   3.3     // Photon ADC range, V
#define SHUNT_V2A_S       1162.42 // Shunt V2A scalar, A/V
#define SHUNT_V2A_A       0.127   // Shunt V2A adder, A
//const int EEPROM_ADDR = 1;          // Flash address
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define NUMFLAKES     10 // Number of snowflakes in the animation example
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
#define DISCONNECT_DELAY    75000   // After these milliseconds no WiFi, disconnect
#define CHECK_INTERVAL      180000  // How often to check for WiFi once disconnected
#define CONNECT_WAIT        10000   // How long after setup that we try WiFi for first time
#define CONFIRMATION_DELAY  10000   // How long to confirm WiFi on before streaming

#ifdef BARE
#define BARE_PHOTON
const boolean bare = true;  // Force continuous calibration mode to run with bare boards (false)
#else
#undef BARE_PHOTON
const boolean bare = false;  // Force continuous calibration mode to run with bare boards (false)
#endif

// Display
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

// Battery voltage gain
const double vbatt_conv_gain = double(PHOTON_ADC_VOLT) * double(VBATT_SENSE_R_HI+VBATT_SENSE_R_LO) /
                              double(VBATT_SENSE_R_LO) / double(PHOTON_ADC_COUNT) * double(VBATT_S);

#endif // CONSTANTS_H_
