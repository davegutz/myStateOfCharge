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
#include "command.h"
#include "myTalk.h"
#include "parameters.h"
#include <math.h>

extern CommandPars cp;          // Various parameters shared at system level
// extern Flt_st mySum[NSUM];      // Summaries for saving charge history
// extern Flt_st myFlt[NFLT];      // Summaries for saving charge history
extern SavedPars sp;
extern eSavedPars esp;

// Process asap commands
void asap()
{
  get_string(&cp.asap_str);
  // if ( cp.token ) Serial.printf("asap:  talk('%s;')\n", cp.input_str.c_str());
}

// Process chat strings
void chat()
{
  if ( cp.soon_str.length() )  // Do SOON first
  {
    get_string(&cp.soon_str);
    // if ( cp.token ) Serial.printf("chat (SOON):  talk('%s;')\n", cp.input_str.c_str());
  }
  else  // Do QUEUE only after SOON empty
  {
    get_string(&cp.queue_str);
    // if ( cp.token ) Serial.printf("chat (QUEUE):  talk('%s;')\n", cp.input_str.c_str());
  }
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for talk()
void chit(const String cmd, const urgency when)
{
  // Serial.printf("chit cmd=%s\n", cmd.c_str());
  if ( when == QUEUE )
    cp.queue_str += cmd;
  else if ( when == SOON )
    cp.soon_str += cmd;
  else
    cp.asap_str += cmd;
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for talk()
void clear_queues()
{
  cp.queue_str = "";
  cp.soon_str = "";
  cp.asap_str = "";
}

// If false token, get new string from source
void get_string(String *source)
{
  while ( !cp.token && source->length() )
  {
    // get the new byte, add to input and check for completion
    char inChar = source->charAt(0);
    source->remove(0, 1);
    cp.input_str += inChar;
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      cp.input_str = ">" + cp.input_str;
      break;  // enable reading multiple inputs
    }
  }
}

// Convert time to decimal for easy lookup
double decimalTime(unsigned long *current_time, char* tempStr, unsigned long now, unsigned long millis_flip)
{
  *current_time = Time.now();  // Seconds since start of epoch
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
        *current_time = Time.now();  // Seconds since start of epoch
        day = Time.day(*current_time);
        hours = Time.hour(*current_time);
      }
  }
  // uint8_t dayOfWeek = Time.weekday(*current_time)-1;  // 0-6
  uint8_t minutes   = Time.minute(*current_time);
  uint8_t seconds   = Time.second(*current_time);

  // Convert the string
  time_long_2_str(*current_time, tempStr);

  // Convert the decimal
  static double cTimeInit = ((( (double(year-2021)*12 + double(month))*30.4375 + double(day))*24.0 + double(hours))*60.0 + double(minutes))*60.0 + \
                      double(seconds) + double(now-millis_flip)/1000.;
  // Ignore Time.now if corrupt
  if ( year<2020 ) cTimeInit = 0.;
  // Serial.printf("y %ld m %d d %d h %d m %d s %d now %ld millis_flip %ld\n", year, month, day, hours, minutes, seconds, now, millis_flip);
  double cTime = cTimeInit + double(now-millis_flip)/1000.;
  // Serial.printf("%ld - %ld = %18.12g, cTimeInit=%18.12g, cTime=%18.12g\n", now, millis_flip, double(now-millis_flip)/1000., cTimeInit, cTime);
  return ( cTime );
}

// Cleanup string for final processing by talk
void finish_request(void)
{
  // Remove whitespace
  cp.input_str.trim();
  cp.input_str.replace("\0","");
  cp.input_str.replace(";","");
  cp.input_str.replace(",","");
  cp.input_str.replace(" ","");
  cp.input_str.replace("=","");
  cp.token = true;  // token:  temporarily inhibits while loop until talk() call resets token
}

/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.

  Particle documentation says not to use something like
  the cp.token in the while loop statement.
  They suggest handling all the data in one call.   But 
  this works, so far.
 */
