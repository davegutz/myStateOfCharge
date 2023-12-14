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
#include "recall_R.h"
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

boolean recall_R(const char letter, BatteryMonitor *Mon, Sensors *Sen)
{
    boolean found = true;
    switch ( letter )
    {
        case ( 'b' ):  // Rb:  Reset battery states (also hys)
            Sen->Sim->init_battery_sim(true, Sen);  // Reset sim battery state
            Mon->init_battery_mon(true, Sen);       // Reset mon battery state
            break;

        case ( 'f' ):  // Rf:  Reset fault Rf
            Serial.printf("Reset latches\n");
            Sen->Flt->reset_all_faults(true);
            break;

        case ( 'i' ):  // Ri:  Reset infinite counter
            Serial.printf("Reset infinite counter\n");
            cp.inf_reset = true;
            break;

        case ( 'r' ):  // Rr:  small reset counters
            Serial.printf("CC reset\n");
            Sen->Sim->apply_soc(1.0, Sen->Tb_filt);
            Mon->apply_soc(1.0, Sen->Tb_filt);
            cp.cmd_reset();
            break;

        case ( 'R' ):  // RR:  large reset
            Serial.printf("RESET\n");
            Serial1.printf("RESET\n");
            Sen->Sim->apply_soc(1.0, Sen->Tb_filt);
            Mon->apply_soc(1.0, Sen->Tb_filt);
            cp.cmd_reset();
            Sen->ReadSensors->delay(READ_DELAY);
            Sen->Talk->delay(TALK_DELAY);
            sp.large_reset();
            sp.large_reset();
            cp.large_reset();
            cp.cmd_reset();
            chit("HR;", SOON);
            chit("Rf;", SOON);
            chit("Hs;", SOON);
            chit("Pf;", SOON);
            break;

        case ( 's' ):  // Rs:  small reset filters
            Serial.printf("reset\n");
            cp.cmd_reset();
            break;

        case ( 'S' ):  // RS: renominalize saved pars
            sp.set_nominal();
            sp.pretty_print(true);
            break;

        case ( 'V' ):  // RV: renominalize volatile pars
            ap.set_nominal();
            ap.pretty_print(true);
            break;

        default:
            found = ap.find_adjust(cp.input_str) || sp.find_adjust(cp.input_str);
            if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());
    }
    return found;
}
