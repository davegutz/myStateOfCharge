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

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#include "local_config.h"
#include "Battery.h"
#include "hardware/SerialRAM.h"
#include "fault.h"
#include "command.h"

extern CommandPars cp;            // Various parameters shared at system level

#undef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_stored parameters at the bottom of Parameters.h are stored in SRAM.  Class initialized by *init in arg list.
*/
class Storage
{
public:
    Storage(){}

    Storage(const String &code, const String &description, const String &units, const boolean _uint8=false)
    {
        code_ = code;
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        is_eeram_ = false;
        units_ = units.substring(0, 10);
        is_uint8_t_ = _uint8;
    }

    Storage(const String &code, SerialRAM *ram, const String &description, const String &units, boolean _uint8=false)
    {
        code_ = code;
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        is_eeram_ = true;
        rP_ = ram;
        is_uint8_t_ = _uint8;
    }

    ~Storage(){}

    String code() { return code_; }

    const char* description() { return description_.c_str(); }

    const char* units() { return units_.c_str(); }

    // Placeholders
    virtual uint16_t assign_addr(uint16_t next){return next;}
    virtual boolean is_corrupt(){return false;};
    virtual boolean is_off(){return false;};
    virtual void print(){};
    virtual void set_default(){};

protected:
    String code_;
    SerialRAM *rP_;
    address16b addr_;
    String units_;
    String description_;
    boolean is_eeram_;
    boolean is_uint8_t_;
};


class DoubleStorage: public Storage
{
public:
    DoubleStorage(){}

