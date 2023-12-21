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
#include "Sensors.h"
#include "command.h"

extern CommandPars cp;


// class Parameters
// Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
// battery.  Small compilation changes can change where in this memory the program points, too
Parameters::Parameters():n_(0) {};

Parameters::~Parameters(){};

boolean Parameters::find_adjust(const String &str)
{
    uint8_t count = 0;
    boolean success = false;
    String substr = str.substring(0, 2);
    String value_str = str.substring(2);
    if ( substr.length()<2 )
    {
        Serial.printf("%s substr of %s is too short\n", substr.c_str(), str.c_str());
        return false;
    }
    for ( uint8_t i=0; i<n_; i++ )
    {
        if ( substr==V_[i]->code() )
        {
            if ( !count ) success = V_[i]->print_adjust(value_str);
            else Serial.printf("REPEAT at i %d %s\n", i, V_[i]->code().c_str());
            count++;
        }
    }
    if ( count==1 && success )
    {
        return success;
    }
    else if ( count > 1 )
    {
        Serial.printf("REPEAT: %s was decoded into code %s and value %s\n", str.c_str(), substr.c_str(), value_str.c_str());
        return false;
    }
    else
    {
        // Serial.printf("Problem: %s was decoded into code %s and value %s\n", str.c_str(), substr.c_str(), value_str.c_str());
        return false;
    }

}

boolean Parameters::is_corrupt()
{
    boolean corruption = false;
    for ( int i=0; i<n_; i++ ) corruption |= V_[i]->is_corrupt();
    if ( corruption )
    {
        Serial.printf("\ncorrupt****\n");
        pretty_print(false);
    }
    return corruption;
}

void Parameters::set_nominal()
{
    for ( uint16_t i=0; i<n_; i++ )  if ( V_[i]->code() != "UT" ) V_[i]->set_nominal();
}


// class VolatilePars
VolatilePars::VolatilePars(): Parameters()
{
    initialize();
    set_nominal();
}

VolatilePars::~VolatilePars(){}

