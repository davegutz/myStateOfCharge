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
void talk(BatteryMonitor *Mon, BatteryModel *Sim, Sensors *Sen, Tweak *Twk_amp, Tweak *Twk_noa)
{
  double SOCS_in = -99.;
  double scale = 1.;
  double Q_in = 0.;
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

          case ( 'c' ):
            rp.vbatt_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.vbatt_bias changed to %7.3f\n", rp.vbatt_bias);
            break;

          case ( 'i' ):
            rp.curr_bias_all = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.curr_bias_all changed to %7.3f\n", rp.curr_bias_all);
            break;

          case ( 't' ):
            rp.t_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("rp.t_bias changed to %7.3f\n", rp.t_bias);
            // TODO:  make soft reset properly reinit t filters
            Serial.printf("****************You should perform a hard reset to initialize temp filters to new values\n");
            break;

          case ( 'v' ):
            Mon->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("Mon.Dv changed to %7.3f\n", Mon->Dv());
            Sim->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("Sim.Dv changed to %7.3f\n", Sim->Dv());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'E' ):
        Serial.printf("Equalizing BatteryModel, and ekf if needed, to Battery\n");

        Serial.printf("\nMyBattModel:  soc=%7.3f, SOC=%7.3f, q=%7.3f, delta_q = %7.3f, q_scaled_rated = %7.3f,\
        q_rated = %7.3f, q_capacity = %7.3f,\n\n", Sim->soc(), Sim->SOC(), Sim->q(), Sim->delta_q(),
        Sim->q_cap_scaled(), Sim->q_cap_rated(), Sim->q_capacity());
        
        Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
        if ( rp.modeling ) Mon->init_soc_ekf(Sim->soc());
        Mon->init_hys(0.0);
        
        Serial.printf("\nMyBattModel:  soc=%7.3f, SOC=%7.3f, q=%7.3f, delta_q = %7.3f, q_scaled_rated = %7.3f,\
        q_rated = %7.3f, q_capacity = %7.3f,\n\n", Sim->soc(), Sim->SOC(), Sim->q(), Sim->delta_q(),
        Sim->q_cap_scaled(), Sim->q_cap_rated(), Sim->q_capacity());
        break;
      
      case ( 'S' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'c' ):
            scale = cp.input_string.substring(2).toFloat();
            rp.s_cap_model = scale;
        
            Serial.printf("Sim.q_cap_rated scaled by %7.3f from %7.3f to ", scale, Sim->q_cap_scaled());
        
            Sim->apply_cap_scale(rp.s_cap_model);
            if ( rp.modeling ) Mon->init_soc_ekf(Sim->soc());
        
            Serial.printf("%7.3f\n", Sim->q_cap_scaled());
            Serial.printf("Sim:  "); Sim->pretty_print(); Sim->Coulombs::pretty_print();
            break;
        
          case ( 'h' ):
            scale = cp.input_string.substring(2).toFloat();
            rp.hys_scale = scale;
            Serial.printf("\nBefore Mon::Hys::scale = %7.3f, Sim::Hys::scale = %7.3f\n",
                Mon->hys_scale(), Sim->hys_scale());
            Serial.printf("\nChanging to Sh= %7.3f\n", scale);
            Mon->hys_scale(scale);
            Sim->hys_scale(scale);
            Serial.printf("\nAfter Mon::Hys::scale = %7.3f, Sim::Hys::scale = %7.3f\n",
                Mon->hys_scale(), Sim->hys_scale());
            break;
        
          case ( 'r' ):
            scale = cp.input_string.substring(2).toFloat();
        
            Serial.printf("\nBefore Sim::StateSpace:\n"); Sim->pretty_print_ss();
            Serial.printf("\nScaling D[0, 0] = -r0 by Sr= %7.3f\n", scale);
        
            Sim->Sr(scale);
        
            Serial.printf("\nAfter Sim::StateSpace:\n"); Mon->pretty_print_ss();
            Serial.printf("\nBefore Mon::StateSpace:\n"); Mon->pretty_print_ss();
            Serial.printf("\nScaling D[0, 0] = -r0 by Sr= %7.3f\n", scale);
        
            Mon->Sr(scale);
        
            Serial.printf("\nAfter Mon::StateSpace:\n"); Mon->pretty_print_ss();
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
            self_talk("h", Mon, Sim, Sen, Twk_amp, Twk_noa);
            cp.cmd_summarize();
            self_talk("Pm", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            if ( rp.modeling )
            {
              Serial.printf("Sim:   rp.modeling = %d\n", rp.modeling);
              self_talk("Ps", Mon, Sim, Sen, Twk_amp, Twk_noa);
              Serial.printf("\n");
            }
            self_talk("Pr", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            print_all_summary(mySum, rp.isum, NSUM);
            self_talk("Q", Mon, Sim, Sen, Twk_amp, Twk_noa);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'i' ):
        Q_in = cp.input_string.substring(1).toFloat();
        Serial.printf("Amp infinite coulomb counter reset from %9.1f ", Twk_amp->delta_q_inf());
        Twk_amp->delta_q_inf(Q_in);
        Serial.printf("to %9.1f\n", Twk_amp->delta_q_inf());
        self_talk("Mp0.0", Mon, Sim, Sen, Twk_amp, Twk_noa);
        Serial.printf("No amp infinite coulomb counter reset from %9.1f ", Twk_noa->delta_q_inf());
        Twk_noa->delta_q_inf(Q_in);
        Serial.printf("to %9.1f\n", Twk_noa->delta_q_inf());
        self_talk("Np0.0", Mon, Sim, Sen, Twk_amp, Twk_noa);
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
          Mon->apply_soc(SOCS_in, Sen->Tbatt_filt);
          Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
          if ( rp.modeling ) Mon->init_soc_ekf(Sim->soc());
          Mon->update(&rp.delta_q, &rp.t_last);
          Sim->update(&rp.delta_q_model, &rp.t_last_model);

          Serial.printf("SOC=%7.3f, soc=%7.3f, modeling = %d, delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
              Mon->SOC(), Mon->soc(), rp.modeling, rp.delta_q, Sim->SOC(), Sim->soc(), rp.delta_q_model, Mon->soc_ekf());
        }

        else
          Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", SOCS_in);
        break;

      case ( 'M' ):
        switch ( cp.input_string.charAt(1) )
        {

          case ( 'C' ):
            Twk_amp->max_change(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->max_change_ changed to %10.6f\n", Twk_amp->max_change());
            break;

          case ( 'g' ):
            Twk_amp->gain(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->gain_ changed to %10.6f\n", Twk_amp->gain());
            break;

          case ( 'k' ):
            Twk_amp->tweak_bias(cp.input_string.substring(2).toFloat());
            Serial.printf("rp.tweak_bias changed to %7.3f\n", Twk_amp->tweak_bias());
            break;

          case ( 'P' ):
            Twk_amp->delta_q_sat_present(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->q_sat_present_ changed to %10.1f\n", Twk_amp->delta_q_sat_present());
            break;

          case ( 'p' ):
            Twk_amp->delta_q_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->q_sat_past_ changed to %10.1f\n", Twk_amp->delta_q_sat_past());
            break;

          case ( 'w' ):
            Twk_amp->time_to_wait(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->time_to_wait changed to %7.3f\n", Twk_amp->time_to_wait());
            break;

          case ( 'x' ):
            Twk_amp->max_tweak(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->max_tweak changed to %7.3f\n", Twk_amp->max_tweak());
            break;

          case ( 'z' ):
            Twk_amp->time_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_amp->time_sat_past changed to %7.3f\n", Twk_amp->time_sat_past());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'N' ):
        switch ( cp.input_string.charAt(1) )
        {

          case ( 'C' ):
            Twk_noa->max_change(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->max_change_ changed to %10.6f\n", Twk_noa->max_change());
            break;

          case ( 'g' ):
            Twk_noa->gain(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->gain_ changed to %10.6f\n", Twk_noa->gain());
            break;

          case ( 'k' ):
            Twk_noa->tweak_bias(cp.input_string.substring(2).toFloat());
            Serial.printf("rp.tweak_bias changed to %7.3f\n", Twk_noa->tweak_bias());
            break;

          case ( 'P' ):
            Twk_noa->delta_q_sat_present(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->q_sat_present_ changed to %10.1f\n", Twk_noa->delta_q_sat_present());
            break;

          case ( 'p' ):
            Twk_noa->delta_q_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->q_sat_past_ changed to %10.1f\n", Twk_noa->delta_q_sat_past());
            break;

          case ( 'w' ):
            Twk_noa->time_to_wait(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->time_to_wait changed to %7.3f\n", Twk_noa->time_to_wait());
            break;

          case ( 'x' ):
            Twk_noa->max_tweak(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->max_tweak changed to %7.3f\n", Twk_noa->max_tweak());
            break;

          case ( 'z' ):
            Twk_noa->time_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("Twk_noa->time_sat_past changed to %7.3f\n", Twk_noa->time_sat_past());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'n' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        if ( SOCS_in<1.1 )   // Apply crude limit to prevent user error
        {
          Sim->apply_soc(SOCS_in, Sen->Tbatt_filt);
          Sim->update(&rp.delta_q_model, &rp.t_last_model);
          if ( rp.modeling )
            Mon->init_soc_ekf(Mon->soc());

          Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
              Mon->SOC(), Mon->soc(), rp.delta_q, Sim->SOC(), Sim->soc(), rp.delta_q_model, Mon->soc_ekf());
        }
        else
          Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", SOCS_in);
        break;

      case ( 'O' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        Mon->apply_SOC(SOCS_in, Sen->Tbatt_filt);
        Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
        if ( rp.modeling ) Mon->init_soc_ekf(Sim->soc());
        Mon->update(&rp.delta_q, &rp.t_last);
        Sim->update(&rp.delta_q_model, &rp.t_last_model);

        Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
            Mon->SOC(), Mon->soc(), rp.delta_q, Sim->SOC(), Sim->soc(), rp.delta_q_model, Mon->soc_ekf());
        break;

      case ( 'P' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):
            self_talk("Pm", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            Serial.printf("Sim:   rp.modeling = %d\n", rp.modeling);
            self_talk("Ps", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            self_talk("Pr", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            self_talk("Pt", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            self_talk("PM", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            self_talk("PN", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("\n");
            break;

          case ( 'c' ):
            Serial.printf("Mon::"); Mon->Coulombs::pretty_print();
            Serial.printf("Sim::");   Sim->Coulombs::pretty_print();
            break;

          case ( 'e' ):
             Serial.printf("Mon::"); Mon->EKF_1x1::pretty_print();
            break;

          case ( 'm' ):
            Serial.printf("Mon:");  Mon->pretty_print();
            Serial.printf("Mon::"); Mon->Coulombs::pretty_print();
            Serial.printf("Mon::"); Mon->pretty_print_ss();
            Serial.printf("Mon::"); Mon->EKF_1x1::pretty_print();
            break;

          case ( 'M' ):
             Serial.printf("Tweak::"); Twk_amp->Tweak::pretty_print();
            break;

          case ( 'N' ):
             Serial.printf("Tweak::"); Twk_noa->Tweak::pretty_print();
            break;

          case ( 'r' ):
            Serial.printf("retained::");
            rp.print_part_1(cp.buffer); Serial.printf("%s", cp.buffer);
            rp.print_part_2(cp.buffer); Serial.printf("%s", cp.buffer);
            Serial.printf("command::");       cp.pretty_print();
            break;

          case ( 's' ):
            Serial.printf("Sim:   rp.modeling = %d\n", rp.modeling);
            Serial.printf("Sim:");    Sim->pretty_print();
            Serial.printf("Sim::");   Sim->Coulombs::pretty_print();
            Serial.printf("Sim::");   Sim->pretty_print_ss();
            break;

          case ( 't' ):
            Serial.printf("Mon::"); Mon->pretty_print_ss();
            Serial.printf("Sim::");   Sim->pretty_print_ss();
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
        Serial.printf("tb  = %7.3f,\nvb  = %7.3f,\nvoc_dyn = %7.3f,\nvoc_filt  = %7.3f,\nvsat = %7.3f,\nib  = %7.3f,\nsoc = %7.3f,\n\
soc_ekf= %7.3f,\nmodeling = %d,\namp delta_q_inf = %10.1f,\namp tweak_bias = %7.3f,\nno amp delta_q_inf = %10.1f,\nno amp tweak_bias = %7.3f,\n",
          Mon->temp_c(), Mon->vb(), Mon->voc_dyn(), Mon->voc_filt(), Mon->vsat(),
          Mon->ib(), Mon->soc(), Mon->soc_ekf(), rp.modeling, Twk_amp->delta_q_inf(), Twk_amp->tweak_bias(),
          Twk_noa->delta_q_inf(), Twk_noa->tweak_bias());
        break;

      case ( 'R' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'e' ):
            Serial.printf("Equalizing reset.   Just set delta_q in BattModel = delta_q in Batt\n");
            Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
            break;

          case ( 'h' ):
            Serial.printf("Resetting monitor hysteresis\n");
            Mon->init_hys(0.0);
            Serial.printf("Resetting model hysteresis\n");
            Sim->init_hys(0.0);
            break;

          case ( 'i' ):
            self_talk("i0", Mon, Sim, Sen, Twk_amp, Twk_noa);
            break;

          case ( 'M' ):
            Serial.printf("Resetting amp tweaker\n");
            Twk_amp->reset();
            Twk_amp->tweak_bias(0.);
            self_talk("PM", Mon, Sim, Sen, Twk_amp, Twk_noa);
            break;

          case ( 'N' ):
            Serial.printf("Resetting no amp tweaker\n");
            Twk_noa->reset();
            Twk_noa->tweak_bias(0.);
            self_talk("PN", Mon, Sim, Sen, Twk_amp, Twk_noa);
            break;

          case ( 'r' ):
            Serial.printf("Small reset.   Just reset all to soc=1.0 and delta_q = 0\n");
            Sim->apply_soc(1.0, Sen->Tbatt_filt);
            Sim->init_hys(0.0);
            Mon->apply_soc(1.0, Sen->Tbatt_filt);
            if ( rp.modeling )
            {
              Mon->init_soc_ekf(Sim->soc());
              Mon->init_hys(0.0);
            }
            break;

          case ( 'R' ):
            self_talk("Rr", Mon, Sim, Sen, Twk_amp, Twk_noa);
            Serial.printf("also large and soft reset.   Initialize all variables to clean run without model at saturation.   Ready to use\n");
            rp.large_reset();
            cp.large_reset();
            cp.cmd_reset();
            self_talk("Hs", Mon, Sim, Sen, Twk_amp, Twk_noa);
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
        talk(Mon, Sim, Sen, Twk_amp, Twk_noa);
        break;

      case ( 'v' ):
        rp.debug = cp.input_string.substring(1).toInt();
        break;

      case ( 'W' ):
        SOCS_in = cp.input_string.substring(1).toFloat();
        Sim->apply_SOC(SOCS_in, Sen->Tbatt_filt);
        Sim->update(&rp.delta_q_model, &rp.t_last_model);
        if ( rp.modeling )
          Mon->init_soc_ekf(Mon->soc());

        Serial.printf("SOC=%7.3f, soc=%7.3f,   delta_q=%7.3f, SOC_model=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f, soc_ekf=%7.3f,\n",
            Mon->SOC(), Mon->soc(), rp.delta_q, Sim->SOC(), Sim->soc(), rp.delta_q_model, Mon->soc_ekf());
        break;

      case ( 'w' ): 
        cp.enable_wifi = !cp.enable_wifi; // not remembered in rp. Photon reset turns this false.
        Serial.printf("Wifi toggled to %d\n", cp.enable_wifi);
        break;

      case ( 'X' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'd' ):
            if ( cp.input_string.substring(2).toInt()>0 )
              cp.dc_dc_on = true;
            else
              cp.dc_dc_on = false;
            Serial.printf("dc_dc_on set to %d\n", cp.dc_dc_on);
            break;

          case ( 'x' ):
            if ( cp.input_string.substring(2).toInt()>0 )
            {
              rp.modeling = true;
              Mon->init_soc_ekf(Sim->soc());
            }
            else
              rp.modeling = false;
            Serial.printf("Modeling set to %d\n", rp.modeling);
            if ( cp.input_string.substring(2).toInt()>1 )
              rp.tweak_test = true;
            else
              rp.tweak_test = false;
            Serial.printf("tweak_test set to %d\n", rp.tweak_test);
            break;

          case ( 'a' ):
            // rp.amp = max(min(cp.input_string.substring(2).toFloat(), 18.3), 0.0);
            rp.amp = cp.input_string.substring(2).toFloat();
            Serial.printf("Modeling injected amp set to %7.3f and inj_soft_bias set to %7.3f\n", rp.amp, rp.inj_soft_bias);
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
              case ( 'o' ):
                rp.type = 8;
                Serial.printf("Setting waveform to cosine.  rp.type = %d\n", rp.type);
                break;

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
            rp.inj_soft_bias = max(min(cp.input_string.substring(2).toFloat(), 18.3), -18.3);
            Serial.printf("Modeling injected inj_soft_bias set to %7.3f\n", rp.inj_soft_bias);
            break;

          case ( 'p' ):
            switch ( cp.input_string.substring(2).toInt() )
            {

              case ( -1 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                self_talk("m0.5", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.modeling = false;
                rp.debug = -12;   // myDisplay = 2
                break;

              case ( 0 ):
                rp.modeling = true;
                rp.type = 0;
                rp.freq = 0.0;
                rp.amp = 0.0;
                if ( !rp.tweak_test ) rp.inj_soft_bias = 0.0;
                rp.curr_bias_all = 0;
                rp.debug = -12;   // myDisplay = 5
                break;

              case ( 1 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                self_talk("m0.5", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 1;
                rp.freq = 0.05;
                rp.amp = 6.;
                if ( !rp.tweak_test ) rp.inj_soft_bias = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
                break;

              case ( 2 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                self_talk("m0.5", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 2;
                rp.freq = 0.10;
                rp.amp = 6.;
                if ( !rp.tweak_test ) rp.inj_soft_bias = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
                break;

              case ( 3 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                self_talk("m0.5", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 3;
                rp.freq = 0.05;
                rp.amp = 6.;
                if ( !rp.tweak_test ) rp.inj_soft_bias = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
                break;

              case ( 4 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 4;
                rp.curr_bias_all = -RATED_BATT_CAP;  // Software effect only
                rp.debug = -12;
                break;

              case ( 5 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 5;
                rp.curr_bias_all = RATED_BATT_CAP; // Software effect only
                rp.debug = -12;
                break;

              case ( 6 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 6;
                rp.amp = RATED_BATT_CAP*0.2;
                rp.debug = -12;
                break;

              case ( 7 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 7;
                self_talk("Xx1", Mon, Sim, Sen, Twk_amp, Twk_noa);    // Run to model
                self_talk("m0.5", Mon, Sim, Sen, Twk_amp, Twk_noa);   // Set all soc=0.5
                self_talk("n0.987", Mon, Sim, Sen, Twk_amp, Twk_noa); // Set model only to near saturation
                self_talk("v2", Mon, Sim, Sen, Twk_amp, Twk_noa);     // Watch sat, soc, and voc vs v_sat
                rp.amp = RATED_BATT_CAP*0.2;              // Hard current charge
                Serial.printf("Run 'n<val> as needed to init south of sat.  Reset this whole thing by running 'Xp-1'\n");
                break;

              case ( 8 ):
                self_talk("Xp0", Mon, Sim, Sen, Twk_amp, Twk_noa);
                self_talk("m0.5", Mon, Sim, Sen, Twk_amp, Twk_noa);
                rp.type = 8;
                rp.freq = 0.05;
                rp.amp = 6.;
                if ( !rp.tweak_test ) rp.inj_soft_bias = -rp.amp;
                rp.debug = -12;
                rp.freq *= (2. * PI);
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
        talkH(Mon, Sim, Sen, Twk_amp, Twk_noa);
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
void talkH(BatteryMonitor *Mon, BatteryModel *Sim, Sensors *Sen, Tweak *Twk_amp, Tweak *Twk_noa)
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");

  Serial.printf("A=  nominalize the rp structure for clean boots etc'\n");

  Serial.printf("H<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump the summary log to screen\n");
  Serial.printf("  HR= "); Serial.printf("reset the summary log\n");
  Serial.printf("  Hs= "); Serial.printf("save a data point to summary log and print log and status to screen\n");

  Serial.printf("m=  assign curve charge state in fraction to all versions including model- '(0-1.1)'\n"); 
  Serial.printf("n=  assign curve charge state in fraction to model only (ekf if modeling)- '(0-1.1)'\n"); 
  Serial.printf("O=  assign a CHARGE state in percent to all versions including model- '('truncated 0-100')'\n"); 
  Serial.printf("s   curr signal select (0=amp preferred, 1=noamp) = "); Serial.println(rp.curr_sel_noamp);
  Serial.printf("v=  "); Serial.print(rp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("    -<>:   Negative - Arduino plot compatible\n");
  Serial.printf("     +2:   General purpose\n");
  Serial.printf("   +/-5:   Charge time\n");
  Serial.printf("     +6:   Vectoring\n");
  Serial.printf("    -11:   Summary\n");
  Serial.printf("  +/-12:   EKF summary\n");
  Serial.printf("  +/-14:   vshunt and ishunt raw\n");
  Serial.printf("    +15:   vb raw\n");
  Serial.printf("    +33:   state-space\n");
  Serial.printf("  +/-34:   EKF detailed\n");
  Serial.printf("    +35:   Randles balance\n");
  Serial.printf("  +/-37:   EKF short\n");
  Serial.printf("    +76:   vb model\n");
  Serial.printf("  +/-78:   Battery model saturation\n");
  Serial.printf("    +79:   sat_ib model\n");
  Serial.printf("  +/-96:   CC saturation\n");
  Serial.printf("  +/-97:   CC model saturation\n");
  Serial.printf("W=  assign a CHARGE state in percent to model only (ekf if modeling)-- '('truncated 0-100')'\n"); 
  
  Serial.printf("D/S<?> Adjustments.   For example:\n");
  Serial.printf("  Da= "); Serial.printf("%7.3f", rp.curr_bias_amp); Serial.println("    : delta I adder to sensed amplified shunt current, A [0]"); 
  Serial.printf("  Db= "); Serial.printf("%7.3f", rp.curr_bias_noamp); Serial.println("    : delta I adder to sensed shunt current, A [0]"); 
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.curr_bias_all); Serial.println("    : delta I adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.println("    : delta V adder to sensed battery voltage, V [0]"); 
  Serial.printf("  Dt= "); Serial.printf("%7.3f", rp.t_bias); Serial.println("    : delta T adder to sensed Tbatt, deg C [0]"); 
  Serial.printf("  Dv= "); Serial.print(Sim->Dv()); Serial.println("    : delta V adder to Vb measurement, V"); 
  Serial.printf("  Sc= "); Serial.print(Sim->q_capacity()/Mon->q_capacity()); Serial.println("    : Scalar battery model size"); 
  Serial.printf("  Sh= "); Serial.printf("%7.3f", rp.hys_scale); Serial.println("    : hysteresis scalar 1e-6 - 100");
  Serial.printf("  Sr= "); Serial.print(Sim->Sr()); Serial.println("    : Scalar resistor for battery dynamic calculation, V"); 
  Serial.printf("  Sk= "); Serial.print(rp.cutback_gain_scalar); Serial.println("    : Saturation of model cutback gain scalar"); 

  Serial.printf("E=  set the BatteryModel delta_q to same value as Battery'\n"); 

  Serial.printf("i=,<inp> set the BatteryMonitor delta_q_inf to <inp> (-360000 - 3600000'\n"); 

  Serial.printf("M<?> Amp tweaks.   For example:\n");
  Serial.printf("  MC= "); Serial.printf("%7.3f", Twk_amp->max_change()); Serial.println("    : tweak amp max change allowed, A [0.05]"); 
  Serial.printf("  Mg= "); Serial.printf("%7.6f", Twk_amp->gain()); Serial.println("    : tweak amp gain = correction to be made for charge, A/Coulomb [0.0001]"); 
  Serial.printf("  Mi= "); Serial.printf("%7.3f", Twk_amp->delta_q_inf()); Serial.println("    : tweak amp value for state of infinite counter, C [varies]"); 
  Serial.printf("  Mk= "); Serial.printf("%7.3f", Twk_amp->tweak_bias()); Serial.println("    : tweak amp adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Mp= "); Serial.printf("%10.1f", Twk_amp->delta_q_sat_past()); Serial.println("    : tweak amp past charge infinity at sat, C [varies]"); 
  Serial.printf("  MP= "); Serial.printf("%10.1f", Twk_amp->delta_q_sat_present()); Serial.println("    : tweak amp present charge infinity at sat, C [varies]"); 
  Serial.printf("  Mw= "); Serial.printf("%7.3f", Twk_amp->time_to_wait()); Serial.println("    : tweak amp time to wait for next tweak, hr [18]]"); 
  Serial.printf("  Mx= "); Serial.printf("%7.3f", Twk_amp->max_tweak()); Serial.println("    : tweak amp adder maximum, A [1]"); 
  Serial.printf("  Mz= "); Serial.printf("%7.3f", Twk_amp->time_sat_past()); Serial.println("    : tweak amp time since last tweak, hr [varies]"); 


  Serial.printf("N<?> No amp tweaks.   For example:\n");
  Serial.printf("  NC= "); Serial.printf("%7.3f", Twk_noa->max_change()); Serial.println("    : tweak no amp max change allowed, A [0.05]"); 
  Serial.printf("  Ng= "); Serial.printf("%7.6f", Twk_noa->gain()); Serial.println("    : tweak no amp gain = correction to be made for charge, A/Coulomb [0.0001]"); 
  Serial.printf("  Ni= "); Serial.printf("%7.3f", Twk_noa->delta_q_inf()); Serial.println("    : tweak no amp value for state of infinite counter, C [varies]"); 
  Serial.printf("  Nk= "); Serial.printf("%7.3f", Twk_noa->tweak_bias()); Serial.println("    : tweak no amp adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Np= "); Serial.printf("%10.1f", Twk_noa->delta_q_sat_past()); Serial.println("    : tweak no amp past charge infinity at sat, C [varies]"); 
  Serial.printf("  NP= "); Serial.printf("%10.1f", Twk_noa->delta_q_sat_present()); Serial.println("    : tweak no amp present charge infinity at sat, C [varies]"); 
  Serial.printf("  Nw= "); Serial.printf("%7.3f", Twk_noa->time_to_wait()); Serial.println("    : tweak no amp time to wait for next tweak, hr [18]]"); 
  Serial.printf("  Nx= "); Serial.printf("%7.3f", Twk_noa->max_tweak()); Serial.println("    : tweak no amp adder maximum, A [1]"); 
  Serial.printf("  Nz= "); Serial.printf("%7.3f", Twk_noa->time_sat_past()); Serial.println("    : tweak no amp time since last tweak, hr [varies]"); 

  Serial.printf("P<?>   Print Battery values\n");
  Serial.printf("  Pa= "); Serial.printf("print all\n");
  Serial.printf("  Pc= "); Serial.printf("print all coulombs\n");
  Serial.printf("  Pe= "); Serial.printf("print ekf\n");
  Serial.printf("  Pm= "); Serial.printf("print monitor\n");
  Serial.printf("  PM= "); Serial.printf("print amp tweak\n");
  Serial.printf("  PN= "); Serial.printf("print no amp tweak\n");
  Serial.printf("  Pr= "); Serial.printf("print retained and command parameters\n");
  Serial.printf("  Ps= "); Serial.printf("print simulation\n");
  Serial.printf("  Pt= "); Serial.printf("print all state-space\n");
  Serial.printf("  Px= "); Serial.printf("print current signal selection\n");
  Serial.printf("  Pv= "); Serial.printf("print voltage signal details\n");

  Serial.printf("Q      print vital stats\n");

  Serial.printf("R<?>   Reset\n");
  Serial.printf("  Re= "); Serial.printf("equalize delta_q in model to battery monitor\n");
  Serial.printf("  Rh= "); Serial.printf("reset monitor and model hysteresis to 0.0\n");
  Serial.printf("  Ri= "); Serial.printf("reset delta_q_inf to 0.0\n");
  Serial.printf("  RM= "); Serial.printf("reset amp tweaker and tweak_bias to 0.0\n");
  Serial.printf("  RN= "); Serial.printf("reset no amp tweaker and tweak_bias to 0.0\n");
  Serial.printf("  Rr= "); Serial.printf("saturate battery monitor and equalize model to monitor\n");
  Serial.printf("  RR= "); Serial.printf("saturate, equalize, and nominalize all testing for DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small reset.  reset flags to reinitialize filters\n");

  Serial.printf("T<new cmd>  Send in a new command.  Used to test calling Talk from itself.   For Example:\n");
  Serial.printf("  Tv=-78  sends v=-78 to talk\n");

  Serial.printf("w   turn on wifi = "); Serial.println(cp.enable_wifi);

  Serial.printf("X<?> - Test Mode.   For example:\n");
  Serial.printf("  Xd= "); Serial.printf("%d,   dc-dc charger on [0]\n", cp.dc_dc_on);
  Serial.printf("  Xx= "); Serial.printf("%d,   use model for Vbatt(1) and tweak_test(2) [0]\n", rp.modeling + rp.tweak_test);
  Serial.printf("  Xa= "); Serial.printf("%7.3f", rp.amp); Serial.println("  : Injection amplitude A pk (0-18.3) [0]");
  Serial.printf("  Xf= "); Serial.printf("%7.3f", rp.freq/2./PI); Serial.println("  : Injection frequency Hz (0-2) [0]");
  Serial.printf("  Xt= "); Serial.printf("%d", rp.type); Serial.println("  : Injection type.  'c', 's', 'q', 't' (cosine, sine, square, triangle)");
  Serial.printf("  Xo= "); Serial.printf("%7.3f", rp.inj_soft_bias); Serial.println("  : Injection inj_soft_bias A (-18.3-18.3) [0]");
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
  Serial.printf("       8:  1 Hz cosine centered at 0 with largest supported amplitude\n");

  Serial.printf("h   this menu\n");
}

// Recursion
void self_talk(const String cmd, BatteryMonitor *Mon, BatteryModel *Sim, Sensors *Sen, Tweak *Twk_amp, Tweak *Twk_noa)
{
  cp.input_string = cmd;
  Serial.printf("self_talk:  new string = '%s'\n", cp.input_string.c_str());
  cp.string_complete = true;
  talk(Mon, Sim, Sen, Twk_amp, Twk_noa);
}
