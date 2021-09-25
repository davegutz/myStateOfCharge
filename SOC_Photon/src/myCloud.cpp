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
extern boolean enable_wifi;

// Publish1 Blynk
void publish1(void)
{
  if (debug>4) Serial.printf("Blynk write1\n");
  Blynk.virtualWrite(V0,  pubList.Vbatt);
  Blynk.virtualWrite(V2,  pubList.Vbatt_filt_obs);
  Blynk.virtualWrite(V3,  pubList.VOC_solved);
  Blynk.virtualWrite(V4,  pubList.Vbatt_solved);
  //Blynk.virtualWrite(V5,  pubList.Vbatt_model);
}


// Publish2 Blynk
void publish2(void)
{
  if (debug>4) Serial.printf("Blynk write2\n");
  Blynk.virtualWrite(V6,  pubList.socu_free);
  Blynk.virtualWrite(V7,  pubList.Vbatt_solved);
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
  Blynk.virtualWrite(V16, pubList.tcharge);
}


// Publish4 Blynk
void publish4(void)
{
  if (debug>4) Serial.printf("Blynk write4\n");
  Blynk.virtualWrite(V17, pubList.Ishunt);
  Blynk.virtualWrite(V18, pubList.Ishunt_filt_obs);
  Blynk.virtualWrite(V19, pubList.Wshunt);
  Blynk.virtualWrite(V20, pubList.Wshunt_filt);
  Blynk.virtualWrite(V21, pubList.socu_solved);
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


// Check connection and publish Particle
void publish_particle(unsigned long now, Wifi *wifi, const boolean enable_wifi)
{
  // Forgiving wifi connection logic
  manage_wifi(now, wifi);

  // Publish if valid
  if ( debug>2 ) Serial.printf("Particle write:  ");
  if ( wifi->connected )
  {
    // Create print string
    create_print_string(buffer, &pubList);
 
    unsigned nowSec = now/1000UL;
    unsigned sec = nowSec%60;
    unsigned min = (nowSec%3600)/60;
    unsigned hours = (nowSec%86400)/3600;
    char publishString[40];     // For uptime recording
    sprintf(publishString,"%u:%u:%u",hours,min,sec);
    Particle.publish("Uptime",publishString);
    Particle.publish("stat", buffer);
    if ( debug>2 ) Serial.println(buffer);
  }
  else
  {
    if ( debug>2 ) Serial.printf("nothing to do\n");
    pubList.numTimeouts++;
  }
}


// Assignments
void assignPubList(Publish* pubList, const unsigned long now, const String unit, const String hmString,
  const double controlTime, struct Sensors* sen, const int numTimeouts,
  Battery* myBatt_solved, Battery* myBatt_free)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hmString =hmString;
  pubList->controlTime = controlTime;
  pubList->Vbatt = sen->Vbatt;
  pubList->Vbatt_filt = sen->Vbatt_filt;
  pubList->Vbatt_filt_obs = sen->Vbatt_filt_obs;
  pubList->Tbatt = sen->Tbatt;
  pubList->Tbatt_filt = sen->Tbatt_filt;
  pubList->Vshunt = sen->Vshunt;
  pubList->Vshunt_filt = sen->Vshunt_filt;
  pubList->Ishunt = sen->Ishunt;
  pubList->Ishunt_filt = sen->Ishunt_filt;
  pubList->Ishunt_filt_obs = sen->Ishunt_filt_obs;
  pubList->Wshunt = sen->Wshunt;
  pubList->Wshunt_filt = sen->Wshunt_filt;
  pubList->numTimeouts = numTimeouts;
  pubList->socu_solved = myBatt_solved->socu()*100.0;
  pubList->socu_free = myBatt_free->socu()*100.0;
  pubList->T = sen->T;
  if ( debug==-13 ) Serial.printf("sen->T=%6.3f\n", sen->T);
  pubList->tcharge = myBatt_free->tcharge();
  pubList->VOC_free = myBatt_free->voc();
  pubList->VOC_solved = myBatt_solved->voc();
  pubList->Vbatt_solved = sen->Vbatt_solved;
}