    DoubleStorage(const String &code, const String &description, const String &units, const double min, const double max, double *store,
        const double _default=0, const boolean check_off=false):
        Storage(code, description, units, false)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
        set(*val_); // retained
    }

    DoubleStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const double min, const double max,
    double *store, const double _default=0, const boolean check_off=false):
        Storage(code, ram, description, units, false)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
    }

    ~DoubleStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(double);
    }

    double get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
        return *val_;
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("%s %s corrupt\n", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return *val_ != default_ && !check_off_;
    }

    double max_of() { return max_; }

    double min_of() { return min_; }

    double nominal() { return default_; }

    void print_str()
    {
        if ( !check_off_ )
            sprintf(cp.buffer, " %-20s %9.1f -> %9.1f, %10s (* %s)", description_.c_str(), default_, *val_, units_.c_str(), code_.c_str());
        else
            sprintf(cp.buffer, " %-33s %9.1f, %10s (* %s)", description_.c_str(), *val_, units_.c_str(), code_.c_str());
    }
    
    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help_str()
    {
        sprintf(cp.buffer, "* %s= %9.1f: (%-6.1f - %6.1f) [%6.1f] %s, %s", code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }
    
    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_adj_print(const double input)
    {
        print();
        print1();
        set(input);
        print();
        print1();
    }

    void set(double val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s set:: out of range\n", description_.c_str());
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_default()
    {
        if ( !check_off_ ) set(default_);
    }

protected:
    double *val_;
    double default_;
    double min_;
    double max_;
    boolean check_off_;
};


class FloatStorage: public Storage
{
public:
    FloatStorage(){}

    FloatStorage(const String &code, const String &description, const String &units, const float min, const float max, float *store,
    const float _default=0, const boolean check_off=false):
        Storage(code, description, units, false)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
        val_ = store;
        check_off_ = check_off;
        set(*val_); // retained
    }

    FloatStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max,
    float *store, const float _default=0, const boolean check_off=false):
        Storage(code, ram, description, units, false)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
    }

    ~FloatStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(float);
    }

    float get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
        return *val_;
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("%s %s corrupt\n", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return *val_ != default_ && !check_off_;
    }

    float max_of() { return max_; }

    float min_of() { return min_; }

    float nominal() { return default_; }
    
    void print_str()
    {
        if ( !check_off_ )
            sprintf(cp.buffer, " %-20s %9.3f -> %9.3f, %10s (* %s)", description_.c_str(), default_, *val_, units_.c_str(), code_.c_str());
        else
            sprintf(cp.buffer, " %-33s %9.3f, %10s (* %s)", description_.c_str(), *val_, units_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }
    
    void print_help_str()
    {
        sprintf(cp.buffer, "* %s= %9.3f: (%-6.3g-%6.3g) [%6.3f] %s, %s", code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_adj_print(const float input)
    {
        print();
        print1();
        set(input);
        print();
        print1();
    }

    void set(float val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s set:: out of range\n", description_.c_str());
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_default()
    {
        if ( !check_off_ ) set(default_);
    }

protected:
    float *val_;
    float default_;
    float min_;
    float max_;
    boolean check_off_;
};


class FloatNoStorage: public Storage
{
public:
    FloatNoStorage(){}

    FloatNoStorage(const String &code, const String &description, const String &units, const float min, const float max,
    const float _default=0):
        Storage(code, description, units, false)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    FloatNoStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max,
    const float _default=0):
        Storage(code, ram, description, units, false)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
    }

    ~FloatNoStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        return next;
    }

    float max_of() { return max_; }

    float min_of() { return min_; }

    float nominal() { return default_; }
    
    void print_str()
    {
        sprintf(cp.buffer, " %-20s %9.3f -> %9.3f, %10s (* %s)", description_.c_str(), default_, NAN, units_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help_str()
    {
        sprintf(cp.buffer, "* %s= %9.3f: (%-6.3g-%6.3g) [%6.3f] %s, %s", code_.c_str(), NAN, min_, max_, default_, description_.c_str(), units_.c_str());
    }
    
    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    virtual void set_default(){}

protected:
    float default_;
    float min_;
    float max_;
};


class IntStorage: public Storage
{
public:
    IntStorage(){}

    IntStorage(const String &code, const String &description, const String &units, const int min, const int max, int *store,
    const int _default=0, const boolean check_off=false):
        Storage(code, description, units, false)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
        val_ = store;
        check_off_ = check_off;
        set(*val_); // retained
    }

    IntStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const int min, const int max, int *store,
    const int _default=0, const boolean check_off=false):
        Storage(code, ram, description, units, false)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
    }

    ~IntStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(int);
    }

    int get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
        return *val_;
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("%s %s corrupt\n", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return *val_ != default_ && !check_off_;
    }

    int max_of() { return max_; }

    int min_of() { return min_; }

    int nominal() { return default_; }
    
    void print_str()
    {
        if ( !check_off_ )
            sprintf(cp.buffer, " %-20s %9d -> %9d, %10s (* %s)", description_.c_str(), default_, *val_, units_.c_str(), code_.c_str());
        else
            sprintf(cp.buffer, " %-33s %9d, %10s (* %s)", description_.c_str(), *val_, units_.c_str(), code_.c_str());
    }
    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help_str()
    {
      Serial.printf("* %s= %d: (%-6d - %6d) [%6d] %s, %s", code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }
    
    void print_adj_print(const int input)
    {
        print();
        print1();
        set(input);
        print();
        print1();
    }

    void set(int val)
    {
        if ( val>max_ || val<min_ )
            Serial.printf("%s set:: out of range\n", description_.c_str());
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_default()
    {
        if ( !check_off_ ) set(default_);
    }

protected:
    int *val_;
    int min_;
    int max_;
    int default_;
    boolean check_off_;
};


class Int8tStorage: public Storage
{
public:
    Int8tStorage(){}

    Int8tStorage(const String &code, const String &description, const String &units, const int8_t min, const int8_t max, int8_t *store,
    const int8_t _default=0, const boolean check_off=false):
        Storage(code, description, units, false)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
        val_ = store;
        check_off_ = check_off;
        set(*val_); // retained
    }

    Int8tStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const int8_t min, const int8_t max,
    int8_t *store, const int8_t _default=0, const boolean check_off=false):
        Storage(code, ram, description, units, false)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
    }

    ~Int8tStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(int8_t);
    }

    int8_t get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
        return *val_;
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("%s %s corrupt\n", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return *val_ != default_ && !check_off_;
    }

    int8_t max_of() { return max_; }

    int8_t min_of() { return min_; }

    int8_t nominal() { return default_; }
    
    void print_str()
    {
        if ( !check_off_ )
            sprintf(cp.buffer, " %-20s %9d -> %9d, %10s (* %s)", description_.c_str(), default_, *val_, units_.c_str(), code_.c_str());
        else
            sprintf(cp.buffer, " %-33s %9d, %10s (* %s)", description_.c_str(), *val_, units_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help_str()
    {
      sprintf(cp.buffer, "* %s= %d: (%-6d - %6d) [%6d] %s, %s", code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }
    
    void print_adj_print(const int8_t input)
    {
        print();
        print1();
        set(input);
        print();
        print1();
    }

    void set(int8_t val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s set:: out of range\n", description_.c_str());
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_default()
    {
        if ( !check_off_ ) set(default_);
    }

protected:
    int8_t *val_;
    int8_t min_;
    int8_t max_;
    int8_t default_;
    boolean check_off_;
};


class Uint8tStorage: public Storage
{
public:
    Uint8tStorage(){}

    Uint8tStorage(const String &code, const String &description, const String &units, const uint8_t min, const uint8_t max, uint8_t *store,
    const uint8_t _default=0, const boolean check_off=false):
        Storage(code, description, units, true)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
        val_ = store;
        check_off_ = check_off;
        set(*val_); // retained
    }

    Uint8tStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const uint8_t min, const uint8_t max,
    uint8_t *store, const uint8_t _default=0, const boolean check_off=false):
        Storage(code, ram, description, units, true)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
    }

    ~Uint8tStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(uint8_t);
    }

    uint8_t get()
    {
        if ( is_eeram_ )
            *val_ = rP_->read(addr_.a16);
        return *val_;
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("%s %s corrupt\n", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return *val_ != default_ && !check_off_;
    }

    uint8_t max_of() { return max_; }

    uint8_t min_of() { return min_; }

    uint8_t nominal() { return default_; }
    
    void print_str()
    {
        if ( !check_off_ )
            sprintf(cp.buffer, " %-20s %9d -> %9d, %10s (* %s)", description_.c_str(), default_, *val_, units_.c_str(), code_.c_str());
        else
            sprintf(cp.buffer, " %-33s %9d, %10s (* %s)", description_.c_str(), *val_, units_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help_str()
    {
        sprintf(cp.buffer, "* %s= %d: (%-6d - %6d) [%6d] %s, %s", code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_adj_print(const uint8_t input)
    {
        print();
        print1();
        set(input);
        print();
        print1();
    }
   
    void set(uint8_t val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s set:: out of range\n", description_.c_str());
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
        }
    }

    virtual void set_default()
    {
        if ( !check_off_ ) set(default_);
    }

protected:
    uint8_t *val_;
    uint8_t min_;
    uint8_t max_;
    uint8_t default_;
    boolean check_off_;
};

class ULongStorage: public Storage
{
public:
    ULongStorage(){}

    ULongStorage(const String &code, const String &description, const String &units, const unsigned long min, const unsigned long max, unsigned long *store,
    const unsigned long _default=0, const boolean check_off=true):
        Storage(code, description, units, true)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
        val_ = store;
        check_off_ = check_off;
        set(*val_); // retained
    }


    ULongStorage(const String &code, SerialRAM *ram, const String &description, const String &units, const unsigned long min, const unsigned long max,
    unsigned long *store, const unsigned long _default=0, const boolean check_off=true):
        Storage(code, ram, description, units, true)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        check_off_ = check_off;
    }

    ~ULongStorage(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(unsigned long);
    }

    unsigned long get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
            Time.setTime(*val_);
        }
        return *val_;
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("%s %s corrupt\n", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return *val_ != default_ && !check_off_;
    }

    unsigned long max_of() { return max_; }

    unsigned long min_of() { return min_; }

    unsigned long nominal() { return default_; }

    void print_str()
    {
        if ( !check_off_ )
            sprintf(cp.buffer, " %-18s %10d -> %10d, %10s (* %s)", description_.c_str(), (int)default_, (int)*val_, units_.c_str(), code_.c_str());
        else
            sprintf(cp.buffer, " %-32s %10d, %10s (* %s)", description_.c_str(), (int)*val_, units_.c_str(), code_.c_str());
    }
    
    void print()
    {
        print_str();
        Serial.printf("%s\n", cp.buffer);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_help_str()
    {
        sprintf(cp.buffer, "* %s= %ld: (%-6ld - %6ld) [%6ld] %s, %s", code_.c_str(), min_, max_, default_, *val_, description_.c_str(), units_.c_str());

    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", cp.buffer);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", cp.buffer);
    }

    void print_adj_print(const unsigned long input)
    {
        print();
        print1();
        set(input);
        print();
        print1();
    }
   
    void set(unsigned long val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s set:: out of range\n", description_.c_str());
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
        Time.setTime(*val_);
    }

    virtual void set_default()
    {
        if ( !check_off_ ) set(default_);
    }

protected:
    unsigned long *val_;
    unsigned long min_;
    unsigned long max_;
    unsigned long default_;
    boolean check_off_;
};


