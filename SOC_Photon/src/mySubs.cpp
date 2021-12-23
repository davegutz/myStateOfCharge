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


// Text header
void print_serial_header(void)
{
  Serial.println(F("unit,hm, cTime,  Tbatt,Tbatt_filt, Vbatt,Vbatt_f_o,   curr_sel_amp,  Ishunt,Ishunt_f_o,  Wshunt,  VOC_s,  tcharge,  T,   soc_mod, soc_ekf, soc,    SOC_mod, SOC_ekf, SOC,"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s, %12.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,  %d,   %7.3f,%7.3f,   %7.3f,  %7.3f,  %7.3f,  %6.3f,  %7.3f,%7.3f,%7.3f,    %7.3f,%7.3f,%7.3f,  %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time,
    pubList->Tbatt, pubList->Tbatt_filt,
    pubList->Vbatt, pubList->Vbatt_filt,
    pubList->curr_sel_amp,
    pubList->Ishunt, pubList->Ishunt_filt,
    pubList->Wshunt,
    pubList->VOC,
    pubList->tcharge,
    pubList->T,
    pubList->soc_model, pubList->soc_ekf, pubList->soc, 
    pubList->SOC_model, pubList->SOC_ekf, pubList->SOC, 
    '\0');
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
    if ( SensorTbatt->read() ) temp = SensorTbatt->fahrenheit() + (TBATT_TEMPCAL);
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
    Adafruit_ADS1015 *ads_amp, Adafruit_ADS1015 *ads_noamp, const unsigned long now,
    SlidingDeadband *SdVbatt)
{
  static unsigned long int past = now;
  double T = (now - past)/1e3;
  past = now;

  // Current bias.  Feeds into signal conversion, not to duty injection
  Sen->curr_bias_noamp = rp.curr_bias_noamp + rp.curr_bias_all + rp.offset;
  Sen->curr_bias_amp = rp.curr_bias_amp + rp.curr_bias_all + rp.offset;

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
  Sen->Ishunt_amp_cal = Sen->Vshunt_amp*shunt_amp_v2a_s + Sen->curr_bias_amp;
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
  Sen->Ishunt_noamp_cal = Sen->Vshunt_noamp*shunt_noamp_v2a_s + Sen->curr_bias_noamp;

  // Print results
  if ( rp.debug==14 ) Serial.printf("reset_free,select,   vs_na_int,0_na_int,1_na_int,vshunt_na,ishunt_na, ||, vshunt_a_int,0_a_int,1_a_int,vshunt_a,ishunt_a,  Ishunt_filt,T, %d,%d,%d,%d,%d,%7.3f,%7.3f,||,%d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n",
    reset_free, rp.curr_sel_amp,
    Sen->Vshunt_noamp_int, vshunt_noamp_int_0, vshunt_noamp_int_1, Sen->Vshunt_noamp, Sen->Ishunt_noamp_cal,
    Sen->Vshunt_amp_int, vshunt_amp_int_0, vshunt_amp_int_1, Sen->Vshunt_amp, Sen->Ishunt_amp_cal,
    Sen->Ishunt_filt,
    T);

  // Current signal selection, based on if there or not.
  // Over-ride 'permanent' with Talk(rp.curr_sel_amp) = Talk('s')
  if ( rp.curr_sel_amp && !Sen->bare_ads_amp)
  {
    Sen->Vshunt = Sen->Vshunt_amp;
    Sen->Ishunt = Sen->Ishunt_amp_cal;
    Sen->curr_bias = Sen->curr_bias_amp;
    Sen->shunt_v2a_s = shunt_amp_v2a_s;
  }
  else if ( !Sen->bare_ads_noamp )
  {
    Sen->Vshunt = Sen->Vshunt_noamp;
    Sen->Ishunt = Sen->Ishunt_noamp_cal;
    Sen->curr_bias = Sen->curr_bias_noamp;
    Sen->shunt_v2a_s = shunt_noamp_v2a_s;
  }
  else
  {
    Sen->Vshunt = 0.;
    Sen->Ishunt = 0.;
    Sen->curr_bias = 0.;
    Sen->shunt_v2a_s = shunt_amp_v2a_s; // amp preferred, default to that
  }

  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A) + rp.vbatt_bias;
  if ( rp.modeling ) Sen->Vbatt = Sen->Vbatt_model;
  else Sen->Vbatt = SdVbatt->update(vbatt_free, reset_free);
  if ( rp.debug==15 ) Serial.printf("reset_free,vbatt_free,vbatt,T, %d,%7.3f,%7.3f,%7.3f,\n", reset_free, vbatt_free, Sen->Vbatt, T);

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
  Sen->Wcharge = Sen->Ishunt * NOM_SYS_VOLT;
}

