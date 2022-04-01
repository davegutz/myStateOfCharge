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
#include "command.h"
#include "local_config.h"
#include <math.h>

extern CommandPars cp;          // Various parameters shared at system level
extern RetainedPars rp;         // Various parameters to be static at system level

// Text header
void print_serial_header(void)
{
  Serial.println(F("unit,           hm,                  cTime,        T,         Tb_f,   Tb_f_m, Vb,     voc_stat,   voc,    vsat,    sat,  sel, mod, Ib,       tcharge,   soc_m, soc_ekf, soc,   SOC_m, SOC_ekf, SOC,"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s, %12.3f,%6.3f,    %7.3f,%7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,  %d,    %d,   %d, %7.3f,   %7.3f,   %5.3f,%5.3f,%5.3f,    %5.1f,%5.1f,%5.1f,  %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time, pubList->T,
    pubList->Tbatt, pubList->Tbatt_filt_model,
    pubList->Vbatt, pubList->voc_stat, pubList->voc,
    pubList->vsat, pubList->sat,
    pubList->curr_sel_noamp,
    rp.modeling,
    pubList->Ishunt,
    pubList->tcharge,
    pubList->soc_model, pubList->soc_ekf, pubList->soc, 
    pubList->SOC_model, pubList->SOC_ekf, pubList->SOC, 
    '\0');
}

// Time synchro for web information
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
  if ( rp.debug >= 100 )
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
    if ( rp.debug >= 100 ) Serial.printf("wifi turned off\n");
  }
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL && cp.enable_wifi )
  {
    wifi->last_disconnect = now;   // Give it a chance
    wifi->lastAttempt = now;
    WiFi.on();
    Particle.connect();
    if ( rp.debug >= 100 ) Serial.printf("wifi reattempted\n");
  }
  if ( now-wifi->lastAttempt>=CONFIRMATION_DELAY )
  {
    wifi->connected = Particle.connected();
    if ( rp.debug >= 100 ) Serial.printf("wifi disconnect check\n");
  }
  wifi->particle_connected_last = wifi->particle_connected_now;
}

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(cp.buffer, &cp.pubList);
  if ( rp.debug >= 100 ) Serial.printf("serial_print:  ");
  Serial.println(cp.buffer);  //Serial1.println(cp.buffer);
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
    if ( SensorTbatt->read() ) temp = SensorTbatt->celsius() + (TBATT_TEMPCAL);
    delay(1);
  }
  if ( count<MAX_TEMP_READS )
  {
    Sen->Tbatt = SdTbatt->update(temp);
    if ( rp.debug>102 ) Serial.printf("Temperature read on count=%d\n", count);
  }
  else
  {
    if ( rp.debug>102 ) Serial.printf("Did not read DS18 1-wire temperature sensor, using last-good-value\n");
    // Using last-good-value:  no assignment
  }
}

// Load all others
void load(const boolean reset_free, Sensors *Sen, Pins *myPins,
    Adafruit_ADS1015 *ads_amp, Adafruit_ADS1015 *ads_noamp, const unsigned long now)
{
  static unsigned long int past = now;
  double T = (now - past)/1e3;
  past = now;

  // Current bias.  Feeds into signal conversion, not to duty injection
  cp.curr_bias_noamp = rp.curr_bias_noamp + rp.curr_bias_all + rp.offset;
  cp.curr_bias_amp = rp.curr_bias_amp + rp.curr_bias_all + rp.offset;

  // Read Sensors
  // ADS1015 conversion
  // Amp
  int16_t vshunt_amp_int_0 = 0;
  int16_t vshunt_amp_int_1 = 0;
  if (!Sen->bare_ads_amp)
  {
    Sen->Vshunt_amp_int = ads_amp->readADC_Differential_0_1();
    if ( rp.debug==-14 ){vshunt_amp_int_0 = ads_amp->readADC_SingleEnded(0); vshunt_amp_int_1 = ads_amp->readADC_SingleEnded(1);}
  }
  else
    Sen->Vshunt_amp_int = 0;
  Sen->Vshunt_amp = ads_amp->computeVolts(Sen->Vshunt_amp_int);
  Sen->Ishunt_amp_cal = Sen->Vshunt_amp*shunt_amp_v2a_s + cp.curr_bias_amp;
  // No amp
  int16_t vshunt_noamp_int_0 = 0;
  int16_t vshunt_noamp_int_1 = 0;
  if (!Sen->bare_ads_noamp)
  {
    Sen->Vshunt_noamp_int = ads_noamp->readADC_Differential_0_1();
    if ( rp.debug==-14 ){vshunt_noamp_int_0 = ads_noamp->readADC_SingleEnded(0); vshunt_noamp_int_1 = ads_noamp->readADC_SingleEnded(1);}
  }
  else
    Sen->Vshunt_noamp_int = 0;
  Sen->Vshunt_noamp = ads_noamp->computeVolts(Sen->Vshunt_noamp_int);
  Sen->Ishunt_noamp_cal = Sen->Vshunt_noamp*shunt_noamp_v2a_s + cp.curr_bias_noamp;

  // Print results
  if ( rp.debug==14 ) Serial.printf("reset_free,select,duty,  ||,  vs_na_int,0_na_int,1_na_int,vshunt_na,ishunt_na, ||, vshunt_a_int,0_a_int,1_a_int,vshunt_a,ishunt_a,  ||,  Ishunt,T=,    %d,%d,%ld,  ||,  %d,%d,%d,%7.3f,%7.3f,  ||,  %d,%d,%d,%7.3f,%7.3f,  ||,  %7.3f,%7.3f,\n",
    reset_free, rp.curr_sel_noamp, rp.duty,
    Sen->Vshunt_noamp_int, vshunt_noamp_int_0, vshunt_noamp_int_1, Sen->Vshunt_noamp, Sen->Ishunt_noamp_cal,
    Sen->Vshunt_amp_int, vshunt_amp_int_0, vshunt_amp_int_1, Sen->Vshunt_amp, Sen->Ishunt_amp_cal,
    Sen->Ishunt, T);

  // Current signal selection, based on if there or not.
  // Over-ride 'permanent' with Talk(rp.curr_sel_noamp) = Talk('s')
  if ( !rp.curr_sel_noamp && !Sen->bare_ads_amp)
  {
    Sen->Vshunt = Sen->Vshunt_amp;
    Sen->Ishunt = Sen->Ishunt_amp_cal;
    Sen->shunt_v2a_s = shunt_amp_v2a_s;
  }
  else if ( !Sen->bare_ads_noamp )
  {
    Sen->Vshunt = Sen->Vshunt_noamp;
    Sen->Ishunt = Sen->Ishunt_noamp_cal;
    Sen->shunt_v2a_s = shunt_noamp_v2a_s;
  }
  else
  {
    Sen->Vshunt = 0.;
    Sen->Ishunt = 0.;
    Sen->shunt_v2a_s = shunt_noamp_v2a_s; // noamp preferred, default to that
  }

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A) + rp.vbatt_bias;
  if ( rp.modeling ) Sen->Vbatt = Sen->Vbatt_model;
  else Sen->Vbatt = vbatt_free;

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
  Sen->Wcharge = Sen->Ishunt * NOM_SYS_VOLT;
}

