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
#include "application.h"
#include "recall_X.h"
#include "../mySubs.h"
#include "../command.h"
#include "../local_config.h"
#include "../mySummary.h"
#include "../parameters.h"
#include <math.h>
#include "../debug.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

boolean recall_X(const char letter_1, BatteryMonitor *Mon, Sensors *Sen)
{
    boolean found = true;
    int INT_in;
    String murmur;
    switch ( letter_1 )
    {

        case ( 'p' ): // Xp<>:  injection program
            INT_in = cp.input_str.substring(2).toInt();
            switch ( INT_in )
            {

                case ( -1 ):  // Xp-1:  full reset
                    chit("Xp0;", ASAP);
                    chit("Ca.5;", SOON);
                    chit("Xm0;", SOON);
                    break;

                case ( 0 ):  // Xp0:  reset stop
                    Serial.printf("**************Xp0\n");
                    if ( !sp.tweak_test() ) chit("Xm247;", ASAP);  // Prevent upset of time in Xp7, Xp9, Xp10, Xp11, etc
                    chit("Xf0;Xa0;Xtn;", ASAP);
                    if ( !sp.tweak_test() ) chit("Xb0;", ASAP);
                    chit("BZ;", SOON);
                    break;

                #ifndef CONFIG_PHOTON
                case ( 2 ):  // Xp2:  
                    chit("Xp0;", QUEUE);
                    chit("Xtc;", QUEUE);
                    chit("DI-40;", QUEUE);
                    chit("Rs", QUEUE);
                    break;

                case ( 3 ):  // Xp3:  
                    chit("Xp0;", QUEUE);
                    chit("Xtc;", QUEUE);
                    chit("DI40;", QUEUE);
                    chit("Rs", QUEUE);
                    break;

                case ( 4 ):  // Xp4:  
                    chit("Xp0;", QUEUE);
                    chit("Xtc;", QUEUE);
                    chit("DI-100;", QUEUE);
                    chit("Rs", QUEUE);
                    break;

                case ( 5 ):  // Xp5:  
                    chit("Xp0;", QUEUE);
                    chit("Xtc;", QUEUE);
                    chit("DI100;", QUEUE);
                    chit("Rs", QUEUE);
                    break;

                #endif

                case ( 6 ):  // Xp6:  Program a pulse for EKF test
                    chit("XS;Dm0;Dn0;vv0;Xm255;Ca.5;Pm;Dr100;DP20;vv4;", QUEUE);  // setup
                    chit("Dn.00001;Dm500;Dm-500;Dm0;", QUEUE);  // run
                    chit("W10;Pm;vv0;", QUEUE);  // finish
                    break;

                case ( 7 ):  // Xp7:  Program a sensor pulse for State Space test
                    chit("XS;Dm0;Dn0;vv0;Xm255;Ca.5;Pm;Dr100;DP1;D>100;vv2;", QUEUE);  // setup
                    chit("Dn.00001;Dm500;Dm-500;Dm0;", QUEUE);  // run
                    murmur = "D>" + String(TALK_DELAY) + ";W10;Pm;vv0;Dr" + String(READ_DELAY) + ";Dh" + String(SUMMARY_DELAY) + ";";
                    chit(murmur, QUEUE);  // finish
                    break;

                case ( 8 ):  // Xp8:  Program a hardware pulse for State Space test
                    chit("XS;Di0;vv0;Xm255;Ca.5;Pm;Dr100;DP1;vv2;", QUEUE);  // setup
                    chit("DI500;DI-500;DI0;", QUEUE);  // run
                    chit("W10;Pm;vv0;", QUEUE);  // finish
                    break;

                case ( 9 ): case( 10 ): case ( 11 ): case( 12 ): case( 13 ): // Xp9: Xp10: Xp11: Xp12: Xp13:  Program regression
                    // Regression tests 9=tweak, 10=tweak w data, 11=cycle, 12 1/2 cycle
                    chit("Xp0;", QUEUE);      // Reset nominal
                    chit("vv0;", QUEUE);       // Turn off debug temporarily so not snowed by data dumps
                    chit("Xm255;", QUEUE);    // Modeling (for totally digital test of logic) and tweak_test=true to disable cutback in Sim.  Leaving cutback on would mean long run times (~30:00) (May need a way to test features affected by cutback, such as tweak, saturation logic)
                    chit("Xts;", QUEUE);      // Start up a sine wave
                    chit("Ca1;", QUEUE);      // After restarting with sine running, soc will not be at 1.  Reset them all to 1
                    chit("Dm1;Dn1;", ASAP);   // Slight positive current so sat logic is functional.  ASAP so synchronized and ib_diff flat.
                    chit("DP1;", QUEUE);      // Fast data collection (may cause trouble in CoolTerm.  if so, try Dr200)
                    chit("Rb;", QUEUE);       // Reset battery states
                    // chit("Pa;", QUEUE);       // Print all for record
                    if ( INT_in == 10 )  // Xp10:  rapid tweak
                    {
                        chit("Xf.02;", QUEUE);  // Frequency 0.02 Hz
                        chit("Xa-2000;", QUEUE);// Amplitude -2000 A
                        chit("XW5000;", QUEUE);    // Wait time before starting to cycle
                        chit("XT5000;", QUEUE);    // Wait time after cycle to print
                        chit("XC3;", QUEUE);    // Number of injection cycles
                        chit("W2;", QUEUE);     // Wait
                        chit("vv4;", QUEUE);     // Data collection
                    }
                    else if ( INT_in == 11 )  // Xp11:  slow tweak
                    {
                        chit("Xf.002;", QUEUE); // Frequency 0.002 Hz
                        chit("Xa-60;", QUEUE);  // Amplitude -60 A
                        chit("XW60000;", QUEUE);   // Wait time before starting to cycle
                        chit("XT60000;", QUEUE);  // Wait time after cycle to print
                        chit("XC1;", QUEUE);    // Number of injection cycles
                        chit("W2;", QUEUE);     // Wait
                        chit("vv2;", QUEUE);     // Data collection
                    }
                    else if ( INT_in == 12 )  // Xp12:  slow half tweak
                    {
                        chit("Xf.0002;", QUEUE);  // Frequency 0.002 Hz
                        chit("Xa-6;", QUEUE);     // Amplitude -60 A
                        chit("XW60000;", QUEUE);     // Wait time before starting to cycle
                        chit("XT240000;", QUEUE);   // Wait time after cycle to print
                        chit("XC.5;", QUEUE);     // Number of injection cycles
                        chit("W2;", QUEUE);       // Wait
                        chit("vv2;", QUEUE);       // Data collection
                    }
                    else if ( INT_in == 13 )  // Xp13:  tri tweak
                    {
                        chit("Xtt;", QUEUE);      // Start up a triangle wave
                        chit("Xf.02;", QUEUE);  // Frequency 0.02 Hz
                        chit("Xa-29500;", QUEUE);// Amplitude -2000 A
                        chit("XW5000;", QUEUE);    // Wait time before starting to cycle
                        chit("XT5000;", QUEUE);    // Wait time after cycle to print
                        chit("XC3;", QUEUE);    // Number of injection cycles
                        chit("W2;", QUEUE);     // Wait
                        chit("vv2;", QUEUE);     // Data collection
                    }
                    chit("W2;", QUEUE);       // Wait
                    chit("XR;", QUEUE);       // Run cycle
                    break;

                case( 20 ): case ( 21 ):    // Xp20:  Xp21:  20= 0.5 s sample/2.0s print, 21= 2 s sample/8 s print
                    chit("vv0;", QUEUE);       // Turn off debug temporarily so not snowed by data dumps
                    chit("Pa;", QUEUE);       // Print all for record
                    if ( INT_in == 20 )
                    {
                        chit("Dr500;", QUEUE);  // 5x sample time, > ChargeTransfer_T_MAX.  ChargeTransfer dynamics disabled in Photon
                        chit("DP4;", QUEUE);    // 4x data collection, > ChargeTransfer_T_MAX.  ChargeTransfer dynamics disabled in Python
                        chit("vv2;", QUEUE);     // Large data set
                    }
                    else if ( INT_in == 21 )
                    {
                        chit("DP20;", QUEUE);   // 20x data collection
                        chit("vv2;", QUEUE);     // Slow data collection
                    }
                    chit("Rb;", QUEUE);       // Large data set
                    break;

                default:
                    Serial.printf("Xp=%d unk.  see 'h'\n", INT_in);
            }
            break;

        case ( 'R' ): // XR:  Start injection now
            if ( Sen->now>TEMP_INIT_DELAY )
            {
                Sen->start_inj = ap.wait_inj + Sen->now;
                Sen->stop_inj = ap.wait_inj + (Sen->now + min((unsigned long int)(ap.cycles_inj / max(sp.Freq()/(2.*PI), 1e-6) *1000.), ULLONG_MAX));
                Sen->end_inj = Sen->stop_inj + ap.tail_inj;
                Serial.printf("**\n*** RUN: at %ld, %7.3f cycles %ld to %ld with %ld wait and %ld tail\n\n",
                Sen->now, ap.cycles_inj, Sen->start_inj, Sen->stop_inj, ap.wait_inj, ap.tail_inj);
            }
            else Serial.printf("Wait%5.1fs for init\n", float(TEMP_INIT_DELAY-Sen->now)/1000.);
            break;

        case ( 'S' ): // XS:  Stop injection now
            Serial.printf("STOP\n");
            Sen->start_inj = 0UL;
            Sen->stop_inj = 0UL;
            Sen->end_inj = 0UL;
            Sen->elapsed_inj = 0UL;
            chit("vv0;", ASAP);     // Turn off echo
            chit("Xm247;", SOON);  // Turn off tweak_test
            chit("Xp0;", SOON);    // Reset
            break;

        case ( 't' ): //*  Xt<>:  injection type
            switch ( cp.input_str.charAt(2) )
            {
                case ( 'n' ):  // Xtn:  none
                    sp.put_Type(0);
                    Serial.printf("Set none. sp.type() %d\n", sp.type());
                    break;

                case ( 's' ):  // Xts:  sine
                    sp.put_Type(1);
                    Serial.printf("Set sin. sp.type() %d\n", sp.type());
                    break;

                case ( 'q' ):  // Xtq:  square
                    sp.put_Type(2);
                    Serial.printf("Set square. sp.type() %d\n", sp.type());
                    break;

                case ( 't' ):  // Xtt:  triangle
                    sp.put_Type(3);
                    Serial.printf("Set tri. sp.type() %d\n", sp.type());
                    break;

                case ( 'c' ):  // Xtc:  charge rate
                    sp.put_Type(4);
                    Serial.printf("Set 1C charge. sp.type() %d\n", sp.type());
                    break;

                case ( 'd' ):  // Xtd:  discharge rate
                    sp.put_Type(5);
                    Serial.printf("Set 1C disch. sp.type() %d\n", sp.type());
                    break;

                case ( 'o' ):  // Xto:  cosine
                    sp.put_Type(8);
                    Serial.printf("Set cos. sp.type() %d\n", sp.type());
                    break;

                default:
                    if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());
            }
            break;

        default:
            found = ap.find_adjust(cp.input_str) || sp.find_adjust(cp.input_str);
            if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());
    }
    return found;
}