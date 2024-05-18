/*  Heart rate and pulseox calculation Constants

18-Dec-2020 	DA Gutz 	Created from MAXIM code.
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

*/

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Hardware configuration
#undef HDWE_UNIT
#undef HDWE_BARE
#undef HDWE_PHOTON
#undef HDWE_ARGON
#undef HDWE_PHOTON2
#undef SOFT_SBAUD
#undef SOFT_S1BAUD
#undef HDWE_47L16_EERAM
#undef HDWE_ADS1013_AMP_NOA
#undef HDWE_TSC2010_DUAL
#undef HDWE_INA181_HI_LO
#undef HDWE_SSD1306_OLED
#undef HDWE_DS18B20_SWIRE
#undef HDWE_DS2482_1WIRE

// Software configuration
#undef SOFT_DEPLOY_PHOTON
#undef SOFT_DEBUG_QUEUE

// Setup
// #include "pro0p.h"
// #include "pro1a.h"
// #include "pro2p2.h"
// #include "soc0p.h"
// #include "soc1a.h"
// #include "soc3p2.h"
#include "soc2p2.h"
const String unit = version + "_" + HDWE_UNIT;

// Constants always defined
#define ONE_HOUR_MILLIS       3600000UL // Number of milliseconds in one hour (60*60*1000)
#define ONE_DAY_MILLIS        86400000UL// Number of milliseconds in one day (24*60*60*1000)
#define TALK_DELAY            313UL      // Talk wait, ms (313UL = 0.313 sec)
#define READ_DELAY            100UL     // Sensor read wait, ms (100UL = 0.1 sec) Dr
#define READ_TEMP_DELAY       6011UL    // Sensor read wait, ms (6011UL = 6.011 sec)
#define SUMMARY_DELAY         1800000UL // Battery state tracking and reporting, ms (1800000UL = 30 min) Dh
#define SUMMARY_WAIT          60000UL   // Summarize alive time before first save, ms (60000UL = 1 min) Dh
#define PUBLISH_SERIAL_DELAY  400UL     // Serial print interval (400UL = 0.4 sec)
#define DISPLAY_USER_DELAY    1200UL    // User display update (1200UL = 1.2 sec)
#define CONTROL_DELAY         100UL     // Control read wait, ms (100UL = 0.1 sec)
#define SNAP_WAIT             10000ULL  // Interval between fault snapshots (10000ULL = 10 sec)
#define DP_MULT               4         // Multiples of read to capture data DP
#define TBATT_TEMPCAL         0.56      // Maxim 1-wire plenum temp sense calibrate (0.56), C
#define MAX_TEMP_READS        10        // Number of consequetive temp queries allowed (10)
#define TEMP_RANGE_CHECK      -5.       // Minimum expected temp reading, C (-5.)
#define TEMP_RANGE_CHECK_MAX  70.       // Maximum allowed temp reading, C (70.)
#define VB_S                  1.0       // Vb sense scalar (1.0)
#define VB_A                  0.0       // Vb sense adder, V (0)
#define PHOTON_ADC_COUNT      4096      // Photon ADC range, counts (4096)
#define PHOTON_ADC_VOLT       3.3       // Photon ADC range, V (3.3)
#define SCREEN_WIDTH          128       // OLED display width, in pixels (128)
#define SCREEN_HEIGHT         32        // OLED display height, in pixels (4)
#define OLED_RESET            4         // Reset pin # (or -1 if sharing Arduino reset pin) (4)
#define SCREEN_ADDRESS        0x3C      // See datasheet for Address; 0x3D for 128x64, (0x3C for 128x32)
#define F_MAX_T               ChargeTransfer_T_MAX  // Maximum call update time sensors and coulomb counter (0.5)
#define F_MAX_T_TEMP          18.0      // Maximum call update time filters (18.0)
#define F_W_T                 0.05      // Temperature filter wn, r/s (0.05)   
#define F_Z_T                 0.80      // Temperature filter zeta (0.80)
#define F_W_I                 0.5       // Current filter wn, r/s (0.5)   
#define F_Z_I                 0.80      // Current filter zeta (0.80)

