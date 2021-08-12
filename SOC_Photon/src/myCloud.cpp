//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
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

#ifndef ARDUINO
#include "application.h" // Should not be needed if file .ino or Arduino
#endif
#include "mySubs.h"
#include "myCloud.h"
#include "constants.h"
#include <math.h>
#include "Blynk/BlynkHandlers.h"
#include "Blynk/BlynkTimer.h"
#include "BlynkParticle.h"

extern const int8_t debug;
extern Publish pubList;
extern char buffer[256];
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;     // Time Blynk events
extern BlynkParticle Blynk;
//extern BlynkParticle Blynk;

// Publish1 Blynk
void publish1(void)
{
  if (debug>4) Serial.printf("Blynk write1\n");
  Blynk.virtualWrite(V0,  pubList.Vbatt);
  Blynk.virtualWrite(V2,  pubList.Vbatt_filt);
  //Blynk.virtualWrite(V3,  pubList.hum);
  // Blynk.virtualWrite(V4,  intentionally blank; used elsewhere);
  //Blynk.virtualWrite(V5,  pubList.Tp);
}


// Publish2 Blynk
void publish2(void)
{
  if (debug>4) Serial.printf("Blynk write2\n");
  //Blynk.virtualWrite(V7,  pubList.held);
  Blynk.virtualWrite(V8,  pubList.T);
  Blynk.virtualWrite(V9,  pubList.Tbatt);
  Blynk.virtualWrite(V10, pubList.Tbatt_filt);
}


// Publish3 Blynk
void publish3(void)
{
  if (debug>4) Serial.printf("Blynk write3\n");
  Blynk.virtualWrite(V12, pubList.Vshunt);
  Blynk.virtualWrite(V13, pubList.Vshunt_filt);
  Blynk.virtualWrite(V14, pubList.I2C_status);
  Blynk.virtualWrite(V15, pubList.hmString);
  //Blynk.virtualWrite(V16, pubList.duty);
}


// Publish4 Blynk
void publish4(void)
{
  if (debug>4) Serial.printf("Blynk write4\n");
  Blynk.virtualWrite(V17, false);
  //Blynk.virtualWrite(V18, pubList.OAT);
  //Blynk.virtualWrite(V19, pubList.Ta_obs);
  //Blynk.virtualWrite(V20, pubList.heat_o);
}


// Attach a Slider widget to the Virtual pin 4 IN in your Blynk app
// - and control the web desired temperature.
// Note:  there are separate virtual IN and OUT in Blynk.
BLYNK_WRITE(V4) {
    if (param.asInt() > 0)
    {
        //pubList.webDmd = param.asDouble();
    }
}


// Attach a switch widget to the Virtual pin 6 in your Blynk app - and demand continuous web control
// Note:  there are separate virtual IN and OUT in Blynk.
BLYNK_WRITE(V6) {
//    pubList.webHold = param.asInt();
}
