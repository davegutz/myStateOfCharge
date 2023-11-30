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
#include "Variable.h"

/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_stored parameters at the bottom of Parameters.h are stored in SRAM
*/

// class SavedPars 
SavedPars::SavedPars()
{
  nflt_ = int( NFLT ); 
  nhis_ = int( NHIS ); 
}

#ifndef CONFIG_47L16
SavedPars::SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt)
{
    nhis_ = nhis;
    nflt_ = nflt;
    history_ = hist;
    fault_ = faults;
    size_ = 0;

    // Input definitions
    Amp_p               = new FloatStorage(  "Xf", "Inj amp",             "Amps pk",  -1e6, 1e6, &Amp_stored, 0.);
    Cutback_gain_sclr_p = new FloatStorage(  "Sk", "Cutback gain scalar", "slr",      -1e6, 1e6, &Cutback_gain_sclr_stored, 1.);
    Debug_p             = new IntStorage(    "v",  "Verbosity",           "int",      -128, 128, &Debug_stored, 0);
    Delta_q_p           = new DoubleStorage( "na", "Charge chg",          "C",        -1e8, 1e5, &Delta_q_stored, 0., true);
    Delta_q_model_p     = new DoubleStorage( "na", "Charge chg Sim",      "C",        -1e8, 1e5, &Delta_q_model_stored, 0., true);
    Dw_p                = new FloatStorage(  "Dw", "Tab mon adj",         "v",        -1e2, 1e2, &Dw_stored, VTAB_BIAS);
    Freq_p              = new FloatStorage(  "Xf", "Inj freq",            "Hz",        0.,  2.,  &Freq_stored, 0.);
    Ib_bias_all_p       = new FloatStorage(  "DI", "Add all",             "A",        -1e5, 1e5, &Ib_bias_all_stored, CURR_BIAS_ALL);
    Ib_bias_all_nan_p   = new FloatNoStorage("Di", "DI + reset",          "A",        -1e5, 1e5, CURR_BIAS_ALL);
    Ib_bias_amp_p       = new FloatStorage(  "DA", "Add amp",             "A",        -1e5, 1e5, &Ib_bias_amp_stored, CURR_BIAS_AMP);
    Ib_bias_noa_p       = new FloatStorage(  "DB", "Add noa",             "A",        -1e5, 1e5, &Ib_bias_noa_stored, CURR_BIAS_NOA);
    Ib_scale_amp_p      = new FloatStorage(  "SA", "Slr amp",             "A",        -1e5, 1e5, &Ib_scale_amp_stored, CURR_SCALE_AMP);
    Ib_scale_noa_p      = new FloatStorage(  "SB", "Slr noa",             "A",        -1e5, 1e5, &Ib_scale_noa_stored, CURR_SCALE_NOA);
    Ib_select_p         = new Int8tStorage(  "si", "curr sel mode",       "(-1=n, 0=auto, 1=M)", -1, 1, &Ib_select_stored, int8_t(FAKE_FAULTS));
    Modeling_p          = new Uint8tStorage( "Xm", "Modeling bitmap",     "[0x00000000]", 0, 255, &Modeling_stored, MODELING);
    Mon_chm_p           = new Uint8tStorage( "Bm", "Monitor battery",     "0=BB, 1=CH", 0, 1, &Mon_chm_stored, MON_CHEM);
    nP_p                = new FloatStorage(  "BP", "Number parallel",     "units",     1e-6, 100, &nP_stored, NP);
    nS_p                = new FloatStorage(  "BS", "Number series",       "units",     1e-6, 100, &nS_stored, NS);
    S_cap_mon_p         = new FloatStorage(  "SQ", "Scalar cap Mon",      "slr",       0,  1000, &S_cap_mon_stored, 1.);
    S_cap_sim_p         = new FloatStorage(  "Sq", "Scalar cap Sim",      "slr",       0,  1000, &S_cap_sim_stored, 1.);
    Sim_chm_p           = new Uint8tStorage( "Bs", "Sim battery",         "0=BB, 1=CH", 0, 1, &Sim_chm_stored, SIM_CHEM);
    Tb_bias_hdwe_p      = new FloatStorage(  "Dt", "Bias Tb sensor",      "dg C",     -500, 500, &Tb_bias_hdwe_stored, TEMP_BIAS);
    Time_now_p          = new ULongStorage(  "UT", "UNIX tim since epoch","sec",       0UL, 3400000000UL, &Time_now_stored, 1669801880UL, true);
    Type_p              = new Uint8tStorage( "Xt", "Inj type",            "1sn 2sq 3tr 4 1C, 5 -1C, 8cs",      0,   10,  &Type_stored, 0);
    T_state_p            = new FloatStorage(  "na", "Tb rate lim mem",     "dg C",     -10,  70,  &T_state_stored, RATED_TEMP, false);
    T_state_model_p      = new FloatStorage(  "na", "Tb Sim rate lim mem", "dg C",     -10,  70,  &T_state_model_stored, RATED_TEMP, false);
    Vb_bias_hdwe_p      = new FloatStorage(  "Dc", "Bias Vb sensor",      "v",        -10,  70,  &Vb_bias_hdwe_stored, VOLT_BIAS);
    Vb_scale_p          = new FloatStorage(  "SV", "Scale Vb sensor",     "v",        -1e5, 1e5, &Vb_scale_stored, VB_SCALE);

    // Memory map
    Z_[size_++] = Amp_p;                
    Z_[size_++] = Cutback_gain_sclr_p;
    Z_[size_++] = Debug_p;
    Z_[size_++] = Delta_q_p;
    Z_[size_++] = Delta_q_model_p;
    Z_[size_++] = Dw_p;
    Z_[size_++] = Freq_p;
    Z_[size_++] = Ib_bias_all_p;
    Z_[size_++] = Ib_bias_all_nan_p;
    Z_[size_++] = Ib_bias_amp_p;
    Z_[size_++] = Ib_bias_noa_p;
    Z_[size_++] = Ib_scale_amp_p;
    Z_[size_++] = Ib_scale_noa_p;
    Z_[size_++] = Ib_select_p;
    Z_[size_++] = Modeling_p;
    Z_[size_++] = Mon_chm_p;
    Z_[size_++] = nP_p;
    Z_[size_++] = nS_p;
    Z_[size_++] = S_cap_mon_p;
    Z_[size_++] = S_cap_sim_p;
    Z_[size_++] = Sim_chm_p;
    Z_[size_++] = Tb_bias_hdwe_p;
    Z_[size_++] = Time_now_p;
    Z_[size_++] = Type_p;
    Z_[size_++] = T_state_p;
    Z_[size_++] = T_state_model_p;
    Z_[size_++] = Vb_bias_hdwe_p;
    Z_[size_++] = Vb_scale_p;
}
#else
SavedPars::SavedPars(SerialRAM *ram)
{
    next_ = 0x000;
    rP_ = ram;
    size_ = 0;

    // Input definitions
    Amp_p               = new FloatStorage(  "Xf", rP_, "Inj amp",             "Amps pk",  -1e6, 1e6, &Amp_stored, 0.);
    Cutback_gain_sclr_p = new FloatStorage(  "Sk", rP_, "Cutback gain scalar", "slr",      -1e6, 1e6, &Cutback_gain_sclr_stored, 1.);
    Debug_p             = new IntStorage(    "v",  rP_, "Verbosity",           "int",      -128, 128, &Debug_stored, 0);
    Delta_q_p           = new DoubleStorage( "na", rP_, "Charge chg",          "C",        -1e8, 1e5, &Delta_q_stored, 0., true);
    Delta_q_model_p     = new DoubleStorage( "na", rP_, "Charge chg Sim",      "C",        -1e8, 1e5, &Delta_q_model_stored, 0., true);
    Dw_p                = new FloatStorage(  "Dw", rP_, "Tab mon adj",         "v",        -1e2, 1e2, &Dw_stored, VTAB_BIAS);
    Freq_p              = new FloatStorage(  "Xf", rP_, "Inj freq",            "Hz",        0.,  2.,  &Freq_stored, 0.);
    Ib_bias_all_p       = new FloatStorage(  "DI", rP_, "Del all",             "A",        -1e5, 1e5, &Ib_bias_all_stored, CURR_BIAS_ALL);
    Ib_bias_all_nan_p   = new FloatNoStorage("Di",      "DI + reset",          "A",        -1e5, 1e5, CURR_BIAS_ALL);
    Ib_bias_amp_p       = new FloatStorage(  "DA", rP_, "Add amp",             "A",        -1e5, 1e5, &Ib_bias_amp_stored, CURR_BIAS_AMP);
    Ib_bias_noa_p       = new FloatStorage(  "DB", rP_, "Add noa",             "A",        -1e5, 1e5, &Ib_bias_noa_stored, CURR_BIAS_NOA);
    Ib_scale_amp_p      = new FloatStorage(  "SA", rP_, "Slr amp",             "A",        -1e5, 1e5, &Ib_scale_amp_stored, CURR_SCALE_AMP);
    Ib_scale_noa_p      = new FloatStorage(  "SB", rP_, "Slr noa",             "A",        -1e5, 1e5, &Ib_scale_noa_stored, CURR_SCALE_NOA);
    Ib_select_p         = new Int8tStorage(  "si", rP_, "curr sel mode",       "(-1=n, 0=auto, 1=M)", -1, 1, &Ib_select_stored, int8_t(FAKE_FAULTS));
    Modeling_p          = new Uint8tStorage( "Xm", rP_, "Modeling bitmap",     "[0x00000000]", 0, 255, &Modeling_stored, MODELING);
    Mon_chm_p           = new Uint8tStorage( "Bm", rP_, "Monitor battery",     "0=BB, 1=CH", 0, 1, &Mon_chm_stored, MON_CHEM);
    nP_p                = new FloatStorage(  "BP", rP_, "Number parallel",     "units",     1e-6, 100, &nP_stored, NP);
    nS_p                = new FloatStorage(  "BS", rP_, "Number series",       "units",     1e-6, 100, &nS_stored, NS);
    S_cap_mon_p         = new FloatStorage(  "SQ", rP_, "Scalar cap Mon",      "slr",       0,  1000, &S_cap_mon_stored, 1.);
    S_cap_sim_p         = new FloatStorage(  "Sq", rP_, "Scalar cap Sim",      "slr",       0,  1000, &S_cap_sim_stored, 1.);
    Sim_chm_p           = new Uint8tStorage( "Bs", rP_, "Sim battery",         "0=BB, 1=CH", 0, 1, &Sim_chm_stored, SIM_CHEM);
    Tb_bias_hdwe_p      = new FloatStorage(  "Dt", rP_, "Bias Tb sensor",      "dg C",     -500, 500, &Tb_bias_hdwe_stored, TEMP_BIAS);
    Time_now_p          = new ULongStorage(  "UT", rP_, "UNIX tim since epoch","sec",       0UL, 3400000000UL, &Time_now_stored, 1669801880UL, true);
    Type_p              = new Uint8tStorage( "Xt", rP_, "Inj type",            "1sn 2sq 3tr 4 1C, 5 -1C, 8cs",      0,   10,  &Type_stored, 0);
    T_state_p            = new FloatStorage(  "na", rP_, "Tb rate lim mem",     "dg C",    -10,  70,  &T_state_stored, RATED_TEMP, false);
    T_state_model_p      = new FloatStorage(  "na", rP_, "Tb Sim rate lim mem", "dg C",    -10,  70,  &T_state_model_stored, RATED_TEMP, false);
    Vb_bias_hdwe_p      = new FloatStorage(  "Dc", rP_, "Bias Vb sensor",      "v",        -10,  70,  &Vb_bias_hdwe_stored, VOLT_BIAS);
    Vb_scale_p          = new FloatStorage(  "SV", rP_, "Scale Vb sensor",     "v",        -1e5, 1e5, &Vb_scale_stored, VB_SCALE);

    // Memory map
    Z_[size_++] = Amp_p;                
    Z_[size_++] = Cutback_gain_sclr_p;
    Z_[size_++] = Debug_p;
    Z_[size_++] = Delta_q_p;
    Z_[size_++] = Delta_q_model_p;
    Z_[size_++] = Dw_p;
    Z_[size_++] = Freq_p;
    Z_[size_++] = Ib_bias_all_p;
    Z_[size_++] = Ib_bias_all_nan_p;
    Z_[size_++] = Ib_bias_amp_p;
    Z_[size_++] = Ib_bias_noa_p;
    Z_[size_++] = Ib_scale_amp_p;
    Z_[size_++] = Ib_scale_noa_p;
    Z_[size_++] = Ib_select_p;
    Z_[size_++] = Modeling_p;
    Z_[size_++] = Mon_chm_p;
    Z_[size_++] = nP_p;
    Z_[size_++] = nS_p;
    Z_[size_++] = S_cap_mon_p;
    Z_[size_++] = S_cap_sim_p;
    Z_[size_++] = Sim_chm_p;
    Z_[size_++] = Tb_bias_hdwe_p;
    Z_[size_++] = Time_now_p;
    Z_[size_++] = T_state_p;
    Z_[size_++] = T_state_model_p;
    Z_[size_++] = Type_p;
    Z_[size_++] = Vb_bias_hdwe_p;
    Z_[size_++] = Vb_scale_p;
    for ( int i=0; i<size_; i++ )
    {
        next_ = Z_[i]->assign_addr(next_);
    }
    iflt_eeram_.a16 =  next_;  next_ += sizeof(iflt_);
    ihis_eeram_.a16 =  next_;  next_ += sizeof(ihis_);
    inj_bias_eeram_.a16 =  next_;  next_ += sizeof(inj_bias_);
    isum_eeram_.a16 =  next_;  next_ += sizeof(isum_);
    preserving_eeram_.a16 =  next_;  next_ += sizeof(preserving_);

    nflt_ = int( NFLT ); 
    fault_ = new Flt_ram[nflt_];
    for ( int i=0; i<nflt_; i++ )
    {
        fault_[i].instantiate(rP_, &next_);
    }
    nhis_ = int( (MAX_EERAM - next_) / sizeof(Flt_st) ); 
    history_ = new Flt_ram[nhis_];
    for ( int i=0; i<nhis_; i++ )
    {
        history_[i].instantiate(rP_, &next_);
    }
}
#endif