// If NSUM too large, will get flashing red with auto reboot on 'Hs' or compile error `.data' will not fit in region `APP_FLASH'
// For all, there are 40 bytes for each unit of NSUM

#ifdef HDWE_PHOTON  // dec ~134000  units: pro0p, soc0p
    #ifdef SOFT_DEPLOY_PHOTON
        #define NFLT   7  // Number of saved SRAM/EERAM fault data slices 10 s intervals.  If too large, will get compile error BACKUPSRAM (7)
        #define NHIS  56  // Number of saved SRAM history data slices. Sized to approx match  Photon2  (56)
        #define NSUM 206  // Number of saved summaries. If NFLT + NHIS + NSUM too large, will get compile error BACKUPSRAM, or GUI FRAG msg  (206)
    #else
        #ifdef DEBUG_INIT
            #error("Not possible to deploy Photon with DEBUG_INIT")
        #else
            #ifdef SOFT_DEBUG_QUEUE
                #define NFLT  7  // Number of saved SRAM/EERAM fault data slices 10 s intervals.  If too large, will get compile error BACKUPSRAM (7)
                #define NHIS 36  // Number of saved SRAM history data slices. Sized to approx match Photon2 (36)
                #define NSUM 16  // Number of saved summaries. If NFLT + NHIS + NSUM too large, will get compile error BACKUPSRAM  (16)
            #else
                #define NFLT  7  // Number of saved SRAM/EERAM fault data slices 10 s intervals.  If too large, will get compile error BACKUPSRAM (7)
                #define NHIS 56  // Number of saved SRAM history data slices. Sized to approx match  Photon2  (56)
                #define NSUM  9  // Number of saved summaries. If NFLT + NHIS + NSUM too large, will get compile error BACKUPSRAM  (9)
            #endif
        #endif
    #endif
#endif

#ifdef HDWE_ARGON  // dec ~222350  units: pro1a, soc1a
    #define NFLT    7  // Number of saved SRAM/EERAM fault data slices 10 s intervals (7)
    #define NHIS 1000  // Ignored Argon.  Actual nhis_ is dynamically allocated based on EERAM size, holding NFLT constant. 
    #define NSUM 2000  // Number of saved summaries. If NFLT + NSUM ttoo large, will get compile error BACKUPSRAM, or GUI FRAG msg (2000)
#endif

#ifdef HDWE_PHOTON2  // dec ~ 256268  units: pro2p2, soc3p2
    #define NFLT    7  // Number of saved SRAM fault data slices 10 s intervals (7)
    #define NHIS   61  // Number of saved SRAM history data slices. If NFLT + NHIS too large will get compile error BACKUPSRAM (61)
    #define NSUM 3150  // Number of saved summaries. If NFLT + NHIS + NSUM too large, will get compile error BACKUPSRAM, or GUI FRAG msg (3171) or SOS 4 Bus Fault
#endif

