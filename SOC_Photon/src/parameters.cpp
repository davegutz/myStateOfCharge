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
#include "Battery.h"
#include "parameters.h"
#include "mySensors.h"


// class SavedPars 
SavedPars::SavedPars()
{
  nflt_ = int( NFLT ); 
  nhis_ = int( NHIS ); 
}
#if PLATFORM_ID == 6
    SavedPars::SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt)
    {
        nhis_ = nhis;
        nflt_ = nflt;
        history_ = hist;
        fault_ = faults;
    }
#endif
SavedPars::SavedPars(SerialRAM *ram)
{
    next_ = 0x000;
    #if PLATFORM_ID == PLATFORM_ARGON
        rP_ = ram;
        // Memory map
        amp_eeram_.a16 = next_; next_ += sizeof(amp);
        cutback_gain_sclr_eeram_.a16 = next_; next_ += sizeof(cutback_gain_sclr);
        debug_eeram_.a16 = next_; next_ += sizeof(debug);
        delta_q_eeram_.a16 = next_;  next_ += sizeof(delta_q);
        delta_q_model_eeram_.a16 = next_;  next_ += sizeof(delta_q_model);
        freq_eeram_.a16 = next_; next_ += sizeof(freq);
        hys_scale_eeram_.a16 = next_; next_ += sizeof(hys_scale);
        Ib_bias_all_eeram_.a16 =  next_;  next_ += sizeof(Ib_bias_all);
        Ib_bias_amp_eeram_.a16 =  next_;  next_ += sizeof(Ib_bias_amp);
        Ib_bias_noa_eeram_.a16 =  next_;  next_ += sizeof(Ib_bias_noa);
        ib_scale_amp_eeram_.a16 =  next_;  next_ += sizeof(ib_scale_amp);
        ib_scale_noa_eeram_.a16 =  next_;  next_ += sizeof(ib_scale_noa);
        ib_select_eeram_.a16 =  next_;  next_ += sizeof(ib_select);
        iflt_eeram_.a16 =  next_;  next_ += sizeof(iflt);
        ihis_eeram_.a16 =  next_;  next_ += sizeof(ihis);
        inj_bias_eeram_.a16 =  next_;  next_ += sizeof(inj_bias);
        isum_eeram_.a16 =  next_;  next_ += sizeof(isum);
        mon_chm_eeram_.a16 =  next_;  next_ += sizeof(mon_chm);
        modeling_eeram_.a16 =  next_;  next_ += sizeof(modeling_);
        nP_eeram_.a16 = next_; next_ += sizeof(nP);
        nS_eeram_.a16 = next_; next_ += sizeof(nS);
        preserving_eeram_.a16 =  next_;  next_ += sizeof(preserving);
        shunt_gain_sclr_eeram_.a16 = next_;  next_ += sizeof(shunt_gain_sclr);
        sim_chm_eeram_.a16 =  next_;  next_ += sizeof(sim_chm);
        s_cap_model_eeram_.a16 = next_;  next_ += sizeof(s_cap_model);
        Tb_bias_hdwe_eeram_.a16 = next_; next_ += sizeof(Tb_bias_hdwe);
        time_now_eeram_.a16 = next_; next_ += sizeof(time_now);
        type_eeram_.a16 =  next_;  next_ += sizeof(type);
        t_last_eeram_.a16 =  next_;  next_ += sizeof(t_last);
        t_last_model_eeram_.a16 =  next_; next_ += sizeof(t_last_model);
        Vb_bias_hdwe_eeram_.a16 = next_; next_ += sizeof(Vb_bias_hdwe);
        Vb_scale_eeram_.a16 = next_; next_ += sizeof(Vb_scale);
        nflt_ = int( NFLT ); 
        fault_ = new Flt_ram[nflt_];
        for ( int i=0; i<nflt_; i++ )
        {
            fault_[i].instantiate(ram, &next_);
        }
        nhis_ = int( (MAX_EERAM - next_) / sizeof(Flt_st) ); 
        history_ = new Flt_ram[nhis_];
        for ( int i=0; i<nhis_; i++ )
        {
            history_[i].instantiate(ram, &next_);
        }
    #endif
}
SavedPars::~SavedPars() {}
// operators
// functions

// Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
// battery.  Small compilation changes can change where in this memory the program points, too
boolean SavedPars::is_corrupt()
{
    boolean corruption = 
        is_val_corrupt(amp, float(-1e6), float(1e6)) ||
        is_val_corrupt(cutback_gain_sclr, float(-1000.), float(1000.)) ||
        is_val_corrupt(debug, -100, 100) ||
        is_val_corrupt(delta_q, -1e8, 1e5) ||
        is_val_corrupt(delta_q, -1e8, 1e5) ||
        is_val_corrupt(delta_q_model, -1e8, 1e5) ||
        is_val_corrupt(freq, float(0.), float(2.)) ||
        is_val_corrupt(Ib_bias_all, float(-1e5), float(1e5)) ||
        is_val_corrupt(Ib_bias_amp, float(-1e5), float(1e5)) ||
        is_val_corrupt(Ib_bias_noa, float(-1e5), float(1e5)) ||
        is_val_corrupt(ib_scale_amp, float(-1e6), float(1e6)) ||
        is_val_corrupt(ib_scale_noa, float(-1e6), float(1e6)) ||
        is_val_corrupt(ib_select, int8_t(-1), int8_t(1)) ||
        is_val_corrupt(iflt, -1, nflt_+1) ||
        is_val_corrupt(ihis, -1, nhis_+1) ||
        is_val_corrupt(inj_bias, float(-100.), float(100.)) ||
        is_val_corrupt(isum, -1, NSUM+1) ||
        // is_val_corrupt(modeling_, uint8_t(0), uint8_t(255)) ||  // pointless to check this
        is_val_corrupt(mon_chm, uint8_t(0), uint8_t(10)) ||
        is_val_corrupt(nP, float(1e-6), float(100.)) ||
        is_val_corrupt(nS, float(1e-6), float(100.)) ||
        is_val_corrupt(preserving, uint8_t(0), uint8_t(1)) ||
        is_val_corrupt(shunt_gain_sclr, float(-1e6), float(1e6)) ||
        is_val_corrupt(sim_chm, uint8_t(0), uint8_t(10)) ||
        is_val_corrupt(s_cap_model, float(0.), float(1000.)) ||
        is_val_corrupt(Tb_bias_hdwe, float(-500.), float(500.)) ||
        // is_val_corrupt(time_now, 0UL, 0UL) ||
        is_val_corrupt(type, uint8_t(0), uint8_t(10)) ||
        is_val_corrupt(t_last, float(-10.), float(70.)) ||
        is_val_corrupt(t_last_model, float(-10.), float(70.)) ||
        is_val_corrupt(Vb_bias_hdwe, float(-10.), float(70.)) ||
        is_val_corrupt(Vb_scale, float(-1e6), float(1e6)) ;
        if ( corruption )
        {
            Serial.printf("corrupt*********\n");
            pretty_print(true);
        }
        return corruption;
}

// Assign all save EERAM to RAM
#if PLATFORM_ID == PLATFORM_ARGON
    void SavedPars::load_all()
    {
        get_amp();
        get_cutback_gain_sclr();
        get_debug();
        get_delta_q();
        get_delta_q_model();
        get_freq();
        get_hys_scale();
        get_Ib_bias_all();
        get_Ib_bias_amp();
        get_Ib_bias_noa();
        get_ib_scale_amp();
        get_ib_scale_noa();
        get_ib_select();
        get_iflt();
        get_inj_bias();
        get_isum();
        get_modeling();
        get_mon_chm();
        get_nP();
        get_nS();
        get_preserving();
        get_shunt_gain_sclr();
        get_sim_chm();
        get_s_cap_model();
        get_Tb_bias_hdwe();
        get_time_now();
        get_type();
        get_t_last();
        get_t_last_model();
        get_Vb_bias_hdwe();
        get_Vb_scale();
        for ( int i=0; i<nflt_; i++ ) fault_[i].get();
        for ( int i=0; i<nhis_; i++ ) history_[i].get();
    }
#endif