SavedPars::~SavedPars() {}
// operators
// functions

// Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
// battery.  Small compilation changes can change where in this memory the program points, too
boolean SavedPars::is_corrupt()
{
    boolean corruption = false;
    for ( int i=0; i<size_; i++ ) corruption |= Z_[i]->is_corrupt();
    corruption = corruption ||
        is_val_corrupt(iflt_, -1, nflt_+1) ||
        is_val_corrupt(ihis_, -1, nhis_+1) ||
        is_val_corrupt(inj_bias_, float(-100.), float(100.)) ||
        is_val_corrupt(isum_, -1, NSUM+1) ||
        is_val_corrupt(preserving_, uint8_t(0), uint8_t(1));
    if ( corruption )
    {
        Serial.printf("corrupt****\n");
        pretty_print(true);
    }
    return corruption;
}

// Assign all save EERAM to RAM
#ifdef CONFIG_47L16
    void SavedPars::load_all()
    {
        get_Amp();
        get_Cutback_gain_sclr();
        get_Dw();
        get_Debug();
        get_Delta_q();
        get_Delta_q_model();
        get_Freq();
        get_Ib_bias_all();
        get_Ib_bias_amp();
        get_Ib_bias_noa();
        get_Ib_scale_amp();
        get_Ib_scale_noa();
        get_Ib_select();

        get_iflt();
        get_inj_bias();
        get_isum();

        get_Modeling();
        get_Mon_chm();
        get_nP();
        get_nS();

        get_preserving();

        get_Sim_chm();
        get_S_cap_mon();
        get_S_cap_sim();
        get_Tb_bias_hdwe();
        get_Time_now();
        get_Type();
        get_T_state();
        get_T_state_model();
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
    // if ( int(-1) != iflt_ )    //     n++;
    // if ( int(-1) != isum_ )    //     n++;
    // if ( uint8_t(0) != preserving_ )    //     n++;
    if ( Amp_p->is_off() ) n++;
    if ( Cutback_gain_sclr_p->is_off() ) n++;
    if ( Dw_p->is_off() ) n++;
    if ( Debug_p->is_off() ) n++;
    if ( Delta_q_p->is_off() ) n++;
    if ( Delta_q_model_p->is_off() ) n++;
    if ( Freq_p->is_off() ) n++;
    if ( Ib_bias_all_p->is_off() ) n++;
    if ( Ib_bias_amp_p->is_off() ) n++;
    if ( Ib_bias_noa_p->is_off() ) n++;
    if ( Ib_scale_amp_p->is_off() ) n++;
    if ( Ib_scale_noa_p->is_off() ) n++;
    if ( Ib_select_p->is_off() ) n++;
    
    if ( float(0.) != inj_bias_ ) n++;
    
    if ( Modeling_p->is_off() ) n++;
    if ( Mon_chm_p->is_off() ) n++;
    if ( nP_p->is_off() ) n++;
    if ( nS_p->is_off() ) n++;
    if ( S_cap_mon_p->is_off() ) n++;
    if ( S_cap_sim_p->is_off() ) n++;
    if ( Sim_chm_p->is_off() ) n++;
    if ( Tb_bias_hdwe_p->is_off() ) n++;
    if ( Time_now_p->is_off() ) n++;
    if ( T_state_p->is_off() ) n++;
    if ( T_state_model_p->is_off() ) n++;
    if ( Type_p->is_off() ) n++;
    if ( Vb_bias_hdwe_p->is_off() ) n++;
    if ( Vb_scale_p->is_off() ) n++;
    return ( n );
}

