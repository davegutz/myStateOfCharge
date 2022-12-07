//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "Battery.h"
#include "parameters.h"

// class SavedPars 
SavedPars::SavedPars() {}
SavedPars::SavedPars(SerialRAM *ram): rP_(ram)
{
    modeling_.a16 = 0x000;
}
SavedPars::~SavedPars() {}
// operators
// functions

// Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
// battery.  Small compilation changes can change where in this memory the program points, too.
boolean SavedPars::is_corrupt()
{
    return ( this->modeling()<0 || this->modeling()>15 );
    // return ( this->nP==0 || this->nS==0 || this->mon_chm>10 || isnan(this->amp) || this->freq>2. ||
    //  abs(this->ib_bias_amp)>500. || abs(this->cutback_gain_scalar)>1000. || abs(this->ib_bias_noa)>500. ||
    //  this->t_last_model<-10. || this->t_last_model>70. );
}

// Nominalize
void SavedPars::nominal()
{
    // this->debug = 0;
    // this->delta_q = 0.;
    // this->t_last = RATED_TEMP;
    // this->delta_q_model = 0.;
    // this->t_last_model = RATED_TEMP;
    // this->shunt_gain_sclr = 1.;
    // this->Ib_scale_amp = CURR_SCALE_AMP;
    // this->ib_bias_amp = CURR_BIAS_AMP;
    // this->Ib_scale_noa = CURR_SCALE_NOA;
    // this->ib_bias_noa = CURR_BIAS_NOA;
    // this->ib_bias_all = CURR_BIAS_ALL;
    // this->ib_select = FAKE_FAULTS;
    // this->Vb_bias_hdwe = VOLT_BIAS;
    modeling(uint8_t(MODELING));
    // this->amp = 0.;
    // this->freq = 0.;
    // this->type = 0;
    // this->inj_bias = 0.;
    // this->Tb_bias_hdwe = TEMP_BIAS;
    // this->s_cap_model = 1.;
    // this->cutback_gain_scalar = 1.;
    // this->isum = -1;    this->hys_scale = HYS_SCALE;
    // this->nP = NP;
    // this->nS = NS;
    // this->mon_chm = MON_CHEM;
    // this->sim_chm = SIM_CHEM;
    // this->Vb_scale = VB_SCALE;
}

// Number of differences between SRAM and actual
int SavedPars::num_diffs()
{
    int n = 0;

    // if ( RATED_TEMP != t_last )
    //   n++;
    // if ( RATED_TEMP != t_last_model )
    //   n++;
    // if ( 0. != delta_q )
    //   n++;
    // if ( 0. != delta_q_model )
    //   n++;


    // if ( 1. != shunt_gain_sclr )
    //   n++;
    // if ( 0 != debug )
    //   n++;
    // if ( float(CURR_SCALE_AMP) != Ib_scale_amp )
    //   n++;
    // if ( float(CURR_BIAS_AMP) != ib_bias_amp )
    //   n++;
    // if ( float(CURR_SCALE_NOA) != Ib_scale_noa )
    //   n++;
    // if ( float(CURR_BIAS_NOA) != ib_bias_noa )
    //   n++;
    // if ( float(CURR_BIAS_ALL) != ib_bias_all )
    //   n++;
    // if ( FAKE_FAULTS != ib_select )
    //   n++;
    // if ( float(VOLT_BIAS) != Vb_bias_hdwe )
    //   n++;
    if ( MODELING != modeling() )
        n++;
    // if ( 0. != amp )
    //   n++;
    // if ( 0. != freq )
    //   n++;
    // if ( 0 != type )
    //   n++;
    // if ( 0. != inj_bias )
    //   n++;
    // if ( float(TEMP_BIAS) != Tb_bias_hdwe )
    //   n++;
    // if ( 1. != s_cap_model )
    //   n++;
    // if ( 1. != cutback_gain_scalar )
    //   n++;
    // if ( float(HYS_SCALE) != hys_scale )
    //   n++;
    // if ( float(NP) != nP )
    //   n++;
    // if ( float(NS) != nS )
    //   n++;
    // if ( MON_CHEM != mon_chm )
    //   n++;
    // if ( SIM_CHEM != sim_chm )
    //   n++;
    // if ( float(VB_SCALE) != Vb_scale )
    //   n++;

    return ( n );
}

// Configuration functions