// Number of differences between nominal EERAM and actual (don't count integator memories because they always change)
int SavedPars::num_diffs()
{
    int n = 0;
    // Don't count memories
    // if ( float(RATED_TEMP) != t_last )    //   n++;
    // if ( float(RATED_TEMP) != t_last_model )    //   n++;
    // if ( 0. != delta_q )    //   n++;
    // if ( 0. != delta_q_model )    //   n++;
    // if ( int(-1) != iflt )    //     n++;
    // if ( int(-1) != isum )    //     n++;
    // if ( uint8_t(0) != preserving )    //     n++;
    // if ( 0UL < time_now ) n++;
    if ( float(0.) != amp ) n++;
    if ( float(1.) != cutback_gain_sclr ) n++;
    if ( int(0) != debug ) n++;
    if ( float(0.) != freq ) n++;
    if ( float(HYS_SCALE) != hys_scale ) n++;
    if ( float(CURR_BIAS_ALL) != Ib_bias_all ) n++;
    if ( float(CURR_BIAS_AMP) != Ib_bias_amp ) n++;
    if ( float(CURR_BIAS_NOA) != Ib_bias_noa ) n++;
    if ( float(CURR_SCALE_AMP) != ib_scale_amp ) n++;
    if ( float(CURR_SCALE_NOA) != ib_scale_noa ) n++;
    if ( int8_t(FAKE_FAULTS) != ib_select ) n++;
    if ( float(0.) != inj_bias ) n++;
    if ( uint8_t(MODELING) != modeling_ ) n++;
    if ( uint8_t(MON_CHEM) != mon_chm ) n++;
    if ( float(NP) != nP ) n++;
    if ( float(NS) != nS ) n++;
    if ( float(1.) != shunt_gain_sclr ) n++;
    if ( uint8_t(SIM_CHEM) != sim_chm ) n++;
    if ( float(1.) != s_cap_model ) n++;
    if ( float(TEMP_BIAS) != Tb_bias_hdwe ) n++;
    if ( uint8_t(0) != type ) n++;
    if ( float(VOLT_BIAS) != Vb_bias_hdwe ) n++;
    if ( float(VB_SCALE) != Vb_scale ) n++;
    return ( n );
}

// Configuration functions

// Print memory map
void SavedPars::mem_print()
{
    #if PLATFORM_ID == PLATFORM_ARGON
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        Serial.printf("Temp mem map print\n");
        for ( uint16_t i=0x0000; i<MAX_EERAM; i++ ) Serial.printf("0x%X ", rP_->read(i));
    #endif
}

// Manage changes to modeling configuration
void SavedPars::modeling(const uint8_t input, Sensors *Sen)
{
    modeling_ = input;
    put_modeling(modeling_);
    Sen->ShuntAmp->dscn_cmd(mod_ib_amp_dscn());
    Sen->ShuntNoAmp->dscn_cmd(mod_ib_noa_dscn());
}