void  VolatilePars::initialize()
{
    #define NVOL 37
    V_ = new Variable*[NVOL];
    V_[n_++] =(cc_diff_slr_p    = new FloatV("  ", "Fc", NULL,"Slr cc_diff thr",      "slr",    0,    1000, &cc_diff_slr,       1));
    V_[n_++] =(cycles_inj_p     = new FloatV("  ", "XC", NULL,"Number prog cycle",    "float",  0,    1000, &cycles_inj,        0));
    V_[n_++] =(dc_dc_on_p     = new BooleanV("  ", "Xd", NULL,"DC-DC charger on",     "T=on",   0,    1,    &dc_dc_on,          false));
    V_[n_++] =(disab_ib_fa_p  = new BooleanV("  ", "FI", NULL,"Disab hard range ib",  "T=disab",0,    1,    &disab_ib_fa,       false));
    V_[n_++] =(disab_tb_fa_p  = new BooleanV("  ", "FT", NULL,"Disab hard range tb",  "T=disab",0,    1,    &disab_tb_fa,       false));
    V_[n_++] =(disab_vb_fa_p  = new BooleanV("  ", "FV", NULL,"Disab hard range vb",  "T=disab",0,    1,    &disab_vb_fa,       false));
    V_[n_++] =(ds_voc_soc_p     = new FloatV("  ", "Ds", NULL,"VOC(SOC) del soc",     "slr",    -0.5, 0.5,  &ds_voc_soc,        0));
    V_[n_++] =(dv_voc_soc_p     = new FloatV("  ", "Dy", NULL,"VOC(SOC) del v",       "v",      -50,  50,   &dv_voc_soc,        0));
    V_[n_++] =(eframe_mult_p   = new Uint8tV("  ", "DE", NULL,"EKF frame rate x Dr",  "uint",   0,    UINT8_MAX, &eframe_mult,  EKF_EFRAME_MULT));
    V_[n_++] =(ewhi_slr_p       = new FloatV("  ", "Fi", NULL,"Slr wrap hi thr",      "slr",    0,    1000, &ewhi_slr,          1));
    V_[n_++] =(ewlo_slr_p       = new FloatV("  ", "Fo", NULL,"Slr wrap lo thr",      "slr",    0,    1000, &ewlo_slr,          1));
    V_[n_++] =(fail_tb_p      = new BooleanV("  ", "Xu", NULL,"Ignore Tb & fail",     "T=Fail", false,true, &fail_tb,           false));
    V_[n_++] =(fake_faults_p  = new BooleanV("  ", "Ff", NULL,"Faults ignored",       "T=ign",  0,    1,    &fake_faults,       FAKE_FAULTS));
    V_[n_++] =(his_delay_p      = new ULongV("  ", "Dh", NULL,"History frame",        "ms",    1000UL,SUMMARY_DELAY,&his_delay, SUMMARY_DELAY));
    V_[n_++] =(hys_scale_p      = new FloatV("  ", "Sh", NULL,"Sim hys scale",        "slr",    0,    100,  &hys_scale,         HYS_SCALE));
    V_[n_++] =(hys_state_p      = new FloatV("  ", "SH", NULL,"Sim hys state",        "v",      -10,  10,   &hys_state,         0));
    V_[n_++] =(ib_amp_add_p     = new FloatV("  ", "Dm", NULL,"Amp signal add",       "A",      -1000,1000, &ib_amp_add,        0));
    V_[n_++] =(ib_diff_slr_p    = new FloatV("  ", "Fd", NULL,"Slr ib_diff thr",      "A",      0,    1000, &ib_diff_slr,       1));
    V_[n_++] =(ib_noa_add_p     = new FloatV("  ", "Dn", NULL,"No amp signal add",    "A",      -1000,1000, &ib_noa_add,        0));
    V_[n_++] =(Ib_amp_noise_amp_p= new FloatV("  ","DM", NULL,"Amp amp noise",        "A",      0,    1000, &Ib_amp_noise_amp,  IB_AMP_NOISE));
    V_[n_++] =(Ib_noa_noise_amp_p= new FloatV("  ","DN", NULL,"Amp noa noise",        "A",      0,    1000, &Ib_noa_noise_amp,  IB_NOA_NOISE));
    V_[n_++] =(ib_quiet_slr_p   = new FloatV("  ", "Fq", NULL,"Ib quiet det slr",     "slr",    0,    1000, &ib_quiet_slr,      1));
    V_[n_++] =(init_all_soc_p   = new FloatV("  ", "Ca", NULL,"Init all to this",     "soc",    -0.5, 1.1,  &init_all_soc,      1));
    V_[n_++] =(init_sim_soc_p   = new FloatV("  ", "Cm", NULL,"Init sim to this",     "soc",    -0.5, 1.1,  &init_sim_soc,      1));
    V_[n_++] =(print_mult_p    = new Uint8tV("  ", "DP", NULL,"Print mult x Dr",      "uint",   0,    UINT8_MAX, &print_mult,   DP_MULT));
    V_[n_++] =(read_delay_p     = new ULongV("  ", "Dr", NULL,"Minor frame",          "ms",     0UL,  1000000UL,  &read_delay,  READ_DELAY));
    V_[n_++] =(slr_res_p        = new FloatV("  ", "Sr", NULL,"Scalar Randles R0",    "slr",    0,    100,  &slr_res,           1));
    V_[n_++] =(s_t_sat_p        = new FloatV("  ", "Xs", NULL,"Scalar on T_SAT",      "slr",    0,    100,  &s_t_sat,           1));
    V_[n_++] =(tail_inj_p       = new ULongV("  ", "XT", NULL,"Tail end inj",         "ms",     0UL,  120000UL,&tail_inj,       0UL));
    V_[n_++] =(talk_delay_p     = new ULongV("  ", "D>", NULL,"Talk frame",           "ms",     0UL,  120000UL,&talk_delay,     TALK_DELAY));
    V_[n_++] =(Tb_bias_model_p  = new FloatV("  ", "D^", NULL,"Del model",            "dg C",   -50,  50,   &Tb_bias_model,     TEMP_BIAS));
    V_[n_++] =(Tb_noise_amp_p   = new FloatV("  ", "DT", NULL,"Tb noise",             "dg C pk-pk", 0,50,   &Tb_noise_amp,      TB_NOISE));
    V_[n_++] =(tb_stale_time_slr_p=new FloatV("  ","Xv", NULL,"Scale Tb 1-wire pers", "slr",    0,    100,  &tb_stale_time_slr,1));
    V_[n_++] =(until_q_p        = new ULongV("  ", "XQ", NULL,"Time until vv0",       "ms",     0UL,  1000000UL,  &until_q,     0UL));
    V_[n_++] =(vb_add_p         = new FloatV("  ", "Dv", NULL,"Bias on vb",           "v",      -15,  15,   &vb_add,            0));
    V_[n_++] =(Vb_noise_amp_p   = new FloatV("  ", "DV", NULL,"Vb noise",             "v pk-pk",0,    10,   &Vb_noise_amp,      VB_NOISE));
    V_[n_++] =(wait_inj_p       = new ULongV("  ", "XW", NULL,"Wait start inj",       "ms",     0UL,  120000UL, &wait_inj,      0UL));
}