// Filter temperature only
void filter_temp(int reset, Sensors *Sen, General2_Pole* TbattSenseFilt)
{
  int reset_loc = reset;

  // Temperature
  Sen->Tbatt_filt = TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP)) + rp.t_bias;
  Sen->Tbatt += rp.t_bias;
}

// Filter all other inputs
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFilt,  General2_Pole* IshuntSenseFilt)
{
  int reset_loc = reset;

  // Shunt
  Sen->Ishunt_filt = IshuntSenseFilt->calculate( Sen->Ishunt, reset_loc, min(Sen->T_filt, F_O_MAX_T));
  
  // Voltage
  if ( rp.modeling )
    Sen->Vbatt_filt = Sen->Vbatt_model;
  else
    Sen->Vbatt_filt = VbattSenseFilt->calculate(Sen->Vbatt, reset_loc, min(Sen->T_filt, F_O_MAX_T));

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
  sprintf(dispString, "%3.0f %5.2f %5.1f", cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt);
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
      cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt_filt, cp.pubList.amp_hrs_remaining_ekf, cp.pubList.tcharge, cp.pubList.amp_hrs_remaining);
  if ( rp.debug==-5 ) Serial.printf("Tb, Vb, Ib, Ahrs_rem_ekf, tcharge, Ahrs_rem,\n%3.0f, %5.2f, %5.1f,  %3.0f,%5.1f,%3.0f,\n",
      cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt_filt, cp.pubList.amp_hrs_remaining_ekf, cp.pubList.tcharge, cp.pubList.amp_hrs_remaining);

}


// Write to the D/A converter
uint32_t pwm_write(uint32_t duty, Pins *myPins)
{
    analogWrite(myPins->pwm_pin, duty, pwm_frequency);
    return duty;
}


