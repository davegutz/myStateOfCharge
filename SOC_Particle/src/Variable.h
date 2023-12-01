#ifndef VARIABLE_H
#define VARIABLE_H

#include "parameters.h"

void bitMapPrint(char *buf, const int16_t fw, const uint8_t num);


class Vars
{
public:
    Vars(){};
    Vars(SavedPars *sp, SerialRAM *ram);
    Vars(SavedPars *sp);
    ~Vars(){};
    boolean mod_all_dscn() { return ( 111<sp_->Modeling_z ); }               // Bare all
    boolean mod_any() { return ( mod_ib() || mod_tb() || mod_vb() ); }  // Modeing any
    boolean mod_any_dscn() { return ( 15<sp_->Modeling_z ); }                 // Bare any
    boolean mod_ib() { return ( 1<<2 & sp_->Modeling_z || mod_ib_all_dscn() ); }  // Using Sim as source of ib
    boolean mod_ib_all_dscn() { return ( 191<sp_->Modeling_z ); }             // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_amp_dscn() { return ( 1<<6 & sp_->Modeling_z ); }          // Nothing connected to amp ib sensors in I2C on SDA/SCL
    boolean mod_ib_any_dscn() { return ( mod_ib_amp_dscn() || mod_ib_noa_dscn() ); }  // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_noa_dscn() { return ( 1<<7 & sp_->Modeling_z ); }          // Nothing connected to noa ib sensors in I2C on SDA/SCL
    boolean mod_none() { return ( 0==sp_->Modeling_z ); }                     // Using all
    boolean mod_none_dscn() { return ( 16>sp_->Modeling_z ); }                // Bare nothing
    boolean mod_tb() { return ( 1<<0 & sp_->Modeling_z || mod_tb_dscn() ); }  // Using Sim as source of tb
    boolean mod_tb_dscn() { return ( 1<<4 & sp_->Modeling_z ); }              // Nothing connected to one-wire Tb sensor on D6
    boolean mod_vb() { return ( 1<<1 & sp_->Modeling_z || mod_vb_dscn() ); }  // Using Sim as source of vb
    boolean mod_vb_dscn() { return ( 1<<5 & sp_->Modeling_z ); }              // Nothing connected to vb on A1
    boolean tweak_test() { return ( 1<<3 & sp_->Modeling_z ); }               // Driving signal injection completely using software inj_bias 
    void put_Modeling(const uint8_t input, Sensors *Sen);
protected:
    SavedPars *sp_;
};

#endif
