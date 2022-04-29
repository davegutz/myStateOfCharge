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
#include "debug.h"

extern CommandPars cp;          // Various parameters shared at system level
extern RetainedPars rp;         // Various parameters to be static at system level


// class Shunt
// constructors
Shunt::Shunt()
: Tweak(), Adafruit_ADS1015(), name_("None"), port_(0x00), bare_(false){}
Shunt::Shunt(const String name, const uint8_t port, double *rp_delta_q_inf, double *rp_tweak_bias, double *cp_curr_bias,
  const double v2a_s)
: Tweak(name, TWEAK_GAIN, TWEAK_MAX_CHANGE, TWEAK_MAX, TWEAK_WAIT, rp_delta_q_inf, rp_tweak_bias),
  Adafruit_ADS1015(),
  name_(name), port_(port), bare_(false), cp_curr_bias_(cp_curr_bias), v2a_s_(v2a_s),
  vshunt_int_(0), vshunt_int_0_(0), vshunt_int_1_(0), vshunt_(0), ishunt_cal_(0)
{
  if ( name_=="No Amp")
    setGain(GAIN_SIXTEEN, GAIN_SIXTEEN); // 16x gain differential and single-ended  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  else if ( name_=="Amp")
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  else
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  if (!begin(port_)) {
    Serial.printf("FAILED to initialize ADS SHUNT MONITOR %s\n", name_.c_str());
    bare_ = true;
  }
  else Serial.printf("SHUNT MONITOR %s initialized\n", name_.c_str());
}
Shunt::~Shunt() {}
// operators
// functions

void Shunt::pretty_print()
{
  Serial.printf("Shunt(%s)::\n", name_.c_str());
  Serial.printf("  port_ =                0x%X; // I2C port used by Acafruit_ADS1015\n", port_);
  Serial.printf("  bare_ =                   %d; // If ADS to be ignored\n", bare_);
  Serial.printf("  *cp_curr_bias_ =    %7.3f; // Global bias, A\n", *cp_curr_bias_);
  Serial.printf("  v2a_s_ =            %7.2f; // Selected shunt conversion gain, A/V\n", v2a_s_);
  Serial.printf("  vshunt_int_ =           %d; // Sensed shunt voltage, count\n", vshunt_int_);
  Serial.printf("  ishunt_cal_ =       %7.3f; // Sensed, calibrated ADC, A\n", ishunt_cal_);
  Serial.printf("Shunt(%s)::", name_.c_str()); Tweak::pretty_print();
  Serial.printf("Shunt(%s)::", name_.c_str()); Adafruit_ADS1015::pretty_print(name_);
}

// load
void Shunt::load()
{
  if (!bare_)
  {
    if ( rp.debug>102 ) Serial.printf("begin %s->readADC_Differential_0_1 at %ld...", name_.c_str(), millis());

    vshunt_int_ = readADC_Differential_0_1();
    
    if ( rp.debug>102 ) Serial.printf("done at %ld\n", millis());
    if ( rp.debug==-14 ) { vshunt_int_0_ = readADC_SingleEnded(0);  vshunt_int_1_ = readADC_SingleEnded(1); }
                    else { vshunt_int_0_ = 0;                       vshunt_int_1_ = 0; }
  }
  else
  {
    vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0;
  }
  vshunt_ = computeVolts(vshunt_int_);
  ishunt_cal_ = vshunt_*v2a_s_ + *cp_curr_bias_;
}


// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s, %12.3f,%6.3f,    %7.3f,%7.3f,   %7.3f,%7.3f,%7.3f,%7.3f,  %d,    %d,   %d, %7.3f,   %7.3f,   %5.3f,%5.3f,%5.3f,%5.3f,    %5.1f,%5.1f,%5.1f,%5.1f,  %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time, pubList->T,
    pubList->Tbatt, pubList->Tbatt_filt_model,
    pubList->Vbatt, pubList->voc_dyn, pubList->voc, pubList->vsat,
    pubList->sat, pubList->curr_sel_noamp, rp.modeling,
    pubList->Ishunt,
    pubList->tcharge,
    pubList->soc_model, pubList->soc_ekf, pubList->soc, pubList->soc_weight,
    pubList->SOC_model, pubList->SOC_ekf, pubList->SOC, pubList->SOC_weight,
    '\0');
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

// Filter temperature only
void filter_temp(const int reset_loc, const double t_rlim, Sensors *Sen, const double t_bias, double *t_bias_last)
{
  // Rate limit the temperature bias
  if ( reset_loc ) *t_bias_last = t_bias;
  double t_bias_loc = max(min(t_bias, *t_bias_last + t_rlim*Sen->T_temp), *t_bias_last - t_rlim*Sen->T_temp);
  *t_bias_last = t_bias_loc;

  // Filter and add rate limited bias
  if ( reset_loc && Sen->Tbatt>40. )
  {
    Sen->Tbatt = RATED_TEMP + t_bias_loc; // Cold startup T=85.5 C
    Sen->Tbatt_filt = Sen->TbattSenseFilt->calculate(RATED_TEMP, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP)) + t_bias_loc;
  }
  else
  {
    Sen->Tbatt_filt = Sen->TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP)) + t_bias_loc;
    Sen->Tbatt += t_bias_loc;
  }
}

