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

#ifndef ADJUST_H_
#define ADJUST_H_

#include "hardware/SerialRAM.h"
#include "PrinterPars.h"
#include <list>
extern PrinterPars pr;  // Print buffer

#undef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_z parameters at the bottom of Parameters.h are stored in SRAM.  Class initialized by *init in arg list.
*/

template <typename T>
class Adjust
{
public:
    Adjust(){}

    Adjust(const String &code, SerialRAM *ram, const String &description, const String &units,
     const T _min, const T _max, T *store, const T _default, const boolean no_check=false)
    {
        code_ = code;
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        if ( ram==NULL ) is_eeram_ = false;
        else is_eeram_ = true;
        rP_ = ram;
        min_ = _min;
        max_ = _max;
        val_ptr_ = store;
        default_ = _default;
        if ( ram==NULL && no_check )
        {
            set_push(*val_ptr_);
            prefix_ = "  ";
        }
        else  // EERAM
        {
            prefix_ = "* ";
        }
        no_check_ = no_check;
        // Serial.printf("Adjust::Adjust cpt store 0x%Adjust ", store);
        // Serial.printf("val_ptr_ 0x%Adjust\n", val_ptr_);
    }

    ~Adjust(){}

    String code() { return code_; }
    const char* description() { return description_.c_str(); }
    const char* units() { return units_.c_str(); }

    boolean is_off()
    {
        return off_nominal() && !no_check_;
    }

    boolean off_nominal()
    {
        // Serial.printf("val_ptr_ 0x%Adjust\n", val_ptr_);
        return *val_ptr_ != default_;
        // return false;
    }

    void print()
    {
        print_str();
        Serial.printf("%s\n", pr.buff);
    }
    
    void print_off()
    {
        if ( *val_ptr_ != default_ )
        {
            print_str();
            Serial.printf("%s\n", pr.buff);
        }
    }
    
    void print1()
    {
        print_str();
        Serial1.printf("%s\n", pr.buff);
    }

    void print_adj_print(const T input)
    {
        print();
        print1();
        set_push(input);
        print();
        print1();
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

    T value() { return *val_ptr_; }

    // Placeholders
    virtual uint16_t assign_addr(uint16_t next){return 0;};
    virtual void get(){};
    virtual boolean is_corrupt(){return false;};
    virtual void print_help_str(){};
    virtual void print_str(){};
    virtual void pull_set_nominal(){};
    virtual void set_push(const T input){};

protected:
    String code_;
    SerialRAM *rP_;
    address16b addr_;
    String units_;
    String description_;
    boolean is_eeram_;
    T *val_ptr_;
    T min_;
    T max_;
    T default_;
    boolean no_check_;
    String prefix_;
};


class AjBoolean: public Adjust <boolean>
{
public:
    AjBoolean(){}

    AjBoolean(const String &code, SerialRAM *ram, const String &description, const String &units, const boolean _min, const boolean _max,
    boolean *store, const boolean _default=false, const boolean no_check_=false):
        Adjust(code, ram, description, units, _min, _max, store, _default, no_check_)
    {
        pull_set_nominal();
    }

    ~AjBoolean(){}

    virtual uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(boolean);
    }

    virtual void get()
    {
        if ( is_eeram_ ) *val_ptr_ = rP_->read(addr_.a16);
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ptr_ > max_ || *val_ptr_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual void print_str()
    {
        if ( !no_check_ )
            sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
        else
            sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    virtual void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_ptr_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    virtual void pull_set_nominal()
    {
        *val_ptr_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_ptr_);
    }

    virtual void set_push(const boolean val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %d (%d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ptr_ = val;
            if ( is_eeram_ ) rP_->write(addr_.a16, *val_ptr_);
        }
    }

protected:
};


class AjDouble: public Adjust <double>
{
public:
    AjDouble(){}

    AjDouble(const String &code, SerialRAM *ram, const String &description, const String &units, const double _min, const double _max,
    double *store, const double _default=false, const boolean no_check_=false):
        Adjust(code, ram, description, units, _min, _max, store, max(min(_default, _max), _min), no_check_)
    {
        pull_set_nominal();
    }

    ~AjDouble(){}

    virtual uint16_t assign_addr(uint16_t next)
    {
        addr_.a16 = next;
        return next + sizeof(double);
    }

    virtual void get()
    {
        if ( is_eeram_ ) rP_->get(addr_.a16, *val_ptr_);
    }

    virtual boolean is_corrupt()
    {
        boolean corrupt = *val_ptr_ > max_ || *val_ptr_ < min_;
        if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
        return corrupt;
    }

    virtual void print_str()
    {
        if ( !no_check_ )
            sprintf(pr.buff, " %-20s %9.3f -> %9.3f, %10s (%s%-2s)", description_.c_str(), default_, *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
        else
            sprintf(pr.buff, " %-33s %9.3f, %10s (%s%-2s)", description_.c_str(), *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
    }

    virtual void print_help_str()
    {
        sprintf(pr.buff, "%s%-2s= %6.3f: (%-6.3f-%6.3f) [%6.3f] %s, %s", prefix_.c_str(), code_.c_str(), *val_ptr_, min_, max_, default_, description_.c_str(), units_.c_str());
    }

    virtual void pull_set_nominal()
    {
        *val_ptr_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_ptr_);
    }

    virtual void set_push(double val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %7.3f (%7.3f, %7.3f)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ptr_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_ptr_);
        }
    }

protected:
};

#endif