#define HDB_TBATT             0.06      // Half deadband to filter Tb, F (0.06)
#define HDB_VB                0.05      // Half deadband to filter Vb, V (0.05)
#define T_SAT                 22        // Saturation time, sec (>21 for no SAT with Dv0.82)
const float T_DESAT =         20;       // De-saturation time, sec
#define TEMP_PARASITIC        true      // DS18 sensor power. true means leave it on all the time (true)
#define TEMP_DELAY            1         // Time to block temperature sensor read in DS18 routine, ms (1)
#define TEMP_INIT_DELAY       10000     // It takes 10 seconds first read of DS18 (10000)
#define CC_DIFF_LO_SOC_SLR    4.        // Large to disable cc_diff
#define TAU_ERR_FILT          5.        // Current sensor difference filter time constant, s (5.)
#define MAX_ERR_FILT          10.       // Current sensor difference Filter maximum windup, A (10.)
#define MAX_ERR_T             10.       // Maximum update time allowed to avoid instability, s (10.)
#define IB_HARD_SET           1.        // Signal selection volt range fail persistence, s (1.)
#define IB_HARD_RESET         1.        // Signal selection volt range fail reset persistence, s (1.)
#define VB_MAX                17.       // Signal selection hard fault threshold, V (17. < VB_CONV_GAIN*4095)
#define VB_MIN                2.        // Signal selection hard fault threshold, V (0.  < 2. < 10 bms shutoff, reads ~3 without power when off)
#define VC_MAX                1.85      // Signal selection hard fault threshold, V (3.9/2 +20%)
#define VC_MIN                1.4       // Signal selection hard fault threshold, V (2.8/2 -20%)
#define IB_MIN_UP             0.2       // Min up charge current for come alive, BMS logic, and fault
#define VB_HARD_SET           1.        // Signal selection volt range fail persistence, s (1.)
#define VB_HARD_RESET         1.        // Signal selection volt range fail reset persistence, s (1.)
#define VC_HARD_SET           1.        // Signal selection volt range fail persistence, s (1.)
#define VC_HARD_RESET         1.        // Signal selection volt range fail reset persistence, s (1.)
#define TB_NOISE              0.        // Tb added noise amplitude, deg C pk-pk
#define TB_NOISE_SEED         0xe2      // Tb added noise seed 0-255 = 0x00-0xFF (0xe2) 
#define VB_NOISE              0.        // Vb added noise amplitude, V pk-pk
#define VB_NOISE_SEED         0xb2      // Vb added noise seed 0-255 = 0x00-0xFF (0xb2)
#define IB_AMP_NOISE          0.        // Ib amplified sensor added noise amplitude, A pk-pk
#define IB_NOA_NOISE          0.        // Ib non-amplified sensor added noise amplitude, A pk-pk
#define IB_AMP_NOISE_SEED     0x01      // Ib amplified sensor added noise seed 0-255 = 0x00-0xFF (0x01) 
#define IB_NOA_NOISE_SEED     0x0a      // Ib non-amplified sensor added noise seed 0-255 = 0x00-0xFF (0x0a) 
#define WRAP_ERR_FILT         4.        // Wrap error filter time constant, s (4)
#define F_MAX_T_WRAP          2.8       // Maximum update time of Wrap filter for stability at WRAP_ERR_FILT (0.7*T for Tustin), s (2.8)
#define MAX_WRAP_ERR_FILT     10.       // Anti-windup wrap error filter, V (10)
const float WRAP_LO_S =       9.;       // Wrap low failure set time, sec (9) // 9 is legacy must be quicker than SAT test
const float WRAP_LO_R = (WRAP_LO_S/2.); // Wrap low failure reset time, sec ('up 1, down 2')
const float WRAP_HI_S = WRAP_LO_S;      // Wrap high failure set time, sec (WRAP_LO_S)
const float WRAP_HI_R = (WRAP_HI_S/2.); // Wrap high failure reset time, sec ('up 1, down 2')
#define WRAP_HI_A       32.             // Wrap high voltage threshold, A (32 after testing; 16=0.2v)
#define WRAP_LO_A       -40.            // Wrap high voltage threshold, A (-40, -20 too small on truck -16=-0.2v, -32 marginal)
#define WRAP_HI_SAT_MARG  0.2           // Wrap voltage margin to saturation, V (0.2)
#define WRAP_HI_SAT_SLR   2.0           // Wrap voltage margin scalar when saturated (2.0)
#define IBATT_DISAGREE_THRESH 10.       // Signal selection threshold for current disagree test, A (10.)
const float IBATT_DISAGREE_SET = (WRAP_LO_S-1.); // Signal selection current disagree fail persistence, s (WRAP_LO_S-1) // must be quicker than wrap lo
#define IBATT_DISAGREE_RESET  1.        // Signal selection current disagree reset persistence, s (1.)
#define TAU_Q_FILT      0.5             // Quiet rate time constant, sec (0.5)
#define MIN_Q_FILT      -5.             // Quiet filter minimum, V (-0.5)
#define MAX_Q_FILT      5.              // Quiet filter maximum, V (0.5)
#define WN_Q_FILT       1.0             // Quiet filter-2 natural frequency, r/s (1.0)
#define ZETA_Q_FILT     0.9             // Quiet fiter-2 damping factor (0.9)
#define MAX_T_Q_FILT    0.2             // Quiet filter max update time (0.2)
#define QUIET_A         0.005           // Quiet set threshold, sec (0.005, 0.01 too large in truck)
#define QUIET_S         60.             // Quiet set persistence, sec (60.)
const float QUIET_R   (QUIET_S/10.);    // Quiet reset persistence, sec ('up 1 down 10')
#define TB_STALE_SET    3600.           // Tb read from one-wire stale persistence for failure, s (3600, 1 hr)
#define TB_STALE_RESET  0.              // Tb read from one-wire stale persistence for reset, s (0)
#define NOMINAL_TB      15.             // Middle of the road Tb for decent reversionary operation, deg C (15.)
#define NOMINAL_VB   (13.*NS)           // Middle of the road Vb for decent reversionary operation, V (13.)
#define NOMINAL_VSAT    13.85           // Nominal saturation voltage, V (13.85)
#define IMAX_NUM        100000.         // Simulation limit to prevent NaN, A (1e5)
#define WRAP_SOC_HI_OFF     0.97        // Disable e_wrap_hi when saturated (0.97)
#define WRAP_SOC_HI_SLR     1000.       // Huge to disable e_wrap (1000)
#define WRAP_SOC_LO_OFF_ABS 0.35        // Disable e_wrap when near empty (soc lo any Tb, 0.35)
#define WRAP_SOC_LO_OFF_REL 0.2         // Disable e_wrap when near empty (soc lo for high Tb where soc_min=.2, voltage cutback, 0.2)
#define WRAP_SOC_LO_SLR     60.         // Large to disable e_wrap (60. for startup)
#define WRAP_MOD_C_RATE     .02         // Moderate charge rate threshold to engage wrap threshold (0.02 to prevent trip near saturation .05 too large)
#define WRAP_SOC_MOD_OFF    0.85        // Disable e_wrap_lo when nearing saturated and moderate C_rate (0.85)
#define VC_S                1.0         // Vc sense scalar (1.0)
#define VO_S                1.0         // Vo sense scalar (1.0)
#define AMP_FILT_TAU        4.0         // Ib filters time constant for calibration only, s (4.0)
#define VC_BARE_DETECTED    0.16        // Level of common voltage to declare circuit unconnected, V (0.16)
#define V3V3                3.3         // Theoretical nominal V3v3, V (3.3)
#define HALF_V3V3         (V3V3/2.)     // Theoretical center of differential TSC2010

