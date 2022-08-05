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
#include "Battery.h"
#include "local_config.h"
#include "mySummary.h"
#include <math.h>
#include "debug.h"

extern CommandPars cp;          // Various parameters shared at system level
extern RetainedPars rp;         // Various parameters to be static at system level
extern Sum_st mySum[NSUM];      // Summaries for saving charge history

// Collisions with Serial.read unlikely because Serial.read kicks off commands to chit and chat.
// Prioritize urgency SOON over QUEUE.   The chit() call specifies urgency.  QUEUE is default
void asap()
{
    if ( cp.queue_str.length() ) Serial.printf("cmd from '%s'\n", cp.queue_str.c_str());
  while ( !cp.string_complete && cp.asap_str.length() )
  {
    // get the new byte:
    char inChar = cp.asap_str.charAt(0);
    cp.asap_str.remove(0, 1);
    // add it to the cp.input_string:
    cp.input_string += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
    {
      // Remove whitespace
      cp.input_string.trim();
      cp.input_string.replace("\0","");
      cp.input_string.replace(";","");
      cp.input_string.replace(",","");
      cp.input_string.replace(" ","");
      cp.input_string.replace("=","");
      cp.string_complete = true;  // token:  temporarily inhibits while loop until talk() call resets string_complete
      Serial.printf("chat (ASAP):  talk('%s;')\n", cp.input_string.c_str());
      break;  // enable reading multiple inputs
    }
  }
}



// Process chat strings and feed to talk using input_string and string_complete
// Collisions with Serial.read unlikely because Serial.read kicks off commands to chit and chat.
// Prioritize urgency SOON over QUEUE.   The chit() call specifies urgency.  QUEUE is default
void chat()
{
  if ( cp.soon_str.length() )  // Do SOON first
  {
    if ( cp.queue_str.length() ) Serial.printf("cmd from '%s'\n", cp.queue_str.c_str());
    while ( !cp.string_complete && cp.soon_str.length() )
    {
      // get the new byte:
      char inChar = cp.soon_str.charAt(0);
      cp.soon_str.remove(0, 1);
      // add it to the cp.input_string:
      cp.input_string += inChar;
      // if the incoming character is a newline, set a flag
      // so the main loop can do something about it:
      if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
      {
        // Remove whitespace
        cp.input_string.trim();
        cp.input_string.replace("\0","");
        cp.input_string.replace(";","");
        cp.input_string.replace(",","");
        cp.input_string.replace(" ","");
        cp.input_string.replace("=","");
        cp.string_complete = true;  // token:  temporarily inhibits while loop until talk() call resets string_complete
        Serial.printf("chat (SOON):  talk('%s;')\n", cp.input_string.c_str());
        break;  // enable reading multiple inputs
      }
    }
    return;
  }   // end soon
  else  // Do QUEUE only after SOON empty
  {
    if ( cp.queue_str.length() ) Serial.printf("cmd from '%s'\n", cp.queue_str.c_str());
    while ( !cp.string_complete && cp.queue_str.length() )
    {
      // get the new byte:
      char inChar = cp.queue_str.charAt(0);
      cp.queue_str.remove(0, 1);
      // add it to the cp.input_string:
      cp.input_string += inChar;
      // if the incoming character is a newline, set a flag
      // so the main loop can do something about it:
      if (inChar=='\n' || inChar=='\0' || inChar==';' || inChar==',') // enable reading multiple inputs
      {
        // Remove whitespace
        cp.input_string.trim();
        cp.input_string.replace("\0","");
        cp.input_string.replace(";","");
        cp.input_string.replace(",","");
        cp.input_string.replace(" ","");
        cp.input_string.replace("=","");
        cp.string_complete = true;  // token:  temporarily inhibits while loop until talk() call resets string_complete
        Serial.printf("QUEUE:  talk('%s;')\n", cp.input_string.c_str());
        break;  // enable reading multiple inputs
      }
    }
  } // end queue
}

// Call talk from within, a crude macro feature.   cmd should by semi-colon delimited commands for talk()
void chit(const String cmd, const urgency when)
{
  Serial.printf("chit cmd=%s,\n", cmd.c_str());
  if ( when == QUEUE )
    cp.queue_str += cmd;
  else if ( when == SOON )
    cp.soon_str += cmd;
  else
    cp.asap_str += cmd;
}

