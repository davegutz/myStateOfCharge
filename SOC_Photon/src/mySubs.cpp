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
extern String input_string;      // A string to hold incoming data
extern boolean string_complete;  // Whether the string is complete
extern boolean stepping;        // Active step adder
extern double step_val;          // Step size
extern boolean vectoring;       // Active battery test vector
extern int8_t vec_num;          // Active vector number
extern unsigned long vec_start; // Start of active vector
extern boolean enable_wifi;     // Enable wifi
extern double socu_free;           // Free integrator state

void sync_time(unsigned long now, unsigned long *last_sync, unsigned long *millis_flip)
{
  if (now - *last_sync > ONE_DAY_MILLIS) 
  {
    *last_sync = millis();

    // Request time synchronization from the Particle Cloud
    if ( Particle.connected() ) Particle.syncTime();

    // Refresh millis() at turn of Time.now
    long time_begin = Time.now();
    while ( Time.now()==time_begin )
    {
      delay(1);
      *millis_flip = millis()%1000;
    }
  }
}

void manage_wifi(unsigned long now, Wifi *wifi)
{
  if ( debug > 2 )
  {
    Serial.printf("P.connected=%i, disconnect check: %ld >=? %ld, turn on check: %ld >=? %ld, confirmation check: %ld >=? %ld, connected=%i, blynk_started=%i,\n",
      Particle.connected(), now-wifi->last_disconnect, DISCONNECT_DELAY, now-wifi->lastAttempt,  CHECK_INTERVAL, now-wifi->lastAttempt, CONFIRMATION_DELAY, wifi->connected, wifi->blynk_started);
  }
  wifi->particle_connected_now = Particle.connected();
  if ( wifi->particle_connected_last && !wifi->particle_connected_now )  // reset timer
  {
    wifi->last_disconnect = now;
  }
  if ( !wifi->particle_connected_now && now-wifi->last_disconnect>=DISCONNECT_DELAY )
  {
    wifi->last_disconnect = now;
    WiFi.off();
    wifi->connected = false;
    if ( debug > 2 ) Serial.printf("wifi turned off\n");
  }
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL && enable_wifi )
  {
    wifi->last_disconnect = now;   // Give it a chance
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
  Serial.println(F("unit,hm, cTime,  Tbatt,Tbatt_filt, Vbatt,Vbatt_filt_obs,  Ishunt,Ishunt_filt_obs,  Wshunt,Wshunt_filt,  VOC_s,  SOCU_s,Vbatt_s, SOCU_f, tcharge,  T"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s, %12.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f, %7.3f,  %7.3f,  %7.3f,%7.3f,  %7.3f,  %6.3f, %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time,
    pubList->Tbatt, pubList->Tbatt_filt,     pubList->Vbatt, pubList->Vbatt_filt_obs,
    pubList->Ishunt, pubList->Ishunt_filt_obs, pubList->Wshunt, pubList->Wshunt_filt,
    pubList->VOC_solved,
    pubList->socu_solved, pubList->Vbatt_solved,
    pubList->socu_free, pubList->tcharge,
    pubList->T, '\0');
}

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(buffer, &pubList);
  if ( debug > 2 ) Serial.printf("serial_print:  ");
  Serial.println(buffer);  //Serial1.println(buffer);
}

// Load temperature only
void load_temp(Sensors *Sen, DS18 *SensorTbatt, SlidingDeadband *SdTbatt)
{
  // Read Sensor
  // MAXIM conversion 1-wire Tp plenum temperature
  uint8_t count = 0;
  double temp = 0.;
  while ( ++count<MAX_TEMP_READS && temp==0)
  {
    if ( SensorTbatt->read() ) temp = SensorTbatt->fahrenheit() + (TBATT_TEMPCAL);
    delay(1);
  }
  if ( count<MAX_TEMP_READS )
  {
    Sen->Tbatt = SdTbatt->update(temp);
    if ( debug>2 ) Serial.printf("Temperature read on count=%d\n", count);
  }
  else
  {
    if ( debug>2 ) Serial.printf("Did not read DS18 1-wire temperature sensor, using last-good-value\n");
    // Using last-good-value:  no assignment
  }
}