// Print only the volatile paramters (non-eeram)
void VolatilePars::pretty_print(const boolean all)
{
    #ifndef DEPLOY_PHOTON
        if ( all )
        {
            Serial.printf("volatile all:\n");
            for (uint8_t i=0; i<n_; i++ )
            {
                if ( !(V_[i]->is_eeram()) )
                {
                    V_[i]->print();
                }
            }
        }
    #endif
    if ( !all )
    {
        Serial.printf("volatile off:\n");
        uint8_t count = 0;
        for (uint8_t i=0; i<n_; i++ )
        {
            if ( !(V_[i]->is_eeram()) )
            {
                if ( all || V_[i]->is_off() )
                {
                    count++;
                    V_[i]->print();
                }
            }
        }
        if ( count==0 ) Serial.printf("**none**\n\n");
    }
    while ( n_ != NVOL ) { delay(5000); Serial.printf("set NVOL=%d\n", n_); }
}


/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_z parameters at the bottom of Parameters.h are stored in SRAM
*/
// class SavedPars 
SavedPars::SavedPars(): Parameters()
{
    nflt_ = uint16_t( NFLT ); 
    nhis_ = uint16_t( NHIS );
    nsum_ = 0;
}

SavedPars::SavedPars(Flt_st *hist, const uint16_t nhis, Flt_st *faults, const uint16_t nflt): Parameters()
{
    rP_ = NULL;
    nflt_ = nflt;
    nhis_ = nhis;
    nsum_ = 0;
    #ifndef CONFIG_47L16_EERAM
        history_ = hist;
        fault_ = faults;
    #endif
    initialize();
}

SavedPars::SavedPars(SerialRAM *ram): Parameters()
{
    rP_ = ram;
    next_ = 0x000;
    nflt_ = uint16_t( NFLT ); 
    initialize();

    // TODO:  why was this here?  land mine!!
    // for ( uint8_t i=0; i<n_; i++ ) if ( !V_[i]->is_eeram() ) V_[i]->set_nominal();

    #ifdef CONFIG_47L16_EERAM
        for ( int i=0; i<n_; i++ )
        {
            next_ = V_[i]->assign_addr(next_);
        }
        fault_ = new Flt_ram[nflt_];
        for ( uint16_t i=0; i<nflt_; i++ )
        {
            fault_[i].instantiate(rP_, &next_);
        }
        nhis_ = uint16_t( (MAX_EERAM - next_) / sizeof(Flt_st) ); 
        history_ = new Flt_ram[nhis_];
        for ( uint16_t i=0; i<nhis_; i++ )
        {
            history_[i].instantiate(rP_, &next_);
        }
    #endif
}