// Talk Executive
void talk(BatteryMonitor *Mon, Sensors *Sen)
{
  const String set_nom_coul_eff = "Dn" + String(COULOMBIC_EFF);
  double FP_in = -99.;
  int INT_in = -1;
  double scale = 1.;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.string_complete)
  {
    // Limited echoing of Serial1 commands available
    if ( !cp.blynking )
    {
      Serial1.printf("echo:  %s\n", cp.input_string.c_str());
    }
    Serial.printf("echo:%s,\n", cp.input_string.c_str());
    switch ( cp.input_string.charAt(0) )
    {
      case ( '-' ):  // ASAP
        Serial.printf("asap:%s,\n", cp.input_string.substring(1).c_str());
        chit( cp.input_string.substring(1)+";", ASAP);
        break;

      case ( '*' ): // SOON
        Serial.printf("soon:%s,\n", cp.input_string.substring(1).c_str());
        chit( cp.input_string.substring(1)+";", SOON);
        break;

      case ( '+' ): // QUEUE
        Serial.printf("queue:%s,\n", cp.input_string.substring(1).c_str());
        chit( cp.input_string.substring(1)+";", QUEUE);
        break;

      case ( 'B' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'm' ):  // B:  Monitor chemistry change
            INT_in = cp.input_string.substring(2).toInt();
            switch ( INT_in )
            {
              case ( 0 ):  // Bm0:
                Serial.printf("Mon chem from %d", Mon->mod_code());
                Mon->assign_mod("Battleborn");
                Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_rand();
                cp.cmd_reset();
                break;

              case ( 1 ):  // Bm1:
                Serial.printf("Mon chem from %d", Mon->mod_code());
                Mon->assign_mod("LION");
                Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_rand();
                cp.cmd_reset();
                break;

              default:
                Serial.printf("%d unk.  See 'h'", INT_in);
            }
            break;

          case ( 's' ):  // B:  Simulation chemistry change
            INT_in = cp.input_string.substring(2).toInt();
            switch ( INT_in )
            {
              case ( 0 ):  // Bs0:
                Serial.printf("Sim chem from %d", Sen->Sim->mod_code());
                Sen->Sim->assign_mod("Battleborn"); Sen->Sim->assign_rand();
                Serial.printf(" to %d ('Battleborn')\n", Sen->Sim->mod_code());
                cp.cmd_reset();
                break;

              case ( 1 ):  // Bs1:
                Serial.printf("Sim chem from %d", Sen->Sim->mod_code());
                Sen->Sim->assign_mod("LION"); Sen->Sim->assign_rand();
                Serial.printf(" to %d ('LION')\n", Sen->Sim->mod_code());
                cp.cmd_reset();
                break;

              default:
                Serial.printf("%d unk.  See 'h'", INT_in);
            }
            break;

          case ( 'P' ):  // BP<>:  Number of parallel batteries in bank, e.g. '2P1S'
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in>0 )  // Apply crude limit to prevent user error
            {
              Serial.printf("Mon/Sim->nP from %5.2f / %5.2f ", Mon->nP(), Sen->Sim->nP());
              rp.nP = FP_in;
              Serial.printf("to %5.2f / %5.2f\n", Mon->nP(), Sen->Sim->nP());
            }
            else
              Serial.printf("nP = %5.2f; must be >0.\n", FP_in);
            break;

          case ( 'S' ):  // BS<>:  Number of series batteries in bank, e.g. '2P1S'
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in>0 )  // Apply crude limit to prevent user error
            {
              Serial.printf("Mon/Sim->nS from %5.2f / %5.2f ", Mon->nS(), Sen->Sim->nS());
              rp.nS = FP_in;
              Serial.printf("to %5.2f / %5.2f\n", Mon->nS(), Sen->Sim->nS());
            }
            else
              Serial.printf("nP = %5.2f; must be >0.\n", FP_in);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'C' ):  // C:
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):  // Ca<>:  assign charge state in fraction to all versions including model
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in<1.1 )  // Apply crude limit to prevent user error
            {
              Mon->apply_soc(FP_in, Sen->Tbatt_filt);
              Sen->Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
              Serial.printf("soc=%7.3f, modeling = %d, delta_q=%7.3f, soc_model=%8.4f,   delta_q_model=%7.3f, soc_ekf=%8.4f, delta_q_ekf=%7.3f,\n",
                  Mon->soc(), rp.modeling, Mon->delta_q(), Sen->Sim->soc(), Sen->Sim->delta_q(), Mon->soc_ekf(), Mon->delta_q_ekf());
              cp.cmd_reset();
              chit("W3;", SOON);  // Wait 3 passes of Control
            }
            else
              Serial.printf("soc = %8.4f; must be 0-1.1\n", FP_in);
            break;

          case ( 'm' ):  // Cm<>:  assign curve charge state in fraction to model only (ekf if modeling)
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in<1.1 )   // Apply crude limit to prevent user error
            {
              Sen->Sim->apply_soc(FP_in, Sen->Tbatt_filt);
              Serial.printf("soc=%8.4f,   delta_q=%7.3f, soc_model=%8.4f,   delta_q_model=%7.3f,\n",
                  Mon->soc(), Mon->delta_q(), Sen->Sim->soc(), Sen->Sim->delta_q());
              cp.cmd_reset();
            }
            else
              Serial.printf("soc = %8.4f; must be 0-1.1\n", FP_in);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'D' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):  // Da<>:  Amp sensor bias
            Serial.printf("rp.ibatt_bias_amp from %7.3f to ", rp.ibatt_bias_amp);
            rp.ibatt_bias_amp = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.ibatt_bias_amp);
            break;

          case ( 'b' ):  // Db<>:  No Amp sensor bias
            Serial.printf("rp.ibatt_bias_noamp from %7.3f to ", rp.ibatt_bias_noamp);
            rp.ibatt_bias_noamp = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.ibatt_bias_noamp);
            break;

          case ( 'c' ):  // Dc<>:  Vbatt bias
            Serial.printf("rp.vbatt_bias from %7.3f to ", rp.vbatt_bias);
            rp.vbatt_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.vbatt_bias);
            break;

          case ( 'i' ):  // Di<>:  Bias all current sensors (same way as Da and Db)
            Serial.printf("rp.ibatt_bias_all from %7.3f to ", rp.ibatt_bias_all);
            rp.ibatt_bias_all = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.ibatt_bias_all);
            break;

          case ( 'm' ):  // Dm<>:  Amp signal adder for faults
            Serial.printf("ShuntAmp.add from %7.3f to ", Sen->ShuntAmp->add());
            Sen->ShuntAmp->add(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->add());
            break;

          case ( 'n' ):  // Dn<>:  No Amp signal adder for faults
            Serial.printf("ShuntNoAmp.add from %7.3f to ", Sen->ShuntNoAmp->add());
            Sen->ShuntNoAmp->add(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->add());
            break;

          case ( 'p' ):  // Dp<>:  PRINT interval input
            if ( !cp.blynking )
            {
              Serial.printf("PublishSerial from %ld to ", Sen->PublishSerial->delay());
              Sen->PublishSerial->delay(cp.input_string.substring(2).toInt());
            }
            Sen->PublishSerial->delay(cp.input_string.substring(2).toInt());
            Serial.printf("%ld\n", Sen->PublishSerial->delay());
            break;

          case ( 'r' ):  // Dr<>:  READ sample time input
            Serial.printf("ReadSensors from %ld to ", Sen->ReadSensors->delay());
            Sen->ReadSensors->delay(cp.input_string.substring(2).toInt());
            Serial.printf("%ld\n", Sen->ReadSensors->delay());
            break;

          case ( 't' ):  // Dt<>:  Temp bias change
            Serial.printf("rp.tbatt_bias from %7.3f to ", rp.tbatt_bias);
            rp.tbatt_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.tbatt_bias);
            rp.debug = 0;
            Serial.printf("***** reset ****\n");
            break;

          case ( 'u' ):  // Du<>:  Coulombic Efficiency change
            FP_in = cp.input_string.substring(2).toFloat();
            Serial.printf("coulombic eff from %7.4f,%7.4f,%7.4f,%7.4f, to ", Sen->Sim->coul_eff(), Mon->coul_eff(), Sen->ShuntAmp->coul_eff(), Sen->ShuntNoAmp->coul_eff());
            Sen->Sim->coul_eff(FP_in);
            Mon->coul_eff(FP_in);
            Sen->ShuntAmp->coul_eff(FP_in);
            Sen->ShuntNoAmp->coul_eff(FP_in);
            Serial.printf("%7.4f,%7.4f,%7.4f,%7.4f\n", Sen->Sim->coul_eff(), Mon->coul_eff(), Sen->ShuntAmp->coul_eff(), Sen->ShuntNoAmp->coul_eff());
            break;

          case ( 'v' ):  // Dv<>:
            Serial.printf("Mon.Dv from %7.3f to ", Mon->Dv());
            Mon->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Mon->Dv());
            Serial.printf("Sim.Dv from %7.3f to ", Sen->Sim->Dv());
            Sen->Sim->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->Sim->Dv());
            break;

          case ( 'T' ):  // DT<>:  Tbatt noise
            Serial.printf("Sen.Tbatt_noise_amp_ from %7.3f to ", Sen->Tbatt_noise_amp());
            Sen->Tbatt_noise_amp(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->Tbatt_noise_amp());
            break;

          case ( 'V' ):  // DV<>:  Vbatt noise
            Serial.printf("Sen.Vbatt_noise_amp_ from %7.3f to ", Sen->Vbatt_noise_amp());
            Sen->Vbatt_noise_amp(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->Vbatt_noise_amp());
            break;

          case ( 'I' ):  // DI<>:  Ibatt noise
            Serial.printf("Sen.Ibatt_noise_amp_ from %7.3f to ", Sen->Ibatt_noise_amp());
            Sen->Ibatt_noise_amp(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->Ibatt_noise_amp());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'S' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'c' ):  // Sc<>: scale capacity
            scale = cp.input_string.substring(2).toFloat();
            rp.s_cap_model = scale;
        
            Serial.printf("Sim.q_cap_rated scaled by %7.3f from %7.3f to ", scale, Sen->Sim->q_cap_scaled());
        
            Sen->Sim->apply_cap_scale(rp.s_cap_model);
            if ( rp.modeling ) Mon->init_soc_ekf(Sen->Sim->soc());
        
            Serial.printf("%7.3f\n", Sen->Sim->q_cap_scaled());
            Serial.printf("Sim:  "); Sen->Sim->pretty_print(); Sen->Sim->Coulombs::pretty_print();
            break;
        
          case ( 'h' ):  // Sh<>: scale hysteresis
            scale = cp.input_string.substring(2).toFloat();

            Serial.printf("\nBefore Hys::scale = %7.3f & %7.3f\n", Mon->hys_scale(), Sen->Sim->hys_scale());
            rp.hys_scale = scale;
            Serial.printf("scale= %7.3f\n", scale);
            Mon->hys_scale(scale);
            Sen->Sim->hys_scale(scale);
            Serial.printf("After = %7.3f & %7.3f\n",
                Mon->hys_scale(), Sen->Sim->hys_scale());
            break;

          case ( 'm' ):  // Sm<>:  Amp signal scalar for faults
            Serial.printf("ShuntAmp.slr from %7.3f to ", Sen->ShuntAmp->slr());
            Sen->ShuntAmp->slr(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->slr());
            break;

          case ( 'n' ):  // Sn<>:  No Amp signal scalar for faults
            Serial.printf("ShuntNoAmp.slr from %7.3f to ", Sen->ShuntNoAmp->slr());
            Sen->ShuntNoAmp->slr(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->slr());
            break;
        
          case ( 'r' ):  // Sr<>:
            scale = cp.input_string.substring(2).toFloat();
        
            Serial.printf("\nBefore Sim::StateSpace:\n"); Sen->Sim->pretty_print_ss();
            Serial.printf("\nScaling D[0, 0] = -r0 by Sr= %7.3f\n", scale);
        
            Sen->Sim->Sr(scale);
        
            Serial.printf("\nAfter Sim::StateSpace:\n"); Mon->pretty_print_ss();
            Serial.printf("\nBefore Mon::StateSpace:\n"); Mon->pretty_print_ss();
            Serial.printf("\nScaling D[0, 0] = -r0 by Sr= %7.3f\n", scale);
        
            Mon->Sr(scale);
        
            Serial.printf("\nAfter Mon::StateSpace:\n"); Mon->pretty_print_ss();
            break;
        
          case ( 'k' ):  // Sk<>:
            scale = cp.input_string.substring(2).toFloat();
            rp.cutback_gain_scalar = scale;
            Serial.printf("rp.cutback_gain_scalar set to %7.3f\n", rp.cutback_gain_scalar);
            break;
        
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;
      case ( 'H' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'd' ):  // Hd:
            print_all_summary(mySum, rp.isum, NSUM);
            break;

          case ( 'R' ):  // HR:
            large_reset_summary(mySum, rp.isum, NSUM);
            break;

          case ( 's' ):  // Hs:
            chit("h;", QUEUE);
            cp.cmd_summarize();
            chit("Pm;", QUEUE);
            Serial.printf("\n");
            if ( rp.modeling )
            {
              Serial.printf("Sim:   rp.modeling = %d\n", rp.modeling);
              chit("Ps;", QUEUE);
              Serial.printf("\n");
            }
            chit("Pr;", QUEUE);
            Serial.printf("\n");
            print_all_summary(mySum, rp.isum, NSUM);
            chit("Q;", QUEUE);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'i' ): // i<>:  
        Serial.printf("Amp cinf CC reset from %9.1f ", Sen->ShuntAmp->delta_q_cinf());
        Sen->ShuntAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
        Serial.printf("to %9.1f\n", Sen->ShuntAmp->delta_q_cinf());
        Serial.printf("Amp dinf CC reset from %9.1f ", Sen->ShuntAmp->delta_q_dinf());
        Sen->ShuntAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
        Serial.printf("to %9.1f\n", Sen->ShuntAmp->delta_q_dinf());
        Serial.printf("No amp cinf CC reset from %9.1f ", Sen->ShuntNoAmp->delta_q_cinf());
        Sen->ShuntNoAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
        Serial.printf("to %9.1f\n", Sen->ShuntNoAmp->delta_q_cinf());
        Serial.printf("No amp dinf CC reset from %9.1f ", Sen->ShuntNoAmp->delta_q_dinf());
        Sen->ShuntNoAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
        Serial.printf("to %9.1f\n", Sen->ShuntNoAmp->delta_q_dinf());
        break;

      case ( 'l' ):
        switch ( rp.debug )
        {
          case ( -1 ):  // l-1:
            Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt_amp  ,Ishunt_noamp  ,Vbat_fo*10-110  ,voc_s*10-110  ,dv_dyn_s*10  ,v_s*10-110  , voc_dyn*10-110,,,,,,,,,,,\n");
            break;
          case ( 14 ):  // l14:
            print_serial_sim_header();
          case ( 4 ):  // l4:
            print_serial_header();
            break;
          default:
            print_serial_header();
        }
        break;

      case ( 'M' ):
        switch ( cp.input_string.charAt(1) )
        {

          case ( 'C' ):  // MC<>:
            Serial.printf("Amp->max_change_ %10.6f to ", Sen->ShuntAmp->max_change());
            Sen->ShuntAmp->max_change(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.6f\n", Sen->ShuntAmp->max_change());
            break;

          case ( 'k' ):  // Mk<>:
            Serial.printf("rp.tweak_sclr %7.3f to ", Sen->ShuntAmp->tweak_sclr());
            Sen->ShuntAmp->tweak_sclr(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->tweak_sclr());
            break;

          case ( 'w' ):  // Mw<>:
            Serial.printf("Amp->time_to_wait from %7.3f to ", Sen->ShuntAmp->time_to_wait());
            Sen->ShuntAmp->time_to_wait(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->time_to_wait());
            break;

          case ( 'x' ):  // Mx<>:
            Serial.printf("Amp->max_tweak from %7.3f to ", Sen->ShuntAmp->max_tweak());
            Sen->ShuntAmp->max_tweak(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->max_tweak());
            break;

          case ( 'z' ):  // Mz<>:
            Serial.printf("Amp->time_sat_past from %7.3f to ", Sen->ShuntAmp->time_sat_past());
            Sen->ShuntAmp->time_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->time_sat_past());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'N' ):
        switch ( cp.input_string.charAt(1) )
        {

          case ( 'C' ):  // NC<>:
            Serial.printf("NoAmp->max_change_ %10.6f to ", Sen->ShuntNoAmp->max_change());
            Sen->ShuntNoAmp->max_change(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.6f\n", Sen->ShuntNoAmp->max_change());
            break;

          case ( 'k' ):  // Nk<>:
            Serial.printf("rp.tweak_sclr from %7.3f to ", Sen->ShuntNoAmp->tweak_sclr());
            Sen->ShuntNoAmp->tweak_sclr(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->tweak_sclr());
            break;

          case ( 'w' ):  // Nw<>:
            Serial.printf("NoAmp->time_to_wait %7.3f to ", Sen->ShuntNoAmp->time_to_wait());
            Sen->ShuntNoAmp->time_to_wait(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->time_to_wait());
            break;

          case ( 'x' ):  // Nx<>:
            Serial.printf("NoAmp->max_tweak %7.3f to ", Sen->ShuntNoAmp->max_tweak());
            Sen->ShuntNoAmp->max_tweak(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->max_tweak());
            break;

          case ( 'z' ):  // Nz<>:
            Serial.printf("NoAmp->time_sat_past %7.3f to ", Sen->ShuntNoAmp->time_sat_past());
            Sen->ShuntNoAmp->time_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->time_sat_past());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'P' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):  // Pa:  Print all
            chit("Pm;", QUEUE);
            chit("Ps;", QUEUE);
            chit("Pr;", QUEUE);
            chit("Pt;", QUEUE);
            chit("PM;", QUEUE);
            chit("PN;", QUEUE);
            break;

          case ( 'c' ):  // Pc:  Print coulombs
            Serial.printf("\nMon::"); Mon->Coulombs::pretty_print();
            Serial.printf("\nSim::"); Sen->Sim->Coulombs::pretty_print();
            break;

          case ( 'e' ):  // Pe:  Print EKF
             Serial.printf("\nMon::"); Mon->EKF_1x1::pretty_print();
            break;

          case ( 'm' ):  // Pm:  Print mon
            Serial.printf("\nMon:"); Mon->pretty_print();
            Serial.printf("Mon::"); Mon->Coulombs::pretty_print();
            Serial.printf("Mon::"); Mon->pretty_print_ss();
            Serial.printf("Mon::"); Mon->EKF_1x1::pretty_print();
            Serial.printf("\nSim:   rp.modeling = %d\n", rp.modeling);
            break;

          case ( 'M' ):  // PM:  Print shunt Amp
             Serial.printf("\nTweak::"); Sen->ShuntAmp->pretty_print();
            break;

          case ( 'N' ):  // PN:  Print shunt no amp
             Serial.printf("\nTweak::"); Sen->ShuntNoAmp->pretty_print();
            break;

          case ( 'r' ):  // Pr:  Print retained
            Serial.printf("\n"); rp.pretty_print();
            Serial.printf("\n"); cp.pretty_print();
            break;

          case ( 's' ):  // Ps:  Print sim
            Serial.printf("\nSim:   rp.modeling = %d\n", rp.modeling);
            Serial.printf("Sim:");  Sen->Sim->pretty_print();
            Serial.printf("Sim::"); Sen->Sim->Coulombs::pretty_print();
            Serial.printf("Sim::"); Sen->Sim->pretty_print_ss();
            break;

          case ( 't' ):  // Pt:  Print mon and sim
            Serial.printf("\nMon::"); Mon->pretty_print_ss();
            Serial.printf("\nSim::"); Sen->Sim->pretty_print_ss();
            break;

          case ( 'x' ):  // Px:  Print shunt measure
            Serial.printf("\nAmp:   "); Serial.printf("Vshunt_int, Vshunt, cp.ibatt_tot_bias, Ishunt_cal=, %d, %7.3f, %7.3f, %7.3f,\n", 
              Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), cp.ibatt_tot_bias_amp, Sen->ShuntAmp->ishunt_cal());
            Serial.printf("No Amp:"); Serial.printf("Vshunt_int, Vshunt, cp.ibatt_tot_bias, Ishunt_cal=, %d, %7.3f, %7.3f, %7.3f,\n", 
              Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), cp.ibatt_tot_bias_noamp, Sen->ShuntNoAmp->ishunt_cal());
            Serial.printf("Selected:  NoAmp,Ibatt=,  %d, %7.3f\n", rp.ibatt_select, Sen->Ibatt);
            break;

          case ( 'v' ):  // Pv:  Print Vbatt measure
            Serial.printf("\nVolt:   ");   Serial.printf("rp.vbatt_bias, Vbatt_model, rp.modeling, Vbatt=, %7.3f, %7.3f, %d, %7.3f,\n", 
              rp.vbatt_bias, Sen->Vbatt_model, rp.modeling, Sen->Vbatt);
            break;

          default:
            Serial.println("");Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'Q' ):  // Q:  quick critical
        debug_q(Mon, Sen);
        break;

      case ( 'R' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'b' ):  // Rb:  Reset battery states
            Sen->Sim->init_battery(true, Sen);  // Reset sim battery state
            Mon->init_battery(true, Sen);       // Reset mon battery state
            break;

          case ( 'e' ):  // Re:  equalize
            Serial.printf("Equalizing counters\n");
            Sen->Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
            break;

          case ( 'f' ):  // Rf:  Reset fault latches
            Serial.printf("Resetting fault latches\n");
            chit("Dm0;s1;W10;s0;W50;", SOON);
            break;

          case ( 'h' ):  // Rh:  hys
            Serial.printf("Resetting monitor hys\n");
            Mon->init_hys(0.0);
            Serial.printf("Resetting model hys\n");
            Sen->Sim->init_hys(0.0);
            break;

          case ( 'i' ):  // Ri:
            Serial.printf("Amp cinf CC reset from %9.1f ", Sen->ShuntAmp->delta_q_cinf());
            Sen->ShuntAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
            Serial.printf("to %9.1f\n", Sen->ShuntAmp->delta_q_cinf());
            Serial.printf("Amp dinf CC reset from %9.1f ", Sen->ShuntAmp->delta_q_dinf());
            Sen->ShuntAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
            Serial.printf("to %9.1f\n", Sen->ShuntAmp->delta_q_dinf());
            Serial.printf("No amp cinf CC reset from %9.1f ", Sen->ShuntNoAmp->delta_q_cinf());
            Sen->ShuntNoAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
            Serial.printf("to %9.1f\n", Sen->ShuntNoAmp->delta_q_cinf());
            Serial.printf("No amp dinf CC reset from %9.1f ", Sen->ShuntNoAmp->delta_q_dinf());
            Sen->ShuntNoAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
            Serial.printf("to %9.1f\n", Sen->ShuntNoAmp->delta_q_dinf());
            break;

          case ( 'r' ):  // Rr:  small reset counters
            Serial.printf("Small reset all counters\n");
            Sen->Sim->apply_soc(1.0, Sen->Tbatt_filt);
            Mon->apply_soc(1.0, Sen->Tbatt_filt);
            cp.cmd_reset();
            break;

          case ( 'R' ):  // RR:  large reset
            Serial.printf("Large reset\n");
            Sen->Sim->apply_soc(1.0, Sen->Tbatt_filt);
            Mon->apply_soc(1.0, Sen->Tbatt_filt);
            cp.cmd_reset();
            Sen->ReadSensors->delay(READ_DELAY);
            Serial.printf("Clean. Ready to deploy\n");
            rp.large_reset();
            cp.large_reset();
            cp.cmd_reset();
            chit("W3;", SOON);
            chit("Hs;", SOON);
            break;

          case ( 's' ):  // Rs:  small reset filters
            Serial.printf("Small reset. Filters reinit\n");
            cp.cmd_reset();
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk. Try typing 'h'");
        }
        break;

      case ( 's' ):  // s<>:  select amp or noamp
        if ( cp.input_string.substring(1).toInt()>0 )
        {
          rp.ibatt_select = 1;
        }
        else if ( cp.input_string.substring(1).toInt()<0 )
          rp.ibatt_select = -1;
        else
          rp.ibatt_select = 0;
        Serial.printf("Sig ( -1=noamp, 0=auto, 1=amp,) set %d\n", rp.ibatt_select);
        break;

      case ( 'v' ):  // v<>:  verbose level
        rp.debug = cp.input_string.substring(1).toInt();
        break;

      case ( 'w' ):   // w:  toggle wifi
        cp.enable_wifi = !cp.enable_wifi; // not remembered in rp. Photon reset turns this false
        Serial.printf("Wifi togg %d\n", cp.enable_wifi);
        break;

      case ( 'W' ):   // W<>:  wait.  Skip
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
          Serial.printf(".....Wait...\n");
        }
        break;

      case ( 'z' ):  // z:  toggle Blynk
        // For this to do anything need #define USE_BLYNK in main.h
        // I don't think Blynk works with 115200 baud.  Meanwhile 9600 isn't fast enough to prevent
        // clogging IPC.   If try starting Blynk with 115200 as presently configured both in this application
        // and on the HC-06 device, the Blynk.begin() blocks after entering 'z' and have to reset.
        Serial.printf("toggling cp.blynking from %d to ", cp.blynking);
        cp.blynking = !cp.blynking;
        Serial.printf("%d\n", cp.blynking);
        break;

      case ( 'X' ):  // X
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'd' ):   // Xd<>:  on/off dc-dc charger manual setting
            if ( cp.input_string.substring(2).toInt()>0 )
              cp.dc_dc_on = true;
            else
              cp.dc_dc_on = false;
            Serial.printf("dc_dc_on set to %d\n", cp.dc_dc_on);
            break;

          case ( 'm' ):   // Xm<>:  code for modeling level
            INT_in =  cp.input_string.substring(2).toInt();
            if ( INT_in>=0 && INT_in<16 )
            {
              boolean reset = rp.modeling != INT_in;
              Serial.printf("modeling from %d to ", rp.modeling);
              rp.modeling = INT_in;
              Serial.printf("%d\n", rp.modeling);
              if ( reset )
              {
                Serial.printf("Changed...soft reset\n");
                cp.cmd_reset();
              }
            }
            else
            {
              Serial.printf("invalid %d, modeling is 0-7.  Try 'h'\n", INT_in);
            }
            Serial.printf("Modeling is %d\n", rp.modeling);
            Serial.printf("tweak_test is %d\n", rp.tweak_test());
            Serial.printf("mod_ib is %d\n", rp.mod_ib());
            Serial.printf("mod_vb is %d\n", rp.mod_vb());
            Serial.printf("mod_tb is %d\n", rp.mod_tb());
            break;

          case ( 'a' ): // Xa<>:  injection amplitude
            rp.amp = cp.input_string.substring(2).toFloat();
            Serial.printf("Inj amp set %7.3f & inj_bias set %7.3f\n", rp.amp, rp.inj_bias);
            break;

          case ( 'f' ): // Xf<>:  injection frequency
            rp.freq = max(min(cp.input_string.substring(2).toFloat(), 2.0), 0.0);
            Serial.printf("Injected freq set %7.3f Hz =", rp.freq);
            rp.freq *= (2. * PI);
            Serial.printf(" %7.3f r/s\n", rp.freq);
            break;

          case ( 'b' ): // Xb<>:  injection bias
            rp.inj_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("Inj_bias set %7.3f\n", rp.inj_bias);
            break;

          case ( 't' ): // Xt<>:  injection type
            switch ( cp.input_string.charAt(2) )
            {
              case ( 'o' ):  // Xto:  cosine
                rp.type = 8;
                Serial.printf("Set cos.  rp.type = %d\n", rp.type);
                break;

              case ( 's' ):  // Xts:  sine
                rp.type = 1;
                Serial.printf("Set sin.  rp.type = %d\n", rp.type);
                break;

              case ( 'q' ):  // Xtq:  square
                rp.type = 2;
                Serial.printf("Set square.  rp.type = %d\n", rp.type);
                break;

              case ( 't' ):  // Xtt:  triangle
                rp.type = 3;
                Serial.printf("Set tri.  rp.type = %d\n", rp.type);
                break;

              case ( 'c' ):  // Xtc:  charge rate
                rp.type = 4;
                Serial.printf("Set 1C charge.  rp.type = %d\n", rp.type);
                break;

              case ( 'd' ):  // Xtd:  discharge rate
                rp.type = 5;
                Serial.printf("Set 1C discharge.  rp.type = %d\n", rp.type);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
            }
            break;

          case ( 'o' ): // Xo<>:  injection dc offset
            rp.inj_bias = max(min(cp.input_string.substring(2).toFloat(), 18.3), -18.3);
            Serial.printf("inj_bias set %7.3f\n", rp.inj_bias);
            break;

          case ( 'p' ): // Xp<>:  injection program
            INT_in = cp.input_string.substring(2).toInt();
            switch ( INT_in )
            {

              case ( -1 ):  // Xp-1:  full reset
                chit("Xp0;", ASAP);
                chit("Ca0.5;", SOON);
                chit("Xm0;", SOON);
                break;

              case ( 0 ):  // Xp0:  reset stop
                chit("Xm7;", ASAP);
                chit("Xf0.; Xa0.", ASAP);
                if ( !rp.tweak_test() ) chit("Xb0.", ASAP);
                chit("XS; Mk1; Nk1;", ASAP);  // Stop any injection
                chit(set_nom_coul_eff, ASAP);
                chit("Di0;", ASAP);
                break;

              case ( 1 ):  // Xp1:  sine
                chit("Xp0;", QUEUE);
                chit("Ca0.5;", QUEUE);
                chit("Xts; Xf0.05; Xa6.;", QUEUE);
                if ( !rp.tweak_test() ) chit("Xb-6.", QUEUE);
                break;

              case ( 2 ):  // Xp2:  
                chit("Xp0;", QUEUE);
                chit("Ca0.5;", QUEUE);
                chit("Xtq; Xf0.10; Xa6.;", QUEUE);
                if ( !rp.tweak_test() ) chit("Xb-6.", QUEUE);
                break;

              case ( 3 ):  // Xp3:  
                chit("Xp0;", QUEUE);
                chit("Ca0.5;", QUEUE);
                chit("Xtt; Xf0.05; Xa6.;", QUEUE);
                if ( !rp.tweak_test() ) chit("Xb-6.", QUEUE);
                break;

              case ( 4 ):  // Xp4:  
                chit("Xp0;", QUEUE);
                chit("Xtc;", QUEUE);
                chit("Di-100;", QUEUE);
                break;

              case ( 5 ):  // Xp5:  
                chit("Xp0;", QUEUE);
                chit("Xtc;", QUEUE);
                chit("Di100;", QUEUE);
                break;

              case ( 6 ):  // Xp6:  Program a pulse
                chit("XS;Dm0;Dn0;v0;Xm7;Ca0.5;Pm;Dr100;Dp100;v26;", QUEUE);  // setup
                chit("Dn0.00001;Dm500;Pt;Pt;Pt;Pt;Pt;Pt;Pt;Dm-500;Pt;Pt;Pt;Pt;Pt;Pt;Pt;Dm0;Pt;Pt;Pt;Pt;Pt;Pt;Pt;", QUEUE);  // run
                chit("W10;Pm;v0;", QUEUE);  // finish
                break;

              case ( 9 ): case( 10 ): case ( 11 ): case( 12 ):  // Xp9: Xp10: Xp11: Xp12:  Program regression
                          // Regression tests 9=tweak, 10=tweak w data, 11=cycle, 12 1/2 cycle
                chit("Xp0;", QUEUE);     // Reset nominal
                chit("v0;", QUEUE);      // Turn off debug temporarily so not snowed by data dumps
                chit("Bm0;Bs0;", QUEUE); // Set Battleborn configuration
                chit("Xm15;", QUEUE);    // Modeling (for totally digital test of logic) and tweak_test=true to disable cutback in Sim.  Leaving cutback on would mean long run times (~30:00) (May need a way to test features affected by cutback, such as tweak, saturation logic)
                chit("Xts;", QUEUE);     // Start up a sine wave
                chit("Ca1;", QUEUE);     // After restarting with sine running, soc will not be at 1.  Reset them all to 1
                chit("Ri;", QUEUE);      // Reset the delta_q's
                chit("Mw0;Nw0;", QUEUE); // Allow tweak bias to work immediately instead of waiting several hours
                chit("MC0.004;", QUEUE); // Give tweak bias logic a large adjustment range to quickly converge
                chit("Mx0.04;", QUEUE);  // Give tweak bias logic a large adjustment range to quickly converge
                chit("NC0.004;", QUEUE); // Give tweak bias logic a large adjustment range to quickly converge
                chit("Nx0.04;", QUEUE);  // Give tweak bias logic a large adjustment range to quickly converge
                chit("Mk1;Nk1;", QUEUE); // Reset the tweak biases to 1 for new count
                chit("Dn1;", QUEUE);   // Disable Coulombic efficiency logic, otherwise tweak_test causes tweak logic to make bias ~1 A
                chit("Dp100;", QUEUE); // Fast data collection
                if ( INT_in == 9 )// Xp9:  silent tweak test
                {
                  chit("Xf0.02;", QUEUE);  // Frequency 0.02 Hz
                  chit("XW5;", QUEUE);     // Wait time before starting to cycle
                  chit("XT5;", QUEUE);     // Wait time after cycle to print
                  chit("Xa-2000;", QUEUE); // Amplitude -2000 A
                  chit("XC20;", QUEUE);    // Number of injection cycles
                  chit("v0;", QUEUE);      // Silent
                }
                else if ( INT_in == 10 )  // Xp10:  rapid tweak
                {
                  chit("Xf0.02;", QUEUE);  // Frequency 0.02 Hz
                  chit("Xa-2000;", QUEUE); // Amplitude -2000 A
                  chit("XW5;", QUEUE);     // Wait time before starting to cycle
                  chit("XT5;", QUEUE);     // Wait time after cycle to print
                  chit("XC3;", QUEUE);     // Number of injection cycles
                  chit("v24;", QUEUE);     // Data collection
                }
                else if ( INT_in == 11 )  // Xp11:  slow tweak
                {
                  chit("Xf0.002;", QUEUE); // Frequency 0.002 Hz
                  chit("Xa-60;", QUEUE);   // Amplitude -60 A
                  chit("XW60;", QUEUE);    // Wait time before starting to cycle
                  chit("XT600;", QUEUE);   // Wait time after cycle to print
                  chit("XC1;", QUEUE);     // Number of injection cycles
                  chit("v24;", QUEUE);     // Data collection
                }
                else if ( INT_in == 12 )  // Xp12:  slow half tweak
                {
                  chit("Xf0.0002;", QUEUE); // Frequency 0.002 Hz
                  chit("Xa-6;", QUEUE);     // Amplitude -60 A
                  chit("XW60;", QUEUE);     // Wait time before starting to cycle
                  chit("XT2400;", QUEUE);   // Wait time after cycle to print
                  chit("XC0.5;", QUEUE);    // Number of injection cycles
                  chit("v24;", QUEUE);      // Data collection
                }
                chit("Rb;", QUEUE);    // Reset battery states
                chit("Pa;", QUEUE);    // Print all for record
                chit("XR;", QUEUE);    // Run cycle
                break;

              case ( 13 ):  // Xp13:  Program hi amp fail
                chit("Xm7;Ca0.5;Dr100;Dp100;v26;W50;Dm500;Dn0.0001;", QUEUE);  // begin
                Serial.printf("Run 'Rf' to finish\n");
                break;

              case( 20 ): case ( 21 ):  // Xp20:  Xp21:  20=tweak-like data, 21= 2 sec data cycle
                chit("v0;", QUEUE);    // Turn off debug temporarily so not snowed by data dumps
                chit("Pa;", QUEUE);    // Print all for record
                if ( INT_in == 20 )
                {
                  chit("Dp100;", QUEUE);  // Tweak-like data collection
                  chit("v24;", QUEUE);    // Tweak-like data collection
                }
                else if ( INT_in == 21 )
                {
                  chit("Dp2000;", QUEUE); // Fast data collection
                  chit("v4;", QUEUE);      // Slow data collection
                }
                chit("Rb;", QUEUE);    // Reset battery states
                break;

              default:
                Serial.printf("Xp=%d unk.  see 'h'\n", INT_in);
            }
            break;

          case ( 'C' ): // XC:  injection number of cycles
            Sen->cycles_inj = max(min(cp.input_string.substring(2).toFloat(), 10000.), 0);
            Serial.printf("Num of inj cycles set to %7.3f\n", Sen->cycles_inj);
            break;

          case ( 'R' ): // XR:  Start injection now
            if ( Sen->now>TEMP_INIT_DELAY )
            {
              Sen->start_inj = Sen->wait_inj + Sen->now;
              Sen->stop_inj = Sen->wait_inj + (Sen->now + min((unsigned long int)(Sen->cycles_inj / max(rp.freq/(2.*PI), 1e-6) *1000.), ULLONG_MAX));
              Sen->end_inj = Sen->stop_inj + Sen->tail_inj;
              Serial.printf("RUN: at %ld, %7.3f cycles from %ld to %ld with %ld wait and %ld tail\n",
                Sen->now, Sen->cycles_inj, Sen->start_inj, Sen->stop_inj, Sen->wait_inj, Sen->tail_inj);
            }
            else Serial.printf("Wait %5.1f s for init\n", float(TEMP_INIT_DELAY-Sen->now)/1000.);
            break;

          case ( 'S' ): // XS:  Stop injection now
            Sen->start_inj = 0UL;
            Sen->stop_inj = 0UL;
            Sen->end_inj = 0UL;
            Serial.printf("STOPPED\n");
            break;

          case ( 'W' ):  // XW<>:  Wait beginning of programmed transient
            FP_in = cp.input_string.substring(2).toFloat();
            Sen->wait_inj = (unsigned long int)(max(FP_in, 0.))*1000;
            Serial.printf("Waiting %7.1f s to start inj\n", FP_in);
            break;

          case ( 'T' ):  // XT<>:  Tail
            FP_in = cp.input_string.substring(2).toFloat();
            Sen->tail_inj = (unsigned long int)(max(FP_in, 0.))*1000;
            Serial.printf("Waiting %7.1f s tail after inj\n", FP_in);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unk.  See 'h'");
        }
        break;

      case ( 'h' ):  // h: help
        talkH(Mon, Sen);
        break;

      default:
        Serial.print(cp.input_string.charAt(0)); Serial.println(" unk.  See 'h'");
        break;
    }

    cp.input_string = "";
    cp.string_complete = false;
  }
}