// Load all others
void load(const bool reset_free, Sensors *Sen, Pins *myPins, Adafruit_ADS1015 *ads, const unsigned long now,
      SlidingDeadband *SdIshunt, SlidingDeadband *SdVbatt)
{
  // Read Sensor
  // ADS1015 conversion
  if (!Sen->bare_ads)
  {
    Sen->Vshunt_int = ads->readADC_Differential_0_1();
  }
  else
  {
    Sen->Vshunt_int = 0;
  }
  Sen->Vshunt = ads->computeVolts(Sen->Vshunt_int);
  double ishunt_free = Sen->Vshunt*SHUNT_V2A_S + SHUNT_V2A_A;
  Sen->Ishunt = SdIshunt->update(ishunt_free, reset_free);
  if ( debug==-14 ) Serial.printf("reset_free,ishunt_free,Ishunt, %d,%7.3f,%7.3f\n", reset_free, ishunt_free, Sen->Ishunt);

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A);
  Sen->Vbatt = SdVbatt->update(vbatt_free, reset_free);
  if ( debug==-15 ) Serial.printf("reset_free,vbatt_free,vbatt, %d,%7.3f,%7.3f\n", reset_free, vbatt_free, Sen->Vbatt);

  // Vector model
  static double elapsed_loc = 0.;
  if ( vectoring )
  {
    if ( reset_free || (elapsed_loc > t_min_v1[n_v1-1]) )
    {
      vec_start = now;
    }
    elapsed_loc = double(now - vec_start)/1000./60.;
    Sen->Ishunt =  I_T1->interp(elapsed_loc);
    Sen->Vshunt = (Sen->Ishunt - SHUNT_V2A_A) / SHUNT_V2A_S;
    Sen->Vshunt_int = -999;
    Sen->Tbatt =  T_T1->interp(elapsed_loc);
    Sen->Vbatt =  V_T1->interp(elapsed_loc) + Sen->Ishunt*(batt_r1 + batt_r2)*batt_num_cells;
  }
  else elapsed_loc = 0.;

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
  Sen->Wbatt = Sen->Vbatt*Sen->Ishunt - Sen->Ishunt*Sen->Ishunt*(batt_r1 + batt_r2)*batt_num_cells; 

  if ( debug==-6 ) Serial.printf("vectoring,reset_free,vec_start,now,elapsed_loc,Vbatt,Ishunt,Tbatt:  %d,%d,%ld, %ld,%7.3f,%7.3f,%7.3f,%7.3f\n", vectoring, reset_free, vec_start, now, elapsed_loc, Sen->Vbatt, Sen->Ishunt, Sen->Tbatt);
}

// Filter temperature only
void filter_temp(int reset, Sensors *Sen, General2_Pole* TbattSenseFilt)
{
  int reset_loc = reset || vectoring;

  // Temperature
  Sen->Tbatt_filt = TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP));
}

// Filter all other inputs
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFiltObs, General2_Pole* VshuntSenseFiltObs, 
  General2_Pole* VbattSenseFilt,  General2_Pole* VshuntSenseFilt)
{
  int reset_loc = reset || vectoring;

  // Shunt
  Sen->Vshunt_filt = VshuntSenseFilt->calculate( Sen->Vshunt, reset_loc, min(Sen->T_filt, F_MAX_T));
  Sen->Vshunt_filt_obs = VshuntSenseFiltObs->calculate( Sen->Vshunt, reset_loc, min(Sen->T_filt, F_O_MAX_T));
  Sen->Ishunt_filt = Sen->Vshunt_filt*SHUNT_V2A_S + SHUNT_V2A_A;
  Sen->Ishunt_filt_obs = Sen->Vshunt_filt_obs*SHUNT_V2A_S + SHUNT_V2A_A;

  // Voltage
  Sen->Vbatt_filt_obs = VbattSenseFiltObs->calculate(Sen->Vbatt, reset_loc, min(Sen->T_filt, F_O_MAX_T));
  Sen->Vbatt_filt = VbattSenseFilt->calculate(Sen->Vbatt, reset_loc,  min(Sen->T_filt, F_MAX_T));

  // Power
  Sen->Wshunt_filt = Sen->Vbatt_filt*Sen->Ishunt_filt;
}


// Returns any text found between a start and end string inside 'str'
// example: startfooend  -> returns foo
String tryExtractString(String str, const char* start, const char* end)
{
  if (str=="")
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
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip)
{
    *current_time = Time.now();
    uint32_t year = Time.year(*current_time);
    uint8_t month = Time.month(*current_time);
    uint8_t day = Time.day(*current_time);
    uint8_t hours = Time.hour(*current_time);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(*current_time);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          *current_time = Time.now();
          day = Time.day(*current_time);
          hours = Time.hour(*current_time);
        }
    }
    uint8_t dayOfWeek = Time.weekday(*current_time)-1;  // 0-6
    uint8_t minutes   = Time.minute(*current_time);
    uint8_t seconds   = Time.second(*current_time);

    // Convert the string
    time_long_2_str(*current_time, tempStr);

    // Convert the decimal
    if ( debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    return (((( (float(year-2021)*12 + float(month))*30.4375 + float(day))*24.0 + float(hours))*60.0 + float(minutes))*60.0 + \
                        float(seconds) + float((now-millis_flip)%1000)/1000. );
}


void myDisplay(Adafruit_SSD1306 *display)
{
  display->clearDisplay();

  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner
  char dispString[21];
  sprintf(dispString, "%3.0f %5.2f %5.1f", pubList.Tbatt, pubList.Vbatt, pubList.Ishunt_filt_obs);
  display->println(dispString);

  display->println(F(""));

  display->setTextColor(SSD1306_WHITE);
  char dispStringT[9];
  sprintf(dispStringT, "%3.0f%5.1f", min(pubList.socu_solved, 101.), pubList.tcharge);
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text
  char dispStringS[4];
  sprintf(dispStringS, "%3.0f", min(pubList.socu_free, 999.));
  display->print(dispStringS);

  display->display();
}


