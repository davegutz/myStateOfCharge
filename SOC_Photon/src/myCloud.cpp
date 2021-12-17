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
#include "Blynk/BlynkParticle.h"
#include "command.h"
extern CommandPars cp;            // Various parameters to be common at system level (reset on PLC reset)
// extern const int8_t debug;
// extern Publish pubList;
// extern char buffer[256];
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;     // Time Blynk events
extern BlynkParticle Blynk;
// extern boolean enable_wifi;

// Publish1 Blynk
void publish1(void)
{
  if (rp.debug>4) Serial.printf("Blynk write1\n");
  Blynk.virtualWrite(V0,  cp.pubList.Vbatt);
  Blynk.virtualWrite(V2,  cp.pubList.Vbatt_filt_obs);
  Blynk.virtualWrite(V3,  cp.pubList.VOC_solved);
  Blynk.virtualWrite(V4,  cp.pubList.Vbatt_solved);
  //Blynk.virtualWrite(V5,  pubList.Vbatt_model);
}


// Publish2 Blynk
void publish2(void)
{
  if (rp.debug>4) Serial.printf("Blynk write2\n");
  Blynk.virtualWrite(V6,  cp.pubList.socu_free);
  Blynk.virtualWrite(V7,  cp.pubList.Vbatt_solved);
  Blynk.virtualWrite(V8,  cp.pubList.T);
  Blynk.virtualWrite(V9,  cp.pubList.Tbatt);
  Blynk.virtualWrite(V10, cp.pubList.Tbatt_filt);
}


// Publish3 Blynk
void publish3(void)
{
  if (rp.debug>4) Serial.printf("Blynk write3\n");
  Blynk.virtualWrite(V12, cp.pubList.Vshunt_amp);
  Blynk.virtualWrite(V13, cp.pubList.Vshunt_filt);
  Blynk.virtualWrite(V14, cp.pubList.I2C_status);
  Blynk.virtualWrite(V15, cp.pubList.hm_string);
  Blynk.virtualWrite(V16, cp.pubList.tcharge);
}


// Publish4 Blynk
void publish4(void)
{
  if (rp.debug>4) Serial.printf("Blynk write4\n");
  Blynk.virtualWrite(V17, cp.pubList.Ishunt);
  Blynk.virtualWrite(V18, cp.pubList.Ishunt_filt_obs);
  Blynk.virtualWrite(V19, cp.pubList.Wshunt);
  // Blynk.virtualWrite(V20, cp.pubList.Wshunt_filt);
  Blynk.virtualWrite(V21, cp.pubList.socu_solved);
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
  if ( rp.debug>2 ) Serial.printf("Particle write:  ");
  if ( wifi->connected )
  {
    // Create print string
    create_print_string(cp.buffer, &cp.pubList);
 
    unsigned nowSec = now/1000UL;
    unsigned sec = nowSec%60;
    unsigned min = (nowSec%3600)/60;
    unsigned hours = (nowSec%86400)/3600;
    char publishString[40];     // For uptime recording
    sprintf(publishString,"%u:%u:%u",hours,min,sec);
    Particle.publish("Uptime",publishString);
    Particle.publish("stat", cp.buffer);
    if ( rp.debug>2 ) Serial.println(cp.buffer);
  }
  else
  {
    if ( rp.debug>2 ) Serial.printf("nothing to do\n");
    cp.pubList.num_timeouts++;
  }
}


// Assignments
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  const double control_time, struct Sensors* Sen, const int num_timeouts,
  Battery* MyBattSolved, Battery* MyBattFree)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hm_string =hm_string;
  pubList->control_time = control_time;
  pubList->Vbatt = Sen->Vbatt;
  pubList->Vbatt_filt_obs = Sen->Vbatt_filt_obs;
  pubList->Tbatt = Sen->Tbatt;
  pubList->Tbatt_filt = Sen->Tbatt_filt;
  pubList->Vshunt_amp = Sen->Vshunt_amp;
  pubList->Vshunt_noamp = Sen->Vshunt_noamp;
  pubList->Vshunt = Sen->Vshunt;
  pubList->Vshunt_filt = Sen->Vshunt_filt;
  pubList->Ishunt_amp_cal = Sen->Ishunt_amp_cal;
  pubList->Ishunt_noamp_cal = Sen->Ishunt_noamp_cal;
  pubList->Ishunt = Sen->Ishunt;
  pubList->Ishunt_filt_obs = Sen->Ishunt_filt_obs;
  pubList->Wshunt = Sen->Wshunt;
  pubList->Vshunt_amp = Sen->Vshunt_amp;
  pubList->num_timeouts = num_timeouts;
  pubList->socu_solved = MyBattSolved->socu()*100.0;
  pubList->socu_free = MyBattFree->socu()*100.0;
  pubList->socu = rp.socu*100.;
  pubList->socs = rp.socs*100.;
  pubList->socs_sat = rp.socs_sat*100.;
  pubList->T = Sen->T;
  if ( rp.debug==-13 ) Serial.printf("Sen->T=%6.3f\n", Sen->T);
  pubList->tcharge = MyBattFree->tcharge();
  pubList->VOC_free = MyBattFree->voc();
  pubList->VOC_solved = MyBattSolved->voc();
  pubList->Vbatt_solved = Sen->Vbatt_solved;
  pubList->soc_avail = MyBattFree->soc_avail()*100.0;
  pubList->curr_sel_amp = rp.curr_sel_amp;
}
