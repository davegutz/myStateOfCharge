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

#ifndef _STORED_H
#define _STORED_H

#include "hardware/SerialRAM.h"


// class Parameter  TODO:  use this to build the stuff in parameters.h
template <class T>
class Parameter {
public:
    Parameter(){}
    Parameter(SerialRAM *ram, uint16_t *next, const T input)
    {
        eeram_.a16 = *next;
        *next += sizeof(input);
        rP_ = ram;
        put_(input);
    }
	~Parameter() {}

    // operators
    operator T()
    {
        return val_;
    }
    Parameter operator = (const T input)
    {
        put_(input);
        return *this;
    }
    Parameter operator = (Parameter& input)
    {
        eeram_ = input.eeram_;
        rP_ = input.rP_;
        val_ = input.val_;
    }

    // functions
    uint16_t address() { return eeram_.a16; };
    void get()
    {
        T value;
        rP_->get(eeram_.a16, value);
        val_ = value;
    }
    boolean is_val_corrupt(T minval, T maxval)
    {
        return isnan(val_) || val_ < minval || val_ > maxval;
    }
    T val()
    {
        return val_;
    }

private:
    void put_(const T input)
    {
        rP_->put(eeram_.a16, input);
        val_ = input;
    }
    address16b eeram_;
    SerialRAM *rP_;
    T val_;
};


#endif
