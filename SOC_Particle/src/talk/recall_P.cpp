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
#include "recall_P.h"
#include "../subs.h"
#include "../command.h"
#include "../constants.h"
#include "../Summary.h"
#include "../parameters.h"
#include <math.h>
#include "../debug.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

boolean recall_P(const char letter_1, BatteryMonitor *Mon, Sensors *Sen)
{
    boolean found = true;
    switch ( letter_1 )
    {
        case ( 'a' ):  // Pa:  Print all
            chit("Pm;Ps;Pr;PM;PN;Hd;Pf;Q;", SOON);
            break;

        case ( 'b' ):  // Pb:  Print Vb measure
            Serial.printf("\nVolt:");   Serial.printf("Vb_bias_hdwe,Vb_m,mod,Vb=,%7.3f,%7.3f,%d,%7.3f,\n", 
                sp.Vb_bias_hdwe(), Sen->Vb_model, sp.modeling(), Sen->Vb);
            break;

        case ( 'e' ):  // Pe:  Print EKF
            Serial.printf ("\nMon::"); Mon->EKF_1x1::pretty_print();
            Serial1.printf("\nMon::"); Mon->EKF_1x1::pretty_print();
            break;

        case ( 'f' ):  // Pf:  Print faults
            // sp.print_history_array();
            // sp.print_fault_header(&pp.pubList);
            sp.print_fault_array();
            sp.print_fault_header(&pp.pubList);
            Serial.printf ("\nSen::\n");
            Sen->Flt->pretty_print (Sen, Mon);
            Serial1.printf("\nSen::\n");
            Sen->Flt->pretty_print1(Sen, Mon);
            break;

        case ( 'm' ):  // Pm:  Print mon
            Serial.printf ("\nM:"); Mon->pretty_print(Sen);
            Serial.printf ("M::"); Mon->EKF_1x1::pretty_print();
            Serial.printf ("\nmodeling %d\n", sp.modeling());
            break;

        case ( 'M' ):  // PM:  Print shunt Amp
            Serial.printf ("\n"); Sen->ShuntAmp->pretty_print();
            break;

        case ( 'N' ):  // PN:  Print shunt no amp
            Serial.printf ("\n"); Sen->ShuntNoAmp->pretty_print();
            break;

        case ( 'R' ):  // PR:  Print retained
            Serial.printf("\n"); sp.pretty_print( true );
            Serial.printf("\n"); sp.pretty_print( false );
            break;

        case ( 'r' ):  // Pr:  Print only off-nominal retained
            Serial.printf("\n"); sp.pretty_print( false );
            break;

        case ( 's' ):  // Ps:  Print sim
            Serial.printf("\nmodeling=%d\n", sp.modeling());
            Serial.printf("S:");  Sen->Sim->pretty_print();
            // Serial.printf("S::"); Sen->Sim->Coulombs::pretty_print();
            break;

        case ( 'V' ):  // PV:  Print all volatile
            Serial.printf("\n"); ap.pretty_print(true);
            Serial.printf("\n"); cp.pretty_print();
            Serial.printf("\n"); ap.pretty_print(false);
            break;

        case ( 'v' ):  // Pv:  Print only off-nominal volatile
            Serial.printf("\n"); ap.pretty_print(false);
            break;

        case ( 'x' ):  // Px:  Print shunt measure
            Serial.printf("\nAmp: "); Serial.printf("Vshunt_int,Vshunt,Vc,Vo,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
                Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), Sen->ShuntAmp->Vc(), Sen->ShuntAmp->Vo(), Sen->ShuntAmp->Ishunt_cal());
            Serial.printf("Noa:"); Serial.printf("Vshunt_int,Vshunt,Vc,Vo,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
                Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), Sen->ShuntNoAmp->Vc(), Sen->ShuntNoAmp->Vo(), Sen->ShuntNoAmp->Ishunt_cal());
            Serial.printf("I_f:Noa,Ib=,%d,%7.3f\n", sp.ib_force(), Sen->Ib);
            break;

        default:
            found = ap.find_adjust(cp.cmd_str) || sp.find_adjust(cp.cmd_str);
            if (!found) Serial.printf("%s NOT FOUND\n", cp.cmd_str.substring(0,2).c_str());
    }
    return found;
}
