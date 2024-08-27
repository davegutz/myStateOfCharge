//
// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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

#include "serial.h"
#include "command.h"
#include "constants.h"
#include "debug.h"

extern CommandPars cp;  // Various parameters shared at system level


// vv1 serial output
void create_rapid_string(Publish *pubList, Sensors *Sen, BatteryMonitor *Mon)
{
  double cTime = double(Sen->now)/1000;
  
  sprintf(pr.buff, "%s, %s,%13.3f,%6.3f, %d,%7.0f,%d, %d, %d, %d, %6.3f,%6.3f,%9.3f,%9.3f,%8.5f,  %7.5f,%8.5f,%8.5f,%8.5f,  %9.6f, %8.5f,%8.5f,%8.5f,%5.3f,", \
    pubList->unit.c_str(), pubList->hm_string.c_str(), cTime, Sen->T,
    CHEM, Mon->q_cap_rated_scaled(), pubList->sat, sp.ib_force(), sp.modeling(), Mon->bms_off(),
    Mon->Tb(), Mon->vb(), Mon->ib(), Mon->ib_charge(), Mon->voc_soc(), 
    Mon->vsat(), Mon->dv_dyn(), Mon->voc_stat(), Mon->hx(),
    Mon->y_ekf(),
    Sen->Sim->soc(), Mon->soc_ekf(), Mon->soc(), Mon->soc_min());
}


// Non-blocking delay
void delay_no_block(const unsigned long long interval)
{
  unsigned long long previousMillis = System.millis();
  unsigned long long currentMillis = previousMillis;
  while( currentMillis - previousMillis < interval )
  {
    currentMillis = System.millis();
  }
}


// Cleanup string for final processing by chitchat
String finish_request(const String in_str)
{
  String out_str = in_str;
  // Remove whitespace
  out_str.trim();
  out_str.replace("\n","");
  out_str.replace("\0","");
  out_str.replace("","");
  out_str.replace(",","");
  out_str.replace(" ","");
  out_str.replace("=","");
  out_str.replace(";","");
  return out_str;
}


// Strip cmd string from front of source string
String chat_cmd_from(String *source)
{
  String out_str = "";

  #ifdef SOFT_DEBUG_QUEUE
    // debug_queue("chat_cmd_from enter");
  #endif

  while ( source->length() )
  {
    // get the new byte, add to input and check for completion
    char in_char = source->charAt(0);
    source->remove(0, 1);
    out_str += in_char;
    if ( is_finished(in_char) )
    {
      out_str = finish_request(out_str);  // remove whitespace and ;, etc
      break;
    }
  }

  #ifdef SOFT_DEBUG_QUEUE
    // debug_queue("chat_cmd_from exit");
  #endif

  return out_str;
}


// Test for string completion character
boolean is_finished(const char in_char)
{
    return  in_char == '\n' ||
            in_char == '\0' ||
            in_char == ';'  ||
            in_char == ',';    
}


// Print consolidation
void print_all_header(void)
{
  print_serial_header();
  if ( sp.debug()==2  )
  {
    print_serial_sim_header();
    print_signal_sel_header();
  }
  if ( sp.debug()==3  )
  {
    print_serial_sim_header();
    print_serial_ekf_header();
  }
  if ( sp.debug()==4  )
  {
    print_serial_sim_header();
    print_signal_sel_header();
    print_serial_ekf_header();
  }
}

void print_rapid_data(const boolean reset, Sensors *Sen, BatteryMonitor *Mon)
{
  static uint8_t last_read_debug = 0;     // Remember first time with new debug to print headers
  if ( ( sp.debug()==1 || sp.debug()==2 || sp.debug()==3 || sp.debug()==4 ) )
  {
    if ( reset || (last_read_debug != sp.debug()) )
    {
      cp.num_v_print = 0UL;
      print_all_header();
    }
    if ( sp.tweak_test() )
    {
      // no print, done by sub-functions
      cp.num_v_print++;
    }
    if ( cp.publishS )
    {
      rapid_print(Sen, Mon);
      cp.num_v_print++;
    }
  }
  last_read_debug = sp.debug();
}

