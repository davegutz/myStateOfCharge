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
#define TA_SENSOR 0x27          // Ambient room Honeywell temp sensor bus address (0x27)
#define TP_TEMPCAL 1            // Maxim 1-wire plenum temp sense calibrate (0), F
#define TA_TEMPCAL -5           // Honeywell calibrate temp sense (0), F
#define HW_HUMCAL -2            // Honeywell calibrate humidity sense (-2), %
#define ONE_DAY_MILLIS 86400000 // Number of milliseconds in one day (24*60*60*1000)
#define NOMSET 67               // Nominal setpoint for modeling etc, F
#define MINSET 50               // Minimum setpoint allowed (50), F
#define MAXSET 75               // Maximum setpoint allowed (75), F
#define CONTROL_DELAY    2000UL     // Control law wait, ms (5000)
#define PUBLISH_DELAY    10000UL    // Time between cloud updates, ms (30000UL)
#define PUBLISH_PARTICLE_DELAY 2000UL // Particle cloud updates (2000UL)
#define READ_DELAY       500UL      // Sensor read wait (5000, 100 for stress test), ms (1000UL)
#define QUERY_DELAY      900000UL   // Web query wait (15000, 100 for stress test), ms (900000UL)
#define DISPLAY_DELAY    300UL      // Serial display scheduling frame time, ms (300UL)
#define SERIAL_DELAY     5000UL     // Serial print interval (5000UL)
#define STAT_RESERVE     200        // Space to reserve for status string publish (150)
#define HYST             0.01       // Heat control law hysteresis (0.75), F
#define WEATHER_WAIT     900UL      // Time to wait for weather webhook, ms (900UL)
#define GMT              -5         // Enter time different to zulu (does not respect DST)
#define USE_DST          1          // Whether to apply DST or not, 0 or 1
#define READ_TP_DELAY    3600000UL  // Time between Tp read shutdowns (1800000UL = 30 min)
#define DWELL_TP_DELAY   30000UL    // Time between Tp read shutdowns (30000UL = 30 sec)
const int EEPROM_ADDR = 1;          // Flash address

#ifdef BARE
#define BARE_PHOTON
const boolean bare = true;  // Force continuous calibration mode to run with bare boards (false)
#else
#undef BARE_PHOTON
const boolean bare = false;  // Force continuous calibration mode to run with bare boards (false)
#endif

const uint32_t pwm_frequency = 5000;    // Photon pwm driver frequency, Hz. (ECMF needs 1-10kHz)

// Model
#define M_CPA           0.23885     // Heat capacity of dry air at 80F, BTU/lbm/F (1.0035 J/g/K)  (0.23885)
#define M_CPW           0.2         // Heat capacity of walls, BTU/lbm/F  (0.2)
#define M_AW            396.        // Surface area room walls and ceiling, ft^2  (396)
#define M_MW            1000.       // Mass of room ws and ceiling, lbm (1000)
#define M_HI            1.4         // Heat transfer resistance inside still air, BTU/hr/ft^2/F.  Approx industry avg (1.4)
#define M_HO            4.4         // Heat transfer resistance outside still air, BTU/hr/ft^2/F.  Approx industry avg (4.4)
#define M_RHOA          0.0739      // Density of dry air at 80F, lbm/ft^3  (0.0739)
#define M_R4            22.5        // Resistance of R4 wall insulation, F-ft^2/(BTU/hr)  (22.5)
#define M_R22           125.        // Resistance of R22 wall insulation, F-ft^2/(BTU/hr)  (125)
#define M_RWALL         M_R4        // Resistance of wall insulation, F-ft^2/(BTU/hr)  (R4)
#define M_DUCT_TEMP_DROP    7.      // Observed using infrared thermometer, F (7)
#define M_DUCT_DIA      0.5         // Duct diameter, ft (0.5)
#define M_MDOTL_INCR    360.        // Duct long term heat soak, s (360)   CoolTerm Capture 2021-01-21 14-12-19.xlsx
#define M_MDOTL_DECR    90.         // Duct long term heat soak, s (90)    data match activities
#define M_MUA           0.04379     // Viscosity air, lbm/ft/hr (0.04379)
#define M_VOL_AIR       1152.       // Volume of air in 8x12x12 room, ft^3 (1152)
#define M_MAIR          (M_VOL_AIR * M_RHOA)        // Mass of air in room, lbm
#define M_SMDOT         1.0         // Scale duct flow
#define M_GCONV         60          // Convective heat flow gain, (BTU/hr)/F
#define M_GLK           50          // Unknown room heat flow gain, (BTU/hr)/F
#define M_QLK           500.        // Model alignment heat loss, BTU/hr (0)
#define M_GLKD          100         // Unknown duct heat flow gain, (BTU/hr)/F
#define M_QLKD          -3537       // Duct model alignment heat loss, BTU/hr (0)
#define M_TK            68          // Kitchen temperature, F
const double M_RSA = 1./M_HI/M_AW + M_RWALL/M_AW + 1./M_HO/M_AW;  // Effective resistance of air,  F-ft^2/(BTU/hr)
const double M_RSAI = 1./M_HI/M_AW;                 // Resistance air to wall,  F-ft^2/(BTU/hr)
const double M_RSAO = M_RWALL/M_AW + 1./M_HO/M_AW;    // Resistance wall to OAT, F-ft^2/(BTU/hr)
const double M_DN_TADOT = 3600. * M_CPA * M_MAIR;   // Heat capacitance of air, (BTU/hr)  / (F/sec)
const double M_DN_TWDOT = 3600. * M_CPW * M_MW;     // Heat capacitance of air, (BTU/hr)  / (F/sec)
const double M_QCON = (M_QLK + 104) * 0.7;          // Model alignment heat gain when cmd = 0, BTU/hr. 
#define M_AP_0          -2.102E-02  // cmd^0 coefficient pressure polynomial cmd-->in H20
#define M_AP_1          3.401E-03   // cmd^1 coefficient pressure polynomial cmd-->in H20
#define M_AP_2          5.592E-05   // cmd^2 coefficient pressure polynomial cmd-->in H20
#define M_AQ_0          0           // cmd^0 coefficient flow polynomial cmd-->cfm
#define M_AQ_1          2.621644    // cmd^1 coefficient flow polynomial cmd-->cfm
#define M_AQ_2          -0.005153   // cmd^2 coefficient flow polynomial cmd-->cfm
#define M_TRANS_CONV_LOW    400     // mdot threshold to begin transitioning from full qconv to 0
#define M_TRANS_CONV_HIGH   700     // mdot threshold to end transition to 0 qconv
#define M_GAIN_O        6           // Change duty into heat for model observer control  btu/hr/duty

#define C_G             0.150       // Control gain, r/s = %/F (0.030)
#define C_TAU           600         // Control lead, s  (600)
#define C_DB            0.1         // Deadband in error, F
#define C_MAX           100         // Integral and overall max limit, %
#define C_MIN           0           // Integral and overall min limit, %
#define C_LLMAX         20          // Proportional path max limit, %
#define C_LLMIN         -20         // Proportional path min limit, %
#define C_DB_O          0.0         // Deadband in error, F
#define C_MAX_O         5000        // Integral and overall max limit, %
#define C_MIN_O         -5000       // Integral and overall min limit, %
#define C_LLMAX_O       20          // Proportional path max limit, %
#define C_LLMIN_O       -20         // Proportional path min limit, %

const double SUN_WALL_AREA = 8*10/2;  // Area of sunshine room wall impacted by sun, ft^2
#define SUN_WALL_REFLECTIVITY 0.8   // Fraction of energy rejected by wall

#endif // CONSTANTS_H_