SavedPars::~SavedPars() {}

void SavedPars::initialize()
{
    #define NSAV 32
    V_ = new Variable*[NSAV];
    V_[n_++] =(amp_p            = new FloatV("* ", "Xa", rP_, "Inj amp",              "Amps pk",-1e6, 1e6,  &amp_z,         0));
    V_[n_++] =(cutback_gain_slr_p=new FloatV("* ", "Sk", rP_, "Cutback gain scalar",  "slr",    -1e6, 1e6,  &cutback_gain_slr_z,1));
    V_[n_++] =(debug_p            = new IntV("* ", "vv", rP_, "Verbosity",            "int",    -128, 128,  &debug_z,       0));
    V_[n_++] =(delta_q_model_p = new DoubleV("* ", "qs", rP_, "Charge chg Sim",       "C",      -1e8, 1e5,  &delta_q_model_z, 0,                false));
    V_[n_++] =(delta_q_p       = new DoubleV("* ", "qm", rP_, "Charge chg",           "C",      -1e8, 1e5,  &delta_q_z,     0,                  false ));
    V_[n_++] =(Dw_p             = new FloatV("* ", "Dw", rP_, "Tab mon adj",          "v",      -1e2, 1e2,  &Dw_z,          VTAB_BIAS));
    V_[n_++] =(freq_p           = new FloatV("* ", "Xf", rP_, "Inj freq",             "Hz",     0,    2,    &freq_z,        0));
    V_[n_++] =(ib_bias_all_p    = new FloatV("* ", "DI", rP_, "Del all",              "A",      -1e5, 1e5,  &ib_bias_all_z, CURR_BIAS_ALL));
    V_[n_++] =(ib_bias_amp_p    = new FloatV("* ", "DA", rP_, "Add amp",              "A",      -1e5, 1e5,  &ib_bias_amp_z, CURR_BIAS_AMP));
    V_[n_++] =(ib_bias_noa_p    = new FloatV("* ", "DB", rP_, "Add noa",              "A",      -1e5, 1e5,  &ib_bias_noa_z, CURR_BIAS_NOA));
    V_[n_++] =(ib_scale_amp_p   = new FloatV("* ", "SA", rP_, "Slr amp",              "A",      -1e5, 1e5,  &ib_scale_amp_z,CURR_SCALE_AMP));
    V_[n_++] =(ib_scale_noa_p   = new FloatV("* ", "SB", rP_, "Slr noa",              "A",      -1e5, 1e5,  &ib_scale_noa_z,CURR_SCALE_NOA));
    V_[n_++] =(ib_select_p      = new Int8tV("* ", "si", rP_, "curr sel mode",        "(-1=n, 0=auto, 1=M)", -1, 1, &ib_select_z, int8_t(FAKE_FAULTS)));
    V_[n_++] =(iflt_p         = new Uint16tV("* ", "if", rP_, "Fault buffer indx",    "uint",   0,nflt_+1,  &iflt_z,        nflt_,              false));
    V_[n_++] =(ihis_p         = new Uint16tV("* ", "ih", rP_, "Hist buffer indx",     "uint",   0,nhis_+1,  &ihis_z,        nhis_,              false));
    V_[n_++] =(inj_bias_p       = new FloatV("* ", "Xb", rP_, "Injection bias",       "A",      -1e5, 1e5,  &inj_bias_z,    0.));
    V_[n_++] =(isum_p         = new Uint16tV("* ", "is", rP_, "Summ buffer indx",     "uint",   0, NSUM+1,  &isum_z,        NSUM,               false));
    V_[n_++] =(modeling_p      = new Uint8tV("* ", "Xm", rP_, "Modeling bitmap",      "[0x]",   0,    255,  &modeling_z,    MODELING));
    V_[n_++] =(Mon_chm_p       = new Uint8tV("* ", "Bm", rP_, "Monitor battery",      "0=BB, 1=CH",0,   1,  &Mon_chm_z,     MON_CHEM));
    V_[n_++] =(nP_p             = new FloatV("* ", "BP", rP_, "Number parallel",      "units",  1e-6, 100,  &nP_z,          NP));
    V_[n_++] =(nS_p             = new FloatV("* ", "BS", rP_, "Number series",        "units",  1e-6, 100,  &nS_z,          NS));
    V_[n_++] =(preserving_p    = new Uint8tV("* ", "X?", rP_, "Preserving fault",     "T=Preserve",0,   1,  &preserving_z,  0,                  false));
    V_[n_++] =(Sim_chm_p       = new Uint8tV("* ", "Bs", rP_, "Sim battery",          "0=BB, 1=CH",0,   1,  &Sim_chm_z,     SIM_CHEM));
    V_[n_++] =(s_cap_mon_p      = new FloatV("* ", "SQ", rP_, "Scalar cap Mon",       "slr",    0,    1000, &s_cap_mon_z,   1.));
    V_[n_++] =(s_cap_sim_p      = new FloatV("* ", "Sq", rP_, "Scalar cap Sim",       "slr",    0,    1000, &s_cap_sim_z,   1.));
    V_[n_++] =(Tb_bias_hdwe_p   = new FloatV("* ", "Dt", rP_, "Bias Tb sensor",       "dg C",   -500, 500,  &Tb_bias_hdwe_z,TEMP_BIAS));
    V_[n_++] =(Time_now_p       = new ULongV("* ", "UT", rP_, "UNIX time epoch",      "sec",    0UL,  2100000000UL, &Time_now_z, 1669801880UL,  false));
    V_[n_++] =(Type_p          = new Uint8tV("* ", "Xt", rP_, "Inj type",             "1sn 2sq 3tr 4 1C, 5 -1C, 8cs",  0,   10,  &type_z, 0));
    V_[n_++] =(T_state_model_p  = new FloatV("* ", "ts", rP_, "Tb Sim rate lim mem",  "dg C",   -10,  70,   &T_state_model_z,RATED_TEMP,       false));
    V_[n_++] =(T_state_p        = new FloatV("* ", "tm", rP_, "Tb rate lim mem",      "dg C",   -10,  70,   &T_state_z,     RATED_TEMP,         false));
    V_[n_++] =(Vb_bias_hdwe_p   = new FloatV("* ", "Dc", rP_, "Bias Vb sensor",       "v",      -10,  70,   &Vb_bias_hdwe_z,VOLT_BIAS));
    V_[n_++] =(Vb_scale_p       = new FloatV("* ", "SV", rP_, "Scale Vb sensor",      "v",      -1e5, 1e5,  &Vb_scale_z,    VB_SCALE));
}

