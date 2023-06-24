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
#ifdef CONFIG_PHOTON
    SavedPars::SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt)
    {
        nhis_ = nhis;
        nflt_ = nflt;
        history_ = hist;
        fault_ = faults;
    }
#elif defined(CONFIG_PHOTON2)
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
    #ifdef CONFIG_ARGON
        rP_ = ram;
        // Memory map
        amp_eeram_.a16 = next_; next_ += sizeof(amp_);
        cutback_gain_sclr_eeram_.a16 = next_; next_ += sizeof(cutback_gain_sclr_);
        debug_eeram_.a16 = next_; next_ += sizeof(debug_);
        delta_q_eeram_.a16 = next_;  next_ += sizeof(delta_q_);
        delta_q_model_eeram_.a16 = next_;  next_ += sizeof(delta_q_model_);
        freq_eeram_.a16 = next_; next_ += sizeof(freq_);
        hys_scale_eeram_.a16 = next_; next_ += sizeof(hys_scale_);
        Ib_bias_all_eeram_.a16 =  next_;  next_ += sizeof(Ib_bias_all_);
        Ib_bias_amp_eeram_.a16 =  next_;  next_ += sizeof(Ib_bias_amp_);
        Ib_bias_noa_eeram_.a16 =  next_;  next_ += sizeof(Ib_bias_noa_);
        ib_scale_amp_eeram_.a16 =  next_;  next_ += sizeof(ib_scale_amp_);
        ib_scale_noa_eeram_.a16 =  next_;  next_ += sizeof(ib_scale_noa_);
        ib_select_eeram_.a16 =  next_;  next_ += sizeof(ib_select_);
        iflt_eeram_.a16 =  next_;  next_ += sizeof(iflt_);
        ihis_eeram_.a16 =  next_;  next_ += sizeof(ihis_);
        inj_bias_eeram_.a16 =  next_;  next_ += sizeof(inj_bias_);
        isum_eeram_.a16 =  next_;  next_ += sizeof(isum_);
        mon_chm_eeram_.a16 =  next_;  next_ += sizeof(mon_chm_);
        modeling_eeram_.a16 =  next_;  next_ += sizeof(modeling_);
        nP_eeram_.a16 = next_; next_ += sizeof(nP_);
        nS_eeram_.a16 = next_; next_ += sizeof(nS_);
        preserving_eeram_.a16 =  next_;  next_ += sizeof(preserving_);
        shunt_gain_sclr_eeram_.a16 = next_;  next_ += sizeof(shunt_gain_sclr_);
        sim_chm_eeram_.a16 =  next_;  next_ += sizeof(sim_chm_);
        s_cap_mon_eeram_.a16 = next_;  next_ += sizeof(s_cap_mon_);
        s_cap_sim_eeram_.a16 = next_;  next_ += sizeof(s_cap_sim_);
        Tb_bias_hdwe_eeram_.a16 = next_; next_ += sizeof(Tb_bias_hdwe_);
        time_now_eeram_.a16 = next_; next_ += sizeof(time_now_);
        type_eeram_.a16 =  next_;  next_ += sizeof(type_);
        t_last_eeram_.a16 =  next_;  next_ += sizeof(t_last_);
        t_last_model_eeram_.a16 =  next_; next_ += sizeof(t_last_model_);
        Vb_bias_hdwe_eeram_.a16 = next_; next_ += sizeof(Vb_bias_hdwe_);
        Vb_scale_eeram_.a16 = next_; next_ += sizeof(Vb_scale_);
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
        is_val_corrupt(amp_, float(-1e6), float(1e6)) ||
        is_val_corrupt(cutback_gain_sclr_, float(-1000.), float(1000.)) ||
        is_val_corrupt(debug_, -100, 100) ||
        is_val_corrupt(delta_q_, -1e8, 1e5) ||
        is_val_corrupt(delta_q_model_, -1e8, 1e5) ||
        is_val_corrupt(freq_, float(0.), float(2.)) ||
        is_val_corrupt(Ib_bias_all_, float(-1e5), float(1e5)) ||
        is_val_corrupt(Ib_bias_amp_, float(-1e5), float(1e5)) ||
        is_val_corrupt(Ib_bias_noa_, float(-1e5), float(1e5)) ||
        is_val_corrupt(ib_scale_amp_, float(-1e6), float(1e6)) ||
        is_val_corrupt(ib_scale_noa_, float(-1e6), float(1e6)) ||
        is_val_corrupt(ib_select_, int8_t(-1), int8_t(1)) ||
        is_val_corrupt(iflt_, -1, nflt_+1) ||
        is_val_corrupt(ihis_, -1, nhis_+1) ||
        is_val_corrupt(inj_bias_, float(-100.), float(100.)) ||
        is_val_corrupt(isum_, -1, NSUM+1) ||
        // is_val_corrupt(modeling_, uint8_t(0), uint8_t(255)) ||  // pointless to check this
        is_val_corrupt(mon_chm_, uint8_t(0), uint8_t(10)) ||
        is_val_corrupt(nP_, float(1e-6), float(100.)) ||
        is_val_corrupt(nS_, float(1e-6), float(100.)) ||
        is_val_corrupt(preserving_, uint8_t(0), uint8_t(1)) ||
        is_val_corrupt(shunt_gain_sclr_, float(-1e6), float(1e6)) ||
        is_val_corrupt(sim_chm_, uint8_t(0), uint8_t(10)) ||
        is_val_corrupt(s_cap_mon_, float(0.), float(1000.)) ||
        is_val_corrupt(s_cap_sim_, float(0.), float(1000.)) ||
        is_val_corrupt(Tb_bias_hdwe_, float(-500.), float(500.)) ||
        // is_val_corrupt(time_now_, 0UL, 0UL) ||
        is_val_corrupt(type_, uint8_t(0), uint8_t(10)) ||
        is_val_corrupt(t_last_, float(-10.), float(70.)) ||
        is_val_corrupt(t_last_model_, float(-10.), float(70.)) ||
        is_val_corrupt(Vb_bias_hdwe_, float(-10.), float(70.)) ||
        is_val_corrupt(Vb_scale_, float(-1e6), float(1e6)) ;
        if ( corruption )
        {
            Serial.printf("corrupt*********\n");
            pretty_print(true);
        }
        return corruption;
}