// Print
void SavedPars::pretty_print(const boolean all)
{
    Serial.printf("saved parameters (sp):\n");
    Serial.printf("             defaults    current EERAM values\n");
    if ( all || float(0.) != amp )              Serial.printf(" inj amp%7.3f  %7.3f *Xa<> A pk\n", 0., amp);
    if ( all || 1. != cutback_gain_sclr )       Serial.printf(" cut_gn_slr%7.3f  %7.3f *Sk<>\n", 1., cutback_gain_sclr);
    if ( all || int(0) != debug )               Serial.printf(" debug  %d  %d *v<>\n", int(0), debug);
    if ( all )                                  Serial.printf(" delta_q%10.1f %10.1f *DQ<>\n", double(0.), delta_q);
    if ( all )                                  Serial.printf(" dq_sim %10.1f %10.1f *Ca<>, *Cm<>, C\n", double(0.), delta_q_model);
    if ( all || float(0.) != freq )             Serial.printf(" inj frq%7.3f  %7.3f *Xf<> r/s\n", 0., freq);
    if ( all || float(HYS_SCALE) != hys_scale ) Serial.printf(" hys_scale     %7.3f    %7.3f *Sh<>\n", HYS_SCALE, hys_scale);
    if ( all || float(CURR_BIAS_ALL) != Ib_bias_all )   Serial.printf(" Ib_bias_all%7.3f  %7.3f *Di<> A\n", CURR_BIAS_ALL, Ib_bias_all);
    if ( all || float(CURR_BIAS_AMP) != Ib_bias_amp )   Serial.printf(" bias_amp%7.3f  %7.3f *DA<>\n", CURR_BIAS_AMP, Ib_bias_amp);
    if ( all || float(CURR_BIAS_NOA) != Ib_bias_noa )   Serial.printf(" bias_noa%7.3f  %7.3f *DB<>\n", CURR_BIAS_NOA, Ib_bias_noa);
    if ( all || float(CURR_SCALE_AMP) != ib_scale_amp ) Serial.printf(" ib_scale_amp%7.3f  %7.3f *SA<>\n", CURR_SCALE_AMP, ib_scale_amp);
    if ( all || float(CURR_SCALE_NOA) != ib_scale_noa ) Serial.printf(" ib_scale_noa%7.3f  %7.3f *SB<>\n", CURR_SCALE_NOA, ib_scale_noa);
    if ( all || int8_t(FAKE_FAULTS) != ib_select )      Serial.printf(" ib_select %d  %d *s<> -1=noa, 0=auto, 1=amp\n", FAKE_FAULTS, ib_select);
    if ( all )                                  Serial.printf(" iflt                           %d flt ptr\n", iflt);
    if ( all || float(0.) != inj_bias )         Serial.printf(" inj_bias%7.3f  %7.3f *Xb<> A\n", 0., inj_bias);
    if ( all )                                  Serial.printf(" isum                           %d tbl ptr\n", isum);
    if ( all || uint8_t(MODELING) != modeling_ ) Serial.printf(" modeling %d  %d *Xm<>\n", uint8_t(MODELING), modeling_);
    if ( all || MON_CHEM != mon_chm )           Serial.printf(" mon chem            %d          %d *Bm<> 0=Battle, 1=LION\n", MON_CHEM, mon_chm);
    if ( all )                                  Serial.printf(" preserving %d  %d *Xm<>\n", uint8_t(0), preserving);
    if ( all || float(NP) != nP )               Serial.printf(" nP            %7.3f    %7.3f *BP<> eg '2P1S'\n", NP, nP);
    if ( all || float(NS) != nS )               Serial.printf(" nS            %7.3f    %7.3f *BS<> eg '2P1S'\n", NS, nS);
    if ( all || float(1.) != shunt_gain_sclr )  Serial.printf(" shunt_gn_slr%7.3f  %7.3f *SG\n", 1., shunt_gain_sclr);
    if ( all || SIM_CHEM != sim_chm )           Serial.printf(" sim chem            %d          %d *Bs<>\n", SIM_CHEM, sim_chm);
    if ( all || float(1.) != s_cap_model )      Serial.printf(" s_cap_model%7.3f  %7.3f *Sc<>\n", 1., s_cap_model);
    if ( all || float(TEMP_BIAS) != Tb_bias_hdwe )      Serial.printf(" Tb_bias_hdwe%7.3f  %7.3f *Dt<> dg C\n", TEMP_BIAS, Tb_bias_hdwe);
    if ( all )                                  Serial.printf(" time_now %d %s *U<> Unix time\n", (int)Time.now(), Time.timeStr().c_str());
    if ( all || uint8_t(0) != type )            Serial.printf(" type inj %d  %d *Xt<> 1=sin, 2=sq, 3=tri, 4=1C, 5=-1C, 8=cos\n", 0, type);
    if ( all )                                  Serial.printf(" t_last %5.2f  %5.2f dg C\n", float(RATED_TEMP), t_last);
    if ( all )                                  Serial.printf(" t_last_sim %5.2f  %5.2f dg C\n", float(RATED_TEMP), t_last_model);
    if ( all || float(VOLT_BIAS) != Vb_bias_hdwe )      Serial.printf(" Vb_bias_hdwe %7.3f  %7.3f *Dv<>,*Dc<> V\n", VOLT_BIAS, Vb_bias_hdwe);
    if ( all || float(VB_SCALE) != Vb_scale )   Serial.printf(" sclr vb       %7.3f    %7.3f *SV<>\n\n", VB_SCALE, Vb_scale);
    // if ( all )
    // {
    //     Serial.printf("history array (%d):\n", nhis_);
    //     print_history_array();
    //     print_fault_header();
    //     Serial.printf("fault array (%d):\n", nflt_);
    //     print_fault_array();
    //     print_fault_header();
    // }
    #if PLATFORM_ID == PLATFORM_ARGON
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        // Serial.printf("Temp mem map print\n");
        // mem_print();
    #endif
}

