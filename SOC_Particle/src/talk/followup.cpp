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
#include "../mySubs.h"
#include "../command.h"
#include "../parameters.h"
#include "../debug.h"

extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
extern CommandPars cp;  // Various parameters shared at system level
extern Flt_st mySum[NSUM];  // Summaries for saving charge history

boolean followup(const char letter_0, const char letter_1, BatteryMonitor *Mon, Sensors *Sen, uint16_t modeling_past)
{
    boolean reset = false;
    boolean found = true;
    switch ( letter_0 )
    {

        case ( 'B' ):
            switch ( letter_1 )
 
            {

                case ( 'm' ):  //* Bm
                if ( sp.Mon_chm_p->success() )
                switch ( sp.Mon_chm_z )
                {
                    case ( 0 ):  // Bm0: Mon Battleborn
                        Mon->assign_all_mod("Battleborn");
                        Mon->chem_pretty_print();
                        cp.cmd_reset();
                        break;

                    case ( 1 ):  // Bm1: Mon CHINS
                        Mon->assign_all_mod("CHINS");
                        Mon->chem_pretty_print();
                        cp.cmd_reset();
                        break;

                    case ( 2 ):  // Bm2: Mon Spare
                        Mon->assign_all_mod("Spare");
                        Mon->chem_pretty_print();
                        cp.cmd_reset();
                        break;

                    }
                    break;

                case ( 's' ):  //* Bs:  Simulation chemistry change
                    if ( sp.Sim_chm_p->success() )
                    switch ( sp.Sim_chm_z )
                    {
                        case ( 0 ):  // Bs0: Sim Battleborn
                            Sen->Sim->assign_all_mod("Battleborn");
                            cp.cmd_reset();
                            break;

                        case ( 1 ):  // Bs1: Sim CHINS
                            Sen->Sim->assign_all_mod("CHINS");
                            cp.cmd_reset();
                            break;

                        case ( 2 ):  // Bs2: Sim Spare
                            Sen->Sim->assign_all_mod("Spare");
                            cp.cmd_reset();
                            break;

                }
                break;
            }
            break;

        case ( 'C' ):  // C:
            switch ( letter_1 )
            {

                case ( 'a' ):  // Ca<>:  assign charge state in fraction to all versions including model
                    if ( ap.init_all_soc_p->success() )
                    {
                        initialize_all(Mon, Sen, ap.init_all_soc, true);
                        #ifdef DEBUG_INIT
                        if ( sp.Debug()==-1 ){ Serial.printf("after initialize_all:"); debug_m1(Mon, Sen);}
                        #endif
                        if ( sp.Modeling() )
                        {
                        cp.cmd_reset();
                        chit("W3;", SOON);  // Wait 10 passes of Control
                        }
                        else
                        {
                        cp.cmd_reset();
                        chit("W3;", SOON);  // Wait 10 passes of Control
                        }
                    }
                    else
                        Serial.printf("skipping %s\n", cp.input_str.c_str());
                    break;

                case ( 'm' ):  // Cm<>:  assign curve charge state in fraction to model only (ekf if modeling)
                    if ( ap.init_sim_soc_p->success() )  // Apply crude limit to prevent user error
                    {
                        Sen->Sim->apply_soc(ap.init_sim_soc, Sen->Tb_filt);
                        Serial.printf("soc%8.4f, dq%7.3f, soc_mod%8.4f, dq mod%7.3f,\n",
                            Mon->soc(), Mon->delta_q(), Sen->Sim->soc(), Sen->Sim->delta_q());
                        if ( sp.Modeling() ) cp.cmd_reset_sim(); // Does not block.  Commands a reset
                    }
                    else
                        Serial.printf("soc%8.4f; must be 0-1.1\n", ap.init_sim_soc);
                    break;

            }
            break;

        case ( 'D' ):
            switch ( letter_1 )
            {

                case ( 'h' ):  //   Dh<>:  READ sample time input
                    if ( ap.his_delay_p->success() )
                        Sen->Summarize->delay(max(ap.read_delay, ap.his_delay));  // validated
                    break;

                case ( 'r' ):  //   Dr<>:  READ sample time input
                    if ( ap.read_delay_p->success() )
                    {
                        Sen->ReadSensors->delay(ap.read_delay);  // validated
                        Sen->Summarize->delay(max(ap.read_delay, ap.his_delay));  // validated
                    }
                    break;

                case ( 't' ):  //*  Dt<>:  Temp bias change hardware
                    if ( sp.Tb_bias_hdwe_p->success() )
                        cp.cmd_reset();
                    break;

                case ( 'v' ):  //     Dv<>:  voltage signal adder for faults
                    if ( ap.vb_add_p->success() )
                        ap.vb_add_p->print1();
                    break;

                case ( '>' ):  //   D><>:  TALK sample time input
                    if ( ap.talk_delay_p->success() )
                        Sen->Talk->delay(ap.talk_delay);  // validated
                    break;

            }
            break;

        case ( 'S' ):
            switch ( letter_1 )
            {

                case ( 'H' ):  //   SH<>: state of all hysteresis
                    if ( ap.hys_state_p->success() )
                    {
                        Sen->Sim->hys_state(ap.hys_state);
                        Sen->Flt->wrap_err_filt_state(-ap.hys_state);
                    }
                    break;

                case ( 'q' ):  //*  Sq<>: scale capacity sim
                    if ( sp.S_cap_sim_p->success() )
                    {
                        Sen->Sim->apply_cap_scale(sp.S_cap_sim());
                        if ( sp.Modeling() ) Mon->init_soc_ekf(Sen->Sim->soc());
                    }
                    break;
            
                case ( 'Q' ):  //*  SQ<>: scale capacity mon
                    if ( sp.S_cap_mon_p->success() )
                    {
                        Mon->apply_cap_scale(sp.S_cap_mon());
                    }
                    break;
            
            }
            break;

        case ( 'F' ):  // Fault stuff
            switch ( letter_1 )
            {

                case ( 'f' ):  //* si, Ff<>:  fake faults
                if ( ap.fake_faults_p->success() )
                {
                    Sen->Flt->reset_all_faults();
                    sp.put_Ib_select(cp.input_str.substring(2).toInt());
                }
                break;

            }
            break;


        case ( 'l' ):
            if ( sp.Debug_p->success() )
            switch ( sp.Debug() )
            {
                case ( -1 ):  // l-1:
                // Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt_amp  ,Ishunt_noa  ,Vb_fo*10-110  ,voc_s*10-110  ,dv_dyn_s*10  ,v_s*10-110  , voc_dyn*10-110,,,,,,,,,,,\n");
                break;
                case ( 1 ):  // l1:
                print_serial_header();
                break;
                case ( 2 ):  // l2:
                print_signal_sel_header();
                print_serial_sim_header();
                print_serial_header();
                break;
                case ( 3 ):  // l3:
                print_serial_ekf_header();
                print_serial_sim_header();
                print_serial_header();
                break;
                default:
                print_serial_header();
            }
            break;

        case ( 'U' ):
            if ( sp.Time_now_p->success() )
            switch ( letter_1 )
            {

                case ( 'T' ):  //*  UT<>:  Unix time since epoch
                Time.setTime(sp.Time_now_z);
                break;

            }
            break;

        case ( 'X' ):  // X
            switch ( letter_1 )
            {

                case ( 'm' ):  // Xm<>:  code for modeling level
                    if ( sp.Modeling_p->success() )
                    {
                        reset = sp.Modeling() != modeling_past;
                        if ( reset )
                        {
                        Serial.printf("Chg...reset\n");
                        cp.cmd_reset();
                        }
                    }
                    break;

                case ( 'a' ): // Xa<>:  injection amplitude
                    if ( sp.Amp_p->success() )
                    {
                        sp.Amp_z *= sp.nP();
                        Serial.printf("Inj amp, %s, %s set%7.3f & inj_bias set%7.3f\n", sp.Amp_p->units(), sp.Amp_p->description(), sp.Amp(), sp.Inj_bias());
                    }
                    break;

                case ( 'f' ): //*  Xf<>:  injection frequency
                    if ( sp.Freq_p->success() ) sp.Freq_z = sp.Freq_z*(2. * PI);
                    break;

                case ( 'b' ): //*  Xb<>:  injection bias
                    Serial.printf("Inj amp, %s, %s set%7.3f & inj_bias set%7.3f\n", sp.Amp_p->units(), sp.Amp_p->description(), sp.Amp(), sp.Inj_bias());
                    break;

                case ( 'Q' ): //  XQ<>: time to quiet
                    Serial.printf("Going black in %7.1f seconds\n", float(ap.until_q) / 1000.);
                    break;
            }
            break;

            default:
                // Serial.print( letter_0 ); Serial.print(" ? 'h'\n");
                break;
    }
    return found;
}