// Assign all save EERAM to RAM
#ifdef CONFIG_47L16_EERAM
    void SavedPars::load_all()
    {
        for (int i=0; i<n_; i++ ) V_[i]->get();
        
        for ( uint16_t i=0; i<nflt_; i++ ) fault_[i].get();
        for ( uint16_t i=0; i<nhis_; i++ ) history_[i].get();
    }
#endif

// Number of differences between nominal EERAM and actual (don't count integator memories because they always change)
int SavedPars::num_diffs()
{
    int n = 0;
    for (int i=0; i<n_; i++ ) if ( V_[i]->is_off() )  n++;
    return ( n );
}

// Configuration functions

// Print memory map
void SavedPars::mem_print()
{
    #ifdef CONFIG_47L16_EERAM
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        Serial.printf("Temp mem map print\n");
        for ( uint16_t i=0x0000; i<MAX_EERAM; i++ ) Serial.printf("0x%X ", rP_->read(i));
    #endif
}

// Print
void SavedPars::pretty_print(const boolean all)
{
    if ( all )
    {
        Serial.printf("saved (sp) all\n");
        for (int i=0; i<n_; i++ )
        {
            V_[i]->print();
        }
        // Serial.printf("history array (%d):\n", nhis_);
        // print_history_array();
        // print_fault_header();
        // Serial.printf("fault array (%d):\n", nflt_);
        // print_fault_array();
        // print_fault_header();
        #ifndef DEPLOY_PHOTON
            Serial.printf("Xm:\n");
            pretty_print_modeling();
        #endif
    }
    else
    {
        Serial.printf("saved (sp) diffs\n");
        uint8_t count = 0;
        for (int i=0; i<n_; i++ )
        {
            if ( V_[i]->is_off() )
            {
                count++;
                V_[i]->print();
            }
        }
        if ( count==0 ) Serial.printf("**none**\n\n");

        // Build integrity test
        while ( n_ != NSAV ) { delay(5000); Serial.printf("set NSAV=%d\n", n_); }
    }

    #ifdef CONFIG_47L16_EERAM
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        // Serial.printf("Temp mem map print\n");
        // mem_print();
    #endif
}

