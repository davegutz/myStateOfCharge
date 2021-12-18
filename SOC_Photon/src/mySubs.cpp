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
#include "myLibrary/injection.h"

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
  Serial.println(F("unit,hm, cTime,  Tbatt,Tbatt_filt, Vbatt,Vbatt_f_o,   curr_sel_amp,  Ishunt,Ishunt_f_o,  Wshunt,  VOC_s,  tcharge,  T,   SOC_sat,    SOC_mod, SOC_ekf, SOC,"));
}

// Print strings
void create_print_string(char *buffer, Publish *pubList)
{
  sprintf(buffer, "%s,%s, %12.3f,   %7.3f,%7.3f,   %7.3f,%7.3f,  %d,   %7.3f,%7.3f,   %7.3f,  %7.3f,  %7.3f,  %6.3f,  %7.3f,    %7.3f,%7.3f,%7.3f,  %c", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), pubList->control_time,
    pubList->Tbatt, pubList->Tbatt_filt,
    pubList->Vbatt, pubList->Vbatt_filt,
    pubList->curr_sel_amp,
    pubList->Ishunt, pubList->Ishunt_filt,
    pubList->Wshunt,
    pubList->VOC,
    pubList->tcharge,
    pubList->T,
    pubList->soc_sat,
    pubList->soc_model, pubList->soc_ekf, pubList->soc, 
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
  double t = now/1e3;
  double sin_bias = 0.;
  double square_bias = 0.;
  double tri_bias = 0.;
  double inj_bias = 0.;
  static SinInj *Sin_inj = new SinInj();
  static SqInj *Sq_inj = new SqInj();
  static TriInj *Tri_inj = new TriInj();


  // Calculate injection amounts from user inputs (talk).
  // One-sided because PWM voltage >0.  rp.offset applied in logic below.
  switch ( rp.type )
  {
    case ( 0 ):   // Nothing
      sin_bias = 0.;
      square_bias = 0.;
      tri_bias = 0.;
      break;
    case ( 1 ):   // Sine wave
      sin_bias = Sin_inj->signal(rp.amp, rp.freq, t, 0.0);
      square_bias = 0.;
      tri_bias = 0.;
      break;
    case ( 2 ):   // Square wave
      sin_bias = 0.;
      square_bias = Sq_inj->signal(rp.amp, rp.freq, t, 0.0);
      tri_bias = 0.;
      break;
    case ( 3 ):   // Triangle wave
      sin_bias = 0.;
      square_bias =  0.;
      tri_bias = Tri_inj->signal(rp.amp, rp.freq, t, 0.0);
      break;
    default:
      break;
  }
  inj_bias = sin_bias + square_bias + tri_bias;
  rp.duty = min(uint32_t(inj_bias / bias_gain), uint32_t(255.));
  if ( rp.debug==-41 )
  Serial.printf("type,amp,freq,sin,square,tri,inj,duty,tnow=%d,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,   %ld,  %7.3f,\n",
            rp.type, rp.amp, rp.freq, sin_bias, square_bias, tri_bias, rp.duty, t);

  // Current bias.  Feeds into signal conversion, not to duty injection
  Sen->curr_bias_noamp = rp.curr_bias_noamp + rp.curr_bias_all + rp.offset;
  Sen->curr_bias_amp = rp.curr_bias_amp + rp.curr_bias_all + rp.offset;

  // Anti-windup used to bias current below (only when modeling)
  double s_sat = 0.;
  if ( rp.modeling && Sen->Wshunt > 0. && Sen->saturated )
      s_sat = max(Sen->Voc - sat_voc(Sen->Tbatt), 0.) / NOM_SYS_VOLT * NOM_BATT_CAP * sat_gain;
  s_sat = 0;

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
  if ( rp.debug==-14 ) Serial.printf("reset_free,select,   vs_na_int,0_na_int,1_na_int,vshunt_na,ishunt_na, ||, vshunt_a_int,0_a_int,1_a_int,vshunt_a,ishunt_a,  Ishunt_filt,T, %d,%d,%d,%d,%d,%7.3f,%7.3f,||,%d,%d,%d,%7.3f,%7.3f,%7.3f,%7.3f,\n",
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
    Sen->Ishunt = Sen->Ishunt_amp_cal - s_sat;
    Sen->curr_bias = Sen->curr_bias_amp;
    Sen->shunt_v2a_s = shunt_amp_v2a_s;
  }
  else if ( !Sen->bare_ads_noamp )
  {
    Sen->Vshunt = Sen->Vshunt_noamp;
    Sen->Ishunt = Sen->Ishunt_noamp_cal - s_sat;
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
  if ( rp.debug==51 )
    Serial.printf("soc,sat,    VOC,v_sat,   ib, adder,%7.3f,%d,   %7.3f,%7.3f,    %7.3f,%7.3f,\n", rp.soc, Sen->saturated, Sen->Voc, sat_voc(Sen->Tbatt), Sen->Ishunt, s_sat);
  if ( rp.debug==-51 )
    Serial.printf("soc,sat,    VOC,v_sat,   ib, adder,\n%7.3f,%d,   %7.3f,%7.3f,    %7.3f,%7.3f,\n", rp.soc, Sen->saturated, Sen->Voc, sat_voc(Sen->Tbatt), Sen->Ishunt, s_sat);


  // Vbatt
  int raw_Vbatt = analogRead(myPins->Vbatt_pin);
  double vbatt_free =  double(raw_Vbatt)*vbatt_conv_gain + double(VBATT_A) + rp.vbatt_bias;
  if ( rp.modeling ) Sen->Vbatt = Sen->Vbatt_model;
  else Sen->Vbatt = SdVbatt->update(vbatt_free, reset_free);
  if ( rp.debug==-15 ) Serial.printf("reset_free,vbatt_free,vbatt, %d,%7.3f,%7.3f\n", reset_free, vbatt_free, Sen->Vbatt);

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
    Sen->Vshunt = (Sen->Ishunt - Sen->curr_bias) / Sen->shunt_v2a_s;
    Sen->Tbatt =  T_T1->interp(elapsed_loc);
    Sen->Vbatt =  V_T1->interp(elapsed_loc) + Sen->Ishunt*(batt_r1 + batt_r2)*double(batt_num_cells);
  }
  else elapsed_loc = 0.;

  // Power calculation
  Sen->Wshunt = Sen->Vbatt*Sen->Ishunt;
  Sen->Wcharge = Sen->Ishunt * NOM_SYS_VOLT;

  if ( rp.debug==-6 ) Serial.printf("cp.vectoring,reset_free,cp.vec_start,now,elapsed_loc,Vbatt,Ishunt,Tbatt:  %d,%d,%ld, %ld,%7.3f,%7.3f,%7.3f,%7.3f\n", cp.vectoring, reset_free, cp.vec_start, now, elapsed_loc, Sen->Vbatt, Sen->Ishunt, Sen->Tbatt);
}

