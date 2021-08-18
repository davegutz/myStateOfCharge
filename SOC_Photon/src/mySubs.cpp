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
extern String inputString;     // a string to hold incoming data
extern boolean stringComplete; // whether the string is complete
extern boolean stepping;       // active step adder
extern double stepVal;                  // Step size


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
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL )
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
  Serial.println(F("unit,hm, cTime,  Tbatt,Tbatt_filt, Vbatt,Vbatt_filt,  Vshunt,Vshunt_filt,  Ishunt,Ishunt_filt,   Wshunt,Wshunt_filt,   SOC,Vbatt_model, SOC_tracked,Vbatt_model_tracked"));
}

// Inputs serial print
void serial_print_inputs(unsigned long now, double T)
{
  sprintf(buffer, "%s,%s,%18.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,  %10.6f,%10.6f,  %7.3f,%7.3f,   %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,\
  %c", \
    pubList.unit.c_str(), pubList.hmString.c_str(), pubList.controlTime,
    pubList.Tbatt, pubList.Tbatt_filt,     pubList.Vbatt, pubList.Vbatt_filt,
    pubList.Vshunt, pubList.Vshunt_filt,
    pubList.Ishunt, pubList.Ishunt_filt, pubList.Wshunt, pubList.Wshunt_filt,
    pubList.SOC, pubList.Vbatt_model,
    pubList.SOC_tracked, pubList.Vbatt_model_tracked, '\0');
  if ( debug > 2 ) Serial.printf("serial_print_inputs:  ");
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
boolean load(int reset, double T, Sensors *sen, DS18 *sensor_tbatt, General2_Pole* VbattSenseFiltObs, General2_Pole* VbattSenseFilt, 
    General2_Pole* TbattSenseFilt, General2_Pole* VshuntSenseFilt, Pins *myPins, Adafruit_ADS1015 *ads,
    Battery *batt, Battery *batt_tracked, double soc_model, double soc_tracked)
{
  static boolean done_testing = false;

  // Read Sensor
  // ADS1015 conversion
  if (!sen->bare_ads)
  {
    sen->Vshunt_int = ads->readADC_Differential_0_1();
  }
  else
  {
    sen->Vshunt_int = 0;
  }
  sen->Vshunt = ads->computeVolts(sen->Vshunt_int);
  sen->Vshunt_filt = VshuntSenseFilt->calculate( sen->Vshunt, reset, T);
  sen->Ishunt = sen->Vshunt*SHUNT_V2A_S + SHUNT_V2A_A;
  sen->Ishunt_filt = sen->Vshunt_filt*SHUNT_V2A_S + SHUNT_V2A_A;
  sen->Wshunt = sen->Vbatt*sen->Ishunt;
  sen->Wshunt_filt = sen->Vbatt_filt*sen->Ishunt_filt;

  // MAXIM conversion 1-wire Tp plenum temperature
  if ( sensor_tbatt->read() ) sen->Tbatt = sensor_tbatt->fahrenheit() + (TBATT_TEMPCAL);
  sen->Tbatt_filt = TbattSenseFilt->calculate( sen->Tbatt, reset, T);

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  sen->Vbatt =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A);
  sen->Vbatt_filt_obs = VbattSenseFiltObs->calculate( sen->Vbatt, reset, T);
  sen->Vbatt_filt = VbattSenseFilt->calculate( sen->Vbatt, reset, T);

  // Battery model 
  sen->Vbatt_model = batt->calculate((sen->Tbatt-32.)*5./9., soc_model, sen->Ishunt);
  sen->Vbatt_model_filt = batt->calculate((sen->Tbatt-32.)*5./9., soc_model, sen->Ishunt_filt);
  sen->Vbatt_model_tracked = batt_tracked->calculate((sen->Tbatt-32.)*5./9., soc_tracked, sen->Ishunt);
  if ( debug==-1 )
  Serial.printf("%7.3f,%7.3f,   %7.3f, %7.3f,%7.3f,%7.3f,\n", soc_tracked, sen->Ishunt,
        sen->Vbatt_filt_obs+double(stepping*stepVal), batt_tracked->vstat(), batt_tracked->vdyn(), batt_tracked->v());

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
          day = Time.day(*currentTime);
          hours = Time.hour(*currentTime);
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


void myDisplay(Adafruit_SSD1306 *display)
{
  display->clearDisplay();

  display->setTextSize(1);             // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE);        // Draw white text
  display->setCursor(0,0);             // Start at top-left corner
  char dispString[21];
  sprintf(dispString, "%3.0f %5.2f %5.1f", pubList.Tbatt, pubList.Vbatt, pubList.Ishunt_filt);
  display->println(dispString);

  display->println(F(""));

  display->setTextSize(2);             // Draw 2X-scale text
  display->setTextColor(SSD1306_WHITE);
  char dispStringS[10];
  // sprintf(dispStringS, "SOC->%4.1f", pubList.SOC);
  sprintf(dispStringS, "%4.1f %4.1f", pubList.SOC, pubList.SOC_tracked);
  display->println(dispStringS);
  display->display();
}


// Talk Executive
void talk(bool *stepping, PID* pid, double *stepVal)
{
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (stringComplete)
  {
    switch ( inputString.charAt(0) )
    {
      case ( 'S' ):
        switch ( inputString.charAt(1) )
        {
          case ( 'd' ):
            pid->Sd(inputString.substring(2).toFloat());
            break;
          case ( 'g' ):
            pid->Sg(inputString.substring(2).toFloat());
            break;
          case ( 't' ):
            pid->St(inputString.substring(2).toFloat());
            break;
        }
        break;
      case ( 'A' ):
        switch ( inputString.charAt(1) )
        {
          case ( 'd' ):
            pid->Ad(inputString.substring(2).toFloat());
            break;
          case ( 'g' ):
            pid->Ag(inputString.substring(2).toFloat());
            break;
          case ( 't' ):
            pid->At(inputString.substring(2).toFloat());
            break;
        }
        break;
      case ( 'v' ):
        debug = inputString.substring(1).toInt();
        break;
      case ( 'T' ):
        talkT(stepping, stepVal);
        break;
      case ('h'): 
        talkh(pid, stepVal);
        break;
      default:
        Serial.print(inputString.charAt(0)); Serial.println(" unknown");
        break;
    }
    inputString = "";
    stringComplete = false;
  }
}

// Talk Tranient Input Settings
void talkT(bool *stepping, double *stepVal)
{
  *stepping = false;
  switch ( inputString.charAt(1) )
  {
    case ( 's' ): 
      *stepping = true;
      *stepVal = inputString.substring(2).toFloat();
      break;
    default:
      Serial.print(inputString); Serial.println(" unknown.  Try typing 'h'");
  }
}

// Talk Help
void talkh(PID *pid, double *stepVal)
{
  Serial.print("Sd= "); Serial.print(pid->Sd());  Serial.println("    : PID derivative deadband scalar [1]");
  Serial.print("Ad= "); Serial.print(pid->Ad());  Serial.println("    : PID derivative deadband adder [0]");
  Serial.print("   ref:   DB= "); Serial.print(pid->DB,3); Serial.println("    : present PID deadband");
  Serial.print("   ref:   tau= "); Serial.print(pid->tau,3); Serial.println("    : present PID lead");
  Serial.print("Sg= "); Serial.print(pid->Sg());  Serial.println("    : PID loopgain (integral) scalar [1]");
  Serial.print("Ag= "); Serial.print(pid->Ag());  Serial.println("    : PID loopgain (integral) adder [0]");
  Serial.print("   ref:   G= "); Serial.print(pid->G); Serial.println("    : present PID gain, frac/V");
  Serial.print("St= "); Serial.print(pid->St());  Serial.println("    : PID lead (proportional) tlead scalar [1]");
  Serial.print("At="); Serial.print(pid->At());  Serial.println("    : PID lead (proportional) tlead adder [0]");
  Serial.print("v=  "); Serial.print(debug);     Serial.println("    : verbosity, 0-10. 2 for save csv [0]");
  Serial.print("T<?>=  "); 
  Serial.println("Transient performed with input");
  Serial.print("          :   stepVal="); Serial.println(*stepVal);
  Serial.print(", stepping=");  Serial.print(stepping);
}
