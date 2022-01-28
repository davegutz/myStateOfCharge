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

extern CommandPars cp;          // Various parameters shared at system level
extern RetainedPars rp;         // Various parameters to be static at system level
extern Sum_st mySum[NSUM];      // Summaries for saving charge history

// Talk Executive
void talk(Battery *Monitor, BatteryModel *Model, Sensors *Sen)
{
  double SOCS_in = -99.;
  double scale = 1.;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.string_complete)
  {
    switch ( cp.input_string.charAt(0) )
    {
      case ( 'A' ):
        rp.nominal();
        rp.print_part_1(cp.buffer);
        Serial.printf("Force nominal rp %s", cp.buffer);
        rp.print_part_2(cp.buffer);
        Serial.printf("%s", cp.buffer);
        Serial.printf("\n\n ************** now Reset to apply nominal rp ***********************\n");
        break;
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
            Model->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("Model.Dv changed to %7.3f\n", Model->Dv());
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'E' ):
        Serial.printf("Equalizing BatteryModel, and ekf if needed, to Battery\n");
        Serial.printf("\nMyBattModel:  soc=%7.3f, SOC=%7.3f, q=%7.3f, delta_q = %7.3f, q_scaled_rated = %7.3f,\
        q_rated = %7.3f, q_capacity = %7.3f,\n\n", Model->soc(), Model->SOC(), Model->q(), Model->delta_q(),
        Model->q_cap_scaled(), Model->q_cap_rated(), Model->q_capacity());
        Model->apply_delta_q(Monitor->delta_q());
        if ( rp.modeling ) Monitor->init_soc_ekf(Model->soc());
        Serial.printf("\nMyBattModel:  soc=%7.3f, SOC=%7.3f, q=%7.3f, delta_q = %7.3f, q_scaled_rated = %7.3f,\
        q_rated = %7.3f, q_capacity = %7.3f,\n\n", Model->soc(), Model->SOC(), Model->q(), Model->delta_q(),
        Model->q_cap_scaled(), Model->q_cap_rated(), Model->q_capacity());
        break;
      case ( 'S' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'c' ):
            scale = cp.input_string.substring(2).toFloat();
            rp.s_cap_model = scale;
            Serial.printf("Model.q_cap_rated scaled by %7.3f from %7.3f to ", scale, Model->q_cap_scaled());
            Model->apply_cap_scale(rp.s_cap_model);
            if ( rp.modeling ) Monitor->init_soc_ekf(Model->soc());
            Serial.printf("%7.3f\n", Model->q_cap_scaled());
            Serial.printf("Model:  "); Model->pretty_print(); Model->Coulombs::pretty_print();
            break;
          case ( 'r' ):
            scale = cp.input_string.substring(2).toFloat();
            Serial.printf("\nBefore Model::StateSpace:\n"); Model->pretty_print_ss();
            Serial.printf("\nScaling D[0, 0] = -r0 by Sr= %7.3f\n", scale);
            Model->Sr(scale);
            Serial.printf("\nAfter Model::StateSpace:\n"); Monitor->pretty_print_ss();
            Serial.printf("\nBefore Monitor::StateSpace:\n"); Monitor->pretty_print_ss();
            Serial.printf("\nScaling D[0, 0] = -r0 by Sr= %7.3f\n", scale);
            Monitor->Sr(scale);
            Serial.printf("\nAfter Monitor::StateSpace:\n"); Monitor->pretty_print_ss();
            break;
          case ( 'k' ):
            scale = cp.input_string.substring(2).toFloat();
            rp.cutback_gain_scalar = scale;
            Serial.printf("rp.cutback_gain_scalar set to %7.3f\n", rp.cutback_gain_scalar);
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'H' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'd' ):
            print_all_summary(mySum, rp.isum, NSUM);
            break;
          case ( 'R' ):
            large_reset_summary(mySum, rp.isum, NSUM);
            break;
          case ( 's' ):
            self_talk("h", Monitor, Model, Sen);
            cp.cmd_summarize();
            print_all_summary(mySum, rp.isum, NSUM);
            self_talk("Pa", Monitor, Model, Sen);
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'l' ):
        switch ( rp.debug )
        {
          case ( -1 ):
            Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt_amp  ,Ishunt_noamp  ,Vbat_fo*10-110  ,voc_s*10-110  ,vdyn_s*10  ,v_s*10-110  , voc_dyn*10-110,,,,,,,,,,,\n");
            break;
          default:
            print_serial_header();
        }
        break;
      case ( 'm' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        if ( SOCS_in<1.1 )  // Apply crude limit to prevent user error
        {
          Monitor->apply_soc(SOCS_in, Sen->Tbatt_filt);
          Model->apply_delta_q(Monitor->delta_q());
          if ( rp.modeling ) Monitor->init_soc_ekf(Model->soc());
          Monitor->update(&rp.delta_q, &rp.t_last);
          Model->update(&rp.delta_q_model, &rp.t_last_model);
          Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
              Monitor->SOC(), Monitor->soc(), rp.delta_q, Model->SOC(), Model->soc(), rp.delta_q_model, Monitor->soc_ekf());
        }
        else
          Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", SOCS_in);
        break;
      case ( 'M' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        Monitor->apply_SOC(SOCS_in, Sen->Tbatt_filt);
        Model->apply_delta_q(Monitor->delta_q());
        if ( rp.modeling ) Monitor->init_soc_ekf(Model->soc());
        Monitor->update(&rp.delta_q, &rp.t_last);
        Model->update(&rp.delta_q_model, &rp.t_last_model);
        Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
            Monitor->SOC(), Monitor->soc(), rp.delta_q, Model->SOC(), Model->soc(), rp.delta_q_model, Monitor->soc_ekf());
        break;
      case ( 'n' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        if ( SOCS_in<1.1 )   // Apply crude limit to prevent user error
        {
          Model->apply_soc(SOCS_in, Sen->Tbatt_filt);
          Model->update(&rp.delta_q_model, &rp.t_last_model);
          if ( rp.modeling )
            Monitor->init_soc_ekf(Monitor->soc());
          Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
              Monitor->SOC(), Monitor->soc(), rp.delta_q, Model->SOC(), Model->soc(), rp.delta_q_model, Monitor->soc_ekf());
        }
        else
          Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", SOCS_in);
        break;
      case ( 'N' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        Model->apply_SOC(SOCS_in, Sen->Tbatt_filt);
        Model->update(&rp.delta_q_model, &rp.t_last_model);
        if ( rp.modeling )
          Monitor->init_soc_ekf(Monitor->soc());
        Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
            Monitor->SOC(), Monitor->soc(), rp.delta_q, Model->SOC(), Model->soc(), rp.delta_q_model, Monitor->soc_ekf());
        break;
      case ( 'P' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):
            self_talk("Pb", Monitor, Model, Sen);
            Serial.printf("\n");
            Serial.printf("Model:   rp.modeling = %d\n", rp.modeling);
            self_talk("Pm", Monitor, Model, Sen);
            Serial.printf("\n");
            self_talk("Pr", Monitor, Model, Sen);
            Serial.printf("\n");
            break;
          case ( 'b' ):
            Serial.printf("Monitor:");  Monitor->pretty_print();
            Serial.printf("Monitor::"); Monitor->Coulombs::pretty_print();
            Serial.printf("Monitor::"); Monitor->pretty_print_ss();
            Serial.printf("Monitor::"); Monitor->EKF_1x1::pretty_print();
            break;
          case ( 'c' ):
            Serial.printf("Monitor::"); Monitor->Coulombs::pretty_print();
            Serial.printf("Model::");   Model->Coulombs::pretty_print();
            break;
          case ( 'e' ):
             Serial.printf("Monitor::"); Monitor->EKF_1x1::pretty_print();
            break;
          case ( 'm' ):
            Serial.printf("Model:   rp.modeling = %d\n", rp.modeling);
            Serial.printf("Model:");    Model->pretty_print();
            Serial.printf("Model::");   Model->Coulombs::pretty_print();
            Serial.printf("Model::");   Model->pretty_print_ss();
            break;
          case ( 'r' ):
            Serial.printf("retained::");
            rp.print_part_1(cp.buffer); Serial.printf("%s", cp.buffer);
            rp.print_part_2(cp.buffer); Serial.printf("%s", cp.buffer);
            Serial.printf("command::");       cp.pretty_print();
            break;
          case ( 's' ):
            Serial.printf("Monitor::"); Monitor->pretty_print_ss();
            Serial.printf("Model::");   Model->pretty_print_ss();
            break;
          case ( 'x' ):
            Serial.printf("Amp:   ");      Serial.printf("Vshunt_int, Vshunt, cp.curr_bias, Ishunt_cal, sel_noamp, Ishunt=, %d, %7.3f, %7.3f, %7.3f,%d, %7.3f\n", 
              Sen->Vshunt_amp_int, Sen->Vshunt_amp, cp.curr_bias_amp, Sen->Ishunt_amp_cal, rp.curr_sel_noamp, Sen->Ishunt);
            Serial.printf("No Amp:");      Serial.printf("Vshunt_int, Vshunt, cp.curr_bias, Ishunt_cal, sel_noamp, Ishunt=, %d, %7.3f, %7.3f, %7.3f, %d, %7.3f\n", 
              Sen->Vshunt_noamp_int, Sen->Vshunt_noamp, cp.curr_bias_noamp, Sen->Ishunt_noamp_cal, rp.curr_sel_noamp, Sen->Ishunt);
            break;
          case ( 'v' ):
            Serial.printf("Volt:   ");      Serial.printf("rp.vbatt_bias, Vbatt_model, rp.modeling, Vbatt=, %7.3f, %7.3f, %d, %7.3f,\n", 
              rp.vbatt_bias, Sen->Vbatt_model, rp.modeling, Sen->Vbatt);
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'Q' ):
        Serial.printf("tb  = %7.3f,\nvb  = %7.3f,\nvoc  = %7.3f,\nvsat = %7.3f,\nib  = %7.3f,\nsoc = %7.3f,\nsoc_ekf= %7.3f,\n",
          Monitor->temp_c(), Monitor->vb(), Monitor->voc(), Monitor->vsat(), Monitor->ib(), Monitor->soc(), Monitor->soc_ekf());
        break;
      case ( 'R' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'e' ):
            Serial.printf("Equalizing reset.   Just set delta_q in BattModel = delta_q in Batt\n");
            Model->apply_delta_q(Monitor->delta_q());
            break;
          case ( 'r' ):
            Serial.printf("Small reset.   Just reset all to soc=1.0 and delta_q = 0\n");
            Monitor->apply_soc(1.0, Sen->Tbatt_filt);
            Model->apply_soc(1.0, Sen->Tbatt_filt);
            if ( rp.modeling ) Monitor->init_soc_ekf(Model->soc());
            break;
          case ( 'R' ):
            self_talk("Rr", Monitor, Model, Sen);
            Serial.printf("also large and soft reset.   Initialize all variables to clean run without model at saturation.   Ready to use\n");
            rp.large_reset();
            cp.large_reset();
            cp.cmd_reset();
            self_talk("Hr", Monitor, Model, Sen);
            break;
          case ( 's' ):
            Serial.printf("Small reset.   Just reset the flags so filters are reinitialized\n");
            cp.cmd_reset();
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 's' ):
        if ( cp.input_string.substring(1).toInt()>0 )
        {
          rp.curr_sel_noamp = false;
        }
        else
          rp.curr_sel_noamp = true;
        Serial.printf("Signal selection (1=noamp, 0=amp) set to %d\n", rp.curr_sel_noamp);
        break;
      case ( 'T' ):   // This is a test feature only
        cp.input_string = cp.input_string.substring(1);
        Serial.printf("new string = '%s'\n", cp.input_string.c_str());
        cp.string_complete = true;
        talk(Monitor, Model, Sen);
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
            if ( cp.input_string.substring(2).toInt()>0 )
            {
              rp.modeling = true;
              Monitor->init_soc_ekf(Model->soc());
            }
            else
              rp.modeling = false;
            Serial.printf("Modeling set to %d\n", rp.modeling);
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
              case ( -1 ):
                self_talk("Xp0", Monitor, Model, Sen);
                self_talk("m0.5", Monitor, Model, Sen);
                rp.modeling = false;
                rp.debug = -12;   // myDisplay = 2
                break;
              case ( 0 ):
                rp.modeling = true;
                rp.type = 0;
                rp.freq = 0.0;
                rp.amp = 0.0;
                rp.offset = 0.0;
                rp.curr_bias_all = 0;
                rp.debug = -12;   // myDisplay = 5
                break;
              case ( 1 ):
                self_talk("Xp0", Monitor, Model, Sen);
                self_talk("m0.5", Monitor, Model, Sen);
                rp.type = 1;
                rp.freq = 0.05;
                rp.amp = 6.;
                rp.offset = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
                break;
              case ( 2 ):
                self_talk("Xp0", Monitor, Model, Sen);
                self_talk("m0.5", Monitor, Model, Sen);
                rp.type = 2;
                rp.freq = 0.10;
                rp.amp = 6.;
                rp.offset = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
                break;
              case ( 3 ):
                self_talk("Xp0", Monitor, Model, Sen);
                self_talk("m0.5", Monitor, Model, Sen);
                rp.type = 3;
                rp.freq = 0.05;
                rp.amp = 6.;
                rp.offset = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
                break;
              case ( 4 ):
                self_talk("Xp0", Monitor, Model, Sen);
                rp.type = 4;
                rp.curr_bias_all = -RATED_BATT_CAP;  // Software effect only
                rp.debug = -12;
                break;
              case ( 5 ):
                self_talk("Xp0", Monitor, Model, Sen);
                rp.type = 5;
                rp.curr_bias_all = RATED_BATT_CAP; // Software effect only
                rp.debug = -12;
                break;
              case ( 6 ):
                self_talk("Xp0", Monitor, Model, Sen);
                rp.type = 6;
                rp.amp = RATED_BATT_CAP*0.2;
                rp.debug = -12;
                break;
              case ( 7 ):
                self_talk("Xp0", Monitor, Model, Sen);
                rp.type = 7;
                self_talk("Xx1", Monitor, Model, Sen);    // Run to model
                self_talk("m0.5", Monitor, Model, Sen);   // Set all soc=0.5
                self_talk("n0.987", Monitor, Model, Sen); // Set model only to near saturation
                self_talk("v2", Monitor, Model, Sen);     // Watch sat, soc, and voc vs v_sat
                rp.amp = RATED_BATT_CAP*0.2;              // Hard current charge
                Serial.printf("Run 'n<val> as needed to init south of sat.  Reset this whole thing by running 'Xp-1'\n");
                break;
              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
            }
            Serial.printf("Setting injection program to:  rp.curr_sel_noamp = %d, rp.modeling = %d, rp.type = %d, rp.freq = %7.3f, rp.amp = %7.3f, rp.debug = %d, rp.curr_bias_all = %7.3f\n",
                                    rp.modeling, rp.curr_sel_noamp, rp.type, rp.freq, rp.amp, rp.debug, rp.curr_bias_all);
            break;
          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;
      case ( 'h' ): 
        talkH(Monitor, Model, Sen);
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
void talkH(Battery *Monitor, BatteryModel *Model, Sensors *Sen)
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");
  Serial.printf("A=  nominalize the rp structure for clean boots etc'\n");
  Serial.printf("H<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump the summary log to screen\n");
  Serial.printf("  HR= "); Serial.printf("reset the summary log\n");
  Serial.printf("  Hs= "); Serial.printf("save a data point to summary log and print log and status to screen\n");
  Serial.printf("m=  assign curve charge state in fraction to all versions including model- '(0-1.1)'\n"); 
  Serial.printf("M=  assign a CHARGE state in percent to all versions including model- '('truncated 0-100')'\n"); 
  Serial.printf("n=  assign curve charge state in fraction to model only (ekf if modeling)- '(0-1.1)'\n"); 
  Serial.printf("N=  assign a CHARGE state in percent to model only (ekf if modeling)-- '('truncated 0-100')'\n"); 
  Serial.printf("s   curr signal select (0=amp preferred, 1=noamp) = "); Serial.println(rp.curr_sel_noamp);
  Serial.printf("v=  "); Serial.print(rp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("D/S<?> Adjustments.   For example:\n");
  Serial.printf("  Da= "); Serial.printf("%7.3f", rp.curr_bias_amp); Serial.println("    : delta I adder to sensed amplified shunt current, A [0]"); 
  Serial.printf("  Db= "); Serial.printf("%7.3f", rp.curr_bias_noamp); Serial.println("    : delta I adder to sensed shunt current, A [0]"); 
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.curr_bias_all); Serial.println("    : delta I adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.println("    : delta V adder to sensed battery voltage, V [0]"); 
  Serial.printf("  Dt= "); Serial.printf("%7.3f", rp.t_bias); Serial.println("    : delta T adder to sensed Tbatt, deg C [0]"); 
  Serial.printf("  Dv= "); Serial.print(Model->Dv()); Serial.println("    : delta V adder to solved battery calculation, V"); 
  Serial.printf("  Sc= "); Serial.print(Model->q_capacity()/Monitor->q_capacity()); Serial.println("    : Scalar battery model size"); 
  Serial.printf("  Sr= "); Serial.print(Model->Sr()); Serial.println("    : Scalar resistor for battery dynamic calculation, V"); 
  Serial.printf("  Sk= "); Serial.print(rp.cutback_gain_scalar); Serial.println("    : Saturation of model cutback gain scalar"); 
  Serial.printf("E=  set the BatteryModel delta_q to same value as Battery'\n"); 
  Serial.printf("P<?>   Print Battery values\n");
  Serial.printf("  Pa= "); Serial.printf("print all\n");
  Serial.printf("  Pb= "); Serial.printf("print battery\n");
  Serial.printf("  Pc= "); Serial.printf("print all coulombs\n");
  Serial.printf("  Pe= "); Serial.printf("print ekf\n");
  Serial.printf("  Pm= "); Serial.printf("print model\n");
  Serial.printf("  Pr= "); Serial.printf("print retained and command parameters\n");
  Serial.printf("  Ps= "); Serial.printf("print all state-space\n");
  Serial.printf("  Px= "); Serial.printf("print current signal selection\n");
  Serial.printf("  Pv= "); Serial.printf("print voltage signal details\n");
  Serial.printf("Q      print vital stats\n");
  Serial.printf("R<?>   Reset\n");
  Serial.printf("  Re= "); Serial.printf("equalize delta_q in model to battery monitor\n");
  Serial.printf("  Rr= "); Serial.printf("saturate battery monitor and equalize model to monitor\n");
  Serial.printf("  RR= "); Serial.printf("saturate, equalize, and nominalize all testing for DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small reset.  reset flags to reinitialize filters\n");
  Serial.printf("T<new cmd>  Send in a new command.  Used to test calling Talk from itself.   For Example:\n");
  Serial.printf("  Tv=-78  sends v=-78 to talk\n");
  Serial.printf("w   turn on wifi = "); Serial.println(cp.enable_wifi);
  Serial.printf("X<?> - Test Mode.   For example:\n");
  Serial.printf("  Xx= "); Serial.printf("%d,   use model for Vbatt [0]\n", rp.modeling);
  Serial.printf("  Xa= "); Serial.printf("%7.3f", rp.amp); Serial.println("  : Injection amplitude A pk (0-18.3) [0]");
  Serial.printf("  Xf= "); Serial.printf("%7.3f", rp.freq/2./PI); Serial.println("  : Injection frequency Hz (0-2) [0]");
  Serial.printf("  Xt= "); Serial.printf("%d", rp.type); Serial.println("  : Injection type.  's', 'q', 't' (0=none, 1=sine, 2=square, 3=triangle)");
  Serial.printf("  Xo= "); Serial.printf("%7.3f", rp.offset); Serial.println("  : Injection offset A (-18.3-18.3) [0]");
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.curr_bias_all); Serial.println("  : Injection  A (unlimited) [0]");
  Serial.printf("  Xp= <?>, programmed injection settings...\n"); 
  Serial.printf("      -1:  Off, modeling false\n");
  Serial.printf("       0:  steady-state modeling\n");
  Serial.printf("       1:  1 Hz sinusoid centered at 0 with largest supported amplitude\n");
  Serial.printf("       2:  1 Hz square centered at 0 with largest supported amplitude\n");
  Serial.printf("       3:  1 Hz triangle centered at 0 with largest supported amplitude\n");
  Serial.printf("       4:  -1C soft discharge until reset by Xp0 or Di0.  Software only\n");
  Serial.printf("       5:  +1C soft charge until reset by Xp0 or Di0.  Software only\n");
  Serial.printf("       6:  +0.2C hard charge until reset by Xp0 or Di0\n");
  Serial.printf("h   this menu\n");
}

// Recursion
void self_talk(const String cmd, Battery *Monitor, BatteryModel *Model, Sensors *Sen)
{
  cp.input_string = cmd;
  Serial.printf("self_talk:  new string = '%s'\n", cp.input_string.c_str());
  cp.string_complete = true;
  talk(Monitor, Model, Sen);
}
