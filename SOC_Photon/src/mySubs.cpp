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
#include "local_config.h"
#include "constants.h"
#include <math.h>

extern const int8_t debug;
extern Publish pubList;
extern char buffer[256];

// Check connection and publish Particle
void publish_particle(unsigned long now)
{
  sprintf(buffer, "%s,%s,%18.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,  %7.6f,%7.6f,\
  %c", \
    pubList.unit.c_str(), pubList.hmString.c_str(), pubList.controlTime, pubList.Vbatt, pubList.Vbatt_filt,
    pubList.Tbatt, pubList.Tbatt_filt, pubList.Vshunt, pubList.Vshunt_filt, '\0');
  
  if ( debug>2 ) Serial.println(buffer);
  if ( Particle.connected() )
  {
    if ( debug>2 ) Serial.printf("Particle write\n");
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
    if ( debug>1 ) Serial.printf("Particle not connected....connecting\n");
    Particle.connect();
    pubList.numTimeouts++;
  }
}

// Text header
void print_serial_header(void)
{
  Serial.println(F("unit,hm, cTime,  Vbatt,Vbatt_filt,  Tbatt,Tbatt_filt,   Vshunt,Vshunt_filt,"));
}

// Inputs serial print
void serial_print_inputs(unsigned long now, double T)
{
  sprintf(buffer, "%s,%s,%18.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,  %7.6f,%7.6f,\
  %c", \
    pubList.unit.c_str(), pubList.hmString.c_str(), pubList.controlTime, pubList.Vbatt, pubList.Vbatt_filt,
    pubList.Tbatt, pubList.Tbatt_filt, pubList.Vshunt, pubList.Vshunt_filt, '\0');
  Serial.println(buffer);
}

// Normal serial print
void serial_print(void)
{
  if ( debug>2 )
  {
    Serial.print(0, 2); Serial.print(F(", "));   // placeholder SOC?
    Serial.print(0, DEC); Serial.print(F(", "));  // placeholder SOC?
    Serial.println("");
  }
}


// Load and filter
// TODO:   move 'read' stuff here
boolean load(int reset, double T, Sensors *sen, DS18 *sensor_tbatt, General2_Pole* VbattSenseFilt, 
    General2_Pole* TbattSenseFilt, General2_Pole* VshuntSenseFilt, Pins *myPins, Adafruit_ADS1015 *ads)
{
  static boolean done_testing = false;

  // Read Sensor
  // ADS1015 conversion
  sen->Vshunt_int = ads->readADC_Differential_0_1();
  sen->Vshunt = ads->computeVolts(sen->Vshunt_int);
  sen->Vshunt_filt = VshuntSenseFilt->calculate( sen->Vshunt, reset, T);

  // MAXIM conversion 1-wire Tp plenum temperature
  if ( sensor_tbatt->read() ) sen->Tbatt = sensor_tbatt->fahrenheit() + (TBATT_TEMPCAL);

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  sen->Vbatt =  double(raw_Vbatt)/4096. * 10 + 70;
  sen->Vbatt_filt = VbattSenseFilt->calculate( sen->Vbatt, reset, T);

  // Built-in-test logic.   Run until finger detected
  if ( true && !done_testing )
  {
    done_testing = true;
  }
  else                    // Possible finger detected
  {
    done_testing = false;
  }

  // Built-in-test signal replaces sensor
  if ( !done_testing )
  {
  }

  return ( !done_testing );
}


// Returns any text found between a start and end string inside 'str'
// example: startfooend  -> returns foo
String tryExtractString(String str, const char* start, const char* end)
{
  if (str == "")
  {
    return "";
  }
  int idx = str.indexOf(start);
  if (idx < 0)
  {
    return "";
  }
  int endIdx = str.indexOf(end);
  if (endIdx < 0)
  {
    return "";
  }
  return str.substring(idx + strlen(start), endIdx);
}


// Convert time to decimal for easy lookup
double decimalTime(unsigned long *currentTime, char* tempStr)
{
    Time.zone(GMT);
    *currentTime = Time.now();
    uint32_t year = Time.year(*currentTime);
    uint8_t month = Time.month(*currentTime);
    uint8_t day = Time.day(*currentTime);
    uint8_t hours = Time.hour(*currentTime);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(*currentTime);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          *currentTime = Time.now();
        }
    }
    #ifndef FAKETIME
        uint8_t dayOfWeek = Time.weekday(*currentTime)-1;  // 0-6
        uint8_t minutes   = Time.minute(*currentTime);
        uint8_t seconds   = Time.second(*currentTime);
        if ( debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    #else
        // Rapid time passage simulation to test schedule functions
        uint8_t dayOfWeek = (Time.weekday(*currentTime)-1)*7/6;// minutes = days
        uint8_t hours     = Time.hour(*currentTime)*24/60; // seconds = hours
        uint8_t minutes   = 0; // forget minutes
        uint8_t seconds   = 0; // forget seconds
    #endif
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    return (((( (float(year-2021)*12 + float(month))*30.4375 + float(day))*24.0 + float(hours))*60.0 + float(minutes))*60.0 + \
                        float(seconds));
}