// Talk Executive
void talk(Battery *MyBatt, BatteryModel *MyBattModel)
{
  double SOCS_in = -99.;
  double scale = 1.;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.string_complete)
  {
    switch ( cp.input_string.charAt(0) )
    {
      case ( 'D' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):
            rp.curr_bias_amp = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.curr_bias_amp changed to %7.3f\n", rp.curr_bias_amp);
            break;
          case ( 'b' ):
            rp.curr_bias_noamp = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.curr_bias_noamp changed to %7.3f\n", rp.curr_bias_noamp);
            break;
          case ( 'i' ):
            rp.curr_bias_all = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.curr_bias_all changed to %7.3f\n", rp.curr_bias_all);
            break;
          case ( 'c' ):
            rp.vbatt_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.vbatt_bias changed to %7.3f\n", rp.vbatt_bias);
            break;
          case ( 't' ):
            rp.t_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.t_bias changed to %7.3f\n", rp.t_bias);
            break;
          case ( 'v' ):
            MyBattModel->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("MyBattModel.Dv changed to %7.3f\n", MyBattModel->Dv());
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'S' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'c' ):
            scale = cp.input_string.substring(2).toFloat();
            rp.s_cap_model = scale;
            Serial.printf("MyBattModel.q_cap_rated scaled by %7.3f from %7.3f to ", scale, MyBattModel->q_cap_rated());
            MyBattModel->apply_cap_scale(rp.s_cap_model);
            Serial.printf("%7.3f\n", MyBattModel->q_cap_rated());
            break;
          case ( 'r' ):
            scale = cp.input_string.substring(2).toFloat();
            MyBattModel->Sr(scale);
            MyBatt->Sr(scale);
            break;
          case ( 'k' ):
            scale = cp.input_string.substring(2).toFloat();
            rp.cutback_gain_scalar = scale;
            Serial.printf("rp.cutback_gain_scalar set to %7.3f\n", rp.cutback_gain_scalar);
            break;
        }
        break;
      case ( 'd' ):
        rp.debug = -4;
        break;
      case ( 'l' ):
        switch ( rp.debug )
        {
          case ( -1 ):
            Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt_amp  ,Ishunt_noamp  ,Vbat_fo*10-110  ,voc_s*10-110  ,vdyn_s*10  ,v_s*10-110  , voc_dyn*10-110,,,,,,,,,,,\n");
            break;
          default:
            Serial.printf("Legend for rp.debug= %d not defined.   Edit mySubs.cpp, search for 'case ( 'l' )' and add it\n", rp.debug);
        }
        break;
      case ( 'm' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        if ( SOCS_in<1.1 )  // TODO:  rationale for this?
        {
          MyBatt->apply_soc(SOCS_in);
          MyBattModel->apply_soc(SOCS_in);
          MyBatt->update(&rp.delta_q, &rp.t_last);
          MyBattModel->update(&rp.delta_q_model, &rp.t_last_model);
          MyBatt->init_soc_ekf(MyBatt->soc());
          Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
              MyBatt->SOC(), MyBatt->soc(), rp.delta_q, MyBattModel->SOC(), MyBattModel->soc(), rp.delta_q_model, MyBatt->soc_ekf());
        }
        else
          Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", SOCS_in);
        break;
      case ( 'M' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        MyBatt->apply_SOC(SOCS_in);
        MyBattModel->apply_SOC(SOCS_in);
        MyBatt->update(&rp.delta_q, &rp.t_last);
        MyBattModel->update(&rp.delta_q_model, &rp.t_last_model);
        MyBatt->init_soc_ekf(MyBatt->soc());
        Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
            MyBatt->SOC(), MyBatt->soc(), rp.delta_q, MyBattModel->SOC(), MyBattModel->soc(), rp.delta_q_model, MyBatt->soc_ekf());
        break;
      case ( 'n' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        if ( SOCS_in<1.1 )  // TODO:  rationale for this?
        {
          MyBattModel->apply_soc(SOCS_in);
          MyBattModel->update(&rp.delta_q_model, &rp.t_last_model);
          if ( rp.modeling )
            MyBatt->init_soc_ekf(MyBatt->soc());
          Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
              MyBatt->SOC(), MyBatt->soc(), rp.delta_q, MyBattModel->SOC(), MyBattModel->soc(), rp.delta_q_model, MyBatt->soc_ekf());
        }
        else
          Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", SOCS_in);
        break;
      case ( 'N' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        MyBattModel->apply_SOC(SOCS_in);
        MyBattModel->update(&rp.delta_q_model, &rp.t_last_model);
        if ( rp.modeling )
          MyBatt->init_soc_ekf(MyBatt->soc());
        Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
            MyBatt->SOC(), MyBatt->soc(), rp.delta_q, MyBattModel->SOC(), MyBattModel->soc(), rp.delta_q_model, MyBatt->soc_ekf());
        break;
      case ( 's' ): 
        rp.curr_sel_amp = !rp.curr_sel_amp;
        Serial.printf("Signal selection (1=amp, 0=no amp) toggled to %d\n", rp.curr_sel_amp);
        break;
      case ( 'v' ):
        rp.debug = cp.input_string.substring(1).toInt();
        break;
      case ( 'w' ): 
        cp.enable_wifi = !cp.enable_wifi; // not remembered in rp. Photon reset turns this false.
        Serial.printf("Wifi toggled to %d\n", cp.enable_wifi);
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
              case ( 'c' ):
                rp.type = 4;
                Serial.printf("Setting waveform to 1C charge.  rp.type = %d\n", rp.type);
                break;
              case ( 'd' ):
                rp.type = 5;
                Serial.printf("Setting waveform to 1C discharge.  rp.type = %d\n", rp.type);
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
                rp.modeling = true;
                rp.type = 0;
                rp.freq = 0.0;
                rp.amp = 0.0;
                rp.offset = 0.0;
                rp.debug = 5;   // myDisplay = 5
                rp.curr_bias_all = 0;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
                break;
              case ( 1 ):
                rp.modeling = true;
                rp.type = 1;
                rp.freq = 0.05;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                rp.debug = -12;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
                rp.freq *= (2. * PI);
                break;
              case ( 2 ):
                rp.modeling = true;
                rp.type = 2;
                rp.freq = 0.10;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                rp.debug = -12;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
                rp.freq *= (2. * PI);
                break;
              case ( 3 ):
                rp.modeling = true;
                rp.type = 3;
                rp.freq = 0.05;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                rp.debug = -12;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
                rp.freq *= (2. * PI);
                break;
              case ( 4 ):
                rp.modeling = true;
                rp.type = 4;
                rp.freq = 0.0;
                rp.amp = 0.0;
                rp.offset = 0;
                rp.curr_bias_all = -RATED_BATT_CAP;  // Software effect only
                rp.debug = -12;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
                break;
              case ( 5 ):
                rp.modeling = true;
                rp.type = 4;
                rp.freq = 0.0;
                rp.amp = 0.0;
                rp.offset = 0;
                rp.curr_bias_all = RATED_BATT_CAP; // Software effect only
                rp.debug = -12;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
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
        talkH(MyBatt, MyBattModel);
        break;
      default:
        Serial.print(cp.input_string.charAt(0)); Serial.println(" unknown.  Try typing 'h'");
        break;
    }
    cp.input_string = "";
    cp.string_complete = false;
  }
}

