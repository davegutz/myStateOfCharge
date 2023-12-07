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
    size_ = 0;
    // Memory map
    init_z();
}

SavedPars::SavedPars(SerialRAM *ram)
{
    rP_ = ram;
    next_ = 0x000;
    size_ = 0;
    nflt_ = uint16_t( NFLT ); 
    // Memory map
    init_z();

    #ifdef CONFIG_47L16_EERAM
        for ( int i=0; i<size_; i++ )
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
    Amp_p               = new FloatZ(  "Xf", rP_, "Inj amp",             "Amps pk",  -1e6, 1e6, &Amp_z, 0.);
    Cutback_gain_sclr_p = new FloatZ(  "Sk", rP_, "Cutback gain scalar", "slr",      -1e6, 1e6, &Cutback_gain_sclr_z, 1.);
    Debug_p             = new IntZ(    "v",  rP_, "Verbosity",           "int",      -128, 128, &Debug_z, 0);
    Delta_q_p           = new DoubleZ( "na", rP_, "Charge chg",          "C",        -1e8, 1e5, &Delta_q_z, 0., true);
    Delta_q_model_p     = new DoubleZ( "na", rP_, "Charge chg Sim",      "C",        -1e8, 1e5, &Delta_q_model_z, 0., true);
    Dw_p                = new FloatZ(  "Dw", rP_, "Tab mon adj",         "v",        -1e2, 1e2, &Dw_z, VTAB_BIAS);
    Freq_p              = new FloatZ(  "Xf", rP_, "Inj freq",            "Hz",        0.,  2.,  &Freq_z, 0.);
    Ib_bias_all_p       = new FloatZ(  "DI", rP_, "Del all",             "A",        -1e5, 1e5, &Ib_bias_all_z, CURR_BIAS_ALL);
    Ib_bias_all_nan_p   = new FloatNoZ("Di", rP_, "DI + reset",          "A",        -1e5, 1e5, CURR_BIAS_ALL);
    Ib_bias_amp_p       = new FloatZ(  "DA", rP_, "Add amp",             "A",        -1e5, 1e5, &Ib_bias_amp_z, CURR_BIAS_AMP);
    Ib_bias_noa_p       = new FloatZ(  "DB", rP_, "Add noa",             "A",        -1e5, 1e5, &Ib_bias_noa_z, CURR_BIAS_NOA);
    Ib_scale_amp_p      = new FloatZ(  "SA", rP_, "Slr amp",             "A",        -1e5, 1e5, &Ib_scale_amp_z, CURR_SCALE_AMP);
    Ib_scale_noa_p      = new FloatZ(  "SB", rP_, "Slr noa",             "A",        -1e5, 1e5, &Ib_scale_noa_z, CURR_SCALE_NOA);
    Ib_select_p         = new Int8tZ(  "si", rP_, "curr sel mode",       "(-1=n, 0=auto, 1=M)", -1, 1, &Ib_select_z, int8_t(FAKE_FAULTS));
    Iflt_p              = new Uint16tZ("na", rP_, "Fault buffer indx",   "uint",      0, nflt_+1, &Iflt_z, nflt_+1, true);
    Ihis_p              = new Uint16tZ("na", rP_, "Hist buffer indx",    "uint",      0, nhis_+1, &Ihis_z, nhis_+1, true);
    Inj_bias_p          = new FloatZ(  "Xb", rP_, "Injection bias",      "A",        -1e5, 1e5, &Inj_bias_z, 0.);
    Isum_p              = new Uint16tZ("na", rP_, "Summ buffer indx",    "uint",      0, NSUM+1,  &Isum_z, NSUM+1, true);
    Modeling_p          = new Uint8tZ( "Xm", rP_, "Modeling bitmap",     "[0x00000000]", 0, 255, &Modeling_z, MODELING);
    Mon_chm_p           = new Uint8tZ( "Bm", rP_, "Monitor battery",     "0=BB, 1=CH", 0, 1, &Mon_chm_z, MON_CHEM);
    nP_p                = new FloatZ(  "BP", rP_, "Number parallel",     "units",     1e-6, 100, &nP_z, NP);
    nS_p                = new FloatZ(  "BS", rP_, "Number series",       "units",     1e-6, 100, &nS_z, NS);
    Preserving_p        = new Uint8tZ( "X?", rP_, "Preserving fault",    "T=Preserve", 0, 1, &Preserving_z, 0, true);
    S_cap_mon_p         = new FloatZ(  "SQ", rP_, "Scalar cap Mon",      "slr",       0,  1000, &S_cap_mon_z, 1.);
    S_cap_sim_p         = new FloatZ(  "Sq", rP_, "Scalar cap Sim",      "slr",       0,  1000, &S_cap_sim_z, 1.);
    Sim_chm_p           = new Uint8tZ( "Bs", rP_, "Sim battery",         "0=BB, 1=CH", 0, 1, &Sim_chm_z, SIM_CHEM);
    Tb_bias_hdwe_p      = new FloatZ(  "Dt", rP_, "Bias Tb sensor",      "dg C",     -500, 500, &Tb_bias_hdwe_z, TEMP_BIAS);
    Time_now_p          = new ULongZ(  "UT", rP_, "UNIX tim since epoch","sec",       0UL, 2100000000UL, &Time_now_z, 1669801880UL, true);
    Type_p              = new Uint8tZ( "Xt", rP_, "Inj type",            "1sn 2sq 3tr 4 1C, 5 -1C, 8cs",      0,   10,  &Type_z, 0);
    T_state_p           = new FloatZ(  "na", rP_, "Tb rate lim mem",     "dg C",    -10,  70,  &T_state_z, RATED_TEMP, true);
    T_state_model_p     = new FloatZ(  "na", rP_, "Tb Sim rate lim mem", "dg C",    -10,  70,  &T_state_model_z, RATED_TEMP, true);
    Vb_bias_hdwe_p      = new FloatZ(  "Dc", rP_, "Bias Vb sensor",      "v",        -10,  70,  &Vb_bias_hdwe_z, VOLT_BIAS);
    Vb_scale_p          = new FloatZ(  "SV", rP_, "Scale Vb sensor",     "v",        -1e5, 1e5, &Vb_scale_z, VB_SCALE);
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
    Z_[size_++] = Iflt_p;
    Z_[size_++] = Ihis_p;
    Z_[size_++] = Inj_bias_p;
    Z_[size_++] = Isum_p;
    Z_[size_++] = Modeling_p;
    Z_[size_++] = Mon_chm_p;
    Z_[size_++] = nP_p;
    Z_[size_++] = nS_p;
    Z_[size_++] = Preserving_p;
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

// Corruption test on bootup.  Needed because retained parameter memory is not managed by the compiler as it relies on
// battery.  Small compilation changes can change where in this memory the program points, too
boolean SavedPars::is_corrupt()
{
    boolean corruption = false;
    for ( int i=0; i<size_; i++ ) corruption |= Z_[i]->is_corrupt();
    if ( corruption )
    {
        Serial.printf("\ncorrupt****\n");
        pretty_print(true);
    }
    return corruption;
}

// Assign all save EERAM to RAM
#ifdef CONFIG_47L16_EERAM
    void SavedPars::load_all()
    {
        for (int i=0; i<size_; i++ ) Z_[i]->get();
        
        for ( uint16_t i=0; i<nflt_; i++ ) fault_[i].get();
        for ( uint16_t i=0; i<nhis_; i++ ) history_[i].get();
    }
#endif

// Number of differences between nominal EERAM and actual (don't count integator memories because they always change)
int SavedPars::num_diffs()
{
    int n = 0;
    for (int i=0; i<size_; i++ ) if ( Z_[i]->is_off() )  n++;
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
    for (int i=0; i<size_; i++ ) if ( all || Z_[i]->is_off() )  Z_[i]->print();

    Serial.printf("Xm:\n");
    pretty_print_modeling();

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
void SavedPars::reset_pars()
{
    for ( uint16_t i=0; i<size_; i++ ) Z_[i]->set_default();

    put_Inj_bias(float(0.));

    put_Preserving(uint8_t(0));
 }
