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
#include "retained.h"
#include "command.h"
#include "mySummary.h"
#include "parameters.h"
#include <math.h>

extern CommandPars cp;          // Various parameters shared at system level
extern RetainedPars rp;         // Various parameters to be static at system level
extern Sum_st mySum[NSUM];      // Summaries for saving charge history
extern Flt_st myFlt[NFLT];      // Summaries for saving charge history
extern SavedPars sp;

// Process asap commands
void asap()
{
  get_string(&cp.asap_str);
  // if ( cp.token ) Serial.printf("asap:  talk('%s;')\n", cp.input_string.c_str());
}

// Process chat strings
void chat()
{
  if ( cp.soon_str.length() )  // Do SOON first
  {
    get_string(&cp.soon_str);
    // if ( cp.token ) Serial.printf("chat (SOON):  talk('%s;')\n", cp.input_string.c_str());
  }
  else  // Do QUEUE only after SOON empty
  {
    get_string(&cp.queue_str);
    // if ( cp.token ) Serial.printf("chat (QUEUE):  talk('%s;')\n", cp.input_string.c_str());
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
    cp.input_string += inChar;
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      cp.input_string = ">" + cp.input_string;
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
  cp.input_string.trim();
  cp.input_string.replace("\0","");
  cp.input_string.replace(";","");
  cp.input_string.replace(",","");
  cp.input_string.replace(" ","");
  cp.input_string.replace("=","");
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

    // add it to the cp.input_string:
    cp.input_string += inChar;

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      finish_request();
      break;  // enable reading multiple inputs
    }
  }
  // if ( cp.token ) Serial.printf("serialEvent:  %s\n", cp.input_string.c_str());
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
    // add it to the cp.input_string:
    cp.input_string += inChar;
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
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.token)
  {
    // Categorize the requests
    char key = cp.input_string.charAt(0);
    if ( key == '-' )
      request = ASAP;
    else if ( key == '+' )
      request = QUEUE;
    else if ( key == '*' )
      request = SOON;
    else if ( key == '>' )
    {
      cp.input_string = cp.input_string.substring(1);  // Delete the leading '>'
      request = INCOMING;
    }
    else
    {
      request = NEW;
      if ( key == 'c' ) clear_queues();
    }

    // Limited echoing of Serial1 commands available
    Serial.printf ("echo: %s, %d\n", cp.input_string.c_str(), request);
    Serial1.printf("echo: %s, %d\n", cp.input_string.c_str(), request);

    // Deal with each request
    switch ( request )
    {
      case ( NEW ):  // Defaults to QUEUE
        // Serial.printf("new:%s,\n", cp.input_string.substring(0).c_str());
        chit( cp.input_string.substring(0) + ";", QUEUE);
        break;

      case ( ASAP ):
        // Serial.printf("asap:%s,\n", cp.input_string.substring(1).c_str());
        chit( cp.input_string.substring(1)+";", ASAP);
        break;

      case ( SOON ):
        // Serial.printf("soon:%s,\n", cp.input_string.substring(1).c_str());
        chit( cp.input_string.substring(1)+";", SOON);
        break;

      case ( QUEUE ):
        // Serial.printf("queue:%s,\n", cp.input_string.substring(1).c_str());
        chit( cp.input_string.substring(1)+";", QUEUE);
        break;

      case ( INCOMING ):
        // Serial.printf("IN:%s,\n", cp.input_string.c_str());
        switch ( cp.input_string.charAt(0) )
        {

          case ( 'P' ):  // P: print
            switch ( cp.input_string.charAt(1) )
            {

              case ( 'S' ):  // PS: print saved pars
                Serial.printf("\n");
                sp.pretty_print(true);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'R' ):  // R: reset
            switch ( cp.input_string.charAt(1) )
            {

              case ( 'S' ):  // RS: reset saved pars
                sp.nominal();
                sp.pretty_print(true);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'D' ):
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'A' ):  // * DA<>:  Amp sensor bias
                Serial.printf("rp.ib_bias_amp%7.3f to", rp.ib_bias_amp);
                rp.ib_bias_amp = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", rp.ib_bias_amp);
                break;

              case ( 'B' ):  // * DB<>:  No Amp sensor bias
                Serial.printf("rp.ib_bias_noa%7.3f to", rp.ib_bias_noa);
                rp.ib_bias_noa = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", rp.ib_bias_noa);
                break;

              case ( 'c' ):  // * Dc<>:  Vb bias
                Serial.printf("rp.Vb_bias_hdwe%7.3f to", rp.Vb_bias_hdwe);
                rp.Vb_bias_hdwe = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", rp.Vb_bias_hdwe);
                break;

              case ( 'E' ):  //   DE<>:  EKF execution frame multiplier
                Serial.printf("Eframe mult %d to ", cp.eframe_mult);
                cp.assign_eframe_mult(max(min(cp.input_string.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.eframe_mult);
                break;

              case ( 'i' ):  // * Di<>:  Bias all current sensors (same way as Da and Db)
                Serial.printf("rp.ib_bias_all%7.3f to", rp.ib_bias_all);
                rp.ib_bias_all = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\nreset\n", rp.ib_bias_all);
                cp.cmd_reset();
                break;

              case ( 'P' ):  //   DP<>:  PRINT multiplier
                Serial.printf("Print int %d to ", cp.print_mult);
                cp.assign_print_mult(max(min(cp.input_string.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.print_mult);
                break;

              case ( 'Q' ):  // * DQ<>:  delta_q
                Serial.printf("sp.delta_q%7.3f to", sp.delta_q);
                sp.put_delta_q(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\nreset\n", sp.delta_q);
                break;

              case ( 't' ):  // * Dt<>:  Temp bias change hardware
                Serial.printf("rp.Tb_bias_hdwe%7.3f to", rp.Tb_bias_hdwe);
                rp.Tb_bias_hdwe = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\nreset\n", rp.Tb_bias_hdwe);
                cp.cmd_reset();
                break;

              case ( '^' ):  // * D^<>:  Temp bias change model for faults
                Serial.printf("cp.Tb_bias_model%7.3f to", cp.Tb_bias_model);
                cp.Tb_bias_model = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", cp.Tb_bias_model);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'S' ):
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'A' ):  // * SA<>:  Amp sensor scalar
                Serial.printf("rp.ib_bias_amp%7.3f to ", rp.Ib_scale_amp);
                rp.Ib_scale_amp = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", rp.Ib_scale_amp);
                break;

              case ( 'B' ):  // * SB<>:  No Amp sensor scalar
                Serial.printf("rp.Ib_scale_noa%7.3f to ", rp.Ib_scale_noa);
                rp.Ib_scale_noa = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", rp.Ib_scale_noa);
                break;
           
              case ( 'k' ):  // * Sk<>:  scale cutback gain for sim rep of BMS
                scale = cp.input_string.substring(2).toFloat();
                rp.cutback_gain_scalar = scale;
                Serial.printf("rp.cutback_gain_scalar to%7.3f\n", rp.cutback_gain_scalar);
                break;
            
              case ( 'V' ):  // * SV<>:  Vb sensor scalar
                Serial.printf("rp.Vb_scale%7.3f to", rp.Vb_scale);
                rp.Vb_scale = cp.input_string.substring(2).toFloat();
                Serial.printf("%7.3f\n", rp.Vb_scale);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'F' ):  // Fault stuff
            switch ( cp.input_string.charAt(1) )
            {

              case ( 'f' ):  //   Ff<>:  fake faults
                INT_in = cp.input_string.substring(2).toInt();
                Serial.printf("cp.fake_faults, rp.ib_select %d, %d to ", cp.fake_faults, rp.ib_select);
                cp.fake_faults = INT_in;
                rp.ib_select = INT_in;
                Serial.printf("%d, %d\n", cp.fake_faults, rp.ib_select);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'H' ):  // History
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'd' ):  // Hd: History dump
                Serial.printf("\n");
                print_all_summary(mySum, rp.isum, NSUM);
                chit("Pr;Q;", QUEUE);
                Serial.printf("\n");
                print_all_fault_buffer(myFlt, rp.iflt, NFLT);
                break;

              case ( 'f' ):  // Hf: History dump faults only
                Serial.printf("\n");
                print_all_fault_buffer(myFlt, rp.iflt, NFLT);
                break;

              case ( 'R' ):  // HR: History reset
                large_reset_summary(mySum, rp.isum, NSUM);
                large_reset_fault_buffer(myFlt, rp.iflt, NFLT);
                break;

              case ( 's' ):  // Hs: History snapshot
                cp.cmd_summarize();
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 's' ):  // s<>:  select amp or noa
            if ( cp.input_string.substring(1).toInt()>0 )
            {
              rp.ib_select = 1;
            }
            else if ( cp.input_string.substring(1).toInt()<0 )
              rp.ib_select = -1;
            else
              rp.ib_select = 0;
            Serial.printf("Sig ( -1=noa, 0=auto, 1=amp,) set %d\n", rp.ib_select);
            break;

          case ( 'v' ):  // v<>:  verbose level
            Serial.printf("sp.debug %d to ", sp.debug);
            sp.put_debug(cp.input_string.substring(1).toInt());
            Serial.printf("%d\n", sp.debug);
            break;

          case ( 'V' ):
            switch ( cp.input_string.charAt(1) )
            {

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'W' ):  // W<>:  wait.  Skip
            if ( cp.input_string.substring(1).length() )
            {
              INT_in = cp.input_string.substring(1).toInt();
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
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'd' ):  // Xd<>:  on/off dc-dc charger manual setting
                if ( cp.input_string.substring(2).toInt()>0 )
                  cp.dc_dc_on = true;
                else
                  cp.dc_dc_on = false;
                Serial.printf("dc_dc_on to %d\n", cp.dc_dc_on);
                break;

              case ( 'm' ):  // Xm<>:  code for modeling level
                INT_in =  cp.input_string.substring(2).toInt();
                if ( INT_in>=0 && INT_in<1000 )
                {
                  boolean reset = rp.modeling != INT_in;
                  Serial.printf("modeling %d to ", sp.modeling);
                  sp.put_modeling(INT_in);
                  Serial.printf("%d\n", sp.modeling);
                }
                else
                {
                  Serial.printf("err %d, modeling 0-7. 'h'\n", INT_in);
                }
                Serial.printf("Modeling %d\n", sp.modeling);
                Serial.printf("tweak_test %d\n", sp.tweak_test());
                Serial.printf("mod_ib %d\n", sp.mod_ib());
                Serial.printf("mod_vb %d\n", sp.mod_vb());
                Serial.printf("mod_tb %d\n", sp.mod_tb());
                break;

              case ( 'a' ): // Xa<>:  injection amplitude
                rp.amp = cp.input_string.substring(2).toFloat();
                Serial.printf("Inj amp set%7.3f & inj_bias set%7.3f\n", rp.amp, rp.inj_bias);
                break;

              case ( 'b' ): // Xb<>:  injection bias
                rp.inj_bias = cp.input_string.substring(2).toFloat();
                Serial.printf("Inj_bias set%7.3f\n", rp.inj_bias);
                break;

              case ( 't' ): // Xt<>:  injection type
                switch ( cp.input_string.charAt(2) )
                {
                  case ( 'n' ):  // Xtn:  none
                    rp.type = 0;
                    Serial.printf("Set none. rp.type %d\n", rp.type);
                    break;

                  case ( 's' ):  // Xts:  sine
                    rp.type = 1;
                    Serial.printf("Set sin. rp.type %d\n", rp.type);
                    break;

                  case ( 'q' ):  // Xtq:  square
                    rp.type = 2;
                    Serial.printf("Set square. rp.type %d\n", rp.type);
                    break;

                  case ( 't' ):  // Xtt:  triangle
                    rp.type = 3;
                    Serial.printf("Set tri. rp.type %d\n", rp.type);
                    break;

                  case ( 'c' ):  // Xtc:  charge rate
                    rp.type = 4;
                    Serial.printf("Set 1C charge. rp.type %d\n", rp.type);
                    break;

                  case ( 'd' ):  // Xtd:  discharge rate
                    rp.type = 5;
                    Serial.printf("Set 1C disch. rp.type %d\n", rp.type);
                    break;

                  case ( 'o' ):  // Xto:  cosine
                    rp.type = 8;
                    Serial.printf("Set cos. rp.type %d\n", rp.type);
                    break;

                  default:
                    Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
                }
                break;

              case ( 'o' ): // Xo<>:  injection dc offset
                rp.inj_bias = max(min(cp.input_string.substring(2).toFloat(), 18.3), -18.3);
                Serial.printf("inj_bias set%7.3f\n", rp.inj_bias);
                break;

              case ( 's' ): // Xs:  scale T_SAT
                FP_in = cp.input_string.substring(2).toFloat();
                Serial.printf("s_t_sat%7.1f s to\n", cp.s_t_sat);
                cp.s_t_sat = max(FP_in, 0.);
                Serial.printf("%7.1f\n", cp.s_t_sat);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'h' ):  // h: help
            talkH();
            break;

          default:
            Serial.print(cp.input_string.charAt(0)); Serial.println(" ? 'h'");
            break;
        }
    }

    cp.input_string = "";
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

  Serial.printf("\nD/S<?> Adj e.g.:\n");
  Serial.printf(" *DA= "); Serial.printf("%6.3f", rp.ib_bias_amp); Serial.printf(": delta amp, A [%6.3f]\n", CURR_BIAS_AMP); 
  Serial.printf(" *DB= "); Serial.printf("%6.3f", rp.ib_bias_noa); Serial.printf(": delta noa, A [%6.3f]\n", CURR_BIAS_NOA); 
  Serial.printf(" *Di= "); Serial.printf("%6.3f", rp.ib_bias_all); Serial.printf(": delta all, A [%6.3f]\n", CURR_BIAS_ALL); 
  Serial.printf(" *Dc= "); Serial.printf("%6.3f", rp.Vb_bias_hdwe); Serial.printf(": delta, V [%6.3f]\n", VOLT_BIAS); 
  Serial.printf("  DE= "); Serial.printf("%d", cp.eframe_mult); Serial.printf(": eframe mult Dr [20]\n");

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

  Serial.printf("\nv= "); Serial.print(rp.debug); Serial.println(": verbosity, -128 - +128. [4]");
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