// Talk Executive
void talk(bool *stepping, double *step_val, bool *vectoring, int8_t *vec_num,
  Battery *MyBattSolved, Battery *MyBattFree)
{
  double SOCU_in = -99.;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (string_complete)
  {
    switch ( input_string.charAt(0) )
    {
      case ( 'D' ):
        switch ( input_string.charAt(1) )
        {
          case ( 'v' ):
            double dv = input_string.substring(2).toFloat();
            MyBattSolved->Dv(dv);
            break;
        }
        break;
      case ( 'S' ):
        switch ( input_string.charAt(1) )
        {
          case ( 'r' ):
            double rscale = input_string.substring(2).toFloat();
            MyBattSolved->Sr(rscale);
            MyBattFree->Sr(rscale);
            break;
        }
        break;
      case ( 'd' ):
        debug = -4;
        break;
      case ( 'm' ):
        SOCU_in = input_string.substring(1).toFloat()/100.;
        socu_free = max(min(SOCU_in, mxepu_bb), mnepu_bb);
        Serial.printf("socu_free=%7.3f\n", socu_free);
        break;
      case ( 'v' ):
        debug = input_string.substring(1).toInt();
        break;
      case ( 'T' ):
        talkT(stepping, step_val, vectoring, vec_num);
        break;
      case ( 'w' ): 
        enable_wifi = true;
        break;
      case ( 'h' ): 
        talkH(step_val, vec_num, MyBattSolved);
        break;
      default:
        Serial.print(input_string.charAt(0)); Serial.println(" unknown");
        break;
    }
    input_string = "";
    string_complete = false;
  }
}

// Talk Tranient Input Settings
void talkT(bool *stepping, double *step_val, bool *vectoring, int8_t *vec_num)
{
  *stepping = false;
  *vectoring = false;
  int num_try = 0;
  switch ( input_string.charAt(1) )
  {
    case ( 's' ): 
      *stepping = true;
      *step_val = input_string.substring(2).toFloat();
      break;
    case ( 'v' ):
      num_try = input_string.substring(2).toInt();
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
      Serial.print(input_string); Serial.println(" unknown.  Try typing 'h'");
  }
}

// Talk Help
void talkH(double *step_val, int8_t *vec_num, Battery *batt_solved)
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");
  Serial.printf("d   dump the summary log\n"); 
  Serial.printf("m=  assign a free memory state in percent - '('truncated 0-100')'\n"); 
  Serial.printf("v=  "); Serial.print(debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("Dv= "); Serial.print(batt_solved->Dv()); Serial.println("    : delta V adder to solved battery calculation, V"); 
  Serial.printf("Sr= "); Serial.print(batt_solved->Sr()); Serial.println("    : Scalar resistor for battery dynamic calculation, V"); 
  Serial.printf("T<?>=  "); 
  Serial.printf("Transient performed with input.   For example:\n");
  Serial.printf("  Ts=<index>  :   index="); Serial.print(*step_val);
  Serial.printf(", stepping=");  Serial.println(stepping);
  Serial.printf("  Tv=<vec_num>  :   vec_num="); Serial.println(*vec_num);
  Serial.printf("    ******Send Tv0 to cancel vector*****\n");
  Serial.printf("   INFO:  vectoringing=");  Serial.println(vectoring);
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
    // add it to the input_string:
    input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      string_complete = true;
     // Remove whitespace
      input_string.trim();
      input_string.replace(" ","");
      input_string.replace("=","");
      Serial.println(input_string);
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
    // add it to the input_string:
    input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      string_complete = true;
     // Remove whitespace
      input_string.trim();
      input_string.replace(" ","");
      input_string.replace("=","");
      Serial1.println(input_string);
    }
  }
}
*/

// For summary prints
String time_long_2_str(const unsigned long current_time, char *tempStr)
{
    uint32_t year = Time.year(current_time);
    uint8_t month = Time.month(current_time);
    uint8_t day = Time.day(current_time);
    uint8_t hours = Time.hour(current_time);

    // Second Sunday Mar and First Sunday Nov; 2:00 am; crude DST handling
    if ( USE_DST)
    {
      uint8_t dayOfWeek = Time.weekday(current_time);     // 1-7
      if (  month>2   && month<12 &&
        !(month==3  && ((day-dayOfWeek)<7 ) && hours>1) &&  // <second Sunday Mar
        !(month==11 && ((day-dayOfWeek)>=0) && hours>0) )  // >=first Sunday Nov
        {
          Time.zone(GMT+1);
          day = Time.day(current_time);
          hours = Time.hour(current_time);
        }
    }
    #ifndef FAKETIME
        uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
        if ( debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    #else
        // Rapid time passage simulation to test schedule functions
        uint8_t dayOfWeek = (Time.weekday(current_time)-1)*7/6;// minutes = days
        uint8_t hours     = Time.hour(current_time)*24/60; // seconds = hours
        uint8_t minutes   = 0; // forget minutes
        uint8_t seconds   = 0; // forget seconds
    #endif
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    return ( String(tempStr) );
}