// Configuration functions

// Print memory map
void SavedPars::mem_print()
{
    #ifdef CONFIG_47L16
        Serial.printf("SavedPars::SavedPars - MEMORY MAP 0x%X < 0x%X\n", next_, MAX_EERAM);
        Serial.printf("Temp mem map print\n");
        for ( uint16_t i=0x0000; i<MAX_EERAM; i++ ) Serial.printf("0x%X ", rP_->read(i));
    #endif
}

// Print
void SavedPars::pretty_print(const boolean all)
{
    Serial.printf("saved (sp): all=%d\n", all);
    Serial.printf("             defaults    current EERAM values\n");
    if ( all )                                  Serial.printf(" iflt                           %d flt ptr\n", iflt_);
    if ( all || float(0.) != inj_bias_ )        Serial.printf(" inj_bias%7.3f  %7.3f *Xb<> A\n", 0., inj_bias_);
    if ( all )                                  Serial.printf(" isum                           %d tbl ptr\n", isum_);
    if ( all )                                  Serial.printf(" preserving %d  %d *Xm<>\n", uint8_t(0), preserving_);
    // if ( all )                                  Serial.printf(" time_now %d %s *U<> Unix time\n", (int)Time.now(), Time.timeStr().c_str());
    for (int i=0; i<size_; i++ ) if ( all || Z_[i]->is_off() )  Z_[i]->print();
    // if ( all )
    // {
    //     Serial.printf("history array (%d):\n", nhis_);
    //     print_history_array();
    //     print_fault_header();
    //     Serial.printf("fault array (%d):\n", nflt_);
    //     print_fault_array();
    //     print_fault_header();
    // }
    #ifdef CONFIG_47L16
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
            put_Time_now((unsigned long)Time.now());  // If happen to connect to wifi (assume updated automatically), save new time
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
    for ( int i=0; i<size_; i++ ) Z_[i]->set_default();
    // Amp_p->set_default();
    // Cutback_gain_sclr_p->set_default();
    // Debug_p->set_default();
    // Delta_q_p->set_default();
    // Delta_q_model_p->set_default();
    // Dw_p->set_default();
    // Freq_p->set_default();
    // Ib_bias_all_p->set_default();
    // Ib_bias_all_nan_p->set_default();
    // Ib_bias_amp_p->set_default();
    // Ib_bias_noa_p->set_default();
    // Ib_scale_amp_p->set_default();
    // Ib_scale_noa_p->set_default();
    // Ib_select_p->set_default();

    put_iflt(int(-1));
    put_ihis(int(-1));
    put_inj_bias(float(0.));
    put_isum(int(-1));
    
    // Modeling_p->set_default();
    // Mon_chm_p->set_default();
    
    // put_nP(float(NP));
    // put_nS(float(NS));
    put_preserving(uint8_t(0));

    // Sim_chm_p->set_default();
    // S_cap_mon_p->set_default();
    // S_cap_sim_p->set_default();
    // Tb_bias_hdwe_p->set_default();
    // Type_p->set_default();

    // put_t_last(float(RATED_TEMP));    
    // put_t_last_model(float(RATED_TEMP));  

    // Vb_bias_hdwe_p->set_default();
    // Vb_scale_p->set_default();
 }
