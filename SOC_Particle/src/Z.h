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

#ifndef Z_H_
#define Z_H_

#include "hardware/SerialRAM.h"
#include "PrinterPars.h"
extern PrinterPars pr;  // Print buffer

#undef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef void (*fptr)();

/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_z parameters at the bottom of Parameters.h are stored in SRAM.  Class initialized by *init in arg list.
*/
class Z
{
public:
    Z(){}

    Z(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units,
        const boolean check_for_off_on_init=true)
    {
        *n = *n + 1;
        prefix_ = prefix;
        code_ = code;
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        check_for_off_on_init_ = check_for_off_on_init;
        is_eeram_ = !(ram==NULL);
        rP_ = ram;
        // app_ = app;
    }

    ~Z(){}

    fptr app_;
    fptr app() { return app_; }
    // void app(fptr ptr) { app_ = ptr; }
    String code() { return code_; }
    const char* description() { return description_.c_str(); }
    const char* units() { return units_.c_str(); }

    // Placeholders
    virtual uint16_t assign_addr(uint16_t next){return next;}
    virtual void get(){};
    virtual boolean is_corrupt(){return false;};
    virtual boolean is_eeram(){return is_eeram_;};
    virtual boolean is_off(){return false;};
    virtual boolean off_nominal(){return false;};
    virtual void print(){};
    virtual void set_nominal(){};

protected:
    String code_;
    SerialRAM *rP_;
    address16b addr_;
    String units_;
    String description_;
    boolean is_eeram_;      // eeram
    boolean check_for_off_on_init_;  // check for off-nominal on initialization
    String prefix_;         // either "* " saved or "  " not saved
};


class BooleanZ: public Z
{
public:
    BooleanZ(){}

    BooleanZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const boolean min, const boolean max,
    boolean *store, const boolean _default=false, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~BooleanZ(){}

    uint16_t assign_addr(boolean next)
    {
        addr_.a16 = next;
        return next + sizeof(boolean);
    }

    virtual void get()
    {
        if ( is_eeram_ ) *val_ = rP_->read(addr_.a16);
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const boolean input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }
   
    void check_set_put(boolean val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    boolean *val_;
    boolean min_;
    boolean max_;
    boolean default_;
};


class DoubleZ: public Z
{
public:
    DoubleZ(){}

    DoubleZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const double min, const double max, double *store,
        const double _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~DoubleZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(double);
    }

    virtual void get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9.1f -> %9.1f, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9.1f, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }
    
    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6.1f: (%-6.1f-%6.1f) [%6.1f] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }
    
    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const double input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }

    void check_set_put(double val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %7.3f (-%7.3f, %7.3f)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    double *val_;
    double default_;
    double min_;
    double max_;
};


class FloatZ: public Z
{
public:
    FloatZ(){}

    FloatZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max,
    float *store, const float _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~FloatZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(float);
    }

    virtual void get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9.3f -> %9.3f, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9.3f, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }
    
    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6.3f: (%-6.3g-%6.3g) [%6.3f] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const float input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }

    void check_set_put(float val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %7.3f (-%7.3f, %7.3f)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    float *val_;
    float default_;
    float min_;
    float max_;
};


class FloatNoZ: public Z
{
public:
    FloatNoZ(){}

    FloatNoZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max,
    const float _default=0):
        Z(n, prefix, code, ram, description, units, false)
    {
        min_ = min;
        max_ = max;
        default_ = max(min(_default, max_), min_);
        prefix_ = "  ";
    }

    ~FloatNoZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        return next;
    }

    void print_str()
    {
        sprintf(pr.buff, " %-20s %9.3f -> %9.3f, %10s (%s%-2s)", description_.c_str(), default_, NAN, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6.3f: (%-6.3g-%6.3g) [%6.3f] %s, %s", prefix_.c_str(), code_.c_str(), NAN, min_, max_, default_, description_.c_str(), units_.c_str());
    }
    
    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    virtual void set_nominal(){}

protected:
    float default_;
    float min_;
    float max_;
    String prefix_;
};