// Print faults
void SavedPars::print_fault_array()
{
  int i = iflt;  // Last one written was iflt
  int n = -1;
  while ( ++n < nflt_ )
  {
    if ( ++i > (nflt_-1) ) i = 0; // circular buffer
    fault_[i].print("unit_f");
  }
}

// Print faults
void SavedPars::print_fault_header()
{
  Serial.printf ("fltb,  date,                time,    Tb_h, vb_h, ibah, ibnh, Tb, vb, ib, soc, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
}

// Print history
void SavedPars::print_history_array()
{
  int i = ihis;  // Last one written was iflt
  int n = -1;
  while ( ++n < nhis_ )
  {
    if ( ++i > (nhis_-1) ) i = 0; // circular buffer
    history_[i].print("unit_h");
  }
}

// Dynamic parameters saved
// This saves a lot of througput.   Without it, there are six put calls each 'read' minor frame at 1 ms each call
void SavedPars::put_all_dynamic()
{
    static uint8_t blink = 0;
    switch ( blink++ )
    {
        case ( 0 ):
            put_delta_q();
            break;

        case ( 1 ):
            put_delta_q_model();
            break;

        case ( 2 ):
            put_hys_scale();
            break;

        case ( 3 ):
            put_mon_chm();
            break;

        case ( 4 ):
            put_sim_chm();
            break;

        case ( 5 ):
            put_t_last();
            break;

        case ( 6 ):
            put_t_last_model();
            break;

        case ( 7 ):
            put_time_now(Time.now());  // If happen to connect to wifi (assume updated automatically), save new time
            blink = 0;
            break;

        default:
            blink = 0;
            break;
    }
}
 
 // Bounce history elements
Flt_st SavedPars::put_history(Flt_st input, const uint8_t i)
{
    Flt_st bounced_sum;
    bounced_sum.copy_to_Flt_ram_from(history_[i]);
    history_[i].put(input);
    return bounced_sum;
}

// Reset arrays
void SavedPars::reset_flt()
{
    for ( int i=0; i<nflt_; i++ )
    {
        fault_[i].put_nominal();
    }
 }
void SavedPars::reset_his()
{
    for ( int i=0; i<nhis_; i++ )
    {
        history_[i].put_nominal();
    }
 }
void SavedPars::reset_pars()
{
    put_amp(float(0));
    put_cutback_gain_sclr(float(1.));
    put_debug(int(0));
    put_delta_q(double(0.));
    put_delta_q_model(double(0.));
    put_freq(float(0));
    put_hys_scale(HYS_SCALE);
    put_Ib_bias_all(float(CURR_BIAS_ALL));
    put_Ib_bias_amp(float(CURR_BIAS_AMP));
    put_Ib_bias_noa(float(CURR_BIAS_NOA));
    put_ib_scale_amp(float(CURR_SCALE_AMP));
    put_ib_scale_noa(float(CURR_SCALE_NOA));
    put_ib_select(int8_t(FAKE_FAULTS));
    put_iflt(int(-1));
    put_ihis(int(-1));
    put_inj_bias(float(0.));
    put_isum(int(-1));
    put_modeling(uint8_t(MODELING));
    put_mon_chm(uint8_t(MON_CHEM));
    put_nP(float(NP));
    put_nS(float(NS));
    put_preserving(uint8_t(0));
    put_shunt_gain_sclr(float(1.));
    put_sim_chm(uint8_t(SIM_CHEM));
    put_s_cap_model(float(1.));
    put_Tb_bias_hdwe(float(TEMP_BIAS));
    put_type(uint8_t(0));    
    put_t_last(float(RATED_TEMP));    
    put_t_last_model(float(RATED_TEMP));  
    put_Vb_bias_hdwe(float(VOLT_BIAS));
    put_Vb_scale(float(VB_SCALE));
 }