// class FloatVariable : public Storage <float>
// {
// public:
//     FloatVariable(){}

//     FloatVariable(const String &description, const String &units, const float _default)
//     {
//         Storage <float>(description, units, _default, false);
//     }

//     FloatVariable(SerialRAM *ram, const String &description, const String &units, const float _default)
//     {
//         Storage <float>(ram, description, units, _default, false);
//     }

//     ~FloatVariable(){}
// protected:
// };


// Corruption test
template <typename T>
boolean is_val_corrupt(T val, T minval, T maxval)
{
    return isnan(val) || val < minval || val > maxval;
}


// Definition of structure to be saved in EERAM.  Many are needed to calibrate.  Others are
// needed to allow testing with resets.  Others allow application to remember dynamic
// tweaks.  Default values below are important:  they prevent junk
// behavior on initial build. Don't put anything in here that you can't live with normal running
// because could get set by testing and forgotten.  Not reset by hard reset
// SavedPars Class
class SavedPars
{
public:
    SavedPars();
    SavedPars(Flt_st *hist, const uint8_t nhis, Flt_st *faults, const uint8_t nflt);
    SavedPars(SerialRAM *ram);
    ~SavedPars();
    friend Sensors;
    friend BatteryMonitor;
    friend BatterySim;

    // operators

