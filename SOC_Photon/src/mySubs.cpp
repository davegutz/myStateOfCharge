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
#include "retained.h"
#include "command.h"
#include "local_config.h"
#include <math.h>

// extern int8_t debug;
// extern Publish pubList;
// extern char buffer[256];
// extern String input_string;     // A string to hold incoming data
// extern boolean string_complete; // Whether the string is complete
// extern boolean stepping;        // Active step adder
// extern double step_val;         // Step size
// extern boolean vectoring;       // Active battery test vector
// extern int8_t vec_num;          // Active vector number
// extern unsigned long vec_start; // Start of active vector
// extern boolean enable_wifi;     // Enable wifi
extern CommandPars cp;          // Various parameters shared at system level
extern RetainedPars rp;         // Various parameters to be static at system level

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
  if ( cp.debug > 2 )
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
    if ( cp.debug > 2 ) Serial.printf("wifi turned off\n");
  }
  if ( now-wifi->lastAttempt>=CHECK_INTERVAL && cp.enable_wifi )
  {
    wifi->last_disconnect = now;   // Give it a chance
    wifi->lastAttempt = now;
    WiFi.on();
    Particle.connect();
    if ( cp.debug > 2 ) Serial.printf("wifi reattempted\n");
  }
  if ( now-wifi->lastAttempt>=CONFIRMATION_DELAY )
  {
    wifi->connected = Particle.connected();
    if ( cp.debug > 2 ) Serial.printf("wifi disconnect check\n");
  }
  wifi->particle_connected_last = wifi->particle_connected_now;
}


// Text header
void print_serial_header(void)
{
  Serial.println(F("unit,hm, cTime,  Tbatt,Tbatt_filt, Vbatt,Vbatt_filt_obs,   Ishunt_amp,Ishunt_amp_filt_obs,  Wshunt_amp,Wshunt_amp_filt,  Ishunt,Ishunt_filt_obs,  Wshunt,Wshunt_filt,  VOC_s,  SOCU_s,Vbatt_s, SOCU_f, tcharge,  T, SOCU_m"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s, %12.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f,  %7.3f,%7.3f, %7.3f,  %7.3f,  %7.3f,%7.3f,  %7.3f,  %6.3f, %7.3f, %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time,
    pubList->Tbatt, pubList->Tbatt_filt,     pubList->Vbatt, pubList->Vbatt_filt_obs,
    pubList->Ishunt_amp, pubList->Ishunt_amp_filt_obs, pubList->Wshunt_amp, pubList->Wshunt_amp_filt,
    pubList->Ishunt, pubList->Ishunt_filt_obs, pubList->Wshunt, pubList->Wshunt_filt,
    pubList->VOC_solved,
    pubList->socu_solved, pubList->Vbatt_solved,
    pubList->socu_free, pubList->tcharge,
    pubList->T, pubList->socu_model, '\0');
}

// Inputs serial print
void serial_print(unsigned long now, double T)
{
  create_print_string(cp.buffer, &cp.pubList);
  if ( cp.debug > 2 ) Serial.printf("serial_print:  ");
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
    if ( SensorTbatt->read() ) temp = SensorTbatt->fahrenheit() + (TBATT_TEMPCAL);
    delay(1);
  }
  if ( count<MAX_TEMP_READS )
  {
    Sen->Tbatt = SdTbatt->update(temp);
    if ( cp.debug>2 ) Serial.printf("Temperature read on count=%d\n", count);
  }
  else
  {
    if ( cp.debug>2 ) Serial.printf("Did not read DS18 1-wire temperature sensor, using last-good-value\n");
    // Using last-good-value:  no assignment
  }
}