void print_serial_header(void)
{
  if ( ( sp.debug()==1 || sp.debug()==2 || sp.debug()==3 || sp.debug()==4 ) )
  {
    Serial.printf ("unit,               hm,                  cTime,       dt,       chm,qcrs,sat,sel,mod,bmso, Tb,  vb,  ib,   ib_charge, voc_soc,    vsat,dv_dyn,voc_stat,voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,soc_min,\n");
    #ifdef HDWE_ARGON
      Serial1.printf("unit,               hm,                  cTime,       dt,       chm,qcrs,sat,sel,mod,bmso, Tb,  vb,  ib,   ib_charge, voc_soc,    vsat,dv_dyn,voc_stat,voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,soc_min,\n");
    #endif
  }
}

void print_serial_sim_header(void)
{
  if ( sp.debug()==2  || sp.debug()==3 || sp.debug()==4 ) // print_serial_sim_header
    Serial.printf("unit_m,  c_time,       chm_s, qcrs_s, bmso_s, Tb_s,Tbl_s,  vsat_s, voc_stat_s, dv_dyn_s, vb_s, ib_s, ib_in_s, ib_charge_s, ioc_s, sat_s, dq_s, soc_s, reset_s,\n");
}

void print_signal_sel_header(void)
{
  if ( sp.debug()==2 || sp.debug()==4 ) // print_signal_sel_header
    Serial.printf("unit_s,c_time,res,user_sel,   cc_dif,  ibmh,ibnh,ibmm,ibnm,ibm,   ib_diff, ib_diff_f,");
    Serial.printf("    voc_soc,e_w,e_w_f,e_wm,e_wm_f,e_wn,e_wn_f,  ib_sel_stat,vc_h,ib_h,ib_s,mib,ib, vb_sel,vb_h,vb_s,mvb,vb,  Tb_h,Tb_s,mtb,Tb_f, ");
    Serial.printf("  fltw, falw, ib_rate, ib_quiet, tb_sel, ccd_thr, ewh_thr, ewl_thr, ibd_thr, ibq_thr, preserving,ff,y_ekf_f,ib_dec,\n");
}

void print_serial_ekf_header(void)
{
  if ( sp.debug()==3 || sp.debug()==4 ) // print_serial_ekf_header
    Serial.printf("unit_e,c_time,dt,Fx_, Bu_, Q_, R_, P_, S_, K_, u_, x_, y_, z_, x_prior_, P_prior_, x_post_, P_post_, hx_, H_,\n");
}


// Inputs serial print
void rapid_print(Sensors *Sen, BatteryMonitor *Mon)
{
  create_rapid_string(&pp.pubList, Sen, Mon);
  Serial.printf("%s\n", pr.buff);
  #ifdef HDWE_ARGON
    Serial1.printf("%s\n", pr.buff);
  #endif
}


/*
  Special handler for UART usb that uses built-in callback. SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.

  Particle documentation says not to use something like the cp.inp_token in the while loop statement.
  They suggest handling all the data in one call.   But this works, so far.

  serialEvent handles Serial.  serialEvent1 handles Serial1.
 */
void serialEvent()
{
    static String serial_str = "";
    static boolean serial_ready = false;

    // Each pass try to complete input from avaiable
    while ( !serial_ready && Serial.available() )
    {
        char in_char = (char)Serial.read();  // get the new byte

        // Intake
        // if the incoming character to finish, add a ';' and set flags so the main loop can do something about it:
        if ( is_finished(in_char) )
        {
            serial_str += ';';
            serial_ready = true;
            break;
        }

        else if ( in_char == '\r' )
            Serial.printf("\n");  // scroll user terminal

        else if ( in_char == '\b' && serial_str.length() )
        {
            Serial.printf("\b \b");  // scroll user terminal
            serial_str.remove(serial_str.length() -1 );  // backspace
        }

        else
            serial_str += in_char;  // process new valid character

    }

    // Pass info to inp_str
    if ( serial_ready )
    {
        if ( !cp.inp_token )
        {
            cp.inp_token = true;
            add_verify(&cp.inp_str, serial_str);
            serial_ready = false;
            cp.inp_token = false;
            serial_str = "";
        }
    }

}


