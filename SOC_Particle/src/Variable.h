#ifndef VARIABLE_H
#define VARIABLE_H

#include "parameters.h"

template <typename T>
class Variable
{
public:
    Variable(){}

    Variable(T* val_loc, const String &description, const String &units, const boolean _uint8=false)
    {
        description_ = description;
        eeram_ = false;
        units_ = units;
        uint8_t_ = _uint8;
        val_loc_ = val_loc;
    }

    Variable(T* val_loc, SerialRAM *ram, address16b addr, const String &description, const String &units, boolean _uint8=false)
    {
        addr_ = addr;
        description_ = description;
        eeram_ = true;
        rP_ = ram;
        units_ = units;
        uint8_t_ = _uint8;
        val_loc_ = val_loc;
    }

    ~Variable(){}

    const char* description() { return description_.c_str(); }
    
    T get()
    {
        T value;
        if ( uint8_t_ )
            value = rP_->read(addr_.a16);
        else
            rP_->get(addr_.a16, value);
        *val_loc_ = value; 
        return *val_loc_;
    }

    String print()
    {
        return description_ + " = " + String(*val_loc_) + " " + units_;
    }
    
    void set(T val)
    {
        *val_loc_ = val;
        if ( eeram_ ) 
        {
            if ( uint8_t_ )
                rP_->write(addr_.a16, *val_loc_);
            else
                rP_->put(addr_.a16, *val_loc_);
        }
    }
    
    const char* units() { return units_.c_str(); }

protected:
    T* val_loc_;
    String units_;
    String description_;
    boolean eeram_;
    address16b addr_;
    SerialRAM *rP_;
    boolean uint8_t_;
};


void bitMapPrint(char *buf, const int16_t fw, const uint8_t num);


class Vars
{
public:
    Vars(){};
    Vars(SavedPars sp, SerialRAM *ram);
    ~Vars(){};
    Variable <float> Freq;
    Variable <int8_t> Ib_select;
    Variable <uint8_t> Modeling;
    void put_Modeling(const uint8_t input, Sensors *Sen);
    boolean mod_all_dscn() { return ( 111<Modeling.get() ); }                // Bare all
    boolean mod_any() { return ( mod_ib() || mod_tb() || mod_vb() ); }  // Modeing any
    boolean mod_any_dscn() { return ( 15<Modeling.get() ); }                 // Bare any
    boolean mod_ib() { return ( 1<<2 & Modeling.get() || mod_ib_all_dscn() ); }  // Using Sim as source of ib
    boolean mod_ib_all_dscn() { return ( 191<Modeling.get() ); }             // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_amp_dscn() { return ( 1<<6 & Modeling.get() ); }          // Nothing connected to amp ib sensors in I2C on SDA/SCL
    boolean mod_ib_any_dscn() { return ( mod_ib_amp_dscn() || mod_ib_noa_dscn() ); }  // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_noa_dscn() { return ( 1<<7 & Modeling.get() ); }          // Nothing connected to noa ib sensors in I2C on SDA/SCL
    boolean mod_none() { return ( 0==Modeling.get() ); }                     // Using all
    boolean mod_none_dscn() { return ( 16>Modeling.get() ); }                // Bare nothing
    boolean mod_tb() { return ( 1<<0 & Modeling.get() || mod_tb_dscn() ); }  // Using Sim as source of tb
    boolean mod_tb_dscn() { return ( 1<<4 & Modeling.get() ); }              // Nothing connected to one-wire Tb sensor on D6
    boolean mod_vb() { return ( 1<<1 & Modeling.get() || mod_vb_dscn() ); }  // Using Sim as source of vb
    boolean mod_vb_dscn() { return ( 1<<5 & Modeling.get() ); }              // Nothing connected to vb on A1
    boolean tweak_test() { return ( 1<<3 & Modeling.get() ); }               // Driving signal injection completely using software inj_bias 
protected:
};

#endif
