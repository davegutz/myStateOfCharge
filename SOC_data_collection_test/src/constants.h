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
#define READ_DELAY            1UL    // Sensor read wait (5000, 100 for stress test), ms (1000UL)
#define PUBLISH_SERIAL_DELAY  1UL    // Serial print interval (1000UL)
#define DISCONNECT_DELAY      75000UL   // After these milliseconds no WiFi, disconnect (75000UL)
#define CHECK_INTERVAL        180000UL  // How often to check for WiFi once disconnected (180000UL)
#define CONNECT_WAIT          10000UL   // How long after setup that we try WiFi for first time (10000UL)
#define EST_WAIT              20000UL   // How long after init that we hold estimator integrate to let filters settle (20000UL)
#define INIT_WAIT             30000UL   // How long after setup that we wait for convergence of observer before setting integrator (30000UL)
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
#define SHUNT_AMP_V2A_S       -1189.3   // Shunt amp V2A scalar, A/V (1333 is 100A/0.075V)  (-1189.3)
#define SHUNT_AMP_V2A_A       0         // Shunt amp V2A adder, A derived from charge-discharge integral match (0 in theory)  (0)

// Battery voltage measurement gain
const double vbatt_conv_gain = double(PHOTON_ADC_VOLT) * double(VBATT_SENSE_R_HI+VBATT_SENSE_R_LO) /
                              double(VBATT_SENSE_R_LO) / double(PHOTON_ADC_COUNT) * double(VBATT_S);

#endif // CONSTANTS_H_