    // parameter list
    float Amp() { return Amp_stored; }
    float Cutback_gain_sclr() { return Cutback_gain_sclr_stored; }
    int Debug() { return Debug_stored;}
    double Delta_q() { return Delta_q_stored;}
    double Delta_q_model() { return Delta_q_model_stored;}
    float Dw() { return Dw_stored; }
    float Freq() { return Freq_stored; }
    uint8_t Mon_chm() { return Mon_chm_stored; }
    float S_cap_mon() { return S_cap_mon_stored; }
    float S_cap_sim() { return S_cap_sim_stored; }
    uint8_t Sim_chm() { return Sim_chm_stored; }
    float Ib_bias_all() { return Ib_bias_all_stored; }
    float Ib_bias_amp() { return Ib_bias_amp_stored; }
    float Ib_bias_noa() { return Ib_bias_noa_stored; }
    float ib_scale_amp() { return Ib_scale_amp_stored; }
    float ib_scale_noa() { return Ib_scale_noa_stored; }
    int8_t Ib_select() { return Ib_select_stored; }
    int Iflt() { return Iflt_stored; }
    int Ihis() { return Ihis_stored; }
    float Inj_bias() { return Inj_bias_stored; }
    int Isum() { return Isum_stored; }
    uint8_t Modeling() { return Modeling_stored; }
    float nP() { return nP_stored; }
    float nS() { return nS_stored; }
    uint8_t Preserving() { return Preserving_stored; }
    float Tb_bias_hdwe() { return Tb_bias_hdwe_stored; }
    unsigned long Time_now() { return Time_now_stored; }
    uint8_t type() { return Type_stored; }
    float T_state() { return T_state_stored; }
    float T_state_model() { return T_state_model_stored; }
    float Vb_bias_hdwe() { return Vb_bias_hdwe_stored; }
    float Vb_scale() { return Vb_scale_stored; }

