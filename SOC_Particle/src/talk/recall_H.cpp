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
#include "recall_H.h"
#include "../command.h"
#include "../mySummary.h"
#include "../parameters.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

boolean recall_H(const char letter_1, BatteryMonitor *Mon, Sensors *Sen)
{
    boolean found = true;
    switch ( letter_1 )
    {
    case ( 'd' ):  // Hd: History dump
        Serial.printf("\n");
        print_all_fault_buffer("unit_h", mySum, sp.Isum(), sp.nsum());
        sp.print_fault_header();
        chit("Pr;Q;", QUEUE);
        Serial.printf("\n");
        // sp.print_history_array();
        // sp.print_fault_header();
        // sp.print_fault_array();
        // sp.print_fault_header();
        break;

    case ( 'f' ):  // Hf: History dump faults only
        Serial.printf("\n");
        sp.print_fault_array();
        sp.print_fault_header();
        break;

    case ( 'R' ):  // HR: History reset
        Serial.printf("Reset sum, his, flt...");
        reset_all_fault_buffer("unit_h", mySum, sp.Isum(), sp.nsum());
        sp.reset_his();
        sp.reset_flt();
        Serial.printf("done\n");
        break;

    case ( 's' ):  // Hs: History snapshot
        cp.cmd_summarize();
        break;

    default:
        found = ap.find_adjust(cp.input_str) || sp.find_adjust(cp.input_str);
        if (!found) Serial.printf("%s NOT FOUND\n", cp.input_str.substring(0,2).c_str());
    }
    return found;
}