class IntZ: public Z
{
public:
    IntZ(){}

    IntZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const int min, const int max, int *store,
        const int _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~IntZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(int);
    }

    virtual void get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }
    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
      sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }
    
    void print_adj_print(const int input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }

    void check_set_put(int val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    int *val_;
    int min_;
    int max_;
    int default_;
};


class Int8tZ: public Z
{
public:
    Int8tZ(){}

    Int8tZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const int8_t min, const int8_t max,
        int8_t *store, const int8_t _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~Int8tZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(int8_t);
    }

    virtual void get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
      sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }
    
    void print_adj_print(const int8_t input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }

    void check_set_put(int8_t val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    int8_t *val_;
    int8_t min_;
    int8_t max_;
    int8_t default_;
};


class Uint16tZ: public Z
{
public:
    Uint16tZ(){}

    Uint16tZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const uint16_t min, const uint16_t max,
    uint16_t *store, const uint16_t _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
     }

    ~Uint16tZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(uint16_t);
    }

    virtual void get()
    {
        if ( is_eeram_ ) *val_ = rP_->read(addr_.a16);
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const uint16_t input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }
   
    void check_set_put(uint16_t val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    uint16_t *val_;
    uint16_t min_;
    uint16_t max_;
    uint16_t default_;
};

class Uint8tZ: public Z
{
public:
    Uint8tZ(){}

    Uint8tZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const uint8_t min, const uint8_t max,
    uint8_t *store, const uint8_t _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~Uint8tZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(uint8_t);
    }

    virtual void get()
    {
        if ( is_eeram_ ) *val_ = rP_->read(addr_.a16);
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const uint8_t input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }
   
    void check_set_put(uint8_t val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    uint8_t *val_;
    uint8_t min_;
    uint8_t max_;
    uint8_t default_;
};


class ULongZ: public Z
{
public:
    ULongZ(){}

    ULongZ(int8_t *n, const String &prefix, const String &code, SerialRAM *ram, const String &description, const String &units, const unsigned long min, const unsigned long max,
    unsigned long *store, const unsigned long _default=0, const boolean check_for_off_on_init=true):
        Z(n, prefix, code, ram, description, units, check_for_off_on_init)
    {
        min_ = min;
        max_ = max;
        val_ = store;
        default_ = max(min(_default, max_), min_);
        if ( check_for_off_on_init_ ) check_set_put(*val_);
    }

    ~ULongZ(){}

    uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(unsigned long);
    }

    virtual void get()
    {
        if ( is_eeram_ )
        {
            rP_->get(addr_.a16, *val_);
        }
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ > max_ || *val_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual boolean is_off()
    {
        return off_nominal() && check_for_off_on_init_;
    }

    virtual boolean off_nominal()
    {
        return *val_ != default_;
    }

    void print_str()
    {
        // if ( !check_for_off_on_init_ )
            sprintf(pr.buff, " %-18s %10d -> %10d, %10s (%s%-2s)", description_.c_str(), (int)default_, (int)*val_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-32s %10d, %10s (%s%-2s)", description_.c_str(), (int)*val_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }
    
    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), (int)*val_, (int)min_, (int)max_, (int)default_, description_.c_str(), units_.c_str());
    }

    void print_help()
    {
        print_help_str();
        Serial.printf("%s\n", pr.buff);
    }

    void print1_help()
    {
        print_help_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const unsigned long input)
    {
        print();
        print1();
        check_set_put(input);
        print();
        print1();
    }
   
    void check_set_put(unsigned long val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set:: out range %ld (%-ld, %ld)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
        }
    }

    virtual void set_nominal()
    {
        *val_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
    }

protected:
    unsigned long *val_;
    unsigned long min_;
    unsigned long max_;
    unsigned long default_;
};

#endif
