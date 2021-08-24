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
#include "local_config.h"       // this is not in GitHub repository.  Normally empty file

// Constants always defined
#define ONE_DAY_MILLIS        86400000  // Number of milliseconds in one day (24*60*60*1000)
#define PUBLISH_BLYNK_DELAY   10000UL   // Blynk cloud updates, ms (10000UL)
#define PUBLISH_PARTICLE_DELAY 2000UL   // Particle cloud updates (2000UL)
#define READ_DELAY            1000UL    // Sensor read wait (5000, 100 for stress test), ms (1000UL)
#define SUMMARIZE_DELAY       2000UL   // Battery state tracking and reporting, ms (100000UL)
#define PUBLISH_SERIAL_DELAY  1000UL    // Serial print interval (1000UL)
#define DISCONNECT_DELAY      75000UL   // After these milliseconds no WiFi, disconnect (75000UL)
#define CHECK_INTERVAL        180000UL  // How often to check for WiFi once disconnected (180000UL)
#define CONNECT_WAIT          10000UL   // How long after setup that we try WiFi for first time (10000UL)
#define INIT_WAIT             120000UL  // How long after setup that we wait for convergence of observer before setting integrator (120000UL)
#define CONFIRMATION_DELAY    10000UL   // How long to confirm WiFi on before streaming (10000UL)
#define GMT                   -5        // Enter time different to zulu (does not respect DST) (-5)
#define USE_DST               1         // Whether to apply DST or not, 0 or 1 (1)
#define TBATT_TEMPCAL         1         // Maxim 1-wire plenum temp sense calibrate (1), F
#define NOMVBATT              13.1      // Nominal battery voltage, V (13.1)
#define NOMTBATT              72        // Nominal battery temp, F (72)
#define NOMVSHUNTI            0         // Nominal shunt reading, integer (0)
#define NOMVSHUNT             0         // Nominal shunt reading, V (0)
#define VBATT_SENSE_R_LO      4700      // Vbatt low sense resistor, ohm (4700)
#define VBATT_SENSE_R_HI      20000     // Vbatt high sense resistor, ohm (20000)
#define VBATT_S               1.017     // Vbatt sense scalar (1.017)
#define VBATT_A               0.0       // Vbatt sense adder, V (0)
#define PHOTON_ADC_COUNT      4096      // Photon ADC range, counts (4096)
#define PHOTON_ADC_VOLT       3.3       // Photon ADC range, V (3.3)
#define SHUNT_V2A_S           -1189.3   // Shunt V2A scalar, A/V (1333 is 100A/0.075V)  (-1189.3)
#define SHUNT_V2A_A           0         // Shunt V2A adder, A derived from charge-discharge integral match (0 in theory)  (0)
                                        // observed on prototype over time that 0 V really is 0 A.   Not much bias in ADS device
//const int EEPROM_ADDR = 1;            // Flash address (1)
#define SCREEN_WIDTH          128       // OLED display width, in pixels (128)
#define SCREEN_HEIGHT         32        // OLED display height, in pixels (4)
#define OLED_RESET            4         // Reset pin # (or -1 if sharing Arduino reset pin) (4)
#define SCREEN_ADDRESS        0x3C      // See datasheet for Address; 0x3D for 128x64, (0x3C for 128x32)
#define C_G             0.010       // Control gain, r/s = %/F (about 0.1 is instability point for C_TAU=1) (0.010)
#define C_TAU           0           // Control lead, s  (0)
#define C_DB            0.002       // Deadband in error, V (0.0020)
#define C_MAX           1           // Integral and overall max limit, frac (1)
#define C_MIN           0           // Integral and overall min limit, frac (0)
#define C_LLMAX         0.1         // Proportional path max limit, frac (0.1)
#define C_LLMIN         -0.2        // Proportional path min limit, frac (-0.2)
#define C_SOC_R_MAX     0.01        // Maximum integrating tf rate to limit invalid excursions, frac/sec (0.01)
#define C_KICK_TH       0.2         // Voltage error to kick pid, V
#define C_KICK          8           // Amount to kick pid gain, frac

#define F_O_MAX_T       3.0         // Maximum call update time filters (3.0)
#define F_MAX_T         3.0         // Maximum call update time filters (3.0)
#define F_O_W           0.50        // Observer filter wn, r/s (0.1)   
#define F_O_Z           0.80        // Observer filter zeta (0.80)
#define F_W             0.05        // Filter wn, r/s (0.1)   
#define F_Z             0.80        // Filter zeta (0.80)


// Battery voltage measurement gain
const double vbatt_conv_gain = double(PHOTON_ADC_VOLT) * double(VBATT_SENSE_R_HI+VBATT_SENSE_R_LO) /
                              double(VBATT_SENSE_R_LO) / double(PHOTON_ADC_COUNT) * double(VBATT_S);

#endif // CONSTANTS_H_