// Assign all save EERAM to RAM
#ifdef CONFIG_ARGON
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
        get_s_cap_mon();
        get_s_cap_sim();
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
    // if ( float(RATED_TEMP) != t_last_ )    //   n++;
    // if ( float(RATED_TEMP) != t_last_model_ )    //   n++;
    // if ( 0. != delta_q_ )    //   n++;
    // if ( 0. != delta_q_model_ )    //   n++;
    // if ( int(-1) != iflt_ )    //     n++;
    // if ( int(-1) != isum_ )    //     n++;
    // if ( uint8_t(0) != preserving_ )    //     n++;
    // if ( 0UL < time_now ) n++;
    if ( float(0.) != amp_ ) n++;
    if ( float(1.) != cutback_gain_sclr_ ) n++;
    if ( int(0) != debug_ ) n++;
    if ( float(0.) != freq_ ) n++;
    if ( float(HYS_SCALE) != hys_scale_ ) n++;
    if ( float(CURR_BIAS_ALL) != Ib_bias_all_ ) n++;
    if ( float(CURR_BIAS_AMP) != Ib_bias_amp_ ) n++;
    if ( float(CURR_BIAS_NOA) != Ib_bias_noa_ ) n++;
    if ( float(CURR_SCALE_AMP) != ib_scale_amp_ ) n++;
    if ( float(CURR_SCALE_NOA) != ib_scale_noa_ ) n++;
    if ( int8_t(FAKE_FAULTS) != ib_select_ ) n++;
    if ( float(0.) != inj_bias_ ) n++;
    if ( uint8_t(MODELING) != modeling_ ) n++;
    if ( uint8_t(MON_CHEM) != mon_chm_ ) n++;
    if ( float(NP) != nP_ ) n++;
    if ( float(NS) != nS_ ) n++;
    if ( float(1.) != shunt_gain_sclr_ ) n++;
    if ( uint8_t(SIM_CHEM) != sim_chm_ ) n++;
    if ( float(1.) != s_cap_mon_ ) n++;
    if ( float(1.) != s_cap_sim_ ) n++;
    if ( float(TEMP_BIAS) != Tb_bias_hdwe_ ) n++;
    if ( uint8_t(0) != type_ ) n++;
    if ( float(VOLT_BIAS) != Vb_bias_hdwe_ ) n++;
    if ( float(VB_SCALE) != Vb_scale_ ) n++;
    return ( n );
}