void serialEvent1()
{
    static String serial_str1 = "";
    static boolean serial_ready1 = false;

    // Each pass try to complete input from avaiable
    while ( !serial_ready1 && Serial1.available() )
    {
        char in_char1 = (char)Serial1.read();  // get the new byte

        // Intake
        // if the incoming character to finish, add a ';' and set flags so the main loop can do something about it:
        if ( is_finished(in_char1) )
        {
            serial_str1 += ';';
            serial_ready1 = true;
            break;
        }

        else if ( in_char1 == '\r' )
            Serial1.printf("\n");  // scroll user terminal

        else if ( in_char1 == '\b' && serial_str1.length() )
        {
            Serial1.printf("\b \b");  // scroll user terminal
            serial_str1.remove(serial_str1.length() -1 );  // backspace
        }

        else
            serial_str1 += in_char1;  // process new valid character

    }

    // Pass info to inp_str
    if ( serial_ready1 )
    {
        if ( !cp.inp_token )
        {
            cp.inp_token = true;
            cp.inp_str += serial_str1;
            serial_ready1 = false;
            cp.inp_token = false;
            serial_str1 = "";
        }
    }

}


// Wait on user input to reset EERAM values
void wait_on_user_input(Adafruit_SSD1306 *display)
{
  display->clearDisplay();
  display->setTextSize(1);              // Normal 1:1 pixel scale
  display->setTextColor(SSD1306_WHITE); // Draw white text
  display->setCursor(0,0);              // Start at top-left corner    sp.print_versus_local_config();
  display->println("Waiting for USB/BT talk\n\nignores after 120s");
  display->display();
  uint8_t count = 0;
  uint16_t answer = '\r';
  // Get user input but timeout at 120 seconds if no response
  while ( count<30 && answer!='Y' && answer!='n' && answer!='N' )
  {
    if ( answer=='\r')
    {
      count++;
      if ( count>1 ) delay(4000);
    }
    else delay(100);

    if ( Serial.available() )
      answer=Serial.read();
    else if ( Serial1.available() )
      answer=Serial1.read();
    else
      Serial.printf("unavail\n");

    if ( answer=='\r')
    {
      Serial.printf("\n\n");
      sp.pretty_print( false );
      Serial.printf("Reset to defaults? [Y/n]:"); Serial1.printf("Reset to defaults? [Y/n]:");
    }
    else  // User is typing.  Ignore him until they answer 'Y', 'N', or 'n'.  But timeout seconds if they don't
    {
      while ( answer!='Y' && answer!='N' && answer!='n' && count<30 )
      {
        if ( Serial.available() )
          answer=Serial.read();

        else if ( Serial1.available() )
          answer=Serial1.read();

        else
        {
          Serial.printf("?");
          count++;
          delay(1000);
        }
      }
    }

  }

  // Wrap it up
  if ( answer=='Y' )
  {
    Serial.printf("  Y\n\n"); Serial1.printf("  Y\n\n");
    sp.set_nominal();
    sp.pretty_print( true );
    #ifdef HDWE_PHOTON2
      System.backupRamSync();
    #endif
  }
  else if ( answer=='n' || answer=='N' || count==30 )
  {
    Serial.printf(" N.  moving on...\n\n"); Serial1.printf(" N.  moving on...\n\n");
  }

}


void wait_on_user_input()
{
  uint8_t count = 0;
  uint16_t answer = '\r';
  // Get user input but timeout at 120 seconds if no response
  while ( count<30 && answer!='Y' && answer!='n' && answer!='N' )
  {
    if ( answer=='\r')
    {
      count++;
      if ( count>1 ) delay(4000);
    }
    else delay(100);

    if ( Serial.available() )
      answer=Serial.read();
    else if ( Serial1.available() )
      answer=Serial1.read();
    else
      Serial.printf("unavail\n");

    if ( answer=='\r')
    {
      Serial.printf("\n\n");
      sp.pretty_print( false );
      Serial.printf("Reset to defaults? [Y/n]:"); Serial1.printf("Reset to defaults? [Y/n]:");
    }
    else  // User is typing.  Ignore him until they answer 'Y', 'N', or 'n'.  But timeout at 30 seconds if they don't
    {
      while ( answer!='Y' && answer!='N' && answer!='n' && count<30 )
      {
        if ( Serial.available() )
          answer=Serial.read();

        else if ( Serial1.available() )
          answer=Serial1.read();

        else
        {
          Serial.printf("?");
          count++;
          delay(1000);
        }
      }
    }

  }

  // Wrap it up
  if ( answer=='Y' )
  {
    Serial.printf("  Y\n\n"); Serial1.printf("  Y\n\n");
    sp.set_nominal();
    sp.pretty_print( true );
    #ifdef HDWE_PHOTON2
      System.backupRamSync();
    #endif
  }
  else if ( answer=='n' || answer=='N' || count==30 )
  {
    Serial.printf(" N.  moving on...\n\n"); Serial1.printf(" N.  moving on...\n\n");
  }

}