// Filter temperature only
void filter_temp(int reset, Sensors *Sen, General2_Pole* TbattSenseFilt)
{
  int reset_loc = reset || cp.vectoring;

  // Temperature
  Sen->Tbatt_filt = TbattSenseFilt->calculate(Sen->Tbatt, reset_loc,  min(Sen->T_temp, F_MAX_T_TEMP));
}

// Filter all other inputs
void filter(int reset, Sensors *Sen, General2_Pole* VbattSenseFilt,  General2_Pole* IshuntSenseFilt)
{
  int reset_loc = reset || cp.vectoring;

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
  sprintf(dispString, "%3.0f %5.2f %5.1f", cp.pubList.Tbatt, cp.pubList.Vbatt, cp.pubList.Ishunt_filt);
  display->println(dispString);

  display->println(F(""));

  display->setTextColor(SSD1306_WHITE);
  char dispStringT[9];
  sprintf(dispStringT, "%3.0f%5.1f", cp.pubList.soc_ekf, cp.pubList.tcharge);
  display->print(dispStringT);
  display->setTextSize(2);             // Draw 2X-scale text
  char dispStringS[4];
  if ( pass || !Sen->saturated )
    sprintf(dispStringS, "%3.0f", min(cp.pubList.soc, 999.));
  else if (Sen->saturated)
    sprintf(dispStringS, "SAT");
  display->print(dispStringS);

  display->display();
  pass = !pass;
}


// Write to the D/A converter
uint32_t pwm_write(uint32_t duty, Pins *myPins)
{
    analogWrite(myPins->pwm_pin, duty, pwm_frequency);
    return duty;
}