// Load all others
void load(const boolean reset_free, Sensors *Sen, Pins *myPins,
    Adafruit_ADS1015 *ads, Adafruit_ADS1015 *ads_amp, const unsigned long now,
    SlidingDeadband *SdIshunt, SlidingDeadband *SdIshunt_amp, SlidingDeadband *SdVbatt)
{
  static unsigned long int past = now;
  double T = (now - past)/1e3;
  past = now;

  // TODO:   double inj_bias(const unsigned long now)
  // Calculate injection amounts from user inputs (talk).
  // One-sided because PWM voltage >0.
  double t = now/1e3;
  double sin_bias = 0.;
  double tri_bias = 0.;
  double inj_bias = 0.;
  static double square_bias = 0.;
  static double t_last_sq = 0.;
  static double t_last_tri = 0.;
  double dt = 0.;
  switch ( rp.type )
  {
    case ( 0 ):   // Nothing
      sin_bias = 0.;
      square_bias = 0.;
      tri_bias = 0.;
      break;
    case ( 1 ):   // Sine wave
      sin_bias = rp.amp*(1. + sin(rp.freq*t));
      square_bias = 0.;
      tri_bias = 0.;
      break;
    case ( 2 ):   // Square wave
      sin_bias = 0.;
      double sq_dt;
      if ( rp.freq>1e-6 )
        sq_dt = 1. / rp.freq * PI;
      else
        sq_dt = t;
      if ( t-t_last_sq > sq_dt )
      {
        t_last_sq = t;
        if ( square_bias == 0. )
          square_bias = 2.*rp.amp;
        else
          square_bias = 0.0;
      }
      tri_bias = 0.;
      break;
    case ( 3 ):   // Triangle wave
      sin_bias = 0.;
      square_bias =  0.;
      tri_bias = 0.;
      double tri_dt;
      if ( rp.freq>1e-6 )
        tri_dt = 1. / rp.freq * PI;
      else
        tri_dt = t;
      if ( t-t_last_tri > tri_dt )
        t_last_tri = t;
      dt = t-t_last_tri;
      if ( dt <= tri_dt/2. )
        tri_bias = dt / (tri_dt/2.) * 2. * rp.amp;
      else
        tri_bias = (tri_dt-dt) / (tri_dt/2.) * 2. * rp.amp;
      if ( cp.debug==-41 )
        Serial.printf("tri_dt,dt,t_last_tri,tri_bias=%7.3f,%7.3f,%7.3f,%7.3f,\n",
            tri_dt, dt, t_last_tri, tri_bias);
      break;
    default:
      break;
  }
  inj_bias = sin_bias + square_bias + tri_bias;
  rp.duty = min(uint32_t(inj_bias / bias_gain), uint32_t(255.));
  if ( cp.debug==-41 )
  Serial.printf("type,amp,freq,sin,square,tri,inj,duty,tnow=%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,   %ld,  %7.3f,\n",
            rp.type, rp.amp, rp.freq, sin_bias, square_bias, tri_bias, rp.duty, t);

  // Current bias.  Feeds into signal conversion, not to duty injection
  Sen->curr_bias = rp.curr_bias + rp.offset;
  Sen->amp_curr_bias = rp.curr_amp_bias + rp.offset;

  // Read Sensors
  // ADS1015 conversion
  int16_t vshunt_int_0 = 0;
  int16_t vshunt_int_1 = 0;
  if (!Sen->bare_ads)
  {
    Sen->Vshunt_int = ads->readADC_Differential_0_1();
    if ( cp.debug==-14 ){vshunt_int_0 = ads->readADC_SingleEnded(0); vshunt_int_1 = ads->readADC_SingleEnded(1);}
  }
  else
  {
    Sen->Vshunt_int = 0;
  }
  Sen->Vshunt = ads->computeVolts(Sen->Vshunt_int);
  double ishunt_free = Sen->Vshunt*SHUNT_V2A_S + SHUNT_V2A_A + Sen->curr_bias;
  Sen->Ishunt = SdIshunt->update(ishunt_free, reset_free);

  int16_t vshunt_amp_int_0 = 0;
  int16_t vshunt_amp_int_1 = 0;
  if (!Sen->bare_ads_amp)
  {
    Sen->Vshunt_amp_int = ads_amp->readADC_Differential_0_1();
    if ( cp.debug==-14 ){vshunt_amp_int_0 = ads_amp->readADC_SingleEnded(0); vshunt_amp_int_1 = ads_amp->readADC_SingleEnded(1);}
  }
  else
  {
    Sen->Vshunt_amp_int = 0;
  }
  Sen->Vshunt_amp = ads_amp->computeVolts(Sen->Vshunt_amp_int);
  double ishunt_amp_free = Sen->Vshunt_amp*SHUNT_AMP_V2A_S + SHUNT_AMP_V2A_A + Sen->amp_curr_bias;
  Sen->Ishunt_amp = SdIshunt_amp->update(ishunt_amp_free, reset_free);
  if ( cp.debug==-14 ) Serial.printf("reset_free,vshunt_int,0_int,1_int,ishunt_free,Ishunt,Ishunt_filt,Ishunt_filt_obs,||,vshunt_amp_int,0_amp_int,1_amp_int,ishunt_amp_free,Ishunt_amp,Ishunt_amp_filt,Ishunt_amp_filt_obs,T, %d,%d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f,||,%d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f\n",
    reset_free,
    Sen->Vshunt_int, vshunt_int_0, vshunt_int_1, ishunt_free, Sen->Ishunt, Sen->Ishunt_filt, Sen->Ishunt_filt_obs, 
    Sen->Vshunt_amp_int, vshunt_amp_int_0, vshunt_amp_int_1, ishunt_amp_free, Sen->Ishunt_amp, Sen->Ishunt_amp_filt, Sen->Ishunt_amp_filt_obs,
    T);
  if ( cp.debug==-16 ) Serial.printf("ishunt_free,Ishunt,Ishunt_filt,Ishunt_filt_obs,||,ishunt_amp_free,Ishunt_amp,Ishunt_amp_filt,Ishunt_amp_filt_obs,T, %7.3f,%7.3f,%7.3f,%7.3f,||,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f\n",
    ishunt_free, Sen->Ishunt, Sen->Ishunt_filt, Sen->Ishunt_filt_obs, 
    ishunt_amp_free, Sen->Ishunt_amp, Sen->Ishunt_amp_filt, Sen->Ishunt_amp_filt_obs,
    T);

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A) + rp.vbatt_bias;
  if ( rp.modeling ) Sen->Vbatt = Sen->Vbatt_model;
  else Sen->Vbatt = SdVbatt->update(vbatt_free, reset_free);
  if ( cp.debug==-15 ) Serial.printf("reset_free,vbatt_free,vbatt, %d,%7.3f,%7.3f\n", reset_free, vbatt_free, Sen->Vbatt);

  // Vector model
  static double elapsed_loc = 0.;
  if ( cp.vectoring )
  {
    if ( reset_free || (elapsed_loc > t_min_v1[n_v1-1]) )
    {
      cp.vec_start = now;
    }
    elapsed_loc = double(now - cp.vec_start)/1000./60.;
    Sen->Ishunt =  I_T1->interp(elapsed_loc);
    Sen->Vshunt = (Sen->Ishunt - SHUNT_V2A_A - Sen->curr_bias) / SHUNT_V2A_S;
    Sen->Vshunt_int = -999;
    Sen->Ishunt_amp =  Sen->Ishunt;
    Sen->Vshunt_amp = Sen->Vshunt;
    Sen->Vshunt_amp_int = -999;
    Sen->Tbatt =  T_T1->interp(elapsed_loc);
    Sen->Vbatt =  V_T1->interp(elapsed_loc) + Sen->Ishunt*(batt_r1 + batt_r2)*double(batt_num_cells);
  }
  else elapsed_loc = 0.;

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
  Sen->Wcharge = Sen->Vbatt*Sen->Ishunt - Sen->Ishunt*Sen->Ishunt*(batt_r1 + batt_r2)*double(batt_num_cells); 
  Sen->Wshunt_amp = Sen->Vbatt*Sen->Ishunt_amp;
  Sen->Wcharge_amp = Sen->Vbatt*Sen->Ishunt_amp - Sen->Ishunt_amp*Sen->Ishunt_amp*(batt_r1 + batt_r2)*double(batt_num_cells); 

  if ( cp.debug==-6 ) Serial.printf("cp.vectoring,reset_free,cp.vec_start,now,elapsed_loc,Vbatt,Ishunt,Ishunt_amp,Tbatt:  %d,%d,%ld, %ld,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f\n", cp.vectoring, reset_free, cp.vec_start, now, elapsed_loc, Sen->Vbatt, Sen->Ishunt, Sen->Ishunt_amp, Sen->Tbatt);
}

