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

extern CommandPars cp;            // Various parameters to be common at system level (reset on PLC reset)
extern PublishPars pp;            // For publishing
extern BlynkTimer blynk_timer_1, blynk_timer_2, blynk_timer_3, blynk_timer_4;     // Time Blynk events
extern BlynkParticle Blynk;

// Publish1 Blynk
void publish1(void)
{
  if (rp.debug>104) Serial.printf("Blynk write1\n");
  Blynk.virtualWrite(V2,  pp.pubList.Vbatt);
  Blynk.virtualWrite(V3,  pp.pubList.Voc);
  Blynk.virtualWrite(V4,  pp.pubList.Vbatt);
}


// Publish2 Blynk
void publish2(void)
{
  if (rp.debug>104) Serial.printf("Blynk write2\n");
  Blynk.virtualWrite(V6,  pp.pubList.soc);
  Blynk.virtualWrite(V8,  pp.pubList.T);
  Blynk.virtualWrite(V10, pp.pubList.Tbatt);
}


// Publish3 Blynk
void publish3(void)
{
  if (rp.debug>104) Serial.printf("Blynk write3\n");
  Blynk.virtualWrite(V15, pp.pubList.hm_string);
  Blynk.virtualWrite(V16, pp.pubList.tcharge);
}


// Publish4 Blynk
void publish4(void)
{
  if (rp.debug>104) Serial.printf("Blynk write4\n");
  Blynk.virtualWrite(V18, pp.pubList.Ibatt);
  Blynk.virtualWrite(V20, pp.pubList.Wbatt);
  Blynk.virtualWrite(V21, pp.pubList.soc_ekf);
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
  if ( rp.debug>102 ) Serial.printf("Particle write:  ");
  if ( wifi->connected )
  {
    // Create print string
    create_print_string(&pp.pubList);
 
    unsigned nowSec = now/1000UL;
    unsigned sec = nowSec%60;
    unsigned min = (nowSec%3600)/60;
    unsigned hours = (nowSec%86400)/3600;
    char publishString[40];     // For uptime recording
    sprintf(publishString,"%u:%u:%u",hours,min,sec);
    Particle.publish("Uptime",publishString);
    Particle.publish("stat", cp.buffer);
    if ( rp.debug>102 ) Serial.println(cp.buffer);
  }
  else
  {
    if ( rp.debug>102 ) Serial.printf("nothing to do\n");
    pp.pubList.num_timeouts++;
  }
}


// Assignments
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  const double control_time, struct Sensors* Sen, const int num_timeouts, BatteryMonitor* Mon)
{
  pubList->now = now;
  pubList->unit = unit;
  pubList->hm_string =hm_string;
  pubList->control_time = control_time;
  pubList->Vbatt = Sen->Vbatt;
  pubList->Tbatt = Sen->Tbatt;
  pubList->Tbatt_filt = Sen->Tbatt_filt;
  pubList->Vshunt = Sen->Vshunt;
  pubList->Ibatt = Sen->Ibatt;
  pubList->Wbatt = Sen->Wbatt;
  pubList->num_timeouts = num_timeouts;
  pubList->T = Sen->T;
  if ( rp.debug==-13 ) Serial.printf("Sen->T=%6.3f\n", Sen->T);
  pubList->tcharge = Mon->tcharge();
  pubList->Voc = Mon->Voc();
  pubList->Voc_filt = Mon->Voc_filt();
  pubList->Vsat = Mon->Vsat();
  pubList->sat = Mon->sat();
  pubList->soc_model = Sen->Sim->soc();
  pubList->soc_ekf = Mon->soc_ekf();
  pubList->soc = Mon->soc();
  pubList->soc_wt = Mon->soc_wt();
  pubList->Amp_hrs_remaining_ekf = Mon->Amp_hrs_remaining_ekf();
  pubList->Amp_hrs_remaining_wt = Mon->Amp_hrs_remaining_wt();
  pubList->Vdyn = Mon->Vdyn();
  pubList->Voc_ekf = Mon->Hx();
  pubList->y_ekf = Mon->y_ekf();
}