// Print
void SavedPars::pretty_print(const boolean all )
{
    Serial.printf("\nretained parameters (rp):\n");
    Serial.printf("             defaults    current SRAM values\n");
    if ( all )
    {
        //   Serial.printf(" isum                           %d tbl ptr\n", isum);
        //   Serial.printf(" t_last          %5.2f      %5.2f dg C\n", RATED_TEMP, t_last);
        //   Serial.printf(" t_last_sim      %5.2f      %5.2f dg C\n", RATED_TEMP, t_last_model);
        //   Serial.printf(" delta_q    %10.1f %10.1f *Ca<>, C\n", 0., delta_q);
        //   Serial.printf(" dq_sim     %10.1f %10.1f *Ca<>, *Cm<>, C\n", 0., delta_q_model);
        // }
        // if ( all || 1. != shunt_gain_sclr )
        //   Serial.printf(" shunt_gn_slr  %7.3f    %7.3f ?\n", 1., shunt_gain_sclr);  // TODO:  no talk value
        // if ( all || 0 != debug )
        //   Serial.printf(" debug               %d          %d *v<>\n", 0, debug);
        // if ( all || CURR_SCALE_AMP != Ib_scale_amp )
        //   Serial.printf(" scale_amp     %7.3f    %7.3f *SA<>\n", CURR_SCALE_AMP, Ib_scale_amp);
        // if ( all || float(CURR_BIAS_AMP) != ib_bias_amp )
        //   Serial.printf(" bias_amp      %7.3f    %7.3f *DA<>\n", CURR_BIAS_AMP, ib_bias_amp);
        // if ( all || float(CURR_SCALE_NOA) != Ib_scale_noa )
        //   Serial.printf(" scale_noa     %7.3f    %7.3f *SB<>\n", CURR_SCALE_NOA, Ib_scale_noa);
        // if ( all || float(CURR_BIAS_NOA) != ib_bias_noa )
        //   Serial.printf(" bias_noa      %7.3f    %7.3f *DB<>\n", CURR_BIAS_NOA, ib_bias_noa);
        // if ( all || float(CURR_BIAS_ALL) != ib_bias_all )
        //   Serial.printf(" ib_bias_all   %7.3f    %7.3f *Di<> A\n", CURR_BIAS_ALL, ib_bias_all);
        // if ( all || FAKE_FAULTS != ib_select )
        //   Serial.printf(" ib_select           %d          %d *s<> -1=noa, 0=auto, 1=amp\n", FAKE_FAULTS, ib_select);
        // if ( all || float(VOLT_BIAS) != Vb_bias_hdwe )
        //   Serial.printf(" Vb_bias_hdwe       %7.3f    %7.3f *Dv<>,*Dc<> V\n", VOLT_BIAS, Vb_bias_hdwe);
        if ( all || MODELING != modeling() )
            Serial.printf(" modeling            %d          %d *Xm<>\n", MODELING, modeling());
        // if ( all || 0. != amp )
        //   Serial.printf(" inj amp       %7.3f    %7.3f *Xa<> A pk\n", 0., amp);
        // if ( all || 0. != freq )
        //   Serial.printf(" inj frq       %7.3f    %7.3f *Xf<> r/s\n", 0., freq);
        // if ( all || 0 != type )
        //   Serial.printf(" inj typ             %d          %d *Xt<> 1=sin, 2=sq, 3=tri\n", 0, type);
        // if ( all || 0. != inj_bias )
        //   Serial.printf(" inj_bias      %7.3f    %7.3f *Xb<> A\n", 0., inj_bias);
        // if ( all || float(TEMP_BIAS) != Tb_bias_hdwe )
        //   Serial.printf(" Tb_bias_hdwe  %7.3f    %7.3f *Dt<> dg C\n", TEMP_BIAS, Tb_bias_hdwe);
        // if ( all || 1. != s_cap_model )
        //   Serial.printf(" s_cap_model   %7.3f    %7.3f *Sc<>\n", 1., s_cap_model);
        // if ( all || 1. != cutback_gain_scalar )
        //   Serial.printf(" cut_gn_slr    %7.3f    %7.3f *Sk<>\n", 1., cutback_gain_scalar);
        // if ( all || float(HYS_SCALE) != hys_scale )
        //   Serial.printf(" hys_scale     %7.3f    %7.3f *Sh<>\n", HYS_SCALE, hys_scale);
        // if ( all || float(NP) != nP )
        //   Serial.printf(" nP            %7.3f    %7.3f *BP<> eg '2P1S'\n", NP, nP);
        // if ( all || float(NS) != nS )
        //   Serial.printf(" nS            %7.3f    %7.3f *BP<> eg '2P1S'\n", NS, nS);
        // if ( all || MON_CHEM != mon_chm )
        //   Serial.printf(" mon chem            %d          %d *Bm<> 0=Battle, 1=LION\n", MON_CHEM, mon_chm);
        // if ( all || SIM_CHEM != sim_chm )
        //   Serial.printf(" sim chem            %d          %d *Bs<>\n", SIM_CHEM, sim_chm);
        // if ( all || float(VB_SCALE) != Vb_scale )
        //   Serial.printf(" sclr vb       %7.3f    %7.3f *SV<>\n\n", VB_SCALE, Vb_scale);
    }
}
