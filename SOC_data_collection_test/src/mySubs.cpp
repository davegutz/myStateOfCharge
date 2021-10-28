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
#include <math.h>

extern int8_t debug;
extern Publish pubList;
extern char buffer[256];
extern String inputString;      // A string to hold incoming data
extern boolean stringComplete;  // Whether the string is complete
extern boolean enable_wifi;     // Enable wifi

void manage_wifi(unsigned long now, Wifi *wifi)
{
  if ( debug > 2 )
  {
    Serial.printf("P.connected=%i, disconnect check: %ld >=? %ld, turn on check: %ld >=? %ld, confirmation check: %ld >=? %ld, connected=%i, blynk_started=%i,\n",
      Particle.connected(), now-wifi->lastDisconnect, DISCONNECT_DELAY, now-wifi->lastAttempt,  CHECK_INTERVAL, now-wifi->lastAttempt, CONFIRMATION_DELAY, wifi->connected, wifi->blynk_started);
  }
  wifi->particle_connected_now = Particle.connected();
  if ( wifi->particle_connected_last && !wifi->particle_connected_now )  // reset timer
  {
    wifi->lastDisconnect = now;
  }
  if ( !wifi->particle_connected_now && now-wifi->lastDisconnect>=DISCONNECT_DELAY )
  {
    wifi->lastDisconnect = now;
    WiFi.off();
    wifi->connected = false;
    if ( debug > 2 ) Serial.printf("wifi turned off\n");
  }
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL && enable_wifi )
  {
    wifi->lastDisconnect = now;   // Give it a chance
    wifi->lastAttempt = now;
    WiFi.on();
    Particle.connect();
    if ( debug > 2 ) Serial.printf("wifi reattempted\n");
  }
  if ( now-wifi->lastAttempt>=CONFIRMATION_DELAY )
  {
    wifi->connected = Particle.connected();
    if ( debug > 2 ) Serial.printf("wifi disconnect check\n");
  }
  wifi->particle_connected_last = wifi->particle_connected_now;
}


// Text header
void print_serial_header(void)
{
  Serial.println(F("unit,hm, cTime,  Tbatt, Vbatt, Vshunt_01, Ishunt_01, Vshunt_amp_01, Ishunt_amp_01, T_filt"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s,%18.3f,  %7.3f,  %7.3f, %10.6f, %7.3f, %7.3f, %7.3f, %7.3f, %c", \
    pubList->unit.c_str(), pubList->hmString.c_str(), pubList->controlTime,
    pubList->Tbatt,  pubList->Vbatt,
    pubList->Vshunt_01, pubList->Ishunt_01,
    pubList->Vshunt_amp_01, pubList->Ishunt_amp_01,
    pubList->T, '\0');
}

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(buffer, &pubList);
  if ( debug > 2 ) Serial.printf("serial_print:  ");
  Serial.println(buffer);  //Serial1.println(buffer);
}