// Filter temperature only
void filter_temp(const int reset_loc, const double t_rlim, Sensors *Sen, General2_Pole* TbattSenseFilt, const double t_bias, double *t_bias_last)
{
  // Rate limit the temperature bias
  if ( reset_loc ) *t_bias_last = t_bias;
  double t_bias_loc = max(min(t_bias, *t_bias_last + t_rlim*Sen->T_temp), *t_bias_last - t_rlim*Sen->T_temp);
  *t_bias_last = t_bias_loc;

  // Filter and add rate limited bias
  Sen->Tbatt_filt = TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP)) + t_bias_loc;
  Sen->Tbatt += t_bias_loc;
}

// Filter all other inputs
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFilt,  General2_Pole* IshuntSenseFilt)
{
  int reset_loc = reset;

  // Shunt
  Sen->Ishunt_filt = IshuntSenseFilt->calculate( Sen->Ishunt, reset_loc, min(Sen->T_filt, F_O_MAX_T));
  
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
    if ( rp.debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    return (((( (float(year-2021)*12 + float(month))*30.4375 + float(day))*24.0 + float(hours))*60.0 + float(minutes))*60.0 + \
                        float(seconds) + float((now-millis_flip)%1000)/1000. );
}

void myDisplay(Adafruit_SSD1306 *display, Sensors *Sen)
{
  static boolean pass = false;
  display->clearDisplay();

  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner
  char dispString[21];
  if ( !pass && cp.model_cutback && rp.modeling )
    sprintf(dispString, "%3.0f %5.2f      ", cp.pubList.Tbatt, cp.pubList.voc);
  else
    sprintf(dispString, "%3.0f %5.2f %5.1f", cp.pubList.Tbatt, cp.pubList.voc, cp.pubList.Ishunt);
  display->println(dispString);

  display->println(F(""));

  display->setTextColor(SSD1306_WHITE);
  char dispStringT[9];
  if ( abs(cp.pubList.tcharge) < 24. )
    sprintf(dispStringT, "%3.0f%5.1f", cp.pubList.amp_hrs_remaining_ekf, cp.pubList.tcharge);
  else
    sprintf(dispStringT, "%3.0f --- ", cp.pubList.amp_hrs_remaining_ekf);
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text
  char dispStringS[4];
  if ( pass || !Sen->saturated )
    sprintf(dispStringS, "%3.0f", min(cp.pubList.amp_hrs_remaining, 999.));
  else if (Sen->saturated)
    sprintf(dispStringS, "SAT");
  display->print(dispStringS);

  display->display();
  pass = !pass;

  if ( rp.debug==5 ) Serial.printf("myDisplay: Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem, %3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
      cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt, cp.pubList.amp_hrs_remaining_ekf, cp.pubList.tcharge, cp.pubList.amp_hrs_remaining);
  if ( rp.debug==-5 ) Serial.printf("Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem,\n%3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
      cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt, cp.pubList.amp_hrs_remaining_ekf, cp.pubList.tcharge, cp.pubList.amp_hrs_remaining);

}


// Write to the D/A converter
uint32_t pwm_write(uint32_t duty, Pins *myPins)
{
    analogWrite(myPins->pwm_pin, duty, pwm_frequency);
    return duty;
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
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      cp.string_complete = true;
     // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      Serial.println(cp.input_string);
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
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      cp.string_complete = true;
     // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      Serial1.println(cp.input_string);
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
        uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
        if ( rp.debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
    sprintf(tempStr, "%4u-%02u-%02uT%02u:%02u:%02u", int(year), month, day, hours, minutes, seconds);
    return ( String(tempStr) );
}
