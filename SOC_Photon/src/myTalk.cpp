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

// Talk Executive
void talk(BatteryMonitor *Mon, Sensors *Sen)
{
  double FP_in = -99.;
  int INT_in = -1;
  double scale = 1.;
  double Q_in = 0.;
  // Serial event  (terminate Send String data with 0A using CoolTerm)
  if (cp.string_complete)
  {
    switch ( cp.input_string.charAt(0) )
    {
      case ( 'B' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'm' ):  // Monitor chemistry change
            INT_in = cp.input_string.substring(2).toInt();
            switch ( INT_in )
            {
              case ( 0 ):
                Serial.printf("Changing monitor chemistry from %d", Mon->mod_code());
                Mon->assign_mod("Battleborn");
                Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_rand();
                cp.cmd_reset();
                break;

              case ( 1 ):
                Serial.printf("Changing monitor chemistry from %d", Mon->mod_code());
                Mon->assign_mod("LION");
                Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_rand();
                cp.cmd_reset();
                break;

              default:
                Serial.printf("%d unknown.  Try typing 'h'", INT_in);
            }
            break;

          case ( 's' ):  // Simulation chemistry change
            INT_in = cp.input_string.substring(2).toInt();
            switch ( INT_in )
            {
              case ( 0 ):
                Serial.printf("Changing simulation chemistry from %d", Sen->Sim->mod_code());
                Sen->Sim->assign_mod("Battleborn"); Sen->Sim->assign_rand();
                Serial.printf(" to %d ('Battleborn')\n", Sen->Sim->mod_code());
                cp.cmd_reset();
                break;

              case ( 1 ):
                Serial.printf("Changing simulation chemistry from %d", Sen->Sim->mod_code());
                Sen->Sim->assign_mod("LION"); Sen->Sim->assign_rand();
                Serial.printf(" to %d ('LION')\n", Sen->Sim->mod_code());
                cp.cmd_reset();
                break;

              default:
                Serial.printf("%d unknown.  Try typing 'h'", INT_in);
            }
            break;

          case ( 'P' ):  // Number of parallel batteries in bank, e.g. '2P1S'
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in>0 )  // Apply crude limit to prevent user error
            {
              Serial.printf("Changing Mon/Sim->nP from %5.2f / %5.2f ", Mon->nP(), Sen->Sim->nP());
              rp.nP = FP_in;
              Serial.printf("to %5.2f / %5.2f\n", Mon->nP(), Sen->Sim->nP());
            }
            else
              Serial.printf("nP out of range.  You entered %5.2f; must be >0.\n", FP_in);
            break;

          case ( 'S' ):  // Number of series batteries in bank, e.g. '2P1S'
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in>0 )  // Apply crude limit to prevent user error
            {
              Serial.printf("Changing Mon/Sim->nS from %5.2f / %5.2f ", Mon->nS(), Sen->Sim->nS());
              rp.nS = FP_in;
              Serial.printf("to %5.2f / %5.2f\n", Mon->nS(), Sen->Sim->nS());
            }
            else
              Serial.printf("nP out of range.  You entered %5.2f; must be >0.\n", FP_in);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'C' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):  // assign charge state in fraction to all versions including model
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in<1.1 )  // Apply crude limit to prevent user error
            {
              Mon->apply_soc(FP_in, Sen->Tbatt_filt);
              Sen->Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
              Serial.printf("soc=%7.3f, modeling = %d, delta_q=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f,\n",
                  Mon->soc(), rp.modeling, rp.delta_q, Sen->Sim->soc(), rp.delta_q_model);
              cp.cmd_reset();
            }
            else
              Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'A' instead of 'a'?\n", FP_in);
            break;

          case ( 'm' ):  // assign curve charge state in fraction to model only (ekf if modeling)
            FP_in = cp.input_string.substring(2).toFloat();
            if ( FP_in<1.1 )   // Apply crude limit to prevent user error
            {
              Sen->Sim->apply_soc(FP_in, Sen->Tbatt_filt);
              Serial.printf("soc=%7.3f,   delta_q=%7.3f, soc_model=%7.3f,   delta_q_model=%7.3f,\n",
                  Mon->soc(), rp.delta_q, Sen->Sim->soc(), rp.delta_q_model);
              cp.cmd_reset();
            }
            else
              Serial.printf("soc out of range.  You entered %7.3f; must be 0-1.1.  Did you mean to use 'M' instead of 'm'?\n", FP_in);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'D' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):
            Serial.printf("rp.ibatt_bias_amp from %7.3f to ", rp.ibatt_bias_amp);
            rp.ibatt_bias_amp = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.ibatt_bias_amp);
            break;

          case ( 'b' ):
            Serial.printf("rp.ibatt_bias_noamp from %7.3f to ", rp.ibatt_bias_noamp);
            rp.ibatt_bias_noamp = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.ibatt_bias_noamp);
            break;

          case ( 'c' ):
            Serial.printf("rp.vbatt_bias from %7.3f to ", rp.vbatt_bias);
            rp.vbatt_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.vbatt_bias);
            break;

          case ( 'i' ):
            Serial.printf("rp.ibatt_bias_all from %7.3f to ", rp.ibatt_bias_all);
            rp.ibatt_bias_all = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.ibatt_bias_all);
            break;

          case ( 'n' ):
            FP_in = cp.input_string.substring(2).toFloat();
            Serial.printf("coulombic efficiency from %7.4f,%7.4f,%7.4f,%7.4f, to ", Sen->Sim->coul_eff(), Mon->coul_eff(), Sen->ShuntAmp->coul_eff(), Sen->ShuntNoAmp->coul_eff());
            Sen->Sim->coul_eff(FP_in);
            Mon->coul_eff(FP_in);
            Sen->ShuntAmp->coul_eff(FP_in);
            Sen->ShuntNoAmp->coul_eff(FP_in);
            Serial.printf("%7.4f,%7.4f,%7.4f,%7.4f\n", Sen->Sim->coul_eff(), Mon->coul_eff(), Sen->ShuntAmp->coul_eff(), Sen->ShuntNoAmp->coul_eff());
            break;

          case ( 't' ):
            Serial.printf("rp.tbatt_bias from %7.3f to ", rp.tbatt_bias);
            rp.tbatt_bias = cp.input_string.substring(2).toFloat();
            Serial.printf("%7.3f\n", rp.tbatt_bias);
            rp.debug = 0;
            Serial.printf("**************** reset ****\n");
            break;

          case ( 'v' ):
            Serial.printf("Mon.Dv from %7.3f to ", Mon->Dv());
            Mon->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Mon->Dv());
            Serial.printf("Sim.Dv from %7.3f to ", Sen->Sim->Dv());
            Sen->Sim->Dv(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->Sim->Dv());
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
        
            Serial.printf("Sim.q_cap_rated scaled by %7.3f from %7.3f to ", scale, Sen->Sim->q_cap_scaled());
        
            Sen->Sim->apply_cap_scale(rp.s_cap_model);
            if ( rp.modeling ) Mon->init_soc_ekf(Sen->Sim->soc());
        
            Serial.printf("%7.3f\n", Sen->Sim->q_cap_scaled());
            Serial.printf("Sim:  "); Sen->Sim->pretty_print(); Sen->Sim->Coulombs::pretty_print();
            break;
        
          case ( 'h' ):
            scale = cp.input_string.substring(2).toFloat();
            Serial.printf("\nBefore Mon::Hys::scale = %7.3f, Sim::Hys::scale = %7.3f\n", Mon->hys_scale(), Sen->Sim->hys_scale());
            rp.hys_scale = scale;
            Serial.printf("Changing to Sh= %7.3f\n", scale);
            Mon->hys_scale(scale);
            Sen->Sim->hys_scale(scale);
            Serial.printf("After Mon::Hys::scale = %7.3f, Sim::Hys::scale = %7.3f\n",
                Mon->hys_scale(), Sen->Sim->hys_scale());
            break;
        
          case ( 'r' ):
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
            self_talk("h", Mon, Sen);
            cp.cmd_summarize();
            self_talk("Pm", Mon, Sen);
            Serial.printf("\n");
            if ( rp.modeling )
            {
              Serial.printf("Sim:   rp.modeling = %d\n", rp.modeling);
              self_talk("Ps", Mon, Sen);
              Serial.printf("\n");
            }
            self_talk("Pr", Mon, Sen);
            Serial.printf("\n");
            print_all_summary(mySum, rp.isum, NSUM);
            self_talk("Q", Mon, Sen);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'i' ):
        Q_in = cp.input_string.substring(1).toFloat();
        Serial.printf("Amp infinite coulomb counter reset from %9.1f ", Sen->ShuntAmp->delta_q_inf());
        Sen->ShuntAmp->delta_q_inf(Q_in);
        Serial.printf("to %9.1f\n", Sen->ShuntAmp->delta_q_inf());
        self_talk("Mp0.0", Mon, Sen);
        Serial.printf("No amp infinite coulomb counter reset from %9.1f ", Sen->ShuntNoAmp->delta_q_inf());
        Sen->ShuntNoAmp->delta_q_inf(Q_in);
        Serial.printf("to %9.1f\n", Sen->ShuntNoAmp->delta_q_inf());
        self_talk("Np0.0", Mon, Sen);
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

      case ( 'M' ):
        switch ( cp.input_string.charAt(1) )
        {

          case ( 'C' ):
            Serial.printf("Sen->ShuntAmp->max_change_ from %10.6f to ", Sen->ShuntAmp->max_change());
            Sen->ShuntAmp->max_change(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.6f\n", Sen->ShuntAmp->max_change());
            break;

          case ( 'g' ):
            Serial.printf("Sen->ShuntAmp->gain_ from %10.6f to ", Sen->ShuntAmp->gain());
            Sen->ShuntAmp->gain(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.6f\n", Sen->ShuntAmp->gain());
            break;

          case ( 'k' ):
            Serial.printf("rp.tweak_bias from %7.3f to ", Sen->ShuntAmp->tweak_bias());
            Sen->ShuntAmp->tweak_bias(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->tweak_bias());
            break;

          case ( 'P' ):
            Serial.printf("Sen->ShuntAmp->q_sat_present_ from %10.1f to ", Sen->ShuntAmp->delta_q_sat_present());
            Sen->ShuntAmp->delta_q_sat_present(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.1f\n", Sen->ShuntAmp->delta_q_sat_present());
            break;

          case ( 'p' ):
            Serial.printf("Sen->ShuntAmp->q_sat_past_ from %10.1f to ", Sen->ShuntAmp->delta_q_sat_past());
            Sen->ShuntAmp->delta_q_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.1f\n", Sen->ShuntAmp->delta_q_sat_past());
            break;

          case ( 'w' ):
            Serial.printf("Sen->ShuntAmp->time_to_wait from %7.3f to ", Sen->ShuntAmp->time_to_wait());
            Sen->ShuntAmp->time_to_wait(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->time_to_wait());
            break;

          case ( 'x' ):
            Serial.printf("Sen->ShuntAmp->max_tweak from %7.3f to ", Sen->ShuntAmp->max_tweak());
            Sen->ShuntAmp->max_tweak(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->max_tweak());
            break;

          case ( 'z' ):
            Serial.printf("Sen->ShuntAmp->time_sat_past from %7.3f to ", Sen->ShuntAmp->time_sat_past());
            Sen->ShuntAmp->time_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntAmp->time_sat_past());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'N' ):
        switch ( cp.input_string.charAt(1) )
        {

          case ( 'C' ):
            Serial.printf("Sen->ShuntNoAmp->max_change_ from %10.6f to ", Sen->ShuntNoAmp->max_change());
            Sen->ShuntNoAmp->max_change(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.6f\n", Sen->ShuntNoAmp->max_change());
            break;

          case ( 'g' ):
            Serial.printf("Sen->ShuntNoAmp->gain_ from %10.6f to ", Sen->ShuntNoAmp->gain());
            Sen->ShuntNoAmp->gain(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.6f\n", Sen->ShuntNoAmp->gain());
            break;

          case ( 'k' ):
            Serial.printf("rp.tweak_bias from %7.3f to ", Sen->ShuntNoAmp->tweak_bias());
            Sen->ShuntNoAmp->tweak_bias(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->tweak_bias());
            break;

          case ( 'P' ):
            Serial.printf("Sen->ShuntNoAmp->q_sat_present_ from %10.1f to ", Sen->ShuntNoAmp->delta_q_sat_present());
            Sen->ShuntNoAmp->delta_q_sat_present(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.1f\n", Sen->ShuntNoAmp->delta_q_sat_present());
            break;

          case ( 'p' ):
            Serial.printf("Sen->ShuntNoAmp->q_sat_past_ from %10.1f to ", Sen->ShuntNoAmp->delta_q_sat_past());
            Sen->ShuntNoAmp->delta_q_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("%10.1f\n", Sen->ShuntNoAmp->delta_q_sat_past());
            break;

          case ( 'w' ):
            Serial.printf("Sen->ShuntNoAmp->time_to_wait from %7.3f to ", Sen->ShuntNoAmp->time_to_wait());
            Sen->ShuntNoAmp->time_to_wait(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->time_to_wait());
            break;

          case ( 'x' ):
            Serial.printf("Sen->ShuntNoAmp->max_tweak from %7.3f to ", Sen->ShuntNoAmp->max_tweak());
            Sen->ShuntNoAmp->max_tweak(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->max_tweak());
            break;

          case ( 'z' ):
            Serial.printf("Sen->ShuntNoAmp->time_sat_past from %7.3f to ", Sen->ShuntNoAmp->time_sat_past());
            Sen->ShuntNoAmp->time_sat_past(cp.input_string.substring(2).toFloat());
            Serial.printf("%7.3f\n", Sen->ShuntNoAmp->time_sat_past());
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'P' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'a' ):
            self_talk("Pm", Mon, Sen);
            Serial.printf("\nSim:   rp.modeling = %d\n", rp.modeling);
            self_talk("Ps", Mon, Sen);
            self_talk("Pr", Mon, Sen);
            self_talk("Pt", Mon, Sen);
            self_talk("PM", Mon, Sen);
            self_talk("PN", Mon, Sen);
            break;

          case ( 'c' ):
            Serial.printf("\nMon::"); Mon->Coulombs::pretty_print();
            Serial.printf("\nSim::");   Sen->Sim->Coulombs::pretty_print();
            break;

          case ( 'e' ):
             Serial.printf("\nMon::"); Mon->EKF_1x1::pretty_print();
            break;

          case ( 'm' ):
            Serial.printf("\nMon:");  Mon->pretty_print();
            Serial.printf("Mon::"); Mon->Coulombs::pretty_print();
            Serial.printf("Mon::"); Mon->pretty_print_ss();
            Serial.printf("Mon::"); Mon->EKF_1x1::pretty_print();
            break;

          case ( 'M' ):
             Serial.printf("\nTweak::"); Sen->ShuntAmp->pretty_print();
            break;

          case ( 'N' ):
             Serial.printf("\nTweak::"); Sen->ShuntNoAmp->pretty_print();
            break;

          case ( 'r' ):
            Serial.printf("\n"); rp.pretty_print();
            Serial.printf("\n"); cp.pretty_print();
            break;

          case ( 's' ):
            Serial.printf("\nSim:   rp.modeling = %d\n", rp.modeling);
            Serial.printf("Sim:");    Sen->Sim->pretty_print();
            Serial.printf("Sim::");   Sen->Sim->Coulombs::pretty_print();
            Serial.printf("Sim::");   Sen->Sim->pretty_print_ss();
            break;

          case ( 't' ):
            Serial.printf("\nMon::"); Mon->pretty_print_ss();
            Serial.printf("\nSim::"); Sen->Sim->pretty_print_ss();
            break;

          case ( 'x' ):
            Serial.printf("\nAmp:   "); Serial.printf("Vshunt_int, Vshunt, cp.ibatt_bias, Ishunt_cal=, %d, %7.3f, %7.3f, %7.3f,\n", 
              Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), cp.ibatt_bias_amp, Sen->ShuntAmp->ishunt_cal());
            Serial.printf("No Amp:"); Serial.printf("Vshunt_int, Vshunt, cp.ibatt_bias, Ishunt_cal=, %d, %7.3f, %7.3f, %7.3f,\n", 
              Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), cp.ibatt_bias_noamp, Sen->ShuntNoAmp->ishunt_cal());
            Serial.printf("Selected:  NoAmp,Ibatt=,  %d, %7.3f\n", rp.ibatt_sel_noamp, Sen->Ibatt);
            break;

          case ( 'v' ):
            Serial.printf("\nVolt:   ");      Serial.printf("rp.vbatt_bias, Vbatt_model, rp.modeling, Vbatt=, %7.3f, %7.3f, %d, %7.3f,\n", 
              rp.vbatt_bias, Sen->Vbatt_model, rp.modeling, Sen->Vbatt);
            break;

          default:
            Serial.println("");Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'Q' ):
        Serial.printf("tb  = %7.3f,\nvb  = %7.3f,\nvoc_dyn = %7.3f,\nvoc_filt  = %7.3f,\nvsat = %7.3f,\nib  = %7.3f,\nsoc = %7.3f,\n\
soc_ekf= %7.3f,\nmodeling = %d,\namp delta_q_inf = %10.1f,\namp tweak_bias = %7.3f,\nno amp delta_q_inf = %10.1f,\nno amp tweak_bias = %7.3f,\n",
          Mon->temp_c(), Mon->Vb(), Mon->voc_dyn(), Mon->Voc_filt(), Mon->Vsat(),
          Mon->Ib(), Mon->soc(), Mon->soc_ekf(), rp.modeling, Sen->ShuntAmp->delta_q_inf(), Sen->ShuntAmp->tweak_bias(),
          Sen->ShuntNoAmp->delta_q_inf(), Sen->ShuntNoAmp->tweak_bias());
        break;

      case ( 'R' ):
        switch ( cp.input_string.charAt(1) )
        {
          case ( 'e' ):
            Serial.printf("Equalizing delta_q in BattModel = delta_q in Batt\n");
            Sen->Sim->apply_delta_q_t(Mon->delta_q(), Sen->Tbatt_filt);
            break;

          case ( 'h' ):
            Serial.printf("Resetting monitor hysteresis\n");
            Mon->init_hys(0.0);
            Serial.printf("Resetting model hysteresis\n");
            Sen->Sim->init_hys(0.0);
            break;

          case ( 'i' ):
            self_talk("i0", Mon, Sen);
            break;

          case ( 'M' ):
            Serial.printf("Resetting amp tweaker\n");
            Sen->ShuntAmp->reset();
            Sen->ShuntAmp->tweak_bias(0.);
            self_talk("PM", Mon, Sen);
            break;

          case ( 'N' ):
            Serial.printf("Resetting no amp tweaker\n");
            Sen->ShuntNoAmp->reset();
            Sen->ShuntNoAmp->tweak_bias(0.);
            self_talk("PN", Mon, Sen);
            break;

          case ( 'r' ):
            Serial.printf("Small reset all to soc=1.0 and delta_q = 0\n");
            Sen->Sim->apply_soc(1.0, Sen->Tbatt_filt);
            Mon->apply_soc(1.0, Sen->Tbatt_filt);
            cp.cmd_reset();
            break;

          case ( 'R' ):
            Serial.printf("Large reset\n");
            self_talk("Rr", Mon, Sen);
            Serial.printf("also large and soft reset. Clean run all hardware. Ready to use\n");
            rp.large_reset();
            cp.large_reset();
            cp.cmd_reset();
            self_talk("Hs", Mon, Sen);
            break;

          case ( 's' ):
            Serial.printf("Small reset. Just reset flags so filters are reinitialized\n");
            cp.cmd_reset();
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown. Try typing 'h'");
        }
        break;

      case ( 's' ):
        if ( cp.input_string.substring(1).toInt()>0 )
        {
          rp.ibatt_sel_noamp = true;
        }
        else
          rp.ibatt_sel_noamp = false;
        Serial.printf("Signal selection ( 0=amp, 1=noamp,) set to %d\n", rp.ibatt_sel_noamp);
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
          case ( 'd' ):   // dc-dc charger manual setting
            if ( cp.input_string.substring(2).toInt()>0 )
              cp.dc_dc_on = true;
            else
              cp.dc_dc_on = false;
            Serial.printf("dc_dc_on set to %d\n", cp.dc_dc_on);
            break;

          case ( 'm' ):   // modeling code
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
              Serial.printf("invalid %d, rp.modeling is 0-7.  Try 'h'\n", INT_in);
            }
            Serial.printf("Modeling is %d\n", rp.modeling);
            Serial.printf("tweak_test is %d\n", rp.tweak_test());
            Serial.printf("mod_ib is %d\n", rp.mod_ib());
            Serial.printf("mod_vb is %d\n", rp.mod_vb());
            Serial.printf("mod_tb is %d\n", rp.mod_tb());
            break;

          case ( 'a' ): // injection amplitude
            // rp.amp = max(min(cp.input_string.substring(2).toFloat(), 18.3), 0.0);
            rp.amp = cp.input_string.substring(2).toFloat();
            Serial.printf("Modeling injected amp set to %7.3f and inj_soft_bias set to %7.3f\n", rp.amp, rp.inj_soft_bias);
            break;

          case ( 'f' ): // injection frequency
            rp.freq = max(min(cp.input_string.substring(2).toFloat(), 2.0), 0.0);
            Serial.printf("Modeling injected freq set to %7.3f Hz =", rp.freq);
            rp.freq = rp.freq * 2.0 * PI;
            Serial.printf(" %7.3f r/s\n", rp.freq);
            break;

          case ( 't' ): // injection type
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

          case ( 'o' ): // injection dc offset
            rp.inj_soft_bias = max(min(cp.input_string.substring(2).toFloat(), 18.3), -18.3);
            Serial.printf("Modeling injected inj_soft_bias set to %7.3f\n", rp.inj_soft_bias);
            break;

          case ( 'p' ): // injection program
            switch ( cp.input_string.substring(2).toInt() )
            {

              case ( -1 ):
                self_talk("Xp0", Mon, Sen);
                self_talk("m0.5", Mon, Sen);
                rp.modeling = 0;
                debug_inject();  // Arduino plot
                break;

              case ( 0 ):
                rp.modeling = 7;
                rp.type = 0;
                rp.freq = 0.0;
                rp.amp = 0.0;
                if ( !rp.tweak_test() ) rp.inj_soft_bias = 0.0;
                self_talk("Mk0", Mon, Sen);
                self_talk("Nk0", Mon, Sen);
                rp.ibatt_bias_all = 0;
                debug_inject();  // Arduino plot
                break;

              case ( 1 ):
                self_talk("Xp0", Mon, Sen);
                self_talk("m0.5", Mon, Sen);
                rp.type = 1;
                rp.freq = 0.05;
                rp.amp = 6.;
                if ( !rp.tweak_test() ) rp.inj_soft_bias = -rp.amp;
                rp.freq *= (2. * PI);
                debug_inject();  // Arduino plot
                break;

              case ( 2 ):
                self_talk("Xp0", Mon, Sen);
                self_talk("m0.5", Mon, Sen);
                rp.type = 2;
                rp.freq = 0.10;
                rp.amp = 6.;
                if ( !rp.tweak_test() ) rp.inj_soft_bias = -rp.amp;
                rp.freq *= (2. * PI);
                debug_inject();  // Arduino plot
                break;

              case ( 3 ):
                self_talk("Xp0", Mon, Sen);
                self_talk("m0.5", Mon, Sen);
                rp.type = 3;
                rp.freq = 0.05;
                rp.amp = 6.;
                if ( !rp.tweak_test() ) rp.inj_soft_bias = -rp.amp;
                rp.freq *= (2. * PI);
                debug_inject();  // Arduino plot
                break;

              case ( 4 ):
                self_talk("Xp0", Mon, Sen);
                rp.type = 4;
                rp.ibatt_bias_all = -RATED_BATT_CAP;  // Software effect only
                debug_inject();  // Arduino plot
                break;

              case ( 5 ):
                self_talk("Xp0", Mon, Sen);
                rp.type = 5;
                rp.ibatt_bias_all = RATED_BATT_CAP; // Software effect only
                debug_inject();  // Arduino plot
                break;

              case ( 6 ):
                self_talk("Xp0", Mon, Sen);
                rp.type = 6;
                rp.amp = RATED_BATT_CAP*0.2;
                debug_inject();  // Arduino plot
                break;

              case ( 7 ):
                self_talk("Xp0", Mon, Sen);
                rp.type = 7;
                self_talk("Xx1", Mon, Sen);    // Run to model
                self_talk("m0.5", Mon, Sen);   // Set all soc=0.5
                self_talk("n0.987", Mon, Sen); // Set model only to near saturation
                self_talk("v4", Mon, Sen);     // Watch sat, soc, and Voc vs v_sat
                rp.amp = RATED_BATT_CAP*0.2;              // Hard current charge
                Serial.printf("Run 'n<val> as needed to init south of sat.  Reset this whole thing by running 'Xp-1'\n");
                break;

              case ( 8 ):
                self_talk("Xp0", Mon, Sen);
                self_talk("m0.5", Mon, Sen);
                rp.type = 8;
                rp.freq = 0.05;
                rp.amp = 6.;
                if ( !rp.tweak_test() ) rp.inj_soft_bias = -rp.amp;
                rp.freq *= (2. * PI);
                debug_inject();  // Arduino plot
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
            }
            break;

          case ( 'C' ): // injection number of cycles
            Sen->cycles_inj = max(min(cp.input_string.substring(2).toFloat(), 10000.), 0);
            Serial.printf("Number of injection cycles set to %7.3f\n", Sen->cycles_inj);
            break;

          case ( 'R' ): // Start injection now
            if ( Sen->now>TEMP_INIT_DELAY )
            {
              Sen->start_inj = Sen->wait_inj + Sen->now;
              Sen->stop_inj = Sen->wait_inj + (Sen->now + min((unsigned long int)(Sen->cycles_inj / max(rp.freq/(2.*PI), 1e-6) *1000.), ULLONG_MAX));
              Serial.printf("RUN: at %ld, %7.3f cycles from %ld to %ld with %ld wait\n", Sen->now, Sen->cycles_inj, Sen->start_inj, Sen->stop_inj, Sen->wait_inj);
            }
            else Serial.printf("Wait %5.1f s for init\n", float(TEMP_INIT_DELAY-Sen->now)/1000.);
            break;

          case ( 'S' ): // Stop injection now
            Sen->start_inj = 0UL;
            Sen->stop_inj = 0UL;
            Serial.printf("STOPPED\n");
            break;

          case ( 'W' ):
            FP_in = cp.input_string.substring(2).toFloat();
            Sen->wait_inj = (unsigned long int)(max(min(FP_in, MAX_WAIT), 0.))*1000;
            Serial.printf("Waiting %7.1f s to start inj\n", FP_in);
            break;

          default:
            Serial.print(cp.input_string.charAt(1)); Serial.println(" unknown.  Try typing 'h'");
        }
        break;

      case ( 'h' ): 
        talkH(Mon, Sen);
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
void talkH(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("\n\n******** TALK *********\nHelp for serial talk.   Entries and current values.  All entries follwed by CR\n");

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
  Serial.printf("  Dc= "); Serial.printf("%7.3f", rp.vbatt_bias); Serial.printf("    : delta volt sense, V [%7.3f]\n", VOLT_BIAS); 
  Serial.printf("  Dn= "); Serial.print(Sen->Sim->coul_eff()); Serial.println("       : coulombic efficiency"); 
  Serial.printf("  Dt= "); Serial.printf("%7.3f", rp.tbatt_bias); Serial.printf("    : delta T sense, deg C [%7.3f]\n", TEMP_BIAS); 
  Serial.printf("  Dv= "); Serial.print(Sen->Sim->Dv()); Serial.println("       : Table adjust [0.01]"); 
  Serial.printf("  Sc= "); Serial.print(Sen->Sim->q_capacity()/Mon->q_capacity()); Serial.println("       : Scalar battery model size"); 
  Serial.printf("  Sh= "); Serial.printf("%7.3f", rp.hys_scale); Serial.println("    : hysteresis scalar 1e-6 - 100");
  Serial.printf("  Sr= "); Serial.print(Sen->Sim->Sr()); Serial.println("       : Scalar resistor sim"); 
  Serial.printf("  Sk= "); Serial.print(rp.cutback_gain_scalar); Serial.println("       : Saturation of model cutback gain scalar"); 

  Serial.printf("H<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump the summary log to screen\n");
  Serial.printf("  HR= "); Serial.printf("reset the summary log\n");
  Serial.printf("  Hs= "); Serial.printf("save a data point to summary log and print log and status to screen\n");

  Serial.printf("i=,<inp> set the BatteryMonitor delta_q_inf to <inp> (-360000 - 3600000'\n"); 

  Serial.printf("M<?> Amp tweaks.   For example:\n");
  Serial.printf("  MC= "); Serial.printf("%7.3f", Sen->ShuntAmp->max_change()); Serial.println("    : tweak amp max change allowed, A [0.05]"); 
  Serial.printf("  Mg= "); Serial.printf("%7.6f", Sen->ShuntAmp->gain()); Serial.println("  : tweak amp gain = correction to be made for charge, A/Coulomb/day [0.0001]"); 
  Serial.printf("  Mi= "); Serial.printf("%7.3f", Sen->ShuntAmp->delta_q_inf()); Serial.println("   : tweak amp value for state of infinite counter, C [varies]"); 
  Serial.printf("  Mk= "); Serial.printf("%7.3f", Sen->ShuntAmp->tweak_bias()); Serial.println("    : tweak amp adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Mp= "); Serial.printf("%10.1f", Sen->ShuntAmp->delta_q_sat_past()); Serial.println(" : tweak amp past charge infinity at sat, C [varies]"); 
  Serial.printf("  MP= "); Serial.printf("%10.1f", Sen->ShuntAmp->delta_q_sat_present()); Serial.println(" : tweak amp present charge infinity at sat, C [varies]"); 
  Serial.printf("  Mw= "); Serial.printf("%7.3f", Sen->ShuntAmp->time_to_wait()); Serial.println("    : tweak amp time to wait for next tweak, hr [18]]"); 
  Serial.printf("  Mx= "); Serial.printf("%7.3f", Sen->ShuntAmp->max_tweak()); Serial.println("    : tweak amp adder maximum, A [1]"); 
  Serial.printf("  Mz= "); Serial.printf("%7.3f", Sen->ShuntAmp->time_sat_past()); Serial.println("    : tweak amp time since last tweak, hr [varies]"); 

  Serial.printf("N<?> No amp tweaks.   For example:\n");
  Serial.printf("  NC= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->max_change()); Serial.println("    : tweak no amp max change allowed, A [0.05]"); 
  Serial.printf("  Ng= "); Serial.printf("%7.6f", Sen->ShuntNoAmp->gain()); Serial.println("  : tweak no amp gain = correction to be made for charge, A/Coulomb [0.0001]"); 
  Serial.printf("  Ni= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->delta_q_inf()); Serial.println("   : tweak no amp value for state of infinite counter, C [varies]"); 
  Serial.printf("  Nk= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->tweak_bias()); Serial.println("    : tweak no amp adder to all sensed shunt current, A [0]"); 
  Serial.printf("  Np= "); Serial.printf("%10.1f", Sen->ShuntNoAmp->delta_q_sat_past()); Serial.println(" : tweak no amp past charge infinity at sat, C [varies]"); 
  Serial.printf("  NP= "); Serial.printf("%10.1f", Sen->ShuntNoAmp->delta_q_sat_present()); Serial.println(" : tweak no amp present charge infinity at sat, C [varies]"); 
  Serial.printf("  Nw= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->time_to_wait()); Serial.println("    : tweak no amp time to wait for next tweak, hr [18]]"); 
  Serial.printf("  Nx= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->max_tweak()); Serial.println("    : tweak no amp adder maximum, A [1]"); 
  Serial.printf("  Nz= "); Serial.printf("%7.3f", Sen->ShuntNoAmp->time_sat_past()); Serial.println("    : tweak no amp time since last tweak, hr [varies]"); 

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

  Serial.printf("s   curr signal select (0=amp preferred, 1=noamp) = "); Serial.println(rp.ibatt_sel_noamp);

  Serial.printf("v=  "); Serial.print(rp.debug); Serial.println("    : verbosity, -128 - +128. [2]");
  Serial.printf("    -<>:   Negative - Arduino plot compatible\n");
  Serial.printf("     -1:   General purpose Arduino plot\n");
  Serial.printf("     +4:   General purpose\n");
  Serial.printf("   +/-5:   OLED display\n");
  Serial.printf("     +6:   EKF solver iter during init\n");
  Serial.printf("     +7:   EKF solver summary during init\n");
  Serial.printf("     -7:   Battery i/o Arduino plot\n");
  Serial.printf("     +8:   Randles model state space init\n");
  Serial.printf("    -11:   Summary Arduino\n");
  Serial.printf("  +/-12:   EKF\n");
  Serial.printf("  +/-14:   vshunt and Ibatt raw\n");
  Serial.printf("    +15:   vb raw\n");
  Serial.printf("  +/-34:   EKF detailed\n");
  Serial.printf("    -35:   EKF summary Arduino\n");
  Serial.printf("    +35:   Randles balance\n");
  Serial.printf("  +/-37:   EKF short\n");
  Serial.printf("    -41:   Inj\n");
  Serial.printf("    +75:   voc_low check model\n");
  Serial.printf("    +76:   vb model\n");
  Serial.printf("  +/-78:   Battery model saturation\n");
  Serial.printf("    +79:   sat_ib model\n");
  Serial.printf("  +/-96:   CC saturation\n");
  Serial.printf("  +/-97:   CC model saturation\n");

  Serial.printf("w   turn on wifi = "); Serial.println(cp.enable_wifi);

  Serial.printf("W<?>  - seconds to wait\n");

  Serial.printf("X<?> - Test Mode.   For example:\n");
  Serial.printf("  Xd= "); Serial.printf("%d,   dc-dc charger on [0]\n", cp.dc_dc_on);
  Serial.printf("  Xm= "); Serial.printf("%d,   modeling bitmap [000]\n", rp.modeling);
  Serial.printf("       0x8 tweak_test = %d\n", rp.tweak_test());
  Serial.printf("       0x4 current = %d\n", rp.mod_ib());
  Serial.printf("       0x2 voltage = %d\n", rp.mod_vb());
  Serial.printf("       0x1 temperature = %d\n", rp.mod_tb());
  Serial.printf("  Xa= "); Serial.printf("%7.3f", rp.amp); Serial.println("  : Inj amplitude A pk (0-18.3) [0]");
  Serial.printf("  Xf= "); Serial.printf("%7.3f", rp.freq/2./PI); Serial.println("  : Inj frequency Hz (0-2) [0]");
  Serial.printf("  Xt= "); Serial.printf("%d", rp.type); Serial.println("  : Inj type.  'c', 's', 'q', 't' (cosine, sine, square, triangle)");
  Serial.printf("  Xo= "); Serial.printf("%7.3f", rp.inj_soft_bias); Serial.println("  : Inj inj_soft_bias A (-18.3-18.3) [0]");
  Serial.printf("  Di= "); Serial.printf("%7.3f", rp.ibatt_bias_all); Serial.println("  : Inj  A (unlimited) [0]");
  Serial.printf("  Xp= <?>, programmed injection settings...\n"); 
  Serial.printf("      -1:  Off, modeling false\n");
  Serial.printf("       0:  steady-state modeling\n");
  Serial.printf("       1:  1 Hz sinusoid centered at 0\n");
  Serial.printf("       2:  1 Hz square centered at 0\n");
  Serial.printf("       3:  1 Hz triangle centered at 0\n");
  Serial.printf("       4:  -1C soft discharge until reset by Xp0 or Di0.  Software only\n");
  Serial.printf("       5:  +1C soft charge until reset by Xp0 or Di0.  Software only\n");
  Serial.printf("       6:  +0.2C hard charge until reset by Xp0 or Di0\n");
  Serial.printf("       8:  1 Hz cosine centered at 0\n");
  Serial.printf("  XC= "); Serial.printf("%7.3f cycles inj\n", Sen->cycles_inj);
  Serial.printf("  XR  "); Serial.printf("RUN inj\n");
  Serial.printf("  XS  "); Serial.printf("STOP inj\n");
  Serial.printf("  XW= "); Serial.printf("%6.2f s wait start inj\n", float(Sen->wait_inj)/1000.);
  Serial.printf("h   this menu\n");
}

// Call talk from within, a crude macro feature
void self_talk(const String cmd, BatteryMonitor *Mon, Sensors *Sen)
{
  cp.input_string = cmd;
  Serial.printf("self_talk:  new string = '%s'\n", cp.input_string.c_str());
  cp.string_complete = true;
  talk(Mon, Sen);
}