// Talk Help
void talkH(Battery *MyBatt, BatteryModel *MyBattModel)
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");
  Serial.printf("d   dump the summary log\n"); 
  Serial.printf("m=  assign curve charge state in fraction to all versions including model- '(0-1.1)'\n"); 
  Serial.printf("M=  assign a CHARGE state in percent to all versions including model- '('truncated 0-100')'\n"); 
  Serial.printf("n=  assign curve charge state in fraction to model only (ekf if modeling)- '(0-1.1)'\n"); 
  Serial.printf("N=  assign a CHARGE state in percent to model only (ekf if modeling)-- '('truncated 0-100')'\n"); 
  Serial.printf("s   curr signal select (1=amp preferred, 0=noamp) = "); Serial.println(rp.curr_sel_amp);
  Serial.printf("v=  "); Serial.print(rp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("D/S<?> Adjustments.   For example:\n");
  Serial.printf("  Da= "); Serial.printf("%7.3f", rp.curr_bias_amp); Serial.println("    : delta I adder to sensed amplified shunt current, A [0]"); 
  Serial.printf("  Db= "); Serial.printf("%7.3f", rp.curr_bias_noamp); Serial.println("    : delta I adder to sensed shunt current, A [0]"); 
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.curr_bias_all); Serial.println("    : delta I adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.println("    : delta V adder to sensed battery voltage, V [0]"); 
  Serial.printf("  Dt= "); Serial.printf("%7.3f", rp.t_bias); Serial.println("    : delta T adder to sensed Tbatt, deg C [0]"); 
  Serial.printf("  Dv= "); Serial.print(MyBattModel->Dv()); Serial.println("    : delta V adder to solved battery calculation, V"); 
  Serial.printf("  Sc= "); Serial.print(MyBattModel->q_capacity()/MyBatt->q_capacity()); Serial.println("    : Scalar battery model size"); 
  Serial.printf("  Sr= "); Serial.print(MyBattModel->Sr()); Serial.println("    : Scalar resistor for battery dynamic calculation, V"); 
  Serial.printf("  Sk= "); Serial.print(rp.cutback_gain_scalar); Serial.println("    : Saturation of model cutback gain scalar"); 
  Serial.printf("w   turn on wifi = "); Serial.println(cp.enable_wifi);
  Serial.printf("X<?> - Test Mode.   For example:\n");
  Serial.printf("  Xx= "); Serial.printf("x   toggle model use of Vbatt = "); Serial.println(rp.modeling);
  Serial.printf("  Xa= "); Serial.printf("%7.3f", rp.amp); Serial.println("  : Injection amplitude A pk (0-18.3) [0]");
  Serial.printf("  Xf= "); Serial.printf("%7.3f", rp.freq/2./PI); Serial.println("  : Injection frequency Hz (0-2) [0]");
  Serial.printf("  Xt= "); Serial.printf("%d", rp.type); Serial.println("  : Injection type.  's', 'q', 't' (0=none, 1=sine, 2=square, 3=triangle)");
  Serial.printf("  Xo= "); Serial.printf("%7.3f", rp.offset); Serial.println("  : Injection offset A (-18.3-18.3) [0]");
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.curr_bias_all); Serial.println("  : Injection  A (unlimited) [0]");
  Serial.printf("  Xp= <?>, programmed injection settings...\n"); 
  Serial.printf("       0:  Off, modeling false\n");
  Serial.printf("       1:  1 Hz sinusoid centered at 0 with largest supported amplitude\n");
  Serial.printf("       2:  1 Hz square centered at 0 with largest supported amplitude\n");
  Serial.printf("       3:  1 Hz triangle centered at 0 with largest supported amplitude\n");
  Serial.printf("       4:  -1C soft discharge until reset by Xp0 or Di0\n");
  Serial.printf("       5:  +1C soft charge until reset by Xp0 or Di0\n");
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
        if ( rp.debug>105 ) Serial.printf("DAY %u HOURS %u\n", dayOfWeek, hours);
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

