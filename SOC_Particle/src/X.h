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

#ifndef X_H_
#define X_H_

#include "hardware/SerialRAM.h"
#include "PrinterPars.h"
extern PrinterPars pr;  // Print buffer

#undef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

/* Using pointers in building class so all that stuff does not get saved by 'retained' keyword in SOC_Particle.ino.
    Only the *_z parameters at the bottom of Parameters.h are stored in SRAM.  Class initialized by *init in arg list.
*/

template <typename T>
class X
{
public:
    X(){}

    X(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units,
     const T min, const T max, T *store, const T _default, const boolean check_off=false)
    {
        *n = *n + 1;
        code_ = code;
        description_ = description.substring(0, 20);
        units_ = units.substring(0, 10);
        if ( ram==NULL ) is_eeram_ = false;
        else is_eeram_ = true;
        rP_ = ram;
        min_ = min;
        max_ = max;
        val_ptr_ = store;
        default_ = _default;
        if ( ram==NULL && check_off )
        {
            // set_push(*val_ptr_);
            prefix_ = "  ";
        }
        else  // EERAM
        {
            prefix_ = "* ";
        }
        check_off_ = check_off;
        Serial.printf("X::X cpt store 0x%X\n", store);
        Serial.printf("X::X cpt val_ptr_ 0x%X\n", val_ptr_);
    }

    ~X(){}

    String code() { return code_; }
    const char* description() { return description_.c_str(); }
    const char* units() { return units_.c_str(); }

    // Placeholders
    virtual uint16_t assign_addr(uint16_t next){return next;}
    virtual void get(){};
    virtual boolean is_corrupt(){return false;};

    boolean is_off()
    {
        return off_nominal() && !check_off_;
    }

    boolean max_of() { return max_; }

    boolean min_of() { return min_; }

    T nominal() { return default_; }

    virtual boolean off_nominal()
    {
        // Serial.printf("val_ptr_ 0x%X *val_ptr %d\n", val_ptr_, *val_ptr_);
        Serial.printf("val_ptr_ 0x%X ", val_ptr_);
        Serial.printf("X default_ %7.3f\n", default_);
        // return *val_ptr_ != default_;
        return false;
    }

    virtual void print()
    {
        // print_str();
        sprintf(pr.buff, "testB");
        Serial.printf("%s\n", pr.buff);
    }
    
    virtual void print1()
    {
        // print_str();
        sprintf(pr.buff, "testB");
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

    // virtual uint16_t assign_addr(uint16_t next){return 0;};
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
    boolean check_off_;
    String prefix_;
};


class BooleanX: public X <boolean>
{
public:
    BooleanX(){}

    BooleanX(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const boolean min, const boolean max,
    boolean *store, const boolean _default=false, const boolean check_off_=false)   
    {
        boolean local_def = max(min(_default, max_), min_);
        Serial.printf("BouleanX::BouleanX store 0x%X *store %d default %d\n", store, *store, _default);
        X(n, code, ram, description, units, min, max, store, local_def, check_off_);
        // pull_set_nominal();
    }

    ~BooleanX(){}

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
        // if ( !check_off_ )
        //     sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
        sprintf(pr.buff, "testB");
    }

    virtual void print_help_str()
    {
        // sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_ptr_, min_, max_, default_, description_.c_str(), units_.c_str());
        sprintf(pr.buff, "testB");
    }

    virtual void pull_set_nominal()
    {
        *val_ptr_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_ptr_);
    }

    virtual void set_push(const boolean val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ptr_ = val;
            if ( is_eeram_ ) rP_->write(addr_.a16, *val_ptr_);
        }
    }

protected:
};


class DoubleX: public X <double>
{
public:
    DoubleX(){}

    DoubleX(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const double min, const double max,
    double *store, const double _default=false, const boolean check_off_=false)   
    {
        double local_def = max(min(_default, max_), min_);
        Serial.printf("DoubleX::DoubleX store 0x%X *store %7.3f default %7.3f\n", store, *store, _default);
        X(n, code, ram, description, units, min, max, store, local_def, check_off_);
        // pull_set_nominal();
    }

    ~DoubleX(){}

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
        // if ( !check_off_ )
        //     sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
        // else
        //     sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_ptr_, units_.c_str(), prefix_.c_str(), code_.c_str());
        sprintf(pr.buff, "testD");
    }

    virtual void print_help_str()
    {
        // sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_ptr_, min_, max_, default_, description_.c_str(), units_.c_str());
        sprintf(pr.buff, "testD");
    }

    virtual void pull_set_nominal()
    {
        *val_ptr_ = default_;
        if ( is_eeram_ ) rP_->write(addr_.a16, *val_ptr_);
    }

    virtual void set_push(double val)
    {
        if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %7.3f (-%7.3f, %7.3f)\n", code_.c_str(), description_.c_str(), val, min_, max_);
        else
        {
            *val_ptr_ = val;
            if ( is_eeram_ ) rP_->put(addr_.a16, *val_ptr_);
        }
    }

protected:
};

// class DoubleZ: public Z
// {
// public:
//     DoubleZ(){}