// Talk Executive
void talk(boolean *stepping, double *step_val, boolean *vectoring, int8_t *vec_num,
  Battery *MyBattEKF, Battery *MyBattModel)
{
  double SOCS_in = -99.;
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
          case ( 'r' ):
            double rscale = cp.input_string.substring(2).toFloat();
            MyBattModel->Sr(rscale);
            MyBattEKF->Sr(rscale);
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
        SOCS_in = cp.input_string.substring(1).toFloat()/100.;
        rp.soc = max(min(SOCS_in, mxepu_bb), mnepu_bb);
        rp.soc_model = max(min(SOCS_in, mxepu_bb), mnepu_bb);
        rp.delta_q = max((rp.soc - 1.)*nom_q_cap, -rp.q_sat);
        rp.delta_q_model = max((rp.soc_model - 1.)*true_q_cap, -rp.q_sat_model);
        Serial.printf("soc=%7.3f,   delta_q=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f\n",
                        rp.soc, rp.delta_q, rp.soc_model, rp.delta_q_model);
        break;
      case ( 's' ): 
        rp.curr_sel_amp = !rp.curr_sel_amp;
        Serial.printf("Signal selection (1=amp, 0=no amp) toggled to %d\n", rp.curr_sel_amp);
        break;
      case ( 'v' ):
        rp.debug = cp.input_string.substring(1).toInt();
        break;
      case ( 'T' ):
        talkT(&cp.stepping, &cp.step_val, &cp.vectoring, &cp.vec_num);
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
                rp.debug = 0;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug);
                break;
              case ( 1 ):
                rp.modeling = true;
                rp.type = 1;
                rp.freq = 0.05;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                rp.debug = -12;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug);
                rp.freq *= (2. * PI);
                break;
              case ( 2 ):
                rp.modeling = true;
                rp.type = 2;
                rp.freq = 0.10;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug);
                rp.freq *= (2. * PI);
                rp.debug = -12;
                break;
              case ( 3 ):
                rp.modeling = true;
                rp.type = 3;
                rp.freq = 0.05;
                rp.amp = 18.3;
                rp.offset = -rp.amp;
                Serial.printf("Setting injection program to:  rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d\n",
                                        rp.modeling, rp.type, rp.freq, rp.amp, rp.debug);
                rp.freq *= (2. * PI);
                rp.debug = -12;
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
        talkH(&cp.step_val, &cp.vec_num, MyBattModel);
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
  Serial.printf("s   curr signal select (1=amp preferred, 0=noamp) = "); Serial.println(rp.curr_sel_amp);
  Serial.printf("v=  "); Serial.print(rp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("D/S<?> Adjustments.   For example:\n");
  Serial.printf("  Da= "); Serial.printf("%7.3f", rp.curr_bias_amp); Serial.println("    : delta I adder to sensed amplified shunt current, A [0]"); 
  Serial.printf("  Db= "); Serial.printf("%7.3f", rp.curr_bias_noamp); Serial.println("    : delta I adder to sensed shunt current, A [0]"); 
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.curr_bias_all); Serial.println("    : delta I adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.println("    : delta V adder to sensed battery voltage, V [0]"); 
  Serial.printf("  Dv= "); Serial.print(batt_solved->Dv()); Serial.println("    : delta V adder to solved battery calculation, V"); 
  Serial.printf("  Sr= "); Serial.print(batt_solved->Sr()); Serial.println("    : Scalar resistor for battery dynamic calculation, V"); 
  Serial.printf("T<?>=  "); 
  Serial.printf("T - Transient performed with input.   For example:\n");
  Serial.printf("  Ts=<index>  :   index="); Serial.print(*step_val);
  Serial.printf(", cp.stepping=");  Serial.println(cp.stepping);
  Serial.printf("  Tv=<vec_num>  :  vec_num="); Serial.println(*vec_num);
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
    const double r1, const double r2, const double r2c2, const double batt_vsat, const double dvoc_dt) :
    Battery(x_tab, b_tab, a_tab, c_tab, m, n, d, nz, num_cells, r1, r2, r2c2, batt_vsat, dvoc_dt)
{
  q_cap_ = true_q_cap;
}

