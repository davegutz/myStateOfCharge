#ifndef VARIABLE_H
#define VARIABLE_H

#include "parameters.h"

template <typename T>
class Variable
{
public:
    Variable(){};

    Variable(T* val_loc, const String &description, const String &units)
    {
        description_ = description;
        eeram_ = false;
        units_ = units;
        val_loc_ = val_loc;
    };

    Variable(T* val_loc, SerialRAM *ram, address16b addr, const String &description, const String &units)
    {
        addr_ = addr;
        description_ = description;
        eeram_ = true;
        units_ = units;
        rP_ = ram;
        val_loc_ = val_loc;
    };

    ~Variable(){};

    const char* description() { return description_.c_str(); }
    
    T get()
    {
        T value;
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
            rP_->put(addr_.a16, val);
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
};


class Vars
{
public:
    Vars(){};
    Vars(SavedPars sp, SerialRAM *ram);
    ~Vars(){};
    Variable <float> Freq;
    Variable <int8_t> Ib_select;
protected:
};

#endif