// Battery model class for reference use mainly in jumpered hardware testing
BatteryModel::BatteryModel() : Battery() {}
BatteryModel::BatteryModel(const double *x_tab, const double *b_tab, const double *a_tab, const double *c_tab,
    const double m, const double n, const double d, const unsigned int nz, const int num_cells,
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt,
    const double q_cap_rated, const double t_rated, const double t_rlim) :
    Battery(x_tab, b_tab, a_tab, c_tab, m, n, d, nz, num_cells, r1, r2, r2c2, batt_vsat, dvoc_dt, q_cap_rated, t_rated, t_rlim)
{
    // Randles dynamic model for EKF
    // Resistance values add up to same resistance loss as matched to installed battery
    //   i.e.  (r0_ + rct_ + rdif_) = (r1 + r2)*num_cells
    // tau_ct small as possible for numerical stability and 2x margin.   Original data match used 0.01 but
    // the state-space stability requires at least 0.1.   Used 0.2.
    double c_ct = tau_ct_ / rct_;
    double c_dif = tau_ct_ / rct_;
    int rand_n = 2; // Rows A and B
    int rand_p = 2; // Col B    
    int rand_q = 1; // Row C and D
    rand_A_ = new double[rand_n*rand_n];
    rand_A_[0] = -1./tau_ct_;
    rand_A_[1] = 0.;
    rand_A_[2] = 0.;
    rand_A_[3] = -1/tau_dif_;
    rand_B_ = new double [rand_n*rand_p];
    rand_B_[0] = 1./c_ct;
    rand_B_[1] = 0.;
    rand_B_[2] = 1./c_dif;
    rand_B_[3] = 0.;
    rand_C_ = new double [rand_q*rand_n];
    rand_C_[0] = 1.;
    rand_C_[1] = 1.;
    rand_D_ = new double [rand_q*rand_p];
    rand_D_[0] = r0_;
    rand_D_[1] = 1.;
    Randles_ = new StateSpace(rand_A_, rand_B_, rand_C_, rand_D_, rand_n, rand_p, rand_q);
    Sin_inj_ = new SinInj();
    Sq_inj_ = new SqInj();
    Tri_inj_ = new TriInj();
    sat_ib_null_ = 0.1 * RATED_BATT_CAP; // 0.1C discharge rate at sat_ib_null_, A
    sat_cutback_gain_ = 4.8;  // 0.1C sat_ib_null_ and  voc_ 0.3 volts beyond vsat_
    model_saturated_ = false;
    ib_sat_ = 0.5;  // If smaller, takes forever to saturate the model.

}