// Load all others
void load(const boolean reset_free, const unsigned long now, Sensors *Sen, Pins *myPins)
{
  static unsigned long int past = now;
  double T = (now - past)/1e3;
  past = now;

  // Current bias.  Feeds into signal conversion, not to duty injection
  cp.curr_bias_noamp =  rp.curr_bias_noamp  + rp.curr_bias_all + rp.inj_soft_bias + rp.tweak_bias_noamp;
  cp.curr_bias_amp =    rp.curr_bias_amp    + rp.curr_bias_all + rp.inj_soft_bias + rp.tweak_bias_amp;

  // Read Sensors
  // ADS1015 conversion
  Sen->ShuntAmp->load();
  Sen->ShuntNoAmp->load();

  // Print results
  if ( rp.debug==14 ) Serial.printf("reset_free,select,duty,vs_int_a,vshunt_a,ishunt_cal_a,vs_int_na,vshunt_na,ishunt_cal_na,Ishunt,T=,    %d,%d,%ld,    %d,%7.3f,%7.3f,    %d,%7.3f,%7.3f,    %7.3f,%7.3f,\n",
    reset_free, rp.curr_sel_noamp, rp.duty,
    Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), Sen->ShuntAmp->ishunt_cal(),
    Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), Sen->ShuntNoAmp->ishunt_cal(),
    Sen->Ishunt, T);

  // Current signal selection, based on if there or not.
  // Over-ride 'permanent' with Talk(rp.curr_sel_noamp) = Talk('s')
  if ( !rp.curr_sel_noamp && ! Sen->ShuntAmp->bare())
  {
    Sen->Vshunt = Sen->ShuntAmp->vshunt();
    Sen->Ishunt = Sen->ShuntAmp->ishunt_cal();
    Sen->shunt_v2a_s = Sen->ShuntAmp->v2a_s();
  }
  else if ( !Sen->ShuntNoAmp->bare() )
  {
    Sen->Vshunt = Sen->ShuntNoAmp->vshunt();
    Sen->Ishunt = Sen->ShuntNoAmp->ishunt_cal();
    Sen->shunt_v2a_s = Sen->ShuntNoAmp->v2a_s();
  }
  else
  {
    Sen->Vshunt = 0.;
    Sen->Ishunt = 0.;
    Sen->shunt_v2a_s = Sen->ShuntNoAmp->v2a_s(); // noamp preferred, default to that
  }

  // Vbatt
  if ( rp.debug>102 ) Serial.printf("begin analogRead at %ld...", millis());
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  if ( rp.debug>102 ) Serial.printf("done at %ld\n", millis());
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A) + rp.vbatt_bias;
  if ( rp.modeling ) Sen->Vbatt = Sen->Vbatt_model;
  else Sen->Vbatt = vbatt_free;

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
  Sen->Wcharge = Sen->Ishunt * NOM_SYS_VOLT;
}

// Load temperature only
void load_temp(Sensors *Sen)
{
  // Read Sensor
  // MAXIM conversion 1-wire Tp plenum temperature
  uint8_t count = 0;
  double temp = 0.;
  while ( ++count<MAX_TEMP_READS && temp==0)
  {
    if ( Sen->SensorTbatt->read() ) temp = Sen->SensorTbatt->celsius() + (TBATT_TEMPCAL);
    delay(1);
  }
  if ( count<MAX_TEMP_READS )
  {
    Sen->Tbatt = Sen->SdTbatt->update(temp);
    if ( rp.debug==-103 ) Serial.printf("Temperature %7.3f read on count=%d\n", temp, count);
  }
  else
  {
    Serial.printf("Did not read DS18 1-wire temperature sensor, using last-good-value\n");
    // Using last-good-value:  no assignment
  }
}

// Manage wifi
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

// OLED display drive
void oled_display(Adafruit_SSD1306 *display, Sensors *Sen)
{
  static boolean pass = false;
  display->clearDisplay();

  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner

  boolean no_currents = Sen->ShuntAmp->bare() && Sen->ShuntNoAmp->bare();
  char dispString[21];
  if ( !pass && cp.model_cutback && rp.modeling )
    sprintf(dispString, "%3.0f %5.2f      ", cp.pubList.Tbatt, cp.pubList.voc);
  else
  {
    if (no_currents)
      sprintf(dispString, "%3.0f %5.2f fail", cp.pubList.Tbatt, cp.pubList.voc);
    else
      sprintf(dispString, "%3.0f %5.2f %5.1f", cp.pubList.Tbatt, cp.pubList.voc, cp.pubList.Ishunt);
  }
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
    sprintf(dispStringS, "%3.0f", min(cp.pubList.amp_hrs_remaining_wt, 999.));
  else if (Sen->saturated)
    sprintf(dispStringS, "SAT");
  display->print(dispStringS);

  display->display();
  pass = !pass;

  if ( rp.debug==5 ) debug_5();
  if ( rp.debug==-5 ) debug_m5();  // Arduino plot
}

// Text header
void print_serial_header(void)
{
  Serial.println(F("unit,          hm,                  cTime,        T,         Tb_f,   Tb_f_m,    Vb,  voc_dyn,   voc,    vsat,    sat,  sel, mod, Ib,       tcharge,   soc_m, soc_ekf, soc, soc_wt,   SOC_m, SOC_ekf, SOC, SOC_wt,"));
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
      cp.input_string.replace("\0","");
      cp.input_string.replace(";","");
      cp.input_string.replace(",","");
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      break;  // enable reading multiple inputs
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

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(cp.buffer, &cp.pubList);
  if ( rp.debug >= 100 ) Serial.printf("serial_print:  ");
  Serial.println(cp.buffer);  //Serial1.println(cp.buffer);
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

// Tweak
void tweak_on_new_desat(Sensors *Sen, unsigned long int now)
{

  if ( Sen->ShuntAmp->new_desat(Sen->ShuntAmp->ishunt_cal(), Sen->T, Sen->saturated, now) )
    Sen->ShuntAmp->adjust(now);

  if ( Sen->ShuntNoAmp->new_desat(Sen->ShuntNoAmp->ishunt_cal(), Sen->T, Sen->saturated, now) )
    Sen->ShuntNoAmp->adjust(now);

}