// Talk Help
void talkH(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("\n\n**** TALK ****\nHelp talk.   Entries and curr values.  Entries followed by CR or ';'\n");

  Serial.printf("B<?> Battery assignments.   For example:\n");
  Serial.printf("  Bm=  %d.  Mon chem 0='BB', 1='LI' [%d]\n", rp.mon_mod, MOD_CODE); 
  Serial.printf("  Bs=  %d.  Sim chem 0='BB', 1='LI' [%d]\n", rp.sim_mod, MOD_CODE); 
  Serial.printf("  BP=  %5.2f.  # parallel in bank [%5.2f]'\n", rp.nP, NP); 
  Serial.printf("  BS=  %5.2f.  # series in bank [%5.2f]'\n", rp.nS, NS); 

  Serial.printf("C<?> Charge assignments.   For example:\n");
  Serial.printf("  Ca=  set soc in all - '(0-1.1)'\n"); 
  Serial.printf("  Cm=  set soc model only (and ekf if modeling)- '(0-1.1)'\n"); 

  Serial.printf("D/S<?> Adjustments.   For example:\n");
  Serial.printf("  Da= "); Serial.printf("%7.3f", rp.ibatt_bias_amp); Serial.printf("    : delta amp sense, A [%7.3f]\n", CURR_BIAS_AMP); 
  Serial.printf("  Db= "); Serial.printf("%7.3f", rp.ibatt_bias_noamp); Serial.printf("    : delta noa sense, A [%7.3f]\n", CURR_BIAS_NOAMP); 
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.ibatt_bias_all); Serial.printf("    : delta all sense, A [%7.3f]\n", CURR_BIAS_ALL); 
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.printf("    : delta sense, V [%7.3f]\n", VOLT_BIAS); 
  Serial.printf("  Du= "); Serial.print(Sen->Sim->coul_eff()); Serial.println("       : coul eff"); 
  Serial.printf("  Dm= "); Serial.printf("%7.3f", Sen->ShuntAmp->add()); Serial.printf("    : delta amp, A [0]\n"); 
  Serial.printf("  Dn= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->add()); Serial.printf("    : delta noamp, A [0]\n"); 
  Serial.printf("  Dp= "); Serial.print(Sen->PublishSerial->delay()); Serial.println("       : publish frame, ms [400]"); 
  Serial.printf("  Dr= "); Serial.print(Sen->ReadSensors->delay()); Serial.println("       : minor frame, ms [100]"); 
  Serial.printf("  Dt= "); Serial.printf("%7.3f", rp.tbatt_bias); Serial.printf("    : delta sense, deg C [%7.3f]\n", TEMP_BIAS); 
  Serial.printf("  Dv= "); Serial.print(Sen->Sim->Dv()); Serial.println("       : Table adjust, V [0.01]"); 
  Serial.printf("  DT= "); Serial.printf("%7.3f", Sen->Tbatt_noise_amp()); Serial.printf("    : Tbatt noise for model, deg C pk-pk [%7.3f]\n", TEMP_BIAS); 
  Serial.printf("  DV= "); Serial.printf("%7.3f", Sen->Vbatt_noise_amp()); Serial.printf("    : Vbatt noise for model, V pk-pk [%7.3f]\n", TEMP_BIAS); 
  Serial.printf("  DI= "); Serial.printf("%7.3f", Sen->Ibatt_noise_amp()); Serial.printf("    : Ibatt noise for model, A pk-pk [%7.3f]\n", TEMP_BIAS); 
  Serial.printf("  Sc= "); Serial.print(Sen->Sim->q_capacity()/Mon->q_capacity()); Serial.println("       : Scalar model size"); 
  Serial.printf("  Sh= "); Serial.printf("%7.3f", rp.hys_scale); Serial.println("    : hysteresis scalar 1e-6 - 100 [1]");
  Serial.printf("  Sm= "); Serial.printf("%7.3f", Sen->ShuntAmp->slr()); Serial.printf("    : scalar amp, [1]\n"); 
  Serial.printf("  Sn= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->slr()); Serial.printf("    : scalar noamp [1]\n"); 
  Serial.printf("  Sr= "); Serial.print(Sen->Sim->Sr()); Serial.println("       : Scalar res sim"); 
  Serial.printf("  Sk= "); Serial.print(rp.cutback_gain_scalar); Serial.println("       : Sat model cutback scalar"); 

  Serial.printf("H<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump summary log\n");
  Serial.printf("  HR= "); Serial.printf("reset summary log\n");
  Serial.printf("  Hs= "); Serial.printf("save and print log\n");

  Serial.printf("M<?> Amp tweaks\n");
  Serial.printf("  MC= "); Serial.printf("%7.3f", Sen->ShuntAmp->max_change()); Serial.println("    : Amp Coul eff max change allowed scakar [0.001]"); 
  Serial.printf("  Mk= "); Serial.printf("%7.3f", Sen->ShuntAmp->tweak_sclr()); Serial.println("    : Amp Coul eff scalar [1]"); 
  Serial.printf("  Mw= "); Serial.printf("%7.3f", Sen->ShuntAmp->time_to_wait()); Serial.println("    : Amp time to wait for next tweak, hr [18]]"); 
  Serial.printf("  Mx= "); Serial.printf("%7.3f", Sen->ShuntAmp->max_tweak()); Serial.println("    : Amp Coul eff max allowed scalar [0.01]"); 
  Serial.printf("  Mz= "); Serial.printf("%7.3f", Sen->ShuntAmp->time_sat_past()); Serial.println("    : Amp time since last tweak, hr [varies]"); 

  Serial.printf("N<?> No amp tweaks\n");
  Serial.printf("  NC= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->max_change()); Serial.println("    : No Amp Coul eff max change allowed scakar [0.001]"); 
  Serial.printf("  Nk= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->tweak_sclr()); Serial.println("    : No Amp Coul eff scalar [1]"); 
  Serial.printf("  Nw= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->time_to_wait()); Serial.println("    : No Amp time to wait for next tweak, hr [18]]"); 
  Serial.printf("  Nx= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->max_tweak()); Serial.println("    : Amp Coul eff max allowed scalar [0.01]"); 
  Serial.printf("  Nz= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->time_sat_past()); Serial.println("    : No Amp time since last tweak, hr [varies]"); 

  Serial.printf("P<?>   Print Battery values\n");
  Serial.printf("  Pa= "); Serial.printf("all\n");
  Serial.printf("  Pc= "); Serial.printf("all coulombs\n");
  Serial.printf("  Pe= "); Serial.printf("ekf\n");
  Serial.printf("  Pm= "); Serial.printf("monitor\n");
  Serial.printf("  PM= "); Serial.printf("amp tweak\n");
  Serial.printf("  PN= "); Serial.printf("no amp tweak\n");
  Serial.printf("  Pr= "); Serial.printf("retained and command\n");
  Serial.printf("  Ps= "); Serial.printf("simulation\n");
  Serial.printf("  Pt= "); Serial.printf("all state-space\n");
  Serial.printf("  Px= "); Serial.printf("current signal selection\n");
  Serial.printf("  Pv= "); Serial.printf("voltage signal details\n");

  Serial.printf("Q      vital stats\n");

  Serial.printf("R<?>   Reset\n");
  Serial.printf("  Rb= "); Serial.printf("reset batteries to current inputs\n");
  Serial.printf("  Re= "); Serial.printf("equalize delta_q in Sim to Mon\n");
  Serial.printf("  Rf= "); Serial.printf("reset fault logic latches\n");
  Serial.printf("  Rh= "); Serial.printf("reset all hys\n");
  Serial.printf("  Ri= "); Serial.printf("reset all delta_q_inf\n");
  Serial.printf("  Rr= "); Serial.printf("saturate battery monitor and equalize Sim & Mon\n");
  Serial.printf("  RR= "); Serial.printf("saturate, equalize, & nominalize all testing for DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small reset.  Reset flags to reinitialize filters\n");

  Serial.printf("s   curr signal select (-1=noamp, 0=auto, 1=amp) = "); Serial.println(rp.ibatt_select);

  Serial.printf("v=  "); Serial.print(rp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("    -<>:   Negative - Arduino plot compatible\n");
  Serial.printf("    v-1:   GP Arduino plot\n");
  Serial.printf("  +/-v3:   Powert\n");
  Serial.printf("     v4:   GP\n");
  Serial.printf("  +/-v5:   OLED display\n");
  Serial.printf("     v6:   EKF solver iter during init\n");
  Serial.printf("     v7:   EKF solver summary during init\n");
  Serial.printf("    v-7:   Battery i/o Arduino plot\n");
  Serial.printf("     v8:   Randles state space init\n");
  Serial.printf("   v-11:   Summary Arduino\n");
  Serial.printf(" +/-v12:   EKF\n");
  Serial.printf(" +/-v14:   vshunt and Ibatt raw\n");
  Serial.printf("    v15:   vb raw\n");
  Serial.printf("    v16:   Tbatt\n");
  Serial.printf("    v24:   Sim\n");
  Serial.printf("    v25:   Blynk write\n");
  Serial.printf("    v26:   Signal selection\n");
  Serial.printf(" +/-v34:   EKF detailed\n");
  Serial.printf("   v-35:   EKF summary Arduino\n");
  Serial.printf("    v35:   Randles balance\n");
  Serial.printf(" +/-v37:   EKF short\n");
  Serial.printf("   v-41:   Inj\n");
  Serial.printf("    v75:   voc_low check model\n");
  Serial.printf("    v76:   vb model\n");
  Serial.printf(" +/-v78:   Batt model saturation\n");
  Serial.printf("    v79:   sat_ib model\n");
  Serial.printf(" +/-v96:   CC saturation\n");
  Serial.printf(" +/-v97:   CC model saturation\n");

  Serial.printf("w   turn on wifi = "); Serial.println(cp.enable_wifi);

  Serial.printf("W<?>  - iterations to wait\n");

  Serial.printf("X<?> - Test Mode.   For example:\n");
  Serial.printf("  Xd= "); Serial.printf("%d,   dc-dc charger on [0]\n", cp.dc_dc_on);
  Serial.printf("  Xm= "); Serial.printf("%d,   modeling bitmap [000]\n", rp.modeling);
  Serial.printf("       0x8 tweak_test = %d\n", rp.tweak_test());
  Serial.printf("       0x4 current = %d\n", rp.mod_ib());
  Serial.printf("       0x2 voltage = %d\n", rp.mod_vb());
  Serial.printf("       0x1 temp = %d\n", rp.mod_tb());
  Serial.printf("  Xa= "); Serial.printf("%7.3f", rp.amp); Serial.println("  : Inj amp A pk (0-18.3) [0]");
  Serial.printf("  Xb= "); Serial.printf("%7.3f", rp.inj_bias); Serial.println("  : Inj bias A [0]");
  Serial.printf("  Xf= "); Serial.printf("%7.3f", rp.freq/2./PI); Serial.println("  : Inj freq Hz (0-2) [0]");
  Serial.printf("  Xt= "); Serial.printf("%d", rp.type); Serial.println("  : Inj type.  'c', 's', 'q', 't' (cos, sine, square, tri)");
  Serial.printf("  Xo= "); Serial.printf("%7.3f", rp.inj_bias); Serial.println("  : Inj inj_bias A (-18.3-18.3) [0]");
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.ibatt_bias_all); Serial.println("  : Inj  A (unlimited) [0]");
  Serial.printf("  Xp= <?>, programmed inj settings...\n"); 
  Serial.printf("      Xp-1:  Off, modeling false\n");
  Serial.printf("      Xp0:  steady-state modeling\n");
  Serial.printf("      Xp1:  1 Hz sine about 0\n");
  Serial.printf("      Xp2:  1 Hz square about 0\n");
  Serial.printf("      Xp3:  1 Hz triangle about 0\n");
  Serial.printf("      Xp4:  -1C soft discharge, reset xp0 or Di0\n");
  Serial.printf("      Xp5:  +1C soft charge\n");
  Serial.printf("      Xp6:  +/-500 A pulse\n");
  Serial.printf("      Xp9:  silent tweak reg test\n");
  Serial.printf("      Xp10:  tweak reg test\n");
  Serial.printf("      Xp11:  slow cycle reg test\n");
  Serial.printf("      Xp12:  slow cycle reg test - half cycle\n");
  Serial.printf("      Xp13:  amp hi fail\n");
  Serial.printf("      Xp20:  tweak-like data collection\n");
  Serial.printf("      Xp21:  slow data collection\n");
  Serial.printf("  XC= "); Serial.printf("%7.3f cycles inj\n", Sen->cycles_inj);
  Serial.printf("  XR  "); Serial.printf("RUN inj\n");
  Serial.printf("  XS  "); Serial.printf("STOP inj\n");
  Serial.printf("  XW= "); Serial.printf("%6.2f s wait start inj\n", float(Sen->wait_inj)/1000.);
  Serial.printf("  XT= "); Serial.printf("%6.2f s tail end inj\n", float(Sen->tail_inj)/1000.);
  Serial.printf("z   toggle BLYNK = %d\n", cp.blynking );

  Serial.printf("h   this menu\n");
}