// Configuration functions

// Print memory map
void SavedPars::mem_print()
{
    #ifdef CONFIG_ARGON
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
    if ( all || float(0.) != amp_ )             Serial.printf(" inj amp%7.3f  %7.3f *Xa<> A pk\n", 0., amp_);
    if ( all || 1. != cutback_gain_sclr_ )      Serial.printf(" cut_gn_slr%7.3f  %7.3f *Sk<>\n", 1., cutback_gain_sclr_);
    if ( all || int(0) != debug_ )              Serial.printf(" debug  %d  %d *v<>\n", int(0), debug_);
    if ( all )                                  Serial.printf(" delta_q%10.1f %10.1f *DQ<>\n", double(0.), delta_q_);
    if ( all )                                  Serial.printf(" dq_sim %10.1f %10.1f *Ca<>, *Cm<>, C\n", double(0.), delta_q_model_);
    if ( all || float(0.) != freq_ )            Serial.printf(" inj frq%7.3f  %7.3f *Xf<> r/s\n", 0., freq_);
    if ( all || float(HYS_SCALE) != hys_scale_ ) Serial.printf(" hys_scale     %7.3f    %7.3f *Sh<>\n", HYS_SCALE, hys_scale_);
    if ( all || float(CURR_BIAS_ALL) != Ib_bias_all_ )  Serial.printf(" Ib_bias_all%7.3f  %7.3f *Di<> A\n", CURR_BIAS_ALL, Ib_bias_all_);
    if ( all || float(CURR_BIAS_AMP) != Ib_bias_amp_ )  Serial.printf(" bias_amp%7.3f  %7.3f *DA<>\n", CURR_BIAS_AMP, Ib_bias_amp_);
    if ( all || float(CURR_BIAS_NOA) != Ib_bias_noa_ )  Serial.printf(" bias_noa%7.3f  %7.3f *DB<>\n", CURR_BIAS_NOA, Ib_bias_noa_);
    if ( all || float(CURR_SCALE_AMP) != ib_scale_amp_ )Serial.printf(" ib_scale_amp%7.3f  %7.3f *SA<>\n", CURR_SCALE_AMP, ib_scale_amp_);
    if ( all || float(CURR_SCALE_NOA) != ib_scale_noa_ )Serial.printf(" ib_scale_noa%7.3f  %7.3f *SB<>\n", CURR_SCALE_NOA, ib_scale_noa_);
    if ( all || int8_t(FAKE_FAULTS) != ib_select_ )     Serial.printf(" ib_select %d  %d *s<> -1=noa, 0=auto, 1=amp\n", FAKE_FAULTS, ib_select_);
    if ( all )                                  Serial.printf(" iflt                           %d flt ptr\n", iflt_);
    if ( all || float(0.) != inj_bias_ )        Serial.printf(" inj_bias%7.3f  %7.3f *Xb<> A\n", 0., inj_bias_);
    if ( all )                                  Serial.printf(" isum                           %d tbl ptr\n", isum_);
    if ( all || uint8_t(MODELING) != modeling_ ) Serial.printf(" modeling %d  %d *Xm<>\n", uint8_t(MODELING), modeling_);
    if ( all || MON_CHEM != mon_chm_ )          Serial.printf(" mon chem            %d          %d *Bm<> 0=Battleborn, 1=CHINS, 2=Spare\n", MON_CHEM, mon_chm_);
    if ( all )                                  Serial.printf(" preserving %d  %d *Xm<>\n", uint8_t(0), preserving_);
    if ( all || float(NP) != nP_ )              Serial.printf(" nP            %7.3f    %7.3f *BP<> eg '2P1S'\n", NP, nP_);
    if ( all || float(NS) != nS_ )              Serial.printf(" nS            %7.3f    %7.3f *BS<> eg '2P1S'\n", NS, nS_);
    if ( all || float(1.) != shunt_gain_sclr_ )  Serial.printf(" shunt_gn_slr%7.3f  %7.3f *SG\n", 1., shunt_gain_sclr_);
    if ( all || SIM_CHEM != sim_chm_ )          Serial.printf(" sim chem            %d          %d *Bs<>\n", SIM_CHEM, sim_chm_);
    if ( all || float(1.) != s_cap_mon_ )     Serial.printf(" s_cap_mon%7.3f  %7.3f *SQ<>\n", 1., s_cap_mon_);
    if ( all || float(1.) != s_cap_sim_ )     Serial.printf(" s_cap_sim%7.3f  %7.3f *Sq<>\n", 1., s_cap_sim_);
    if ( all || float(TEMP_BIAS) != Tb_bias_hdwe_ )     Serial.printf(" Tb_bias_hdwe%7.3f  %7.3f *Dt<> dg C\n", TEMP_BIAS, Tb_bias_hdwe_);
    if ( all )                                  Serial.printf(" time_now %d %s *U<> Unix time\n", (int)Time.now(), Time.timeStr().c_str());
    if ( all || uint8_t(0) != type_ )           Serial.printf(" type inj %d  %d *Xt<> 1=sin, 2=sq, 3=tri, 4=1C, 5=-1C, 8=cos\n", 0, type_);
    if ( all )                                  Serial.printf(" t_last %5.2f  %5.2f dg C\n", float(RATED_TEMP), t_last_);
    if ( all )                                  Serial.printf(" t_last_sim %5.2f  %5.2f dg C\n", float(RATED_TEMP), t_last_model_);
    if ( all || float(VOLT_BIAS) != Vb_bias_hdwe_ )     Serial.printf(" Vb_bias_hdwe %7.3f  %7.3f *Dv<>,*Dc<> V\n", VOLT_BIAS, Vb_bias_hdwe_);
    if ( all || float(VB_SCALE) != Vb_scale_ )   Serial.printf(" sclr vb       %7.3f    %7.3f *SV<>\n\n", VB_SCALE, Vb_scale_);
    // if ( all )
    // {
    //     Serial.printf("history array (%d):\n", nhis_);
    //     print_history_array();
    //     print_fault_header();
    //     Serial.printf("fault array (%d):\n", nflt_);
    //     print_fault_array();
    //     print_fault_header();
    // }
    #ifdef CONFIG_ARGON
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        // Serial.printf("Temp mem map print\n");
        // mem_print();
    #endif
}

// Print faults
void SavedPars::print_fault_array()
{
  int i = iflt_;  // Last one written was iflt
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
  Serial.printf ("fltb,  date,                time,    Tb_h, vb_h, ibah, ibnh, Tb, vb, ib, soc, soc_min, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
  Serial1.printf ("fltb,  date,                time,    Tb_h, vb_h, ibah, ibnh, Tb, vb, ib, soc, soc_min, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
}

// Print history
void SavedPars::print_history_array()
{
  int i = ihis_;  // Last one written was ihis
  int n = -1;
  while ( ++n < nhis_ )
  {
    if ( ++i > (nhis_-1) ) i = 0; // circular buffer
    history_[i].print("unit_h");
  }
}

// Dynamic parameters saved
// This saves a lot of througput.   Without it, there are many put calls each 'read' minor frame at 1 ms each call
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
    put_s_cap_mon(float(1.));
    put_s_cap_sim(float(1.));
    put_Tb_bias_hdwe(float(TEMP_BIAS));
    put_type(uint8_t(0));    
    put_t_last(float(RATED_TEMP));    
    put_t_last_model(float(RATED_TEMP));  
    put_Vb_bias_hdwe(float(VOLT_BIAS));
    put_Vb_scale(float(VB_SCALE));
 }