// Load
void load(const bool reset_free, Sensors *sen, DS18 *sensor_tbatt, Pins *myPins, Adafruit_ADS1015 *ads, Adafruit_ADS1015 *ads_amp, const unsigned long now)
{
  static unsigned long int past = now;
  double T = (now - past)/1e3;
  past = now;

  // Read Sensor
  // ADS1015 conversion
  int16_t vshunt_int_0 = 0;
  double vshunt_0 = 0;
  int16_t vshunt_int_1 = 0;
  double vshunt_1 = 0;
  if (!sen->bare_ads)
  {
    sen->Vshunt_int_01 = ads->readADC_Differential_0_1();
    vshunt_int_0 = ads->readADC_SingleEnded(0);
    vshunt_int_1 = ads->readADC_SingleEnded(1);
  }
  else
  {
    sen->Vshunt_int_01 = 0;
  }
  sen->Vshunt_01 = ads->computeVolts(sen->Vshunt_int_01);
  vshunt_0 = ads->computesVolts(vshunt_int_0);
  vshunt_1 = ads->computesVolts(vshunt_int_1);
  sen->Ishunt_01 = sen->Vshunt_01*SHUNT_V2A_S + SHUNT_V2A_A;

  int16_t vshunt_amp_int_0 = 0;
  double vshunt_amp_0 = 0;
  double vshunt_amp_1 = 0;
  int16_t vshunt_amp_int_1 = 0;
  if (!sen->bare_ads_amp)
  {
    sen->Vshunt_amp_int_01 = ads_amp->readADC_Differential_0_1();
    vshunt_amp_int_0 = ads_amp->readADC_SingleEnded(0);
    vshunt_amp_int_1 = ads_amp->readADC_SingleEnded(1);
  }
  else
  {
    sen->Vshunt_amp_int_01 = 0;
  }
  sen->Vshunt_amp_01 = ads_amp->computeVolts(sen->Vshunt_amp_int_01);
  vshunt_amp_0 = ads_amp->computesVolts(vshunt_amp_int_0);
  vshunt_amp_1 = ads_amp->computesVolts(vshunt_amp_int_1);
  sen->Ishunt_amp_01 = sen->Vshunt_amp_01*SHUNT_AMP_V2A_S + SHUNT_AMP_V2A_A;
 
  Serial.printf("vshunt_int,0_int,1_int,v0,v1,Vshunt,Ishunt,|||||,vshunt_amp_int,0_amp_int,1_amp_int,v0_amp,v1_amp,Vshunt_amp,Ishunt_amp,  T, %d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f, ||||, %d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f,   %7.3f,\n",
      sen->Vshunt_int_01, vshunt_int_0, vshunt_int_1, vshunt_0, vshunt_1, sen->Vshunt_01, sen->Ishunt_01,
      sen->Vshunt_amp_int_01, vshunt_amp_int_0, vshunt_amp_int_1,  vshunt_amp_0, vshunt_amp_1, sen->Vshunt_amp_01, sen->Ishunt_amp_01,
      T);

  // MAXIM conversion 1-wire Tp plenum temperature  (0.750 seconds blocking update)
  //if ( sensor_tbatt->read() ) sen->Tbatt = sensor_tbatt->fahrenheit() + (TBATT_TEMPCAL);
  sen->Tbatt = -9.;

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  sen->Vbatt =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A);
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
          day = Time.day(*currentTime);
          hours = Time.hour(*currentTime);
        }
    }
    uint8_t dayOfWeek = Time.weekday(*currentTime)-1;  // 0-6

    // Convert the string
    time_long_2_str(*currentTime, tempStr);

    // Convert the decimal
    if ( debug > 5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    return(double(millis())/1000.);  // get millisecond to print in serial print
}



// Talk Executive
void talk()
{
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (stringComplete)
  {
    switch ( inputString.charAt(0) )
    {
      case ( 'd' ):
        debug = -3;
        break;
      case ( 'v' ):
        debug = inputString.substring(1).toInt();
        break;
      case ( 'w' ): 
        enable_wifi = true;
        break;
      case ( 'h' ): 
        talkH();
        break;
      default:
        Serial.print(inputString.charAt(0)); Serial.println(" unknown");
        break;
    }
    inputString = "";
    stringComplete = false;
  }
}


// Talk Help
void talkH()
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");
  Serial.printf("v=  "); Serial.print(debug); Serial.println("    : verbosity, -128 - +128. 2 for save csv [2]");
  Serial.printf("w   turn on wifi = "); Serial.println(enable_wifi);
  Serial.printf("h   this menu\n");
}


/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      stringComplete = true;
     // Remove whitespace
      inputString.trim();
      inputString.replace(" ","");
      inputString.replace("=","");
      Serial.println(inputString);
    }
  }
}

// For summary prints
String time_long_2_str(const unsigned long currentTime, char *tempStr)
{
    uint32_t year = Time.year(currentTime);
    uint8_t month = Time.month(currentTime);
    uint8_t day = Time.day(currentTime);
    uint8_t hours = Time.hour(currentTime);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(currentTime);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          day = Time.day(currentTime);
          hours = Time.hour(currentTime);
        }
    }
    #ifndef FAKETIME
        uint8_t dayOfWeek = Time.weekday(currentTime)-1;  // 0-6
        uint8_t minutes   = Time.minute(currentTime);
        uint8_t seconds   = Time.second(currentTime);
        if ( debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    #else
        // Rapid time passage simulation to test schedule functions
        uint8_t dayOfWeek = (Time.weekday(currentTime)-1)*7/6;// minutes = days
        uint8_t hours     = Time.hour(currentTime)*24/60; // seconds = hours
        uint8_t minutes   = 0; // forget minutes
        uint8_t seconds   = 0; // forget seconds
    #endif
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    return ( String(tempStr) );
}