void SavedPars::pretty_print_modeling()
{
  bitMapPrint(pr.buff, sp.modeling(), 8);
  Serial.printf(" 0x%s\n", pr.buff);
  Serial.printf(" 0x128 ib_noa_dscn %d\n", mod_ib_noa_dscn());
  Serial.printf(" 0x64  ib_amp_dscn %d\n", mod_ib_amp_dscn());
  Serial.printf(" 0x32  vb_dscn %d\n", mod_vb_dscn());
  Serial.printf(" 0x16  temp_dscn %d\n", mod_tb_dscn());
  Serial.printf(" 0x8   tweak_test %d\n", tweak_test());
  Serial.printf(" 0x4   current %d\n", mod_ib());
  Serial.printf(" 0x2   voltage %d\n", mod_vb());
  Serial.printf(" 0x1   temp %d\n", mod_tb());

  time_long_2_str(Time_now_z, pr.buff);  // TODO: does this do anything/
  Serial.printf(" time %ld hms:  %s\n", Time_now_z, pr.buff);
}

// Print faults
void SavedPars::print_fault_array()
{
  uint16_t i = iflt_z;  // Last one written was iflt
  uint16_t n = 0;
  while ( ++n < nflt_+1 )
  {
    if ( ++i > (nflt_-1) ) i = 0; // circular buffer
    fault_[i].print("unit_f");
  }
}

// Print faults
void SavedPars::print_fault_header()
{
  Serial.printf ("fltb,  date,             time_ux,    Tb_h, vb_h, ibmh, ibnh, Tb, vb, ib, soc, soc_min, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
  Serial1.printf ("fltb,  date,             time_ux,    Tb_h, vb_h, ibmh, ibnh, Tb, vb, ib, soc, soc_min, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
}

// Print history
void SavedPars::print_history_array()
{
  int i = ihis_z;  // Last one written was ihis_z
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
            put_Mon_chm();
            break;

        case ( 3 ):
            put_Sim_chm();
            break;

        case ( 4 ):
            put_T_state();
            break;

        case ( 5 ):
            put_T_state_model();
            break;

        case ( 6 ):
            put_Time_now(max( Time_now_z, (unsigned long)Time.now()));  // If happen to connect to wifi (assume updated automatically), save new time
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
    for ( uint16_t i=0; i<nflt_; i++ )
    {
        fault_[i].put_nominal();
    }
 }
void SavedPars::reset_his()
{
    for ( uint16_t i=0; i<nhis_; i++ )
    {
        history_[i].put_nominal();
    }
 }

void SavedPars::set_nominal()
{
    Parameters::set_nominal();

    put_Inj_bias(float(0.));

    put_Preserving(uint8_t(0));
 }

void app_no() { };

void app_mon_chem()
{
    Serial.printf("app_mon_chem here\n");
    sp.Mon_chm_p->app();
}
