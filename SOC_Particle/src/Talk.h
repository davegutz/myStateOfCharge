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

#include "Particle.h"

class TalkVariableFloat
{
public:
  TalkVariableFloat() 
  {
    func = &TalkVariableFloat::fa;
  }

  void run()
  {
    int foo = 10, bar = 20;
    Serial.printf("%7.3f\n", (this->*func)(foo, bar));
  }

  double fa(int x, int y) { return (double)(x + y); }
  double fb(int x, int y) { return (double)(x - y); }

private:
  typedef double (TalkVariableFloat::*fptr)(int x, int y);
  fptr func;
};

/*
template <typename T>
class TalkVariable {
private:
    String units;
    String description;
    T value;
    function <void(T)> *setFunction;
    function <T()> *getFunction;

public:
    TalkVariable(const String& units, const String& description)
        : units(units), description(description), value(0), 
        setFunction([](T){}), getFunction([](){ return T(); })
    {}

    void setValue(T newValue) {
        value = newValue;
        setFunction(newValue);
    }

    T getValue() const {
        return getFunction();
    }

    void setSetFunction(const <void(T)> &setFn) {
        setFunction = setFn;
    }

    void setGetFunction(const <T()> &getFn) {
        getFunction = getFn;
    }

    String getUnits() const {
        return units;
    }

    String getDescription() const {
        return description;
    }
};
*/
