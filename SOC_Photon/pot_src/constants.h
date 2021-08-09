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

//#define NO_CLOUD              // Turn off Particle cloud functions.  Interact using Blynk.
//#define BARE                  // Run without peripherals
const int8_t debug = 3;         // Level of debug printing (3)
#define TA_SENSOR 0x27          // Ambient room Honeywell temp sensor bus address (0x27)
#define TP_TEMPCAL 0            // Maxim 1-wire plenum temp sense calibrate (0), F
#define TA_TEMPCAL 0            // Honeywell calibrate temp sense (0), F
#define HW_HUMCAL -2            // Honeywell calibrate humidity sense (-2), %
#define ONE_DAY_MILLIS 86400000 // Number of milliseconds in one day (24*60*60*1000)
#define NOMSET 68               // Nominal setpoint for modeling etc, F
#define MINSET 50               // Minimum setpoint allowed (50), F
#define MAXSET 72               // Maximum setpoint allowed (72), F
#define CONTROL_DELAY    5000UL     // Control law wait, ms
#define MODEL_DELAY      5000UL     // Model wait, ms
#define PUBLISH_DELAY    30000UL    // Time between cloud updates (10000), ms
#define READ_DELAY       5000UL     // Sensor read wait (5000, 100 for stress test), ms
#define QUERY_DELAY      15000UL    // Web query wait (15000, 100 for stress test), ms
#define DISPLAY_DELAY    300UL      // LED display scheduling frame time, ms
#define FILTER_DELAY     5000UL             // In range of tau/4 - tau/3  * 1000, ms

#ifdef BARE
const boolean bare = true;  // Force continuous calibration mode to run with bare boards (false)
#else
const boolean bare = false;  // Force continuous calibration mode to run with bare boards (false)
#endif

#endif // CONSTANTS_H_