// Conversion gains
#if defined(HDWE_ADS1013_AMP_NOA)
    const float SHUNT_AMP_GAIN = SHUNT_GAIN * SHUNT_AMP_R1 / SHUNT_AMP_R2;
    const float SHUNT_NOA_GAIN = SHUNT_GAIN;
#elif defined(CONFIG_TSC2010_OPAMP)
    const float SHUNT_AMP_GAIN = SHUNT_GAIN * SHUNT_AMP_R1 / SHUNT_AMP_R2;
    const float SHUNT_NOA_GAIN = SHUNT_GAIN * SHUNT_AMP_R1 / SHUNT_AMP_R2;
#elif defined(HDWE_INA181_HI_LO)
    const float SHUNT_AMP_GAIN = SHUNT_GAIN * SHUNT_AMP_R1 / SHUNT_AMP_R2;
    const float SHUNT_NOA_GAIN = SHUNT_GAIN * SHUNT_NOA_R1 / SHUNT_NOA_R2;
#endif


// Voltage measurement gains
const float VB_CONV_GAIN = float(PHOTON_ADC_VOLT) * float(VB_SENSE_R_HI + VB_SENSE_R_LO) /
                              float(VB_SENSE_R_LO) / float(PHOTON_ADC_COUNT) * float(VB_S);
const float VC_CONV_GAIN = float(PHOTON_ADC_VOLT) / float(PHOTON_ADC_COUNT) * float(VC_S);
const float VO_CONV_GAIN = float(PHOTON_ADC_VOLT) / float(PHOTON_ADC_COUNT) * float(VO_S);
const float VH3V3_CONV_GAIN = float(PHOTON_ADC_VOLT) / float(PHOTON_ADC_COUNT);

#endif // CONSTANTS_H_