void serialEvent()
{
  while ( !cp.token && Serial.available() )
  {
    // get the new byte:
    char inChar = (char)Serial.read();

    // add it to the cp.input_str:
    cp.input_str += inChar;

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      break;  // enable reading multiple inputs
    }
  }
  // if ( cp.token ) Serial.printf("serialEvent:  %s\n", cp.input_str.c_str());
}

/*
  Special handler that uses built-in callback.
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */
void serialEvent1()
{
  while (!cp.token && Serial1.available())
  {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the cp.input_str:
    cp.input_str += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',')
    {
      finish_request();
      break;  // enable reading multiple inputs
    }
  }
}

// Talk Executive
void talk()
{
  double FP_in = -99.;
  int INT_in = -1;
  double scale = 1.;
  urgency request;
  int n, now, then;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.token)
  {
    // Categorize the requests
    char key = cp.input_str.charAt(0);
    if ( key == '-' )
      request = ASAP;
    else if ( key == '+' )
      request = QUEUE;
    else if ( key == '*' )
      request = SOON;
    else if ( key == '>' )
    {
      cp.input_str = cp.input_str.substring(1);  // Delete the leading '>'
      request = INCOMING;
    }
    else
    {
      request = NEW;
      if ( key == 'c' ) clear_queues();
    }

    // Limited echoing of Serial1 commands available
    Serial.printf ("echo: %s, %d\n", cp.input_str.c_str(), request);
    Serial1.printf("echo: %s, %d\n", cp.input_str.c_str(), request);

    // Deal with each request
    switch ( request )
    {
      case ( NEW ):  // Defaults to QUEUE
        // Serial.printf("new:%s,\n", cp.input_str.substring(0).c_str());
        chit( cp.input_str.substring(0) + ";", QUEUE);
        break;

      case ( ASAP ):
        // Serial.printf("asap:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", ASAP);
        break;

      case ( SOON ):
        // Serial.printf("soon:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", SOON);
        break;

      case ( QUEUE ):
        // Serial.printf("queue:%s,\n", cp.input_str.substring(1).c_str());
        chit( cp.input_str.substring(1)+";", QUEUE);
        break;

      case ( INCOMING ):
        // Serial.printf("IN:%s,\n", cp.input_str.c_str());
        switch ( cp.input_str.charAt(0) )
        {

          case ( 'b' ):  // Fault buffer
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'd' ):  // bd: fault buffer dump
                Serial.printf("\n");
                sp.print_history_array();
                sp.print_fault_header();
                sp.print_fault_array();
                sp.print_fault_header();
                esp.print_history_array();
                esp.print_fault_header();
                esp.print_fault_array();
                esp.print_fault_header();
                break;

              case ( 'R' ):  // bR: Fault buffer reset
                Serial.printf("bR large reset\n");
                now = micros();
                n = sp.large_reset();
                then = micros();
                Serial.printf("n %d avg %10.6f\n", n, float(then - now)/1e6/float(n));
                now = micros();
                n = esp.large_reset();
                then = micros();
                Serial.printf("en %d eavg %10.6f\n", n, float(then - now)/1e6/float(n));
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'B' ):  // B: battery
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'm' ):  //   Bm<>:  mon_chm
                Serial.printf("Print mon_chm %d to ", sp.mon_chm());
                sp.put_mon_chm(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", sp.mon_chm());
                Serial.printf("Print emon_chm %d to ", esp.mon_chm());
                esp.put_mon_chm(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", esp.mon_chm());
                break;

              case ( 'P' ):  // BP<>:  Number of parallel batteries in bank, e.g. '2P1S'
                FP_in = cp.input_str.substring(2).toFloat();
                if ( FP_in>0 )  // Apply crude limit to prevent user error
                {
                  Serial.printf("nP%5.2f to", sp.nP());
                  sp.put_nP(FP_in);
                  Serial.printf("%5.2f\n", sp.nP());
                  Serial.printf("enP%5.2f to", esp.nP());
                  esp.put_nP(FP_in);
                  Serial.printf("%5.2f\n", esp.nP());
                }
                else
                  Serial.printf("err%5.2f; <=0\n", FP_in);
                break;

              case ( 'S' ):  // BS<>:  Number of series batteries in bank, e.g. '2P1S'
                FP_in = cp.input_str.substring(2).toFloat();
                if ( FP_in>0 )  // Apply crude limit to prevent user error
                {
                  Serial.printf("nP%5.2f to", sp.nS());
                  sp.put_nS(FP_in);
                  Serial.printf("%5.2f\n", sp.nS());
                  Serial.printf("enP%5.2f to", esp.nS());
                  esp.put_nS(FP_in);
                  Serial.printf("%5.2f\n", esp.nS());
                }
                else
                  Serial.printf("err%5.2f; <=0\n", FP_in);
                break;

              case ( 's' ):  //   Bs<>:  sim_chm
                Serial.printf("Print sim_chm %d to ", sp.sim_chm());
                sp.put_sim_chm(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", sp.sim_chm());
                Serial.printf("Print esim_chm %d to ", esp.sim_chm());
                esp.put_sim_chm(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", esp.sim_chm());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'P' ):  // P: print
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'S' ):  // PS: print saved pars
                Serial.printf("\n");
                sp.pretty_print(true);
                esp.pretty_print(true);
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'R' ):  // R: reset
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'S' ):  // RS: reset saved pars
                sp.reset_pars();
                sp.pretty_print(true);
                esp.reset_pars();
                esp.pretty_print(true);
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'D' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'A' ):  // * DA<>:  Amp sensor bias
                Serial.printf("sp.Ib_bias_amp%7.3f to", sp.Ib_bias_amp());
                sp.put_Ib_bias_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.Ib_bias_amp());
                Serial.printf("esp.Ib_bias_amp%7.3f to", esp.Ib_bias_amp());
                esp.put_Ib_bias_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.Ib_bias_amp());
                break;

              case ( 'B' ):  // * DB<>:  No Amp sensor bias
                Serial.printf("sp.ib_bias_noa%7.3f to", sp.Ib_bias_noa());
                sp.put_Ib_bias_noa(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.Ib_bias_noa());
                Serial.printf("esp.ib_bias_noa%7.3f to", esp.Ib_bias_noa());
                esp.put_Ib_bias_noa(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.Ib_bias_noa());
                break;

              case ( 'c' ):  // * Dc<>:  Vb bias
                Serial.printf("sp.Vb_bias_hdwe%7.3f to", sp.Vb_bias_hdwe());
                sp.put_Vb_bias_hdwe(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.Vb_bias_hdwe());
                Serial.printf("esp.Vb_bias_hdwe%7.3f to", esp.Vb_bias_hdwe());
                esp.put_Vb_bias_hdwe(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.Vb_bias_hdwe());
                break;

              case ( 'E' ):  //   DE<>:  EKF execution frame multiplier
                Serial.printf("Eframe mult %d to ", cp.eframe_mult);
                cp.assign_eframe_mult(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.eframe_mult);
                break;

              case ( 'i' ):  // * Di<>:  Bias all current sensors (same way as Da and Db)
                Serial.printf("sp.ib_bias_all%7.3f to", sp.Ib_bias_all());
                sp.put_Ib_bias_all(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", sp.Ib_bias_all());
                Serial.printf("esp.ib_bias_all%7.3f to", esp.Ib_bias_all());
                esp.put_Ib_bias_all(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", esp.Ib_bias_all());
                cp.cmd_reset();
                break;

              case ( 'P' ):  //   DP<>:  PRINT multiplier
                Serial.printf("Print int %d to ", cp.print_mult);
                cp.assign_print_mult(max(min(cp.input_str.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.print_mult);
                break;

              case ( 'Q' ):  // * DQ<>:  delta_q
                Serial.printf("sp.delta_q%7.3f to", sp.delta_q());
                sp.put_delta_q(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", sp.delta_q());
                Serial.printf("esp.delta_q%7.3f to", esp.delta_q());
                esp.put_delta_q(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", esp.delta_q());
                break;

              case ( 't' ):  // * Dt<>:  Temp bias change hardware
                Serial.printf("sp.Tb_bias_hdwe%7.3f to", sp.Tb_bias_hdwe());
                sp.put_Tb_bias_hdwe(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", sp.Tb_bias_hdwe());
                Serial.printf("esp.Tb_bias_hdwe%7.3f to", esp.Tb_bias_hdwe());
                esp.put_Tb_bias_hdwe(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", esp.Tb_bias_hdwe());
                cp.cmd_reset();
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'S' ):
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'A' ):  // * SA<>:  Amp sensor scalar
                Serial.printf("sp.ib_bias_amp%7.3f to ", sp.ib_scale_amp());
                sp.put_ib_scale_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.ib_scale_amp());
                Serial.printf("esp.ib_bias_amp%7.3f to ", esp.ib_scale_amp());
                esp.put_ib_scale_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.ib_scale_amp());
                break;

              case ( 'B' ):  // * SB<>:  No Amp sensor scalar
                Serial.printf("sp.Ib_scale_noa%7.3f to ", sp.ib_scale_noa());
                sp.put_ib_scale_noa(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.ib_scale_noa());
                Serial.printf("esp.Ib_scale_noa%7.3f to ", esp.ib_scale_noa());
                esp.put_ib_scale_noa(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.ib_scale_noa());
                break;

              case ( 'c' ):  // * Sc<>: scale capacity
                Serial.printf("sp.s_cap_sim%7.3f to ", sp.s_cap_sim());
                scale = cp.input_str.substring(2).toFloat();
                sp.put_s_cap_sim(scale);
                Serial.printf("%7.3f\n", sp.s_cap_sim());
                Serial.printf("esp.s_cap_sim%7.3f to ", esp.s_cap_sim());
                scale = cp.input_str.substring(2).toFloat();
                esp.put_s_cap_sim(scale);
                Serial.printf("%7.3f\n", esp.s_cap_sim());
                break;
                       
              case ( 'G' ):  // * SG<>:  Shunt gain scalar
                Serial.printf("sp.shunt_gain_sclr%7.3f to ", sp.shunt_gain_sclr());
                sp.put_shunt_gain_sclr(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.shunt_gain_sclr());
                Serial.printf("esp.shunt_gain_sclr%7.3f to ", esp.shunt_gain_sclr());
                esp.put_shunt_gain_sclr(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.shunt_gain_sclr());
                break;
           
              case ( 'h' ):  //   Sh<>: scale hysteresis
                Serial.printf("sp.hys_sale%7.3f to ", sp.hys_scale());
                sp.put_hys_scale(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.hys_scale());
                Serial.printf("ep.hys_sale%7.3f to ", esp.hys_scale());
                esp.put_hys_scale(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.hys_scale());
                break;

              case ( 'k' ):  // * Sk<>:  scale cutback gain for sim rep of BMS
                scale = cp.input_str.substring(2).toFloat();
                Serial.printf("sp.cutback_gain_sclr%7.3f to ", sp.cutback_gain_sclr());
                sp.put_cutback_gain_sclr(scale);
                Serial.printf("%7.3f\n", sp.cutback_gain_sclr());
                Serial.printf("esp.cutback_gain_sclr%7.3f to ", esp.cutback_gain_sclr());
                esp.put_cutback_gain_sclr(scale);
                Serial.printf("%7.3f\n", esp.cutback_gain_sclr());
                break;
            
              case ( 'V' ):  // * SV<>:  Vb sensor scalar
                Serial.printf("sp.Vb_scale%7.3f to", sp.Vb_scale());
                sp.put_Vb_scale(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", sp.Vb_scale());
                Serial.printf("esp.Vb_scale%7.3f to", esp.Vb_scale());
                esp.put_Vb_scale(cp.input_str.substring(2).toFloat());
                Serial.printf("%7.3f\n", esp.Vb_scale());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'F' ):  // Fault stuff
            switch ( cp.input_str.charAt(1) )
            {

              case ( 'f' ):  //   Ff<>:  fake faults
                INT_in = cp.input_str.substring(2).toInt();
                Serial.printf("cp.fake_faults, sp.ib_select() %d, %d to ", cp.fake_faults, sp.ib_select());
                cp.fake_faults = INT_in;
                sp.put_ib_select(INT_in);
                Serial.printf("%d, %d\n", cp.fake_faults, sp.ib_select());
                Serial.printf("cp.fake_faults, esp.ib_select() %d, %d to ", cp.fake_faults, esp.ib_select());
                cp.fake_faults = INT_in;
                esp.put_ib_select(INT_in);
                Serial.printf("%d, %d\n", cp.fake_faults, esp.ib_select());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;


          case ( 's' ):  // s<>:  select amp or noa
            if ( cp.input_str.substring(1).toInt()>0 )
            {
              sp.put_ib_select(1);
              esp.put_ib_select(1);
            }
            else if ( cp.input_str.substring(1).toInt()<0 )
            {
              sp.put_ib_select(-1);
              esp.put_ib_select(-1);
            }
            else
            {
              sp.put_ib_select(0);
              esp.put_ib_select(0);
            }
            Serial.printf("Sig ( -1=noa, 0=auto, 1=amp,) set %d\n", sp.ib_select());
            Serial.printf("eSig ( -1=noa, 0=auto, 1=amp,) set %d\n", esp.ib_select());
            break;

          case ( 'v' ):  // v<>:  verbose level
            Serial.printf("sp.debug %d to ", sp.debug());
            sp.put_debug(cp.input_str.substring(1).toInt());
            Serial.printf("%d\n", sp.debug());
            Serial.printf("esp.debug %d to ", esp.debug());
            esp.put_debug(cp.input_str.substring(1).toInt());
            Serial.printf("%d\n", esp.debug());
            break;

          case ( 'V' ):
            switch ( cp.input_str.charAt(1) )
            {

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'W' ):  // W<>:  wait.  Skip
            if ( cp.input_str.substring(1).length() )
            {
              INT_in = cp.input_str.substring(1).toInt();
              if ( INT_in > 0 )
              {
                for ( int i=0; i<INT_in; i++ )
                {
                  chit("W;", SOON);
                }
              }
            }
            else
            {
              Serial.printf("..Wait.\n");
            }
            break;

          case ( 'X' ):  // X
            switch ( cp.input_str.charAt(1) )
            {
              case ( 'd' ):  // Xd<>:  on/off dc-dc charger manual setting
                if ( cp.input_str.substring(2).toInt()>0 )
                  cp.dc_dc_on = true;
                else
                  cp.dc_dc_on = false;
                Serial.printf("dc_dc_on to %d\n", cp.dc_dc_on);
                break;

              case ( 'm' ):  // Xm<>:  code for modeling level
                INT_in =  cp.input_str.substring(2).toInt();
                if ( INT_in>=0 && INT_in<1000 )
                {
                  Serial.printf("modeling %d to ", sp.modeling());
                  sp.put_modeling(INT_in);
                  Serial.printf("%d\n", sp.modeling());
                  Serial.printf("emodeling %d to ", esp.modeling());
                  esp.put_modeling(INT_in);
                  Serial.printf("%d\n", esp.modeling());
                }
                else
                {
                  Serial.printf("err %d, modeling 0-7. 'h'\n", INT_in);
                }
                Serial.printf("Modeling %d\n", sp.modeling());
                Serial.printf("tweak_test %d\n", sp.tweak_test());
                Serial.printf("mod_ib %d\n", sp.mod_ib());
                Serial.printf("mod_vb %d\n", sp.mod_vb());
                Serial.printf("mod_tb %d\n", sp.mod_tb());
                Serial.printf("eModeling %d\n", esp.modeling());
                Serial.printf("etweak_test %d\n", esp.tweak_test());
                Serial.printf("emod_ib %d\n", esp.mod_ib());
                Serial.printf("emod_vb %d\n", esp.mod_vb());
                Serial.printf("emod_tb %d\n", esp.mod_tb());
                break;

              case ( 'a' ): // Xa<>:  injection amplitude
                sp.put_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("Inj amp set%7.3f & inj_bias set%7.3f\n", sp.amp(), sp.inj_bias());
                esp.put_amp(cp.input_str.substring(2).toFloat());
                Serial.printf("eInj amp set%7.3f & inj_bias set%7.3f\n", esp.amp(), esp.inj_bias());
                break;

              case ( 'b' ): // Xb<>:  injection bias
                sp.put_inj_bias(cp.input_str.substring(2).toFloat());
                Serial.printf("Inj_bias set%7.3f\n", sp.inj_bias());
                esp.put_inj_bias(cp.input_str.substring(2).toFloat());
                Serial.printf("eInj_bias set%7.3f\n", esp.inj_bias());
                break;

              case ( 'f' ): // Xf<>:  injection freq
                sp.put_freq(cp.input_str.substring(2).toFloat());
                Serial.printf("Inj freq set%7.3f\n", sp.freq());
                esp.put_freq(cp.input_str.substring(2).toFloat());
                Serial.printf("eInj freq set%7.3f\n", esp.freq());
                break;

              case ( 't' ): // Xt<>:  injection type
                switch ( cp.input_str.charAt(2) )
                {
                  case ( 'n' ):  // Xtn:  none
                    sp.put_type(0);
                    Serial.printf("Set none. sp.type %d\n", sp.type());
                    esp.put_type(0);
                    Serial.printf("eSet none. esp.type %d\n", esp.type());
                    break;

                  case ( 's' ):  // Xts:  sine
                    sp.put_type(1);
                    Serial.printf("Set sin. sp.type %d\n", sp.type());
                    esp.put_type(1);
                    Serial.printf("eSet sin. esp.type %d\n", esp.type());
                    break;

                  case ( 'q' ):  // Xtq:  square
                    sp.put_type(2);
                    Serial.printf("Set square. sp.type %d\n", sp.type());
                    esp.put_type(2);
                    Serial.printf("eSet square. esp.type %d\n", esp.type());
                    break;

                  case ( 't' ):  // Xtt:  triangle
                    sp.put_type(3);
                    Serial.printf("Set tri. sp.type %d\n", sp.type());
                    esp.put_type(3);
                    Serial.printf("eSet tri. esp.type %d\n", esp.type());
                    break;

                  case ( 'c' ):  // Xtc:  charge rate
                    sp.put_type(4);
                    Serial.printf("Set 1C charge. sp.type %d\n", sp.type());
                    esp.put_type(4);
                    Serial.printf("eSet 1C charge. esp.type %d\n", esp.type());
                    break;

                  case ( 'd' ):  // Xtd:  discharge rate
                    sp.put_type(5);
                    Serial.printf("Set 1C disch. sp.type %d\n", sp.type());
                    esp.put_type(5);
                    Serial.printf("eSet 1C disch. esp.type %d\n", esp.type());
                    break;

                  case ( 'o' ):  // Xto:  cosine
                    sp.put_type(8);
                    Serial.printf("Set cos. sp.type %d\n", sp.type());
                    esp.put_type(8);
                    Serial.printf("eSet cos. esp.type %d\n", esp.type());
                    break;

                  default:
                    Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
                }
                break;

              case ( 'o' ): // Xo<>:  injection dc offset
                sp.put_inj_bias(max(min(cp.input_str.substring(2).toFloat(), 18.3), -18.3));
                Serial.printf("inj_bias set%7.3f\n", sp.inj_bias());
                esp.put_inj_bias(max(min(cp.input_str.substring(2).toFloat(), 18.3), -18.3));
                Serial.printf("einj_bias set%7.3f\n", esp.inj_bias());
                break;

              default:
                Serial.print(cp.input_str.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'h' ):  // h: help
            talkH();
            break;

          default:
            Serial.print(cp.input_str.charAt(0)); Serial.println(" ? 'h'");
            break;
        }
    }

    cp.input_str = "";
    cp.token = false;
  }  // if ( cp.token )
}

// Talk Help
void talkH()
{
  Serial.printf("\n\nHelp menu.  End entry with ';'.  SRAM='*'.  May omit '='\n");

  Serial.printf("\nb<?>   Manage fault buffer\n");
  Serial.printf("  bd= "); Serial.printf("dump fault buffer\n");
  Serial.printf("  bR= "); Serial.printf("reset fault buffer\n");

  Serial.printf("\nc  clear talk, esp '-c;'\n");

  Serial.printf("\nB<?> Battery e.g.:\n");
  Serial.printf(" *Bm=  %d.  Mon chem 0='BB', 1='LI' [%d]\n", sp.mon_chm(), MON_CHEM); 
  Serial.printf(" *Bm=  %d.  Mon chem 0='BB', 1='LI' [%d]\n", esp.mon_chm(), MON_CHEM); 
  Serial.printf(" *Bs=  %d.  Sim chem 0='BB', 1='LI' [%d]\n", sp.sim_chm(), SIM_CHEM); 
  Serial.printf(" *Bs=  %d.  Sim chem 0='BB', 1='LI' [%d]\n", esp.sim_chm(), SIM_CHEM); 
  Serial.printf(" *BP=  %4.2f.  parallel in bank [%4.2f]'\n", sp.nP(), NP); 
  Serial.printf(" *BP=  %4.2f.  parallel in bank [%4.2f]'\n", esp.nP(), NP); 
  Serial.printf(" *BS=  %4.2f.  series in bank [%4.2f]'\n", sp.nS(), NS); 
  Serial.printf(" *BS=  %4.2f.  series in bank [%4.2f]'\n", esp.nS(), NS); 

  Serial.printf("\nD/S<?> Adj e.g.:\n");
  Serial.printf(" *Di= "); Serial.printf("%6.3f", sp.Ib_bias_all()); Serial.printf(": delta all, A [%6.3f]\n", CURR_BIAS_ALL); 
  Serial.printf(" *Di= "); Serial.printf("%6.3f", esp.Ib_bias_all()); Serial.printf(": delta all, A [%6.3f]\n", CURR_BIAS_ALL); 
  Serial.printf(" *DA= "); Serial.printf("%6.3f", sp.Ib_bias_amp()); Serial.printf(": delta amp, A [%6.3f]\n", CURR_BIAS_AMP); 
  Serial.printf(" *DA= "); Serial.printf("%6.3f", esp.Ib_bias_amp()); Serial.printf(": delta amp, A [%6.3f]\n", CURR_BIAS_AMP); 
  Serial.printf(" *DB= "); Serial.printf("%6.3f", sp.Ib_bias_noa()); Serial.printf(": delta noa, A [%6.3f]\n", CURR_BIAS_NOA); 
  Serial.printf(" *DB= "); Serial.printf("%6.3f", esp.Ib_bias_noa()); Serial.printf(": delta noa, A [%6.3f]\n", CURR_BIAS_NOA); 
  Serial.printf(" *SA= "); Serial.printf("%6.3f", sp.ib_scale_amp()); Serial.printf(": scale amp [%6.3f]\n", CURR_SCALE_AMP); 
  Serial.printf(" *SA= "); Serial.printf("%6.3f", esp.ib_scale_amp()); Serial.printf(": scale amp [%6.3f]\n", CURR_SCALE_AMP); 
  Serial.printf(" *SB= "); Serial.printf("%6.3f", sp.ib_scale_noa()); Serial.printf(": scale noa [%6.3f]\n", CURR_SCALE_NOA); 
  Serial.printf(" *SB= "); Serial.printf("%6.3f", esp.ib_scale_noa()); Serial.printf(": scale noa [%6.3f]\n", CURR_SCALE_NOA); 
  Serial.printf(" *Dc= "); Serial.printf("%6.3f", sp.Vb_bias_hdwe()); Serial.printf(": delta, V [%6.3f]\n", VOLT_BIAS); 
  Serial.printf(" *Dc= "); Serial.printf("%6.3f", esp.Vb_bias_hdwe()); Serial.printf(": delta, V [%6.3f]\n", VOLT_BIAS); 
  Serial.printf(" *Dt= "); Serial.printf("%6.3f", sp.Tb_bias_hdwe()); Serial.printf(": delta hdwe, deg C [%6.3f]\n", TEMP_BIAS); 
  Serial.printf(" *Dt= "); Serial.printf("%6.3f", esp.Tb_bias_hdwe()); Serial.printf(": delta hdwe, deg C [%6.3f]\n", TEMP_BIAS); 
  Serial.printf(" *SG= "); Serial.printf("%6.3f", sp.shunt_gain_sclr()); Serial.printf(": sp. scale shunt gains [1]\n"); 
  Serial.printf(" *SG= "); Serial.printf("%6.3f", esp.shunt_gain_sclr()); Serial.printf(": sp. scale shunt gains [1]\n"); 
  Serial.printf(" *Sh= "); Serial.printf("%6.3f", sp.hys_scale()); Serial.printf(": hys sclr [%5.2f]\n", HYS_SCALE);
  Serial.printf(" *Sh= "); Serial.printf("%6.3f", esp.hys_scale()); Serial.printf(": hys sclr [%5.2f]\n", HYS_SCALE);
  Serial.printf(" *Sk=  "); Serial.print(sp.cutback_gain_sclr()); Serial.println(": Sat mod ctbk sclr"); 
  Serial.printf(" *Sk=  "); Serial.print(esp.cutback_gain_sclr()); Serial.println(": Sat mod ctbk sclr"); 
  Serial.printf(" *SV= "); Serial.printf("%6.3f", sp.Vb_scale()); Serial.printf(": scale vb sen [%6.3f]\n", VB_SCALE); 
  Serial.printf(" *SV= "); Serial.printf("%6.3f", esp.Vb_scale()); Serial.printf(": scale vb sen [%6.3f]\n", VB_SCALE); 

  Serial.printf("\nF<?>   Faults\n");
  // Serial.printf("  Fc= "); Serial.printf("%6.3f", Sen->Flt->cc_diff_sclr()); Serial.printf(": sclr cc_diff thr ^ [1]\n"); 

  Serial.printf("\nH<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump summ log\n");
  Serial.printf("  HR= "); Serial.printf("reset summ log\n");
  Serial.printf("  Hs= "); Serial.printf("save and print log\n");

  Serial.printf("\nP<?>   Print values\n");
  Serial.printf("  PS= "); Serial.printf("SavedPars\n");


  Serial.printf("\nR<?>   Reset\n");
  Serial.printf("  RS= "); Serial.printf("SavedPars: Reinitialize saved\n");

  Serial.printf("\nv= "); Serial.print(sp.debug()); Serial.println(": verbosity, -128 - +128. [4]");
  Serial.printf("  -<>: Negative - Arduino plot compatible\n");

  Serial.printf("\nW<?> - iters to wait\n");

  Serial.printf("\nurgency of cmds: -=ASAP,*=SOON, '' or +=QUEUE\n");
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
        // uint8_t dayOfWeek = Time.weekday(current_time)-1;  // 0-6
        uint8_t minutes   = Time.minute(current_time);
        uint8_t seconds   = Time.second(current_time);
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
