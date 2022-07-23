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
#define ONE_HOUR_MILLIS       3600000UL // Number of milliseconds in one hour (60*60*1000)
#define ONE_DAY_MILLIS        86400000UL// Number of milliseconds in one day (24*60*60*1000)
#define PUBLISH_BLYNK_DELAY   10000UL   // Blynk cloud updates, ms (10000UL = 10 sec)
#define PUBLISH_PARTICLE_DELAY 2000UL   // Particle cloud updates (2000UL = 2 sec)
#define READ_DELAY            100UL     // Sensor read wait, ms (100UL = 0.1 sec)
#define READ_TEMP_DELAY       6000UL    // Sensor read wait, ms (6000UL = 6 sec)
#define SUMMARIZE_DELAY       1800000UL // Battery state tracking and reporting, ms (1800000UL = 30 min)
#define SUMMARIZE_WAIT        60000UL   // Summarize alive time before first save, ms (60000UL = 1 min)
#define PUBLISH_SERIAL_DELAY  400UL     // Serial print interval (400UL = 0.4 sec)
#define DISPLAY_USER_DELAY    1200UL    // User display update (1200UL = 1.2 sec)
#define CONTROL_DELAY         100UL     // Control read wait, ms (100UL = 0.1 sec)
#define DISCONNECT_DELAY      75000UL   // After these milliseconds no WiFi, disconnect (75000UL = 1:15 min)
#define CHECK_INTERVAL        180000UL  // How often to check for WiFi once disconnected (180000UL = 3 min)
#define CONNECT_WAIT          10000UL   // How long after setup that we try WiFi for first time (10000UL = 10 sec)
#define CONFIRMATION_DELAY    10000UL   // How long to confirm WiFi on before streaming (10000UL = 10 sec)
#define GMT                   -5        // Enter time different to zulu (does not respect DST) (-5)
#define USE_DST               1         // Whether to apply DST or not, 0 or 1 (1)
#define TBATT_TEMPCAL         0.56      // Maxim 1-wire plenum temp sense calibrate (0.56), C
#define MAX_TEMP_READS        10        // Number of consequetive temp queries allowed (10)
#define TEMP_RANGE_CHECK      -5.       // Minimum expected temp reading, C (-5.)
#define TEMP_RANGE_CHECK_MAX  70.       // Maximum allowed temp reading, C (70.)
#define VBATT_SENSE_R_LO      4700      // Vbatt low sense resistor, ohm (4700)
#define VBATT_SENSE_R_HI      20000     // Vbatt high sense resistor, ohm (20000)
#define VBATT_S               1.017     // Vbatt sense scalar (1.017)
#define VBATT_A               0.0       // Vbatt sense adder, V (0)
#define PHOTON_ADC_COUNT      4096      // Photon ADC range, counts (4096)
#define PHOTON_ADC_VOLT       3.3       // Photon ADC range, V (3.3)
#define SHUNT_NOAMP_V2A_S     -1189.3   // Shunt V2A scalar, A/V (1333 is 100A/0.075V)  (-1189.3)
#define SHUNT_AMP_R1          5600.     // Amplifed shunt ADS resistance, ohms
#define SHUNT_AMP_R2          27000.    // Amplifed shunt ADS resistance, ohms
#define SCREEN_WIDTH          128       // OLED display width, in pixels (128)
#define SCREEN_HEIGHT         32        // OLED display height, in pixels (4)
#define OLED_RESET            4         // Reset pin # (or -1 if sharing Arduino reset pin) (4)
#define SCREEN_ADDRESS        0x3C      // See datasheet for Address; 0x3D for 128x64, (0x3C for 128x32)
#define F_MAX_T               0.5       // Maximum call update time sensors and coulomb counter (0.5)
#define F_MAX_T_TEMP          18.0      // Maximum call update time filters (18.0)
#define F_W_T                 0.05      // Temperature filter wn, r/s (0.05)   
#define F_Z_T                 0.80      // Temperature filter zeta (0.80)
#define NSUM                  110       // Number of saved summaries.   If too large, will get compile error BACKUPSRAM
#define HDB_TBATT             0.06      // Half deadband to filter Tbatt, F (0.06)
#define HDB_VBATT             0.05      // Half deadband to filter Vbatt, V (0.05)
#define T_SAT                 5         // Saturation time, sec (5)
#define T_DESAT               (T_SAT*2) // De-saturation time, sec
#define TEMP_PARASITIC        true      // DS18 sensor power. true means leave it on all the time (true)
#define TEMP_DELAY            1         // Time to block temperature sensor read in DS18 routine, ms (1)
#define TEMP_INIT_DELAY       10000     // It takes 10 seconds first read of DS18 (10000)
#define TWEAK_MAX_CHANGE      0.001     // Maximum allowed tweak per charge cycle, scalar +/- (0.001)
#define TWEAK_MAX             0.01      // Maximum tweak allowed, scalar +/- (0.01)
#define TWEAK_WAIT            6.        // Time to persist unsaturated before allowing peak, hrs (6)
#define TWEAK_GAIN            0.66      // Tweak change limit gain to make soft landing (0.66)
#define TT_WAIT               10.       // Before tweak test print wait, s (10)
#define TT_TAIL               60.       // After tweak test print wait, s (60)
#define SOC_DISAGREE_THRESH   0.1       // Signal selection threshold for Coulomb counter EKF disagree test (0.1)
#define TAU_ERR_FILT          5.        // Current sensor difference filter time constant, s (5.)
#define MAX_ERR_FILT          10.       // Current sensor difference Filter maximum windup, A (10.)
#define MAX_ERR_T             10.       // Maximum update time allowed to avoid instability, s (10.)
#define IBATT_DISAGREE_THRESH 5.        // Signal selection threshold for current disagree test, A (5.)
#define IBATT_DISAGREE_SET    20.       // Signal selection current disagree fail persistence, s (20.)
#define IBATT_DISAGREE_RESET  1.        // Signal selection current disagree reset persistence, s (1.)
#define IBATT_HARD_SET        1.        // Signal selection volt range fail persistence, s (1.)
#define IBATT_HARD_RESET      1.        // Signal selection volt range fail reset persistence, s (1.)
#define VBATT_MAX             17.       // Signal selection hard fault threshold, V (17. < vbatt_conv_gain*4095)
#define VBATT_MIN             9.        // Signal selection hard fault threshold, V (0.  < 9. < 10 bms shutoff)
#define VBATT_HARD_SET        1.        // Signal selection volt range fail persistence, s (1.)
#define VBATT_HARD_RESET      1.        // Signal selection volt range fail reset persistence, s (1.)


// Conversion gains
const double shunt_noamp_v2a_s = SHUNT_NOAMP_V2A_S;  
const double shunt_amp_v2a_s = shunt_noamp_v2a_s * SHUNT_AMP_R1/SHUNT_AMP_R2; // Shunt amp V2A scalar

// Battery voltage measurement gain
const double vbatt_conv_gain = double(PHOTON_ADC_VOLT) * double(VBATT_SENSE_R_HI+VBATT_SENSE_R_LO) /
                              double(VBATT_SENSE_R_LO) / double(PHOTON_ADC_COUNT) * double(VBATT_S);

#endif // CONSTANTS_H_