// SOC-OCV curve fit method per Zhang, et al.   Makes a good reference model
double BatteryModel::calculate(const double temp_C, const double soc, const double curr_in, const double dt,
  const double q_capacity, const double q_cap)
{
    dt_ = dt;
    temp_c_ = temp_C;

    double soc_lim = max(min(soc, mxeps_bb), mneps_bb);
    double SOC = soc * q_capacity / q_cap_scaled_ * 100;

    // VOC-OCV model
    double log_soc, exp_n_soc, pow_log_soc;
    calc_soc_voc_coeff(soc_lim, temp_C, &b_, &a_, &c_, &log_soc, &exp_n_soc, &pow_log_soc);
    voc_ = calc_soc_voc(soc_lim, &dv_dsoc_, b_, a_, c_, log_soc, exp_n_soc, pow_log_soc);
    voc_ = min(voc_ + (soc - soc_lim) * dv_dsoc_, max_voc);  // slightly beyond but don't windup
    voc_ +=  dv_;  // Experimentally varied

    // Dynamic emf
    // Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
    double u[2] = {ib_, voc_};
    Randles_->calc_x_dot(u);
    Randles_->update(dt);
    vb_ = Randles_->y(0);
    vdyn_ = vb_ - voc_;

    // Saturation logic
    vsat_ = nom_vsat_ + (temp_C-25.)*dvoc_dt_;
    sat_ib_max_ = sat_ib_null_ + (vsat_ - voc_) / nom_vsat_ * q_capacity / 3600. * sat_cutback_gain_ * rp.cutback_gain_scalar;
    ib_ = min(curr_in, sat_ib_max_);
    model_saturated_ = (voc_ > vsat_) && (ib_ < ib_sat_) && (ib_ == sat_ib_max_);

    if ( rp.debug==78 )Serial.printf("BatteryModel::calculate:,  dt,tempC,tempF,curr,a,b,c,d,n,m,r,soc,logsoc,expnsoc,powlogsoc,voc,vdyn,v,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
     dt,temp_C, temp_C*9./5.+32., ib_, a_, b_, c_, d_, n_, m_, (r1_+r2_)*sr_ , soc, log_soc, exp_n_soc, pow_log_soc, voc_, vdyn_, vb_);
    if ( rp.debug==-78 ) Serial.printf("SOC/10,soc*10,voc,vsat,curr_in,sat_ib_max_,ib,sat,\n%7.3f, %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%d,\n", 
      SOC/10, soc*10, voc_, vsat_, curr_in, sat_ib_max_, ib_, model_saturated_);


    return ( vb_ );
}

