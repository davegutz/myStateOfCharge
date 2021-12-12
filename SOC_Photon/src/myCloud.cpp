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
  Blynk.virtualWrite(V12, pubList.Vshunt_amp);
  Blynk.virtualWrite(V13, pubList.Vshunt_amp_filt);
  Blynk.virtualWrite(V14, pubList.I2C_status);
  Blynk.virtualWrite(V15, pubList.hm_string);
  Blynk.virtualWrite(V16, pubList.tcharge);
}


// Publish4 Blynk
void publish4(void)
{
  if (debug>4) Serial.printf("Blynk write4\n");
  Blynk.virtualWrite(V17, pubList.Ishunt_amp);
  Blynk.virtualWrite(V18, pubList.Ishunt_amp_filt_obs);
  Blynk.virtualWrite(V19, pubList.Wshunt_amp);
  Blynk.virtualWrite(V20, pubList.Wshunt_amp_filt);
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
    pubList.num_timeouts++;
  }
}


// Assignments
void assign_PubList(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  const double control_time, struct Sensors* Sen, const int num_timeouts,
  Battery* MyBattSolved, Battery* MyBattFree)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hm_string =hm_string;
  pubList->control_time = control_time;
  pubList->Vbatt = Sen->Vbatt;
  pubList->Vbatt_filt = Sen->Vbatt_filt;
  pubList->Vbatt_filt_obs = Sen->Vbatt_filt_obs;
  pubList->Tbatt = Sen->Tbatt;
  pubList->Tbatt_filt = Sen->Tbatt_filt;
  pubList->Vshunt = Sen->Vshunt;
  pubList->Vshunt_filt = Sen->Vshunt_filt;
  pubList->Ishunt = Sen->Ishunt;
  pubList->Ishunt_filt = Sen->Ishunt_filt;
  pubList->Ishunt_filt_obs = Sen->Ishunt_filt_obs;
  pubList->Wshunt = Sen->Wshunt;
  pubList->Wshunt_filt = Sen->Wshunt_filt;
  pubList->Vshunt_amp = Sen->Vshunt_amp;
  pubList->Vshunt_amp_filt = Sen->Vshunt_amp_filt;
  pubList->Ishunt_amp = Sen->Ishunt_amp;
  pubList->Ishunt_amp_filt = Sen->Ishunt_amp_filt;
  pubList->Ishunt_amp_filt_obs = Sen->Ishunt_amp_filt_obs;
  pubList->Wshunt_amp = Sen->Wshunt_amp;
  pubList->Wshunt_amp_filt = Sen->Wshunt_amp_filt;
  pubList->num_timeouts = num_timeouts;
  pubList->socu_solved = MyBattSolved->socu()*100.0;
  pubList->socu_free = MyBattFree->socu()*100.0;
  pubList->T = Sen->T;
  if ( debug==-13 ) Serial.printf("Sen->T=%6.3f\n", Sen->T);
  pubList->tcharge = MyBattFree->tcharge();
  pubList->VOC_free = MyBattFree->voc();
  pubList->VOC_solved = MyBattSolved->voc();
  pubList->Vbatt_solved = Sen->Vbatt_solved;
  pubList->soc_avail = MyBattFree->soc_avail()*100.0;
  pubList->socu_model = rp.socu_model*100.0;
}