// SOC-OCV curve fit method per Zhang, et al.   Makes a good reference model
double BatteryModel::calculate(const double temp_C, const double soc, const double curr_in, const double dt)
{
    dt_ = dt;

    soc_ = soc;
    q_ = soc_ * q_cap_;
    double soc_lim = max(min(soc_, mxeps_bb), mneps_bb);
    ib_ = curr_in;

    // VOC-OCV model
    double log_soc, exp_n_soc, pow_log_soc;
    calc_soc_voc_coeff(soc_lim, temp_C, &b_, &a_, &c_, &log_soc, &exp_n_soc, &pow_log_soc);
    voc_ = calc_voc_ocv(soc_lim, &dv_dsoc_, b_, a_, c_, log_soc, exp_n_soc, pow_log_soc)
             + (soc_ - soc_lim) * dv_dsoc_;  // slightly beyond
    voc_ +=  dv_;  // Experimentally varied

    // Dynamic emf
    double u[2] = {ib_, voc_};
    RandlesInv_->calc_x_dot(u);
    RandlesInv_->update(dt);
    vb_ = RandlesInv_->y(0);
    vdyn_ = vb_ - voc_;


    // Summarize   TODO: get rid of the global defines here because they differ from one battery to another
    pow_in_ = vb_*ib_ - ib_*ib_*(r1_+r2_)*sr_*num_cells_;  // Internal resistance of battery is a loss
    vsat_ = nom_vsat_ + (temp_C-25.)*dvoc_dt_;
    // sat_ = voc_ >= vsat_;

    if ( rp.debug==78 ) Serial.printf("calculate_ model:  soc_in,v,curr,pow,vsat,voc= %7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n", 
      soc, vb_, ib_, pow_in_, vsat_, voc_);

    if ( rp.debug==79 )Serial.printf("calculate_model:  tempC,tempF,curr,a,b,c,d,n,m,r,soc,logsoc,expnsoc,powlogsoc,voc,vdyn,v,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
     temp_C, temp_C*9./5.+32., ib_, a_, b_, c_, d_, n_, m_, (r1_+r2_)*sr_ , soc_, log_soc, exp_n_soc, pow_log_soc, voc_, vdyn_, vb_);

    return ( vb_ );
}

/* Count coulombs base on true=actual capacity
    Internal resistance of battery is a loss
    Inputs:
        dt      Integration step, s
        pow_in  Charge power, W
        saturated   Indicator that battery is saturated (VOC>threshold(temp)), T/F
        temp_c  Battery temperature, deg C
    Outputs:
        soc_avail  State of charge, fraction (0-1)
        delta_soc  Iteration rate of change, (0-1)
        t_sat       Battery temperature at saturation, deg C
        soc_sat    State of charge at saturation, fraction (0-1)
*/
double BatteryModel::coulombs(const double dt, const double charge_curr, const double q_cap, const boolean sat,
  const double temp_c, double *delta_q, double *t_sat, double *q_sat)
{
    double soc = 0;   // return value
    double q_avail = *q_sat*(1. - DQDT*(temp_c - *t_sat));
    double d_delta_q = charge_curr * dt;
    if ( sat )
    {
        if ( d_delta_q>0 )
        {
            d_delta_q = 0.;
            *delta_q = 0.;
        }
        *t_sat = temp_c;
        *q_sat = ((*t_sat - 25.)*DQDT + 1.)*q_cap;
        q_avail = *q_sat;
    }
    *delta_q = max(min(*delta_q + d_delta_q, 1.1*(q_cap-q_avail)), -q_avail);
    soc = (q_avail + *delta_q) / q_avail;
    if ( rp.debug==76 )
        Serial.printf("BatteryModel::coulombs:  voc, v_sat, sat, charge_curr, d_d_q, d_q, q_sat, tsat,q_avail,soc=     %7.3f,%7.3f,%d,%7.3f,%10.6f,%10.6f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
                    cp.pubList.VOC,  sat_voc(temp_c), sat, charge_curr, d_delta_q, *delta_q, *q_sat, *t_sat, q_avail, soc);
    if ( rp.debug==-76 )
        Serial.printf("voc, v_sat, sat, charge_curr, d_d_q, d_q, q_sat, tsat,q_avail,soc          \n%7.3f,%7.3f,%d,%7.3f,%10.6f,%10.6f,%7.3f,%7.3f,%7.3f,%7.3f,\n",
                    cp.pubList.VOC, sat_voc(temp_c), sat, charge_curr, d_delta_q, *delta_q, *q_sat, *t_sat, q_avail, soc);
    return ( soc );
}
