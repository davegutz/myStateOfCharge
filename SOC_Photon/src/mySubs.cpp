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
extern boolean stepping;        // Active step adder
extern double stepVal;          // Step size
extern boolean vectoring;       // Active battery test vector
extern int8_t vec_num;          // Active vector number
extern unsigned long vec_start;  // Start of active vector

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
  Serial.println(F("unit,hm, cTime,  Tbatt,Tbatt_filt, Vbatt,Vbatt_filt_obs,  Vshunt,Vshunt_filt,  Ishunt,Ishunt_filt_obs,   Wshunt,Wshunt_filt,   SOC,Vbatt_m,   SOC_s,Vbatt_m_s, SOC_e, tcharge,  T_filt"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s,%18.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,  %10.6f,%10.6f,  %7.3f,%7.3f,   %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,  %7.3f,  %7.3f, %c", \
    pubList->unit.c_str(), pubList->hmString.c_str(), pubList->controlTime,
    pubList->Tbatt, pubList->Tbatt_filt,     pubList->Vbatt, pubList->Vbatt_filt_obs,
    pubList->Vshunt, pubList->Vshunt_filt,
    pubList->Ishunt, pubList->Ishunt_filt_obs, pubList->Wshunt, pubList->Wshunt_filt,
    pubList->SOC, pubList->Vbatt_model,
    pubList->SOC_solved, pubList->Vbatt_model_solved,
    pubList->SOC_free, pubList->tcharge,
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
void load(const bool reset_soc, Sensors *sen, DS18 *sensor_tbatt, Pins *myPins, Adafruit_ADS1015 *ads, Battery *batt, const unsigned long now)
{
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
  sen->Ishunt = sen->Vshunt*SHUNT_V2A_S + SHUNT_V2A_A;

  // MAXIM conversion 1-wire Tp plenum temperature
  if ( sensor_tbatt->read() ) sen->Tbatt = sensor_tbatt->fahrenheit() + (TBATT_TEMPCAL);

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  sen->Vbatt =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A);

  // Vector model
  double elapsed_loc = 0.;
  if ( vectoring )
  {
    if ( reset_soc || (elapsed_loc > t_min_v1[n_v1-1]) )
    {
      vec_start = now;
    }
    elapsed_loc = double(now - vec_start)/1000./60.;
    sen->Ishunt =  I_T1->interp(elapsed_loc);
    sen->Vshunt = (sen->Ishunt - SHUNT_V2A_A) / SHUNT_V2A_S;
    sen->Vshunt_int = -999;
    sen->Tbatt =  T_T1->interp(elapsed_loc);
    sen->Vbatt =  V_T1->interp(elapsed_loc) + sen->Ishunt*(batt_r1 + batt_r2)*batt_num_cells;
  }
  else elapsed_loc = 0.;

  // Power calculation
  sen->Wshunt = sen->Vbatt*sen->Ishunt;
  sen->Wbatt = sen->Vbatt*sen->Ishunt - sen->Ishunt*sen->Ishunt*(batt_r1 + batt_r2)*batt_num_cells; 

  if ( debug == -6 ) Serial.printf("vectoring,reset_soc,vec_start,now,elapsed_loc,Vbatt,Ishunt,Tbatt:  %d,%d,%ld,%ld,%7.3f,%7.3f,%7.3f,%7.3f\n", vectoring, reset_soc, vec_start, now, elapsed_loc, sen->Vbatt, sen->Ishunt, sen->Tbatt);
}

// Filter inputs
void filter(int reset, Sensors *sen, General2_Pole* VbattSenseFiltObs, General2_Pole* VshuntSenseFiltObs, 
  General2_Pole* VbattSenseFilt,  General2_Pole* TbattSenseFilt, General2_Pole* VshuntSenseFilt)
{
  int reset_loc = reset || vectoring;

  // Shunt
  sen->Vshunt_filt = VshuntSenseFilt->calculate( sen->Vshunt, reset_loc, min(sen->T, F_MAX_T));
  sen->Vshunt_filt_obs = VshuntSenseFiltObs->calculate( sen->Vshunt, reset_loc, min(sen->T, F_O_MAX_T));
  sen->Ishunt_filt = sen->Vshunt_filt*SHUNT_V2A_S + SHUNT_V2A_A;
  sen->Ishunt_filt_obs = sen->Vshunt_filt_obs*SHUNT_V2A_S + SHUNT_V2A_A;

  // Temperature
  sen->Tbatt_filt = TbattSenseFilt->calculate(sen->Tbatt, reset_loc,  min(sen->T, F_MAX_T));

  // Voltage
  sen->Vbatt_filt_obs = VbattSenseFiltObs->calculate(sen->Vbatt, reset_loc, min(sen->T, F_O_MAX_T));
  sen->Vbatt_filt = VbattSenseFilt->calculate(sen->Vbatt, reset_loc,  min(sen->T, F_MAX_T));

  // Power
  sen->Wshunt_filt = sen->Vbatt_filt*sen->Ishunt_filt;
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
    uint8_t dayOfWeek = Time.weekday(*currentTime)-1;  // 0-6
    uint8_t minutes   = Time.minute(*currentTime);
    uint8_t seconds   = Time.second(*currentTime);

    // Convert the string
    time_long_2_str(*currentTime, tempStr);

    // Convert the decimal
    if ( debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    return (((( (float(year-2021)*12 + float(month))*30.4375 + float(day))*24.0 + float(hours))*60.0 + float(minutes))*60.0 + \
                        float(seconds));
}


void myDisplay(Adafruit_SSD1306 *display)
{
  display->clearDisplay();

  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner
  char dispString[21];
  sprintf(dispString, "%3.0f %5.2f %5.1f", pubList.Tbatt, pubList.Vbatt, pubList.Ishunt_filt);
  display->println(dispString);

  display->println(F(""));

  display->setTextColor(SSD1306_WHITE);
  char dispStringT[8];
  sprintf(dispStringT, "%3.0f %3.0f", min(pubList.SOC_solved, 101.), min(pubList.SOC_free, 101.));
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text
  char dispStringS[5];
  sprintf(dispStringS, " %3.0f", min(pubList.SOC, 101.));
  display->print(dispStringS);

  display->display();
}


// Talk Executive
void talk(bool *stepping, double *stepVal, bool *vectoring, int8_t *vec_num,
  Battery *myBatt, Battery *myBatt_solved, Battery *myBatt_free)
{
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (stringComplete)
  {
    switch ( inputString.charAt(0) )
    {
      case ( 'S' ):
        switch ( inputString.charAt(1) )
        {
          case ( 'r' ):
            double rscale = inputString.substring(2).toFloat();
            myBatt->Sr(rscale);
            myBatt_solved->Sr(rscale);
            myBatt_free->Sr(rscale);
            break;
        }
        break;
      case ( 'd' ):
        debug = -3;
        break;
      case ( 'v' ):
        debug = inputString.substring(1).toInt();
        break;
      case ( 'T' ):
        talkT(stepping, stepVal, vectoring, vec_num);
        break;
      case ('h'): 
        talkH(stepVal, vec_num);
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
void talkT(bool *stepping, double *stepVal, bool *vectoring, int8_t *vec_num)
{
  *stepping = false;
  *vectoring = false;
  int num_try = 0;
  switch ( inputString.charAt(1) )
  {
    case ( 's' ): 
      *stepping = true;
      *stepVal = inputString.substring(2).toFloat();
      break;
    case ( 'v' ):
      num_try = inputString.substring(2).toInt();
      if ( num_try && num_try>0 && num_try<=NUM_VEC )
      {
        *vectoring = true;
        *vec_num = num_try;
      }
      else
      {
        *vectoring = false;
        *vec_num = 0;
      }
      break;
    default:
      Serial.print(inputString); Serial.println(" unknown.  Try typing 'h'");
  }
}

// Talk Help
void talkH(double *stepVal, int8_t *vec_num)
{
  Serial.println("Help for serial talk.   Entries and current values.  All entries follwed by CR");
  Serial.printf("d   dump the summary log"); 
  Serial.print("v=  "); Serial.print(debug);     Serial.println("    : verbosity, 0-10. 2 for save csv [0]");
  Serial.print("T<?>=  "); 
  Serial.println("Transient performed with input.   For example:");
  Serial.print("  Ts=<stepVal>  :   stepVal="); Serial.println(*stepVal);
  Serial.print(", stepping=");  Serial.print(stepping);
  Serial.print("  Tv=<vec_num>  :   vec_num="); Serial.println(*vec_num);
  Serial.print(", vectoringing=");  Serial.print(vectoring);
  Serial.println("");
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
// Copy for bluetooth connected to TX/RX
/*
void serialEvent1()
{
  while (Serial1.available())
  {
    // get the new byte:
    char inChar = (char)Serial1.read();
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
      Serial1.println(inputString);
    }
  }
}
*/

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