// Injection model, calculate duty
uint32_t BatteryModel::calc_inj_duty(const unsigned long now, const uint8_t type, const double amp, const double freq)
{
  double t = now/1e3;
  double sin_bias = 0.;
  double square_bias = 0.;
  double tri_bias = 0.;
  double inj_bias = 0.;
  // Calculate injection amounts from user inputs (talk).
  // One-sided because PWM voltage >0.  rp.offset applied in logic below.
  switch ( type )
  {
    case ( 0 ):   // Nothing
      sin_bias = 0.;
      square_bias = 0.;
      tri_bias = 0.;
      break;
    case ( 1 ):   // Sine wave
      sin_bias = Sin_inj_->signal(amp, freq, t, 0.0);
      square_bias = 0.;
      tri_bias = 0.;
      break;
    case ( 2 ):   // Square wave
      sin_bias = 0.;
      square_bias = Sq_inj_->signal(amp, freq, t, 0.0);
      tri_bias = 0.;
      break;
    case ( 3 ):   // Triangle wave
      sin_bias = 0.;
      square_bias =  0.;
      tri_bias = Tri_inj_->signal(amp, freq, t, 0.0);
      break;
    default:
      break;
  }
  inj_bias = sin_bias + square_bias + tri_bias;
  duty_ = min(uint32_t(inj_bias / bias_gain), uint32_t(255.));
  if ( rp.debug==-41 )
  Serial.printf("type,amp,freq,sin,square,tri,inj,duty,tnow=%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,   %ld,  %7.3f,\n",
            type, amp, freq, sin_bias, square_bias, tri_bias, duty_, t);

  return ( duty_ );
}

// Count coulombs based on true=actual capacity
double BatteryModel::count_coulombs(const double dt, const double temp_c, const double charge_curr, const double t_last)
{
    /* Count coulombs based on true=actual capacity
    Inputs:
        dt              Integration step, s
        temp_c          Battery temperature, deg C
        charge_curr     Charge, A
        sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
        tlast           Past value of battery temperature used for rate limit memory, deg C
    */
    double d_delta_q = charge_curr * dt;
    t_last_ = t_last;

    // Rate limit temperature
    double temp_lim = t_last_ + max(min( (temp_c-t_last_), t_rlim_*dt), -t_rlim_*dt);

    // Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
    // detection).
    // TODO delete this boolean model_sat = sat_ && d_delta_q > 0 && sat_ib_max_ > 0.1 && charge_curr < 0.5;  // TODO:  is this robust to diff charge currents?
    // Serial.printf("sat,d_delta_q,ib_cutback,charge_curr,model_sat=%d,%7.3f,%7.3f,%7.3f,%d\n", sat_, d_delta_q, sat_ib_max_, charge_curr, model_sat);
    if ( model_saturated_ )
    {
      d_delta_q = 0.;
      if ( !resetting_ ) delta_q_ = 0.;
      else resetting_ = false;     // one pass flag.  Saturation debounce should reset next pass
    }

    // Integration
    q_capacity_ = q_cap_rated_*(1. + DQDT*(temp_lim - t_rated_));
    delta_q_ = max(min(delta_q_ + d_delta_q - DQDT*q_capacity_*(temp_lim-t_last_), 1.1*(q_cap_rated_ - q_capacity_)), -q_capacity_);
    q_ = q_capacity_ + delta_q_;

    // Normalize
    soc_ = q_ / q_capacity_;
    SOC_ = q_ / q_cap_scaled_ * 100;

    if ( rp.debug==97 )
        Serial.printf("BatteryModel::cc,  dt,voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc,SOC,      %7.3f,%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%7.4f,%7.3f,\n",
                    dt,cp.pubList.VOC,  sat_voc(temp_c), temp_lim, model_saturated_, charge_curr, d_delta_q, delta_q_, q_, q_capacity_, soc_, SOC_);
    if ( rp.debug==-97 )
        Serial.printf("voc, v_sat, temp_lim, sat, charge_curr, d_d_q, d_q, q, q_capacity,soc, SOC,          \n%7.3f,%7.3f,%7.3f,%d,%7.3f,%10.6f,%9.1f,%9.1f,%7.3f,%7.4f,%7.3f,\n",
                    cp.pubList.VOC,  sat_voc(temp_c), temp_lim, model_saturated_, charge_curr, d_delta_q, delta_q_, q_, q_capacity_, soc_, SOC_);

    // Save and return
    t_last_ = temp_lim;
    return ( soc_ );
}

// Load states from retained memory
void BatteryModel::load(const double delta_q, const double t_last, const double s_cap_model)
{
    delta_q_ = delta_q;
    t_last_ = t_last;
    apply_cap_scale(s_cap_model);
}