// Filter temperature only
void filter_temp(int reset, Sensors *Sen, General2_Pole* TbattSenseFilt)
{
  int reset_loc = reset || cp.vectoring;

  // Temperature
  Sen->Tbatt_filt = TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP));
}

// Filter all other inputs
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFiltObs,
  General2_Pole* VshuntSenseFiltObs,  General2_Pole* VshuntAmpSenseFiltObs, 
  General2_Pole* VbattSenseFilt,  General2_Pole* VshuntSenseFilt,  General2_Pole* VshuntAmpSenseFilt)
{
  int reset_loc = reset || cp.vectoring;

  // Shunt
  Sen->Vshunt_filt = VshuntSenseFilt->calculate( Sen->Vshunt, reset_loc, min(Sen->T_filt, F_MAX_T));
  Sen->Vshunt_filt_obs = VshuntSenseFiltObs->calculate( Sen->Vshunt, reset_loc, min(Sen->T_filt, F_O_MAX_T));
  Sen->Ishunt_filt = Sen->Vshunt_filt*SHUNT_V2A_S + SHUNT_V2A_A + Sen->curr_bias;
  Sen->Ishunt_filt_obs = Sen->Vshunt_filt_obs*SHUNT_V2A_S + SHUNT_V2A_A + Sen->curr_bias;
  Sen->Vshunt_amp_filt = VshuntAmpSenseFilt->calculate( Sen->Vshunt_amp, reset_loc, min(Sen->T_filt, F_MAX_T));
  Sen->Vshunt_amp_filt_obs = VshuntAmpSenseFiltObs->calculate( Sen->Vshunt_amp, reset_loc, min(Sen->T_filt, F_O_MAX_T));
  Sen->Ishunt_amp_filt = Sen->Vshunt_amp_filt*SHUNT_AMP_V2A_S + SHUNT_AMP_V2A_A + Sen->amp_curr_bias;
  Sen->Ishunt_amp_filt_obs = Sen->Vshunt_amp_filt_obs*SHUNT_AMP_V2A_S + SHUNT_AMP_V2A_A + Sen->amp_curr_bias;

  // Voltage
  if ( rp.modeling )
  {
    Sen->Vbatt_filt_obs = Sen->Vbatt_model;
    Sen->Vbatt_filt = Sen->Vbatt_model;
  }
  else
  {
    Sen->Vbatt_filt_obs = VbattSenseFiltObs->calculate(Sen->Vbatt, reset_loc, min(Sen->T_filt, F_O_MAX_T));
    Sen->Vbatt_filt = VbattSenseFilt->calculate(Sen->Vbatt, reset_loc,  min(Sen->T_filt, F_MAX_T));
  }

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
    if ( cp.debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
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
  sprintf(dispString, "%3.0f %5.2f %5.1f", cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt_filt_obs);
  display->println(dispString);

  display->println(F(""));

  display->setTextColor(SSD1306_WHITE);
  char dispStringT[9];
  sprintf(dispStringT, "%3.0f%5.1f", min(cp.pubList.socu_solved, 101.), cp.pubList.tcharge);
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text
  char dispStringS[4];
  sprintf(dispStringS, "%3.0f", min(cp.pubList.soc_avail, 999.));
  display->print(dispStringS);

  display->display();
}


// Write to the D/A converter
uint32_t pwm_write(uint32_t duty, Pins *myPins)
{
    analogWrite(myPins->pwm_pin, duty, pwm_frequency);
    return duty;
}


// Talk Executive
void talk(boolean *stepping, double *step_val, boolean *vectoring, int8_t *vec_num,
  Battery *MyBattSolved, Battery *MyBattFree, Battery *MyBattModel)
{
  double SOCU_in = -99.;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.string_complete)
  {
    switch ( cp.input_string.charAt(0) )
    {
      case ( 'D' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):
            rp.curr_amp_bias = cp.input_string.substring(2).toFloat();
            break;
          case ( 'b' ):
            rp.curr_bias = cp.input_string.substring(2).toFloat();
            break;
          case ( 'c' ):
            rp.vbatt_bias = cp.input_string.substring(2).toFloat();
            break;
          case ( 'v' ):
            MyBattSolved->Dv(cp.input_string.substring(2).toFloat());
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'S' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'r' ):
            double rscale = cp.input_string.substring(2).toFloat();
            MyBattSolved->Sr(rscale);
            MyBattFree->Sr(rscale);
            break;
        }
        break;
      case ( 'd' ):
        cp.debug = -4;
        break;
      case ( 'l' ):
        switch ( cp.debug )
        {
          case ( -1 ):
            Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt  ,Ishunt_a  ,Vbat_fo*10-110  ,voc_s*10-110  ,vdyn_s*10  ,v_s*10-110  ,,,,,,,,,,,,\n");
            break;
          default:
            Serial.printf("Legend for cp.debug= %d not defined.   Edit mySubs.cpp, search for 'case ( 'l' )' and add it\n", cp.debug);
        }
        break;
      case ( 'm' ):
        SOCU_in = cp.input_string.substring(1).toFloat()/100.;
        rp.socu_free = max(min(SOCU_in, mxepu_bb), mnepu_bb);
        rp.socu_model = rp.socu_free;
        rp.delta_soc = max(rp.socu_free - 1., -rp.soc_sat);
        Serial.printf("socu_free=socu_model=%7.3f,   delta_soc=%7.3f,\n", rp.socu_free, rp.delta_soc);
        break;
      case ( 'n' ):
        SOCU_in = cp.input_string.substring(1).toFloat()/100.;
        rp.socu_model = max(min(SOCU_in, mxepu_bb), mnepu_bb);
        Serial.printf("socu_model=%7.3f\n", rp.socu_model);
        break;
      case ( 'v' ):
        cp.debug = cp.input_string.substring(1).toInt();
        break;
      case ( 'T' ):
        talkT(&cp.stepping, &cp.step_val, &cp.vectoring, &cp.vec_num);
        break;
      case ( 'w' ): 
        cp.enable_wifi = true; // not remembered in rp. Photon reset turns this false.
        break;
      case ( 'X' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'x' ):
            rp.modeling = !rp.modeling;
            Serial.printf("Modeling toggled to %d\n", rp.modeling);
            break;
          case ( 'a' ):
            rp.amp = max(min(cp.input_string.substring(2).toFloat(), 18.3), 0.0);
            Serial.printf("Modeling injected amp set to %7.3f and offset set to %7.3f\n", rp.amp, rp.offset);
            break;
          case ( 'f' ):
            rp.freq = max(min(cp.input_string.substring(2).toFloat(), 2.0), 0.0);
            Serial.printf("Modeling injected freq set to %7.3f Hz =", rp.freq);
            rp.freq = rp.freq * 2.0 * PI;
            Serial.printf(" %7.3f r/s\n", rp.freq);
            break;
          case ( 't' ):
            switch ( cp.input_string.charAt(2) )
            {
              case ( 's' ):
                rp.type = 1;
                Serial.printf("Setting waveform to sinusoid.  rp.type = %d\n", rp.type);
                break;
              case ( 'q' ):
                rp.type = 2;
                Serial.printf("Setting waveform to square.  rp.type = %d\n", rp.type);
                break;
              case ( 't' ):
                rp.type = 3;
                Serial.printf("Setting waveform to triangle inject.  rp.type = %d\n", rp.type);
                break;
              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
            }
            break;
          case ( 'o' ):
            rp.offset = max(min(cp.input_string.substring(2).toFloat(), 18.3), -18.3);
            Serial.printf("Modeling injected offset set to %7.3f\n", rp.offset);
            break;
          case ( 'p' ):
            switch ( cp.input_string.substring(2).toInt() )
            {
              case ( 0 ):
                rp.modeling = false;
                rp.type = 0;
                rp.freq = 0.0;
                rp.amp = 0.0;
                rp.offset = 0.0;
                Serial.printf("Setting injection program to 0:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp);
                break;
              case ( 1 ):
                rp.modeling = true;
                rp.type = 1;
                rp.freq = 0.05;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                Serial.printf("Setting injection program to 1: rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp);
                rp.freq *= (2. * PI);
                break;
              case ( 2 ):
                rp.modeling = true;
                rp.type = 2;
                rp.freq = 0.10;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                Serial.printf("Setting injection program to 2: rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp);
                rp.freq *= (2. * PI);
                break;
              case ( 3 ):
                rp.modeling = true;
                rp.type = 3;
                rp.freq = 0.05;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                Serial.printf("Setting injection program to 3: rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp);
                rp.freq *= (2. * PI);
                break;
              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
            }
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'h' ): 
        talkH(&cp.step_val, &cp.vec_num, MyBattSolved);
        break;
      default:
        Serial.print(cp.input_string.charAt(0)); Serial.println(" unknown.  Try typing 'h'");
        break;
    }
    cp.input_string = "";
    cp.string_complete = false;
  }
}

// Talk Tranient Input Settings
void talkT(boolean *stepping, double *step_val, boolean *vectoring, int8_t *vec_num)
{
  *stepping = false;
  *vectoring = false;
  int num_try = 0;
  switch ( cp.input_string.charAt(1) )
  {
    case ( 's' ): 
      *stepping = true;
      *step_val = cp.input_string.substring(2).toFloat();
      break;
    case ( 'v' ):
      num_try = cp.input_string.substring(2).toInt();
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
      Serial.print(cp.input_string); Serial.println(" unknown.  Try typing 'h'");
  }
}

// Talk Help
void talkH(double *step_val, int8_t *vec_num, Battery *batt_solved)
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");
  Serial.printf("d   dump the summary log\n"); 
  Serial.printf("m=  assign a free memory state in percent to all versions including model- '('truncated 0-100')'\n"); 
  Serial.printf("n=  assign a free memory state in percent to model only - '('truncated 0-100')'\n"); 
  Serial.printf("v=  "); Serial.print(cp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("D/S<?> Adjustments.   For example:\n");
  Serial.printf("  Da= "); Serial.printf("%7.3f", rp.curr_amp_bias); Serial.println("    : delta I adder to sensed amplified shunt current, A [0]"); 
  Serial.printf("  Db= "); Serial.printf("%7.3f", rp.curr_bias); Serial.println("    : delta I adder to sensed shunt current, A [0]"); 
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.println("    : delta V adder to sensed battery voltage, V [0]"); 
  Serial.printf("  Dv= "); Serial.print(batt_solved->Dv()); Serial.println("    : delta V adder to solved battery calculation, V"); 
  Serial.printf("  Sr= "); Serial.print(batt_solved->Sr()); Serial.println("    : Scalar resistor for battery dynamic calculation, V"); 
  Serial.printf("T<?>=  "); 
  Serial.printf("T - Transient performed with input.   For example:\n");
  Serial.printf("  Ts=<index>  :   index="); Serial.print(*step_val);
  Serial.printf(", cp.stepping=");  Serial.println(cp.stepping);
  Serial.printf("  Tv=<vec_num>  :   vec_num="); Serial.println(*vec_num);
  Serial.printf("    ******Send Tv0 to cancel vector*****\n");
  Serial.printf("   INFO:  cp.vectoringing=");  Serial.println(cp.vectoring);
  Serial.printf("w   turn on wifi = "); Serial.println(cp.enable_wifi);
  Serial.printf("X<?> - Test Mode.   For example:\n");
  Serial.printf("  Xx= "); Serial.printf("x   toggle model use of Vbatt = "); Serial.println(rp.modeling);
  Serial.printf("  Xa= "); Serial.printf("%7.3f", rp.amp); Serial.println("  : Injection amplitude A pk (0-18.3) [0]");
  Serial.printf("  Xf= "); Serial.printf("%7.3f", rp.freq/2./PI); Serial.println("  : Injection frequency Hz (0-2) [0]");
  Serial.printf("  Xt= "); Serial.printf("%d", rp.type); Serial.println("  : Injection type.  's', 'q', 't' (0=none, 1=sine, 2=square, 3=triangle)");
  Serial.printf("  Xo= "); Serial.printf("%7.3f", rp.offset); Serial.println("  : Injection offset A (-18.3-18.3) [0]");
  Serial.printf("  Xp= <?>, programmed injection settings...\n"); 
  Serial.printf("       0:  Off, modeling false\n");
  Serial.printf("       1:  1 Hz sinusoid centered at 0 with largest supported amplitude\n");
  Serial.printf("       2:  1 Hz square centered at 0 with largest supported amplitude\n");
  Serial.printf("       3:  1 Hz triangle centered at 0 with largest supported amplitude\n");
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
    #ifndef FAKETIME
        uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
        if ( cp.debug>5 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
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
