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

/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_z parameters at the bottom of Parameters.h are stored in SRAM
*/

// class SavedPars 
SavedPars::SavedPars()
{
  nflt_ = uint16_t( NFLT ); 
  nhis_ = uint16_t( NHIS ); 
  nsum_ = uint16_t( NSUM ); 
}

SavedPars::SavedPars(Flt_st *hist, const uint16_t nhis, Flt_st *faults, const uint16_t nflt)
{
    rP_ = NULL;
    nflt_ = nflt;
    nhis_ = nhis;
    nsum_ = int( NSUM );
    #ifndef CONFIG_47L16_EERAM
        history_ = hist;
        fault_ = faults;
    #endif
    n_ = 0;
    // Memory map
    init_z();
}

SavedPars::SavedPars(SerialRAM *ram)
{
    rP_ = ram;
    next_ = 0x000;
    n_ = 0;
    nflt_ = uint16_t( NFLT ); 
    // Memory map
    init_z();

    #ifdef CONFIG_47L16_EERAM
        for ( int i=0; i<n_; i++ )
        {
            next_ = Z_[i]->assign_addr(next_);
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
// operators
// functions

void SavedPars::init_z()
{
    // Memory map
    // Input definitions
    n_ = -1;
    Z_ = new Z*[50];
    Z_[n_] = (Amp_p            = new FloatZ(&n_, "* ", "Xa", rP_, "Inj amp",                  "Amps pk",-1e6, 1e6,  &Amp_z,           0));
    Z_[n_] = (cc_diff_sclr_p   = new FloatZ(&n_, "  ", "Fc", NULL,"Slr cc_diff thr ",         "slr",    0,    1000, &cc_diff_sclr,     1));
    Z_[n_] = (Cutback_gain_sclr_p=new FloatZ(&n_,"* ", "Sk", rP_, "Cutback gain scalar",      "slr",    -1e6, 1e6,  &Cutback_gain_sclr_z, 1));
    Z_[n_] = (cycles_inj_p     = new FloatZ(&n_, "  ", "XC", NULL,"Number prog cycle",        "float",  0,    1000, &cycles_inj,       0));
    Z_[n_] = (dc_dc_on_p     = new BooleanZ(&n_, "  ", "Xd", NULL,"DC-DC charger on",        "0=F, 1=T",0,    1,    &dc_dc_on,         false));
    Z_[n_] = (Debug_p            = new IntZ(&n_, "* ", "v",  rP_, "Verbosity",                "int",    -128, 128,  &Debug_z,         0));
    Z_[n_] = (Delta_q_model_p = new DoubleZ(&n_, "* ", "qs", rP_, "Charge chg Sim",           "C",      -1e8, 1e5,  &Delta_q_model_z, 0,                false));
    Z_[n_] = (Delta_q_p       = new DoubleZ(&n_, "* ", "qm", rP_, "Charge chg",               "C",      -1e8, 1e5,  &Delta_q_z,       0,                false ));
    Z_[n_] = (Dw_p             = new FloatZ(&n_, "* ", "Dw", rP_, "Tab mon adj",              "v",      -1e2, 1e2,  &Dw_z,            VTAB_BIAS));
    Z_[n_] = (eframe_mult_p   = new Uint8tZ(&n_, "  ", "DE", NULL,"EKF Multiframe rate x Dr", "uint",   0,    UINT8_MAX,  &eframe_mult, EKF_EFRAME_MULT));
    Z_[n_] = (fail_tb_p      = new BooleanZ(&n_, "  ", "Xu", NULL,"Ignore Tb & fail",         "1=Fail", false,true, &fail_tb,          0));
    Z_[n_] = (fake_faults_p  = new BooleanZ(&n_, "  ", "Ff", NULL,"Faults ignored",           "0=F, 1=T",0,    1,    &fake_faults,      FAKE_FAULTS));
    Z_[n_] = (Freq_p           = new FloatZ(&n_, "* ", "Xf", rP_, "Inj freq",                 "Hz",     0.,   2.,   &Freq_z,          0));
    Z_[n_] = (ib_amp_add_p     = new FloatZ(&n_, "  ", "Dm", NULL,"Amp signal add ",          "A",      -1000,1000, &ib_amp_add,       0));
    Z_[n_] = (Ib_bias_all_nan_p=new FloatNoZ(&n_,"* ", "Di", rP_, "DI + reset",               "A",      -1e5, 1e5,  CURR_BIAS_ALL));
    Z_[n_] = (Ib_bias_all_p    = new FloatZ(&n_, "* ", "DI", rP_, "Del all",                  "A",      -1e5, 1e5,  &Ib_bias_all_z, CURR_BIAS_ALL));
    Z_[n_] = (Ib_bias_amp_p    = new FloatZ(&n_, "* ", "DA", rP_, "Add amp",                  "A",      -1e5, 1e5,  &Ib_bias_amp_z, CURR_BIAS_AMP));
    Z_[n_] = (Ib_bias_noa_p    = new FloatZ(&n_, "* ", "DB", rP_, "Add noa",                  "A",      -1e5, 1e5,  &Ib_bias_noa_z, CURR_BIAS_NOA));
    Z_[n_] = (Ib_scale_amp_p   = new FloatZ(&n_, "* ", "SA", rP_, "Slr amp",                  "A",      -1e5, 1e5,  &Ib_scale_amp_z,CURR_SCALE_AMP));
    Z_[n_] = (Ib_scale_noa_p   = new FloatZ(&n_, "* ", "SB", rP_, "Slr noa",                  "A",      -1e5, 1e5,  &Ib_scale_noa_z,CURR_SCALE_NOA));
    Z_[n_] = (Ib_select_p      = new Int8tZ(&n_, "* ", "si", rP_, "curr sel mode",            "(-1=n, 0=auto, 1=M)", -1, 1, &Ib_select_z, int8_t(FAKE_FAULTS)));
    Z_[n_] = (Iflt_p         = new Uint16tZ(&n_, "* ", "if", rP_, "Fault buffer indx",        "uint",   0,    nflt_+1, &Iflt_z,     nflt_+1,            false));
    Z_[n_] = (Ihis_p         = new Uint16tZ(&n_, "* ", "ih", rP_, "Hist buffer indx",         "uint",   0,    nhis_+1, &Ihis_z,     nhis_+1,            false));
    Z_[n_] = (Inj_bias_p       = new FloatZ(&n_, "* ", "Xb", rP_, "Injection bias",           "A",      -1e5, 1e5,  &Inj_bias_z,    0.));
    Z_[n_] = (Isum_p         = new Uint16tZ(&n_, "* ", "is", rP_, "Summ buffer indx",         "uint",   0,    NSUM+1,&Isum_z,       NSUM+1,             false));
    Z_[n_] = (Modeling_p      = new Uint8tZ(&n_, "* ", "Xm", rP_, "Modeling bitmap",          "[0x00000000]",0, 255,  &Modeling_z,    MODELING));
    Z_[n_] = (Mon_chm_p       = new Uint8tZ(&n_, "* ", "Bm", rP_, "Monitor battery",          "0=BB, 1=CH",0,   1,    &Mon_chm_z,     MON_CHEM));
    Z_[n_] = (nP_p             = new FloatZ(&n_, "* ", "BP", rP_, "Number parallel",          "units",  1e-6, 100,  &nP_z,          NP));
    Z_[n_] = (nS_p             = new FloatZ(&n_, "* ", "BS", rP_, "Number series",            "units",  1e-6, 100,  &nS_z,          NS));
    Z_[n_] = (Preserving_p    = new Uint8tZ(&n_, "* ", "X?", rP_, "Preserving fault",         "T=Preserve",0,   1,    &Preserving_z,  0,                false));
    Z_[n_] = (print_mult_p    = new Uint8tZ(&n_, "  ", "DP", NULL,"Print multiplier x Dr",    "uint",   0,    UINT8_MAX, &print_mult, DP_MULT));
    Z_[n_] = (Sim_chm_p       = new Uint8tZ(&n_, "* ", "Bs", rP_, "Sim battery",              "0=BB, 1=CH",0,   1,    &Sim_chm_z,     SIM_CHEM));
    Z_[n_] = (S_cap_mon_p      = new FloatZ(&n_, "* ", "SQ", rP_, "Scalar cap Mon",           "slr",    0,    1000, &S_cap_mon_z,   1.));
    Z_[n_] = (S_cap_sim_p      = new FloatZ(&n_, "* ", "Sq", rP_, "Scalar cap Sim",           "slr",    0,    1000, &S_cap_sim_z,   1.));
    Z_[n_] = (s_t_sat_p        = new FloatZ(&n_, "  ", "Xs", NULL,"Scalar on T_SAT",          "slr",    0,    100,  &s_t_sat,       1));
    Z_[n_] = (tail_inj_p       = new ULongZ(&n_, "  ", "XT", NULL,"Tail end inj",             "ms",     0UL,  120000UL,&tail_inj,   0UL));
    Z_[n_] = (Tb_bias_hdwe_p   = new FloatZ(&n_, "* ", "Dt", rP_, "Bias Tb sensor",           "dg C",   500,  500,  &Tb_bias_hdwe_z,TEMP_BIAS));
    Z_[n_] = (Tb_bias_model_p  = new FloatZ(&n_, "  ", "D^", NULL,"Del model",                "deg C",  -50,  50,   &Tb_bias_model, TEMP_BIAS));
    Z_[n_] = (tb_stale_time_sclr_p=new FloatZ(&n_,"  ","Xv",NULL, "Scl Tb 1-wire stale pers","slr",     0,    100,  &tb_stale_time_sclr, 1));
    Z_[n_] = (Time_now_p       = new ULongZ(&n_, "* ", "UT", rP_, "UNIX tim since epoch",     "sec",    0UL,  2100000000UL, &Time_now_z, 1669801880UL,  false));
    Z_[n_] = (Type_p          = new Uint8tZ(&n_, "* ", "Xt", rP_, "Inj type",                 "1sn 2sq 3tr 4 1C, 5 -1C, 8cs",  0,   10,  &Type_z, 0));
    Z_[n_] = (T_state_model_p  = new FloatZ(&n_, "* ", "ts", rP_, "Tb Sim rate lim mem",      "dg C",   -10,  70,   &T_state_model_z, RATED_TEMP,       false));
    Z_[n_] = (T_state_p        = new FloatZ(&n_, "* ", "tm", rP_, "Tb rate lim mem",          "dg C",   -10,  70,   &T_state_z,     RATED_TEMP,         false));
    Z_[n_] = (until_q_p        = new ULongZ(&n_, "  ", "XQ", NULL,"Time until v0",            "ms",     0UL,  1000000UL,  &until_q, 0UL));
    Z_[n_] = (Vb_bias_hdwe_p   = new FloatZ(&n_, "* ", "Dc", rP_, "Bias Vb sensor",           "v",      -10,  70,   &Vb_bias_hdwe_z,VOLT_BIAS));
    Z_[n_] = (Vb_scale_p       = new FloatZ(&n_, "* ", "SV", rP_, "Scale Vb sensor",          "v",      -1e5, 1e5,  &Vb_scale_z,    VB_SCALE));
    Z_[n_] = (wait_inj_p       = new ULongZ(&n_, "  ", "XW", NULL,"Wait start inj",           "ms",     0UL,  120000UL, &wait_inj,  0UL));
    for ( uint8_t i=0; i<n_; i++ ) if ( !Z_[i]->is_eeram() ) Z_[i]->set_nominal();
}


// Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
// battery.  Small compilation changes can change where in this memory the program points, too
boolean SavedPars::is_corrupt()
{
    boolean corruption = false;
    for ( int i=0; i<n_; i++ ) corruption |= Z_[i]->is_corrupt();
    if ( corruption )
    {
        Serial.printf("\ncorrupt****\n");
        pretty_print(false);
    }
    return corruption;
}

// Assign all save EERAM to RAM
#ifdef CONFIG_47L16_EERAM
    void SavedPars::load_all()
    {
        for (int i=0; i<n_; i++ ) Z_[i]->get();
        
        for ( uint16_t i=0; i<nflt_; i++ ) fault_[i].get();
        for ( uint16_t i=0; i<nhis_; i++ ) history_[i].get();
    }
#endif

// Number of differences between nominal EERAM and actual (don't count integator memories because they always change)
int SavedPars::num_diffs()
{
    int n = 0;
    for (int i=0; i<n_; i++ ) if ( Z_[i]->is_off() )  n++;
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
        // Serial.printf("history array (%d):\n", nhis_);
        // print_history_array();
        // print_fault_header();
        // Serial.printf("fault array (%d):\n", nflt_);
        // print_fault_array();
        // print_fault_header();
    }
    Serial.printf("saved (sp): all=%d\n", all);
    Serial.printf("             defaults    current EERAM values\n");
    for (int i=0; i<n_; i++ ) if ( all || Z_[i]->is_off() )  Z_[i]->print();

    #ifndef DEPLOY_PHOTON
        Serial.printf("Xm:\n");
        pretty_print_modeling();
    #endif

    #ifdef CONFIG_47L16_EERAM
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        // Serial.printf("Temp mem map print\n");
        // mem_print();
    #endif
}

void SavedPars::pretty_print_modeling()
{
  bitMapPrint(pr.buff, sp.Modeling(), 8);
  Serial.printf(" 0x%s\n", pr.buff);
  Serial.printf(" 0x128 ib_noa_dscn %d\n", mod_ib_noa_dscn());
  Serial.printf(" 0x64  ib_amp_dscn %d\n", mod_ib_amp_dscn());
  Serial.printf(" 0x32  vb_dscn %d\n", mod_vb_dscn());
  Serial.printf(" 0x16  temp_dscn %d\n", mod_tb_dscn());
  Serial.printf(" 0x8   tweak_test %d\n", tweak_test());
  Serial.printf(" 0x4   current %d\n", mod_ib());
  Serial.printf(" 0x2   voltage %d\n", mod_vb());
  Serial.printf(" 0x1   temp %d\n", mod_tb());
}

// Print only the volatile paramters (non-eeram)
void SavedPars::pretty_print_volatile(const boolean all)
{
    Serial.printf("volatile all:\n");
    Serial.printf("             defaults    current Volatile values\n");
    Serial.printf("adjust parameters(ap):\n");
    #ifndef DEPLOY_PHOTON
        if ( all )
        {
            for (int i=0; i<n_; i++ )
            {
                if ( !(Z_[i]->is_eeram()) )
                {
                    Z_[i]->print();
                }
            }
        }
    #endif
    Serial.printf("volatile off:\n");
    Serial.printf("             defaults    current Volatile values\n");
    for (int i=0; i<n_; i++ )
    {
        if ( !(Z_[i]->is_eeram()) )
        {
            if ( all || Z_[i]->is_off() )
            {
                Z_[i]->print();
            }
        }
    }
}

// Print faults
void SavedPars::print_fault_array()
{
  uint16_t i = Iflt_z;  // Last one written was iflt
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
  Serial.printf ("fltb,  date,                time,    Tb_h, vb_h, ibah, ibnh, Tb, vb, ib, soc, soc_min, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
  Serial1.printf ("fltb,  date,                time,    Tb_h, vb_h, ibah, ibnh, Tb, vb, ib, soc, soc_min, soc_ekf, voc, voc_stat, e_w_f, fltw, falw,\n");
}

// Print history
void SavedPars::print_history_array()
{
  int i = Ihis_z;  // Last one written was ihis
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
            put_Delta_q();
            break;

        case ( 1 ):
            put_Delta_q_model();
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
    for ( uint16_t i=0; i<n_; i++ ) Z_[i]->set_nominal();

    put_Inj_bias(float(0.));

    put_Preserving(uint8_t(0));
 }