//     DoubleZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const double min, const double max, double *store,
//         const double _default=0, const boolean check_off=false):
//         Z(n, code, ram, description, units, false)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//         if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//     }

//     ~DoubleZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(double);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ )
//         {
//             rP_->get(addr_.a16, *val_);
//         }
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }


//     double max_of() { return max_; }

//     double min_of() { return min_; }

//     double nominal() { return default_; }

//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-20s %9.1f -> %9.1f, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-33s %9.1f, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }
    
//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//         sprintf(pr.buff, "%s%-2s= %6.1f: (%-6.1f-%6.1f) [%6.1f] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
//     }
    
//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_adj_print(const double input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }

//     void set_push(double val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %7.3f (-%7.3f, %7.3f)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     double *val_;
//     double default_;
//     double min_;
//     double max_;
//     boolean check_off_;
//     String prefix_;
// };


// class FloatZ: public Z
// {
// public:
//     FloatZ(){}

//     FloatZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max,
//     float *store, const float _default=0, const boolean check_off=false):
//         Z(n, code, ram, description, units, false)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//         if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//     }

//     ~FloatZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(float);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ )
//         {
//             rP_->get(addr_.a16, *val_);
//         }
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }

//     float max_of() { return max_; }

//     float min_of() { return min_; }

//     float nominal() { return default_; }
    
//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-20s %9.3f -> %9.3f, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-33s %9.3f, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }

//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }
    
//     void print_help_str()
//     {
//         sprintf(pr.buff, "%s%-2s= %6.3f: (%-6.3g-%6.3g) [%6.3f] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
//     }

//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_adj_print(const float input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }

//     void set_push(float val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %7.3f (-%7.3f, %7.3f)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     float *val_;
//     float default_;
//     float min_;
//     float max_;
//     boolean check_off_;
//     String prefix_;
// };


// class FloatNoZ: public Z
// {
// public:
//     FloatNoZ(){}

//     FloatNoZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const float min, const float max,
//     const float _default=0):
//         Z(n, code, ram, description, units, false)
//     {
//         min_ = min;
//         max_ = max;
//         default_ = max(min(_default, max_), min_);
//         prefix_ = "  ";
//     }

//     ~FloatNoZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         return next;
//     }

//     float max_of() { return max_; }

//     float min_of() { return min_; }

//     float nominal() { return default_; }
    
//     void print_str()
//     {
//         sprintf(pr.buff, " %-20s %9.3f -> %9.3f, %10s (%s%-2s)", description_.c_str(), default_, NAN, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }

//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//         sprintf(pr.buff, "%s%-2s= %6.3f: (%-6.3g-%6.3g) [%6.3f] %s, %s", prefix_.c_str(), code_.c_str(), NAN, min_, max_, default_, description_.c_str(), units_.c_str());
//     }
    
//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     virtual void pull_set_nominal(){}

// protected:
//     float default_;
//     float min_;
//     float max_;
//     String prefix_;
// };


// class IntZ: public Z
// {
// public:
//     IntZ(){}

//     IntZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const int min, const int max, int *store,
//     const int _default=0, const boolean check_off=false):
//         Z(n, code, ram, description, units, false)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//         if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//     }

//     ~IntZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(int);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ )
//         {
//             rP_->get(addr_.a16, *val_);
//         }
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }

//     int max_of() { return max_; }

//     int min_of() { return min_; }

//     int nominal() { return default_; }
    
//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }
//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//       sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
//     }

//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }
    
//     void print_adj_print(const int input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }

//     void set_push(int val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     int *val_;
//     int min_;
//     int max_;
//     int default_;
//     boolean check_off_;
//     String prefix_;
// };


// class Int8tZ: public Z
// {
// public:
//     Int8tZ(){}

//     Int8tZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const int8_t min, const int8_t max,
//     int8_t *store, const int8_t _default=0, const boolean check_off=false):
//         Z(n, code, ram, description, units, false)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//         if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//     }

//     ~Int8tZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(int8_t);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ )
//         {
//             rP_->get(addr_.a16, *val_);
//         }
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }

//     int8_t max_of() { return max_; }

//     int8_t min_of() { return min_; }

//     int8_t nominal() { return default_; }
    
//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }

//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//       sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
//     }

//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }
    
//     void print_adj_print(const int8_t input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }

//     void set_push(int8_t val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     int8_t *val_;
//     int8_t min_;
//     int8_t max_;
//     int8_t default_;
//     boolean check_off_;
//     String prefix_;
// };


// class Uint16tZ: public Z
// {
// public:
//     Uint16tZ(){}

//     Uint16tZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const uint16_t min, const uint16_t max,
//     uint16_t *store, const uint16_t _default=0, const boolean check_off=false):
//         Z(n, code, ram, description, units, true)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//        if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//      }

//     ~Uint16tZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(uint16_t);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ ) *val_ = rP_->read(addr_.a16);
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }

//     uint16_t max_of() { return max_; }

//     uint16_t min_of() { return min_; }

//     uint16_t nominal() { return default_; }
    