    // functions
    boolean is_corrupt();
    void large_reset() { reset_pars(); reset_flt(); reset_his(); }
    void reset_flt();
    void reset_his();
    void reset_pars();
    boolean mod_all_dscn() { return ( 111<Modeling() ); }                // Bare all
    boolean mod_any() { return ( mod_ib() || mod_tb() || mod_vb() ); }  // Modeing any
    boolean mod_any_dscn() { return ( 15<Modeling() ); }                 // Bare any
    boolean mod_ib() { return ( 1<<2 & Modeling() || mod_ib_all_dscn() ); }  // Using Sim as source of ib
    boolean mod_ib_all_dscn() { return ( 191<Modeling() ); }             // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_amp_dscn() { return ( 1<<6 & Modeling() ); }          // Nothing connected to amp ib sensors in I2C on SDA/SCL
    boolean mod_ib_any_dscn() { return ( mod_ib_amp_dscn() || mod_ib_noa_dscn() ); }  // Nothing connected to ib sensors in I2C on SDA/SCL
    boolean mod_ib_noa_dscn() { return ( 1<<7 & Modeling() ); }          // Nothing connected to noa ib sensors in I2C on SDA/SCL
    boolean mod_none() { return ( 0==Modeling() ); }                     // Using all
    boolean mod_none_dscn() { return ( 16>Modeling() ); }                // Bare nothing
    boolean mod_tb() { return ( 1<<0 & Modeling() || mod_tb_dscn() ); }  // Using Sim as source of tb
    boolean mod_tb_dscn() { return ( 1<<4 & Modeling() ); }              // Nothing connected to one-wire Tb sensor on D6
    boolean mod_vb() { return ( 1<<1 & Modeling() || mod_vb_dscn() ); }  // Using Sim as source of vb
    boolean mod_vb_dscn() { return ( 1<<5 & Modeling() ); }              // Nothing connected to vb on A1
    // get
    #ifdef CONFIG_47L16
        float get_Amp() { return Amp_p->get(); }
        float get_Cutback_gain_sclr() { return Cutback_gain_sclr_p->get(); }
        int get_Debug() { return Debug_p->get(); }
        double get_Delta_q() { return Delta_q_p->get(); }
        double get_Delta_q_model() { return Delta_q_model_p->get(); }
        float get_Dw() { return Dw_p->get(); }
        float get_Freq() { return Freq_p->get(); }
        float get_Ib_bias_all() { return Ib_bias_all_p->get(); }  // TODO:  should these be Ib_bias_stored
        float get_Ib_bias_amp() { return Ib_bias_amp_p->get(); }
        float get_Ib_bias_noa() { return Ib_bias_noa_p->get(); }
        float get_Ib_scale_amp() { return Ib_scale_amp_p->get(); }
        float get_Ib_scale_noa() { return Ib_scale_noa_p->get(); }
        int8_t get_Ib_select() { return Ib_select_p->get(); }
        int get_Iflt() { return Iflt_stored; }
        int get_Ihis() { return Ihis_stored; }
        int get_Isum() { return Isum_stored; }
        float get_Inj_bias() { return Inj_bias_stored; }
        uint8_t get_Modeling() { return Modeling_p->get(); }
        uint8_t get_Mon_chm() { return Mon_chm_p->get(); }
        uint8_t get_nP() { return nP_p->get(); }
        uint8_t get_nS() { return nS_p->get(); }
        uint8_t get_Preserving() { return Preserving_stored; }
        uint8_t get_Sim_chm() { return Sim_chm_p->get(); }
        float get_S_cap_mon() { return S_cap_mon_p->get(); }  // TODO:  should these be S_cap_mon_stored
        float get_S_cap_sim() { return S_cap_sim_p->get(); }  // TODO:  should these be S_cap_sim_stored
        float get_Tb_bias_hdwe() { return Tb_bias_hdwe_p->get(); }  // TODO:  should these be Tb_bias_hdwe_stored
        unsigned long get_Time_now() { return Time_now_p->get(); }  // TODO:  should these be Time_now_stored
        uint8_t get_Type() { return Type_p->get(); }
        float get_T_state() { return T_state_p->get(); }
        float get_T_state_model() { return T_state_model_p->get(); }
        float get_Vb_bias_hdwe() { return Vb_bias_hdwe_p->get(); }  // TODO:  should these be Vb_bias_hdwe_stored
        float get_Vb_scale() { return Vb_scale_p->get(); }  // TODO:  should these be Vb_scale_stored

        void get_fault(const uint8_t i) { fault_[i].get(); }
        void get_history(const uint8_t i) { history_[i].get(); }
        uint16_t next() { return next_; }
        void load_all();
    #else
        float get_Amp() { return Amp_p->get(); }
        float get_Cutback_gain_sclr() { return Cutback_gain_sclr_p->get(); }
        int get_Debug() { return Debug_p->get(); }
        double get_Delta_q() { return Delta_q_p->get(); }
        double get_Delta_q_model() { return Delta_q_model_p->get(); }
        float get_Dw() { return Dw_p->get(); }
        float get_Freq() { return Freq_p->get(); }
        float get_Ib_bias_all() { return Ib_bias_all_p->get(); }  // TODO:  should these be Ib_bias_stored
        double get_Ib_select() { return Ib_select_p->get(); }
        int get_Iflt() { return Iflt_stored; }
        int get_Ihis() { return Ihis_stored; }
        float get_Inj_bias() { return Inj_bias_stored; }
        int get_Isum() { return Isum_stored; }
        uint8_t get_Modeling() { return Modeling_p->get(); }
        uint8_t get_Mon_chm() { return Mon_chm_p->get(); }
        uint8_t get_nP() { return nP_p->get(); }
        uint8_t get_nS() { return nS_p->get(); }
        uint8_t get_Preserving() { return Preserving_p->get(); }
        float get_S_cap_mon() { return S_cap_mon_p->get(); }  // TODO:  should these be S_cap_mon_stored
        float get_S_cap_sim() { return S_cap_sim_p->get(); }  // TODO:  should these be S_cap_sim_stored
        uint8_t get_Sim_chm() { return Sim_chm_p->get(); }
        float get_Tb_bias_hdwe() { return Tb_bias_hdwe_p->get(); }  // TODO:  should these be Tb_bias_hdwe_stored
        unsigned long get_Time_now() { return Time_now_p->get(); }  // TODO:  should these be Time_now_stored
        uint8_t get_Type() { return Type_p->get(); }  // TODO:  should these be Type_stored
        float get_T_state() { return T_state_p->get(); }
        float get_T_state_model() { return T_state_model_p->get(); }
        float get_Vb_bias_hdwe() { return Vb_bias_hdwe_p->get(); }  // TODO:  should these be Vb_bias_hdwe_stored
        float get_Vb_scale() { return Vb_scale_p->get(); }  // TODO:  should these be Ib_bias_stored
    #endif
    //
    void mem_print();
    uint16_t nflt() { return nflt_; }
    uint16_t nhis() { return nhis_; }
    uint16_t nsum() { return nsum_; }
    void nominalize_fault_array();
    void nominalize_history_array();
    int num_diffs();
    void pretty_print(const boolean all);
    void print_fault_array();
    void print_fault_header();
    void print_history_array();
    // put
    #ifndef CONFIG_47L16
        void put_all_dynamic();
        void put_Amp(const float input) { Amp_p->set(input); }
        void put_Cutback_gain_sclr(const float input) { Cutback_gain_sclr_p->set(input); }
        void put_Debug(const int input) { Debug_p->set(input); }
        void put_Delta_q(const double input) { Delta_q_p->set(input); }
        void put_Delta_q() {}
        void put_Delta_q_model(const double input) { Delta_q_model_p->set(input); }
        void put_Delta_q_model() {}
        void put_Dw(const float input) { Dw_p->set(input); }
        void put_Freq(const float input) { Freq_p->set(input); }
        void put_Ib_bias_all(const float input) { Ib_bias_all_p->set(input); }
        void put_Ib_bias_amp(const float input) { Ib_bias_amp_p->set(input); }
        void put_Ib_bias_noa(const float input) { Ib_bias_noa_p->set(input); }
        void put_Ib_scale_amp(const float input) { Ib_scale_amp_p->set(input); }
        void put_Ib_scale_noa(const float input) { Ib_scale_noa_p->set(input); }
        void put_Ib_select(const int8_t input) { Ib_select_p->set(input); }
        void put_Iflt(const int input) { Iflt_p->set(input); }
        void put_Ihis(const int input) { Ihis_p->set(input); }
        void put_Isum(const int input) { Isum_p->set(input); }
        void put_isum(const int input) { isum_ = input; }
        void put_Inj_bias(const float input) { Inj_bias_p->set(input); }
        void put_Modeling(const uint8_t input) { Modeling_p->set(input); Modeling_stored = Modeling();}
        void put_Mon_chm(const uint8_t input) { Mon_chm_p->set(input); }
        void put_Mon_chm() {}
        void put_nP(const float input) { nP_p->set(input); }
        void put_nS(const float input) { nS_p->set(input); }
        void put_Preserving(const uint8_t input) { Preserving_p->set(input); }
        void put_S_cap_mon(const float input) { S_cap_mon_p->set(input); }
        void put_S_cap_sim(const float input) { S_cap_sim_p->set(input); }
        void put_Sim_chm(const uint8_t input) { Sim_chm_p->set(input); }
        void put_Sim_chm() {}
        void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe_p->set(input); }
        void put_Time_now(const unsigned long input) { Time_now_p->set(input); }
        void put_Type(const uint8_t input) { Type_p->set(input); }
        void put_T_state(const float input) { T_state_p->set(input); }
        void put_T_state() {}
        void put_T_state_model(const float input) { T_state_model_p->set(input); }
        void put_T_state_model() {}
        void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe_p->set(input); }
        void put_Vb_scale(const float input) { Vb_scale_p->set(input); }

        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].copy_to_Flt_ram_from(input); }
    #else
        void put_all_dynamic();
        void put_Amp(const float input) { Amp_p->set(input); }
        void put_Cutback_gain_sclr(const float input) { Cutback_gain_sclr_p->set(input); }
        void put_Debug(const int input) { Debug_p->set(input); }
        void put_Delta_q(const double input) { Delta_q_p->set(input); }
        void put_Delta_q() {}
        void put_Delta_q_model(const double input) { Delta_q_model_p->set(input); }
        void put_Delta_q_model() {}
        void put_Dw(const float input) { Dw_p->set(input); }
        void put_Freq(const float input) { Freq_p->set(input); }
        void put_Ib_bias_all(const float input) { Ib_bias_all_p->set(input); }
        void put_Ib_bias_amp(const float input) { Ib_bias_amp_p->set(input); }
        void put_Ib_bias_noa(const float input) { Ib_bias_noa_p->set(input); }
        void put_Ib_scale_amp(const float input) { Ib_scale_amp_p->set(input); }
        void put_Ib_scale_noa(const float input) { Ib_scale_noa_p->set(input); }
        void put_Ib_select(const int8_t input) { Ib_select_p->set(input); }
        void put_Iflt(const int input) { Iflt_p->set(input); }
        void put_Ihis(const int input) { Ihis_p->set(input); }
        void put_Isum(const int input) { Isum_p->set(input); }
        void put_Inj_bias(const float input) { Inj_bias_p->set(input); }
        void put_Modeling(const uint8_t input) { Modeling_p->set(input); }
        void put_Mon_chm(const uint8_t input) { Mon_chm_p->set(input); }
        void put_Mon_chm() {}
        void put_nP(const float input) { nP_p->set(input); }
        void put_nS(const float input) { nS_p->set(input); }
        void put_Preserving(const uint8_t input) { Preserving_p->set(input); }
        void put_S_cap_mon(const float input) { S_cap_mon_p->set(input); }
        void put_S_cap_sim(const float input) { S_cap_sim_p->set(input); }
        void put_Sim_chm(const uint8_t input) { Sim_chm_p->set(input); }
        void put_Sim_chm() {}
        void put_Tb_bias_hdwe(const float input) { Tb_bias_hdwe_p->set(input); }
        void put_Time_now(const float input) { Time_now_p->set(input); }
        void put_Type(const uint8_t input) { Type_p->set(input); }
        void put_T_state(const float input) { T_state_p->set(input); }
        void put_T_state() { T_state_p->set(T_state_stored); }
        void put_T_state_model(const float input) { T_state_model_p->set(input); }
        void put_T_state_model() { T_state_p->set(T_state_model_stored); }
        void put_Vb_bias_hdwe(const float input) { Vb_bias_hdwe_p->set(input); }
        void put_Vb_scale(const float input) { Vb_scale_p->set(input); }

        void put_fault(const Flt_st input, const uint8_t i) { fault_[i].put(input); }
    #endif
    //
    Flt_st put_history(const Flt_st input, const uint8_t i);
    boolean tweak_test() { return ( 1<<3 & Modeling() ); } // Driving signal injection completely using software inj_bias 
    FloatStorage *Amp_p;
    FloatStorage *Cutback_gain_sclr_p;
    IntStorage *Debug_p;
    DoubleStorage *Delta_q_p;
    DoubleStorage *Delta_q_model_p;
    FloatStorage *Dw_p;
    FloatStorage *Freq_p;
    FloatStorage *Ib_bias_all_p;
    FloatNoStorage *Ib_bias_all_nan_p;
    FloatStorage *Ib_bias_amp_p;
    FloatStorage *Ib_bias_noa_p;
    FloatStorage *Ib_scale_amp_p;
    FloatStorage *Ib_scale_noa_p;
    Int8tStorage *Ib_select_p;
    IntStorage *Iflt_p;
    IntStorage *Ihis_p;
    FloatStorage *Inj_bias_p;
    IntStorage *Isum_p;
    Uint8tStorage *Modeling_p;
    Uint8tStorage *Mon_chm_p;
    FloatStorage *nP_p;
    FloatStorage *nS_p;
    Uint8tStorage *Preserving_p;
    FloatStorage *S_cap_mon_p;
    FloatStorage *S_cap_sim_p;
    Uint8tStorage *Sim_chm_p;
    FloatStorage *Tb_bias_hdwe_p;
    ULongStorage *Time_now_p;
    FloatStorage *T_state_p;
    FloatStorage *T_state_model_p;
    Uint8tStorage *Type_p;
    FloatStorage *Vb_bias_hdwe_p;
    FloatStorage *Vb_scale_p;

    // SRAM storage state "retained" in SOC_Particle.ino.  Very few elements
    float Amp_stored;
    float Cutback_gain_sclr_stored;
    int Debug_stored;
    double Delta_q_stored;
    double Delta_q_model_stored;
    float Dw_stored;
    float Freq_stored;
    float Ib_bias_all_stored;
    int8_t Ib_select_stored;
    float Ib_bias_amp_stored;
    float Ib_bias_noa_stored;
    float Ib_scale_amp_stored;
    float Ib_scale_noa_stored;
    int Iflt_stored;
    int Ihis_stored;
    float Inj_bias_stored;
    int Isum_stored;
    uint8_t Modeling_stored;
    uint8_t Mon_chm_stored;
    float nP_stored;
    float nS_stored;
    uint8_t Preserving_stored;
    float S_cap_mon_stored;
    float S_cap_sim_stored;
    uint8_t Sim_chm_stored;
    float Tb_bias_hdwe_stored;
    unsigned long Time_now_stored;
    uint8_t Type_stored;
    float T_state_stored;
    float T_state_model_stored;
    float Vb_bias_hdwe_stored;
    float Vb_scale_stored;

protected:
    int iflt_;              // Fault snap location.   Begins at -1 because first action is to increment iflt
    int ihis_;              // History location.   Begins at -1 because first action is to increment ihis
    float inj_bias_;        // Constant bias, A
    int isum_;              // Summary location.   Begins at -1 because first action is to increment isum
    uint8_t preserving_;    // Preserving fault buffer
    #ifndef CONFIG_47L16
        Flt_st *fault_;
        Flt_st *history_;
    #else
        SerialRAM *rP_;
        Flt_ram *fault_;
        Flt_ram *history_;
    #endif
    uint16_t next_;
    uint16_t nflt_;         // Length of Flt_ram array for fault snapshot
    uint16_t nhis_;         // Length of Flt_ram array for fault history
    uint16_t nsum_;         // Length of Sum array for history
    uint16_t size_;
    Storage *Z_[50];
};

#endif