//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }

//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//         sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
//     }

//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_adj_print(const uint16_t input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }
   
//     void set_push(uint16_t val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     uint16_t *val_;
//     uint16_t min_;
//     uint16_t max_;
//     uint16_t default_;
//     boolean check_off_;
//     String prefix_;
// };

// class Uint8tZ: public Z
// {
// public:
//     Uint8tZ(){}

//     Uint8tZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const uint8_t min, const uint8_t max,
//     uint8_t *store, const uint8_t _default=0, const boolean check_off=false):
//         Z(n, code, ram, description, units, true)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//         if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//     }

//     ~Uint8tZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(uint8_t);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ ) *val_ = rP_->read(addr_.a16);
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }

//     uint8_t max_of() { return max_; }

//     uint8_t min_of() { return min_; }

//     uint8_t nominal() { return default_; }
    
//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-20s %9d -> %9d, %10s (%s%-2s)", description_.c_str(), default_, *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-33s %9d, %10s (%s%-2s)", description_.c_str(), *val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }

//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//         sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), *val_, min_, max_, default_, description_.c_str(), units_.c_str());
//     }

//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_adj_print(const uint8_t input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }
   
//     void set_push(uint8_t val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %d (%-d, %d)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     uint8_t *val_;
//     uint8_t min_;
//     uint8_t max_;
//     uint8_t default_;
//     boolean check_off_;
//     String prefix_;
// };


// class ULongZ: public Z
// {
// public:
//     ULongZ(){}

//     ULongZ(uint8_t *n, const String &code, SerialRAM *ram, const String &description, const String &units, const unsigned long min, const unsigned long max,
//     unsigned long *store, const unsigned long _default=0, const boolean check_off=true):
//         Z(n, code, ram, description, units, true)
//     {
//         min_ = min;
//         max_ = max;
//         val_ = store;
//         default_ = max(min(_default, max_), min_);
//         check_off_ = check_off;
//         if ( ram==NULL && check_off )
//         {
//             set_push(*val_);
//             prefix_ = "  ";
//         }
//         else  // EERAM
//         {
//             prefix_ = "* ";
//         }
//         pull_set_nominal();
//     }

//     ~ULongZ(){}

//     uint16_t assign_addr(uint16_t next)
//     {
//         addr_.a16 = next;
//         return next + sizeof(unsigned long);
//     }

//     virtual void get()
//     {
//         if ( is_eeram_ )
//         {
//             rP_->get(addr_.a16, *val_);
//         }
//     }

//     virtual boolean is_corrupt()
//     {
//         boolean corrupt = *val_ > max_ || *val_ < min_;
//         if ( corrupt ) Serial.printf("\n%s %s corrupt", code_.c_str(), description_.c_str());
//         return corrupt;
//     }

//     virtual boolean is_off()
//     {
//         return off_nominal() && !check_off_;
//     }

//     virtual boolean off_nominal()
//     {
//         return *val_ != default_;
//     }

//     unsigned long max_of() { return max_; }

//     unsigned long min_of() { return min_; }

//     unsigned long nominal() { return default_; }

//     void print_str()
//     {
//         if ( !check_off_ )
//             sprintf(pr.buff, " %-18s %10d -> %10d, %10s (%s%-2s)", description_.c_str(), (int)default_, (int)*val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//         else
//             sprintf(pr.buff, " %-32s %10d, %10s (%s%-2s)", description_.c_str(), (int)*val_, units_.c_str(), prefix_.c_str(), code_.c_str());
//     }
    
//     void print()
//     {
//         print_str();
//         Serial.printf("%s\n", pr.buff);
//     }
    
//     void print1()
//     {
//         print_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_help_str()
//     {
//         sprintf(pr.buff, "%s%-2s= %6d: (%-6d-%6d) [%6d] %s, %s", prefix_.c_str(), code_.c_str(), (int)*val_, (int)min_, (int)max_, (int)default_, description_.c_str(), units_.c_str());
//     }

//     void print_help()
//     {
//         print_help_str();
//         Serial.printf("%s\n", pr.buff);
//     }

//     void print1_help()
//     {
//         print_help_str();
//         Serial1.printf("%s\n", pr.buff);
//     }

//     void print_adj_print(const unsigned long input)
//     {
//         print();
//         print1();
//         set_push(input);
//         print();
//         print1();
//     }
   
//     void set_push(unsigned long val)
//     {
//         if ( val>max_ || val<min_ ) Serial.printf("%s %s set_push:: out range %ld (%-ld, %ld)\n", code_.c_str(), description_.c_str(), val, min_, max_);
//         else
//         {
//             *val_ = val;
//             if ( is_eeram_ ) rP_->put(addr_.a16, *val_);
//         }
//     }

//     virtual void pull_set_nominal()
//     {
//         *val_ = default_;
//         if ( is_eeram_ ) rP_->write(addr_.a16, *val_);
//     }

// protected:
//     unsigned long *val_;
//     unsigned long min_;
//     unsigned long max_;
//     unsigned long default_;
//     boolean check_off_;
//     String prefix_;
// };



#endif
