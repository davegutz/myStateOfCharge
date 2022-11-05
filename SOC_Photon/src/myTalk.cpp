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
extern Flt_st myFlt[NFLT];      // Summaries for saving charge history

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

// Talk Executive
void talk(BatteryMonitor *Mon, Sensors *Sen)
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
          case ( 'b' ):  // Fault buffer
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'd' ):  // bd: fault buffer dump
                Serial.printf("\n");
                print_all_fault_buffer(myFlt, rp.iflt, NFLT);
                break;

              case ( 'R' ):  // bR: Fault buffer reset
                large_reset_fault_buffer(myFlt, rp.iflt, NFLT);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'B' ):
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'm' ):  // B:  Monitor chemistry change
                INT_in = cp.input_string.substring(2).toInt();
                switch ( INT_in )
                {
                  case ( 0 ):  // Bm0: Mon Battleborn
                    Serial.printf("Mon chem %d", Mon->mod_code());
                    Mon->assign_all_mod("Battleborn");
                    Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_randles();
                    cp.cmd_reset();
                    break;

                  case ( 1 ):  // Bm1: Mon LION
                    Serial.printf("Mon chem %d", Mon->mod_code());
                    Mon->assign_all_mod("LION");
                    Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_randles();
                    cp.cmd_reset();
                    break;

                  case ( 2 ):  // Bm2: Mon LION EKF
                    Serial.printf("Mon chem %d", Mon->mod_code());
                    Mon->assign_all_mod("LIE");
                    Serial.printf(" to %d\n", Mon->mod_code()); Mon->assign_randles();
                    cp.cmd_reset();
                    break;

                  default:
                    Serial.printf("%d ? 'h'", INT_in);
                }
                break;

              case ( 's' ):  // B:  Simulation chemistry change
                INT_in = cp.input_string.substring(2).toInt();
                switch ( INT_in )
                {
                  case ( 0 ):  // Bs0: Sim Battleborn
                    Serial.printf("Sim chem %d", Sen->Sim->mod_code());
                    Sen->Sim->assign_all_mod("Battleborn"); Sen->Sim->assign_randles();
                    Serial.printf(" to %d\n", Sen->Sim->mod_code());
                    cp.cmd_reset();
                    break;

                  case ( 1 ):  // Bs1: Sim LION
                    Serial.printf("Sim chem %d", Sen->Sim->mod_code());
                    Sen->Sim->assign_all_mod("LION"); Sen->Sim->assign_randles();
                    Serial.printf(" to %d\n", Sen->Sim->mod_code());
                    cp.cmd_reset();
                    break;

                  case ( 2 ):  // Bs2: Sim LION EKF
                    Serial.printf("Sim chem %d", Sen->Sim->mod_code());
                    Sen->Sim->assign_all_mod("LIE"); Sen->Sim->assign_randles();
                    Serial.printf(" to %d\n", Sen->Sim->mod_code());
                    cp.cmd_reset();
                    break;

                  default:
                    Serial.printf("%d ? 'h'", INT_in);
                }
                break;

              case ( 'P' ):  // BP<>:  Number of parallel batteries in bank, e.g. '2P1S'
                FP_in = cp.input_string.substring(2).toFloat();
                if ( FP_in>0 )  // Apply crude limit to prevent user error
                {
                  Serial.printf("nP%5.2f/%5.2f ", Mon->nP(), Sen->Sim->nP());
                  rp.nP = FP_in;
                  Serial.printf("to%5.2f/%5.2f\n", Mon->nP(), Sen->Sim->nP());
                }
                else
                  Serial.printf("err%5.2f; <=0.\n", FP_in);
                break;

              case ( 'S' ):  // BS<>:  Number of series batteries in bank, e.g. '2P1S'
                FP_in = cp.input_string.substring(2).toFloat();
                if ( FP_in>0 )  // Apply crude limit to prevent user error
                {
                  Serial.printf("nS%5.2f/%5.2f ", Mon->nS(), Sen->Sim->nS());
                  rp.nS = FP_in;
                  Serial.printf("to%5.2f/%5.2f\n", Mon->nS(), Sen->Sim->nS());
                }
                else
                  Serial.printf("err%5.2f; <=0.\n", FP_in);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'c' ):  // c:  clear queues
            clear_queues();
            break;

          case ( 'C' ):  // C:
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'a' ):  // Ca<>:  assign charge state in fraction to all versions including model
                FP_in = cp.input_string.substring(2).toFloat();
                if ( FP_in<1.1 )  // Apply crude limit to prevent user error
                {
                  initialize_all(Mon, Sen, FP_in, true);
                  // debug_m1(Mon, Sen);
                  if ( rp.modeling )
                  {
                    cp.cmd_reset();
                    chit("W3;", SOON);  // Wait 3 passes of Control
                  }
                }
                else
                  Serial.printf("soc%8.4f; err 0-1.1\n", FP_in);
                break;

              case ( 'm' ):  // Cm<>:  assign curve charge state in fraction to model only (ekf if modeling)
                FP_in = cp.input_string.substring(2).toFloat();
                if ( FP_in<1.1 )   // Apply crude limit to prevent user error
                {
                  Sen->Sim->apply_soc(FP_in, Sen->Tb_filt);
                  Serial.printf("soc%8.4f, dq%7.3f, soc_mod%8.4f, dq mod%7.3f,\n",
                      Mon->soc(), Mon->delta_q(), Sen->Sim->soc(), Sen->Sim->delta_q());
                  if ( rp.modeling ) cp.cmd_reset();
                }
                else
                  Serial.printf("soc%8.4f; must be 0-1.1\n", FP_in);
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

              case ( 'm' ):  //   Dm<>:  Amp signal adder for faults
                Serial.printf("ShuntAmp.add%7.3f to", Sen->ShuntAmp->add());
                Sen->ShuntAmp->add(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntAmp->add());
                break;

              case ( 'n' ):  //   Dn<>:  No Amp signal adder for faults
                Serial.printf("ShuntNoAmp.add%7.3f to", Sen->ShuntNoAmp->add());
                Sen->ShuntNoAmp->add(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntNoAmp->add());
                break;

              case ( 'P' ):  //   DP<>:  PRINT multiplier
                Serial.printf("Print int %d to ", cp.print_mult);
                cp.assign_print_mult(max(min(cp.input_string.substring(2).toInt(), UINT8_MAX), 0));
                Serial.printf("%d\n", cp.print_mult);
                break;

              case ( 'r' ):  //   Dr<>:  READ sample time input
                Serial.printf("ReadSensors %ld to ", Sen->ReadSensors->delay());
                Sen->ReadSensors->delay(cp.input_string.substring(2).toInt());
                Serial.printf("%ld\n", Sen->ReadSensors->delay());
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

              case ( 'u' ):  //   Du<>:  Coulombic Efficiency change
                FP_in = cp.input_string.substring(2).toFloat();
                Serial.printf("coulombic eff%7.4f,%7.4f,%7.4f,%7.4f, to", Sen->Sim->coul_eff(), Mon->coul_eff(), Sen->ShuntAmp->coul_eff(), Sen->ShuntNoAmp->coul_eff());
                Sen->Sim->coul_eff(FP_in);
                Mon->coul_eff(FP_in);
                Sen->ShuntAmp->coul_eff(FP_in);
                Sen->ShuntNoAmp->coul_eff(FP_in);
                Serial.printf("%7.4f,%7.4f,%7.4f,%7.4f\n", Sen->Sim->coul_eff(), Mon->coul_eff(), Sen->ShuntAmp->coul_eff(), Sen->ShuntNoAmp->coul_eff());
                break;

              case ( 'v' ):  //   Dv<>:  voltage signal adder for faults
                Serial.printf("Sen->vb_add%7.3f to", Sen->vb_add());
                Sen->vb_add(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->vb_add());
                break;

              case ( 'w' ):  //   Dw<>:
                Serial.printf("Mon.Dv%7.3f to", Mon->Dv());
                Mon->Dv(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Mon->Dv());
                Serial.printf("Sim.Dv%7.3f to", Sen->Sim->Dv());
                Sen->Sim->Dv(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Sim->Dv());
                break;

              case ( 'T' ):  //   DT<>:  Tb noise
                Serial.printf("Sen.Tb_noise_amp_%7.3f to ", Sen->Tb_noise_amp());
                Sen->Tb_noise_amp(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Tb_noise_amp());
                break;

              case ( 'V' ):  //   DV<>:  Vb noise
                Serial.printf("Sen.Vb_noise_amp_%7.3f to ", Sen->Vb_noise_amp());
                Sen->Vb_noise_amp(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Vb_noise_amp());
                break;

              case ( 'M' ):  //   DM<>:  Ib amp noise
                Serial.printf("Sen.Ib_amp_noise_amp_%7.3f to ", Sen->Ib_amp_noise_amp());
                Sen->Ib_amp_noise_amp(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Ib_amp_noise_amp());
                break;

              case ( 'N' ):  //   DN<>:  Ib noa noise
                Serial.printf("Sen.Ib_noa_noise_amp_%7.3f to ", Sen->Ib_noa_noise_amp());
                Sen->Ib_noa_noise_amp(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Ib_noa_noise_amp());
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

              case ( 'c' ):  // * Sc<>: scale capacity
                scale = cp.input_string.substring(2).toFloat();
                rp.s_cap_model = scale;
            
                Serial.printf("Sim.q_cap_rated%7.3f %7.3f to ", scale, Sen->Sim->q_cap_scaled());
            
                Sen->Sim->apply_cap_scale(rp.s_cap_model);
                if ( rp.modeling ) Mon->init_soc_ekf(Sen->Sim->soc());
            
                Serial.printf("%7.3f\n", Sen->Sim->q_cap_scaled());
                Serial.printf("Sim:"); Sen->Sim->pretty_print(); Sen->Sim->Coulombs::pretty_print();
                break;
            
              case ( 'G' ):  // * SG<>: scale shunt gain both shunts simultaneously
                scale = cp.input_string.substring(2).toFloat();
                Serial.printf("sh gn scl%7.3f/%7.3f to", Sen->ShuntAmp->rp_shunt_gain_sclr(), Sen->ShuntNoAmp->rp_shunt_gain_sclr());
                Sen->ShuntAmp->rp_shunt_gain_sclr(scale);
                Sen->ShuntNoAmp->rp_shunt_gain_sclr(scale);
                Serial.printf("%7.3f/%7.3f\n", Sen->ShuntAmp->rp_shunt_gain_sclr(), Sen->ShuntNoAmp->rp_shunt_gain_sclr());
                break;

              case ( 'h' ):  //   Sh<>: scale hysteresis
                scale = cp.input_string.substring(2).toFloat();

                Serial.printf("\nHys::scl%7.3f &%7.3f\n", Mon->hys_scale(), Sen->Sim->hys_scale());
                rp.hys_scale = scale;
                Serial.printf("scl%7.3f\n", scale);
                Mon->hys_scale(scale);
                Sen->Sim->hys_scale(scale);
                Serial.printf("to%7.3f &%7.3f\n", Mon->hys_scale(), Sen->Sim->hys_scale());
                break;

              case ( 'H' ):  //   SH<>: state of all hysteresis
                scale = cp.input_string.substring(2).toFloat();

                Serial.printf("\nHys::dv_hys%7.3f &%7.3f\n", Mon->hys_state(), Sen->Sim->hys_state());
                Mon->hys_state(scale);
                Sen->Sim->hys_state(scale);
                Serial.printf("to%7.3f &%7.3f\n", Mon->hys_state(), Sen->Sim->hys_state());
                break;

              case ( 'm' ):  //   Sm<>:  Amp signal scalar for faults
                Serial.printf("ShuntAmp.sclr%7.3f to", Sen->ShuntAmp->sclr());
                Sen->ShuntAmp->sclr(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntAmp->sclr());
                break;

              case ( 'n' ):  //   Sn<>:  No Amp signal scalar for faults
                Serial.printf("ShuntNoAmp.sclr%7.3f to", Sen->ShuntNoAmp->sclr());
                Sen->ShuntNoAmp->sclr(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntNoAmp->sclr());
                break;
            
              case ( 'r' ):  //   Sr<>:  scalar sim resistor
                scale = cp.input_string.substring(2).toFloat();
            
                Serial.printf("\nBefore Sim::\n"); Sen->Sim->pretty_print_ss();
                Serial.printf("\nScaling D[0, 0]=-r0 by Sr%7.3f\n", scale);
            
                Sen->Sim->Sr(scale);
            
                Serial.printf("\nAfter Sim::\n"); Mon->pretty_print_ss();
                Serial.printf("\nBefore Mon::\n"); Mon->pretty_print_ss();
                Serial.printf("\nScaling D[0, 0]=-r0 by Sr%7.3f\n", scale);
            
                Mon->Sr(scale);
            
                Serial.printf("\nAfter Mon::SS:\n"); Mon->pretty_print_ss();
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
              case ( 'c' ):  //   Fc<>: scale cc_diff threshold
                scale = cp.input_string.substring(2).toFloat();
                Serial.printf("cc_diff scl%7.3f to", Sen->Flt->cc_diff_sclr());
                Sen->Flt->cc_diff_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->cc_diff_sclr());
                break;

              case ( 'd' ):  //   Fd<>: scale ib_diff threshold
                scale = cp.input_string.substring(2).toFloat();
                Serial.printf("ib_diff scl%7.3f to", Sen->Flt->ib_diff_sclr());
                Sen->Flt->ib_diff_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ib_diff_sclr());
                break;

              case ( 'f' ):  //   Ff<>:  fake faults
                INT_in = cp.input_string.substring(2).toInt();
                Serial.printf("cp.fake_faults, rp.ib_select %d, %d to", cp.fake_faults, rp.ib_select);
                cp.fake_faults = INT_in;
                rp.ib_select = INT_in;
                Serial.printf("%d, %d\n", cp.fake_faults, rp.ib_select);
                break;

              case ( 'I' ):  //   FI<>:  Fault disable ib hard
                INT_in = cp.input_string.substring(2).toInt();
                Serial.printf("Sen->Flt->disab_ib_fa %d to ", Sen->Flt->disab_ib_fa());
                Sen->Flt->disab_ib_fa(INT_in);
                Serial.printf("%d\n", Sen->Flt->disab_ib_fa());
                break;

              case ( 'i' ):  //   Fi<>: scale e_wrap_hi threshold
                scale = cp.input_string.substring(2).toFloat();
                Serial.printf("e_wrap_hi scl%7.3f to", Sen->Flt->ewhi_sclr());
                Sen->Flt->ewhi_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ewhi_sclr());
                break;

              case ( 'o' ):  //   Fo<>: scale e_wrap_lo threshold
                scale = cp.input_string.substring(2).toFloat();
                Serial.printf("e_wrap_lo scl%7.3f to", Sen->Flt->ewlo_sclr());
                Sen->Flt->ewlo_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ewlo_sclr());
                break;

              case ( 'q' ):  //   Fq<>: scale ib_quiet threshold
                scale = cp.input_string.substring(2).toFloat();
                Serial.printf("ib_quiet scl%7.3f to", Sen->Flt->ib_quiet_sclr());
                Sen->Flt->ib_quiet_sclr(scale);
                Serial.printf("%7.3f\n", Sen->Flt->ib_quiet_sclr());
                break;

              case ( 'T' ):  //   FT<>:  Fault disable tb stale
                INT_in = cp.input_string.substring(2).toInt();
                Serial.printf("Sen->Flt->disab_tb_fa %d to ", Sen->Flt->disab_tb_fa());
                Sen->Flt->disab_tb_fa(INT_in);
                Serial.printf("%d\n", Sen->Flt->disab_tb_fa());
                break;

              case ( 'V' ):  //   FV<>:  Fault disable vb hard
                INT_in = cp.input_string.substring(2).toInt();
                Serial.printf("Sen->Flt->disab_vb_fa %d to", Sen->Flt->disab_vb_fa());
                Sen->Flt->disab_vb_fa(INT_in);
                Serial.printf("%d\n", Sen->Flt->disab_vb_fa());
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
                chit("Pr;Hd;Q;", QUEUE);
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'i' ): // i<>:  
            Serial.printf("Amp CC reset%9.1f ", Sen->ShuntAmp->delta_q_cinf());
            Sen->ShuntAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
            Serial.printf("to%9.1f\n", Sen->ShuntAmp->delta_q_cinf());
            Serial.printf("Amp CC reset%9.1f ", Sen->ShuntAmp->delta_q_dinf());
            Sen->ShuntAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
            Serial.printf("to%9.1f\n", Sen->ShuntAmp->delta_q_dinf());
            Serial.printf("Noa CC reset%9.1f ", Sen->ShuntNoAmp->delta_q_cinf());
            Sen->ShuntNoAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
            Serial.printf("to%9.1f\n", Sen->ShuntNoAmp->delta_q_cinf());
            Serial.printf("Noa CC reset%9.1f ", Sen->ShuntNoAmp->delta_q_dinf());
            Sen->ShuntNoAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
            Serial.printf("to%9.1f\n", Sen->ShuntNoAmp->delta_q_dinf());
            break;

          case ( 'l' ):
            switch ( rp.debug )
            {
              case ( -1 ):  // l-1:
                // Serial.printf("SOCu_s-90  ,SOCu_fa-90  ,Ishunt_amp  ,Ishunt_noa  ,Vbat_fo*10-110  ,voc_s*10-110  ,dv_dyn_s*10  ,v_s*10-110  , voc_dyn*10-110,,,,,,,,,,,\n");
                break;
              case ( 1 ):  // l1:
                print_serial_header();
                break;
              case ( 2 ):  // l2:
                print_signal_sel_header();
                print_serial_sim_header();
                print_serial_header();
                break;
              case ( 3 ):  // l3:
                print_serial_ekf_header();
                print_serial_sim_header();
                print_serial_header();
                break;
              default:
                print_serial_header();
            }
            break;

          case ( 'M' ):
            switch ( cp.input_string.charAt(1) )
            {

              case ( 'C' ):  //   MC<>:
                Serial.printf("Amp->max_change_%10.6f to", Sen->ShuntAmp->max_change());
                Sen->ShuntAmp->max_change(cp.input_string.substring(2).toFloat());
                Serial.printf("%10.6f\n", Sen->ShuntAmp->max_change());
                break;

              case ( 'k' ):  // * Mk<>:
                Serial.printf("amp tweak_sclr%7.3f to", Sen->ShuntAmp->tweak_sclr());
                Sen->ShuntAmp->tweak_sclr(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntAmp->tweak_sclr());
                break;

              case ( 'w' ):  //   Mw<>:
                Serial.printf("Amp->time_to_wait%7.3f to", Sen->ShuntAmp->time_to_wait());
                Sen->ShuntAmp->time_to_wait(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntAmp->time_to_wait());
                break;

              case ( 'x' ):  //   Mx<>:
                Serial.printf("Amp->max_tweak%7.3f to", Sen->ShuntAmp->max_tweak());
                Sen->ShuntAmp->max_tweak(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntAmp->max_tweak());
                break;

              case ( 'z' ):  //   Mz<>:
                Serial.printf("Amp->time_sat_past%7.3f to", Sen->ShuntAmp->time_sat_past());
                Sen->ShuntAmp->time_sat_past(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntAmp->time_sat_past());
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'N' ):
            switch ( cp.input_string.charAt(1) )
            {

              case ( 'C' ):  //   NC<>:
                Serial.printf("Noa->max_change_%10.6f to", Sen->ShuntNoAmp->max_change());
                Sen->ShuntNoAmp->max_change(cp.input_string.substring(2).toFloat());
                Serial.printf("%10.6f\n", Sen->ShuntNoAmp->max_change());
                break;

              case ( 'k' ):  // * Nk<>:
                Serial.printf("Noa tweak_sclr%7.3f to", Sen->ShuntNoAmp->tweak_sclr());
                Sen->ShuntNoAmp->tweak_sclr(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntNoAmp->tweak_sclr());
                break;

              case ( 'w' ):  //   Nw<>:
                Serial.printf("Noa->time_to_wait%7.3f to", Sen->ShuntNoAmp->time_to_wait());
                Sen->ShuntNoAmp->time_to_wait(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntNoAmp->time_to_wait());
                break;

              case ( 'x' ):  //   Nx<>:
                Serial.printf("Noa->max_tweak%7.3f to", Sen->ShuntNoAmp->max_tweak());
                Sen->ShuntNoAmp->max_tweak(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntNoAmp->max_tweak());
                break;

              case ( 'z' ):  //   Nz<>:
                Serial.printf("Noa->time_sat_past%7.3f to", Sen->ShuntNoAmp->time_sat_past());
                Sen->ShuntNoAmp->time_sat_past(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->ShuntNoAmp->time_sat_past());
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'P' ):
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'a' ):  // Pa:  Print all
                chit("Pm;", SOON);
                chit("Ps;", SOON);
                chit("Pr;", SOON);
                chit("PM;", SOON);
                chit("PN;", SOON);
                chit("Ph;", SOON);
                chit("Pf;", SOON);
                break;

              case ( 'e' ):  // Pe:  Print EKF
                Serial.printf ("\nMon::"); Mon->EKF_1x1::pretty_print();
                Serial1.printf("\nMon::"); Mon->EKF_1x1::pretty_print();
                break;

              case ( 'f' ):  // Pf:  Print faults
                Serial.printf ("\nSen::\n");
                print_all_fault_buffer(myFlt, rp.iflt, NFLT);
                Sen->Flt->pretty_print (Sen, Mon);
                Serial1.printf("\nSen::\n");
                Sen->Flt->pretty_print1(Sen, Mon);
                break;

              case ( 'h' ):  // Ph:  Print hysteresis
                debug_h(Mon, Sen);
                break;

              case ( 'm' ):  // Pm:  Print mon
                Serial.printf ("\nM:"); Mon->pretty_print(Sen);
                Serial.printf ("M::"); Mon->Coulombs::pretty_print();
                Serial.printf ("M::"); Mon->pretty_print_ss();
                Serial.printf ("M::"); Mon->EKF_1x1::pretty_print();
                Serial.printf ("\nmodeling = %d\n", rp.modeling);
                break;

              case ( 'M' ):  // PM:  Print shunt Amp
                Serial.printf ("\nTweak::"); Sen->ShuntAmp->pretty_print();
                Serial1.printf("\nTweak::"); Sen->ShuntAmp->pretty_print();
                break;

              case ( 'N' ):  // PN:  Print shunt no amp
                Serial.printf ("\nTweak::"); Sen->ShuntNoAmp->pretty_print();
                Serial1.printf("\nTweak::"); Sen->ShuntNoAmp->pretty_print();
                break;

              case ( 'r' ):  // Pr:  Print retained
                Serial.printf("\n"); rp.pretty_print( true );
                Serial.printf("\n"); cp.pretty_print();
                break;

              case ( 's' ):  // Ps:  Print sim
                Serial.printf("\nmodeling=%d\n", rp.modeling);
                Serial.printf("S:");  Sen->Sim->pretty_print();
                Serial.printf("S::"); Sen->Sim->Coulombs::pretty_print();
                Serial.printf("S::"); Sen->Sim->pretty_print_ss();
                break;

              case ( 'x' ):  // Px:  Print shunt measure
                Serial.printf("\nAmp: "); Serial.printf("Vshunt_int,Vshunt,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,\n", 
                  Sen->ShuntAmp->vshunt_int(), Sen->ShuntAmp->vshunt(), cp.ib_tot_bias_amp, Sen->ShuntAmp->ishunt_cal());
                Serial.printf("Noa:"); Serial.printf("Vshunt_int,Vshunt,ib_tot_bias,Ishunt_cal=,%d,%7.3f,%7.3f,%7.3f,\n", 
                  Sen->ShuntNoAmp->vshunt_int(), Sen->ShuntNoAmp->vshunt(), cp.ib_tot_bias_noa, Sen->ShuntNoAmp->ishunt_cal());
                Serial.printf("Sel:Noa,Ib=,%d,%7.3f\n", rp.ib_select, Sen->Ib);
                break;

              case ( 'v' ):  // Pv:  Print Vb measure
                Serial.printf("\nVolt:");   Serial.printf("Vb_bias_hdwe,Vb_m,mod,Vb=,%7.3f,%7.3f,%d,%7.3f,\n", 
                  rp.Vb_bias_hdwe, Sen->Vb_model, rp.modeling, Sen->Vb);
                break;

              default:
                Serial.println("");Serial.print(cp.input_string.charAt(1)); Serial.println("? 'h'");
            }
            break;

          case ( 'Q' ):  // Q:  quick critical
            debug_q(Mon, Sen);
            break;

          case ( 'R' ):
            switch ( cp.input_string.charAt(1) )
            {
              case ( 'b' ):  // Rb:  Reset battery states (also hys)
                Sen->Sim->init_battery_sim(true, Sen);  // Reset sim battery state
                Mon->init_battery_mon(true, Sen);       // Reset mon battery state
                break;

              case ( 'f' ):  // Rf:  Reset fault Rf
                Serial.printf("Reset latches\n");
                Sen->Flt->reset_all_faults(true);
                break;

              case ( 'i' ):  // Ri:
                Serial.printf("Amp chg CC%9.1f ", Sen->ShuntAmp->delta_q_cinf());
                Sen->ShuntAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
                Serial.printf("to%9.1f\n", Sen->ShuntAmp->delta_q_cinf());
                Serial.printf("Amp dchg CC%9.1f ", Sen->ShuntAmp->delta_q_dinf());
                Sen->ShuntAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
                Serial.printf("to%9.1f\n", Sen->ShuntAmp->delta_q_dinf());
                Serial.printf("Noa chg CC%9.1f ", Sen->ShuntNoAmp->delta_q_cinf());
                Sen->ShuntNoAmp->delta_q_cinf(-RATED_BATT_CAP*3600.);
                Serial.printf("to%9.1f\n", Sen->ShuntNoAmp->delta_q_cinf());
                Serial.printf("Noa dchg CC%9.1f ", Sen->ShuntNoAmp->delta_q_dinf());
                Sen->ShuntNoAmp->delta_q_dinf(RATED_BATT_CAP*3600.);
                Serial.printf("to%9.1f\n", Sen->ShuntNoAmp->delta_q_dinf());
                break;

              case ( 'r' ):  // Rr:  small reset counters
                Serial.printf("CC reset\n");
                Sen->Sim->apply_soc(1.0, Sen->Tb_filt);
                Mon->apply_soc(1.0, Sen->Tb_filt);
                cp.cmd_reset();
                break;

              case ( 'R' ):  // RR:  large reset
                Serial.printf("RESET\n");
                Sen->Sim->apply_soc(1.0, Sen->Tb_filt);
                Mon->apply_soc(1.0, Sen->Tb_filt);
                cp.cmd_reset();
                Sen->ReadSensors->delay(READ_DELAY);
                rp.large_reset();
                cp.large_reset();
                cp.cmd_reset();
                chit("Rf;", SOON);
                chit("W3;", SOON);
                chit("Hs;", SOON);
                chit("Pf;", SOON);
                Serial.printf("Clean. Deploy\n");
                break;

              case ( 's' ):  // Rs:  small reset filters
                Serial.printf("reset\n");
                cp.cmd_reset();
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" unk. Try 'h'");
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
            rp.debug = cp.input_string.substring(1).toInt();
            break;

          case ( 'V' ):
            switch ( cp.input_string.charAt(1) )
            {

              case ( 'M' ):  //   VM<>:  delta Mon curve voltage out
                Serial.printf("Mon dv_voc_soc%7.3f to", Mon->dv_voc_soc());
                Mon->dv_voc_soc(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Mon->dv_voc_soc());
                break;

              case ( 'm' ):  //   Vm<>:  delta Mon curve soc in
                Serial.printf("Mon ds_voc_soc%7.3f to", Mon->ds_voc_soc());
                Mon->ds_voc_soc(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Mon->ds_voc_soc());
                break;

              case ( 'S' ):  //   VS<>:  delta Sim curve voltage out
                Serial.printf("Sim dv_voc_soc%7.3f to", Sen->Sim->dv_voc_soc());
                Sen->Sim->dv_voc_soc(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Sim->dv_voc_soc());
                break;

              case ( 's' ):  //   Vs<>:  delta Sim curve soc in
                Serial.printf("Sim ds_voc_soc%7.3f to", Sen->Sim->ds_voc_soc());
                Sen->Sim->ds_voc_soc(cp.input_string.substring(2).toFloat());
                Serial.printf("%7.3f\n", Sen->Sim->ds_voc_soc());
                break;

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
                if ( INT_in>=0 && INT_in<16 )
                {
                  boolean reset = rp.modeling != INT_in;
                  Serial.printf("modeling %d to ", rp.modeling);
                  rp.modeling = INT_in;
                  Serial.printf("%d\n", rp.modeling);
                  if ( reset )
                  {
                    Serial.printf("Chg...reset\n");
                    cp.cmd_reset();
                  }
                }
                else
                {
                  Serial.printf("err %d, modeling 0-7. 'h'\n", INT_in);
                }
                Serial.printf("Modeling %d\n", rp.modeling);
                Serial.printf("tweak_test %d\n", rp.tweak_test());
                Serial.printf("mod_ib %d\n", rp.mod_ib());
                Serial.printf("mod_vb %d\n", rp.mod_vb());
                Serial.printf("mod_tb %d\n", rp.mod_tb());
                break;

              case ( 'a' ): // Xa<>:  injection amplitude
                rp.amp = cp.input_string.substring(2).toFloat();
                Serial.printf("Inj amp set%7.3f & inj_bias set%7.3f\n", rp.amp, rp.inj_bias);
                break;

              case ( 'f' ): // Xf<>:  injection frequency
                rp.freq = max(min(cp.input_string.substring(2).toFloat(), 2.0), 0.0);
                Serial.printf("Injected freq set%7.3f Hz =", rp.freq);
                rp.freq *= (2. * PI);
                Serial.printf("%7.3f r/s\n", rp.freq);
                break;

              case ( 'b' ): // Xb<>:  injection bias
                rp.inj_bias = cp.input_string.substring(2).toFloat();
                Serial.printf("Inj_bias set%7.3f\n", rp.inj_bias);
                break;

              case ( 't' ): // Xt<>:  injection type
                switch ( cp.input_string.charAt(2) )
                {
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
                    if ( !rp.tweak_test() ) chit("Xm7;", ASAP);  // Prevent upset of time in Xp9, Xp10, Xp11, etc
                    chit("Xf0.; Xa0.", ASAP);
                    if ( !rp.tweak_test() ) chit("Xb0.", ASAP);
                    chit("XS; Mk1; Nk1;", ASAP);  // Stop any injection
                    chit("Dn" + String(COULOMBIC_EFF), ASAP);
                    chit("Xs1.0;Di0;Dm0;Dn0;Dv0;DT0;DV0.0;DI0;Xu0;Xv1.;Dr100;DP4;", ASAP);
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
                    chit("XS;Dm0;Dn0;v0;Xm7;Ca0.5;Pm;Dr100;DP1;v2;", QUEUE);  // setup
                    chit("Dn0.00001;Dm500;Dm-500;Dm0;", QUEUE);  // run
                    chit("W10;Pm;v0;", QUEUE);  // finish
                    break;

                  case ( 9 ): case( 10 ): case ( 11 ): case( 12 ):  // Xp9: Xp10: Xp11: Xp12:  Program regression
                              // Regression tests 9=tweak, 10=tweak w data, 11=cycle, 12 1/2 cycle
                    chit("Xp0;", QUEUE);      // Reset nominal
                    chit("v0;", QUEUE);       // Turn off debug temporarily so not snowed by data dumps
                    chit("Bm0;Bs0;", QUEUE);  // Set Battleborn configuration
                    chit("Xm15;", QUEUE);     // Modeling (for totally digital test of logic) and tweak_test=true to disable cutback in Sim.  Leaving cutback on would mean long run times (~30:00) (May need a way to test features affected by cutback, such as tweak, saturation logic)
                    chit("Xts;", QUEUE);      // Start up a sine wave
                    chit("Ca1;", QUEUE);      // After restarting with sine running, soc will not be at 1.  Reset them all to 1
                    chit("Ri;", QUEUE);       // Reset the delta_q's
                    chit("Mw0;Nw0;", QUEUE);  // Allow tweak bias to work immediately instead of waiting several hours
                    chit("MC0.004;", QUEUE);  // Give tweak bias logic a large adjustment range to quickly converge
                    chit("Mx0.04;", QUEUE);   // Give tweak bias logic a large adjustment range to quickly converge
                    chit("NC0.004;", QUEUE);  // Give tweak bias logic a large adjustment range to quickly converge
                    chit("Nx0.04;", QUEUE);   // Give tweak bias logic a large adjustment range to quickly converge
                    chit("Mk1;Nk1;", QUEUE);  // Reset the tweak biases to 1 for new count
                    chit("Dm1;Dn1;", ASAP);   // Slight positive current so sat logic is functional.  ASAP so synchronized and ib_diff flat.
                    chit("DP2;", QUEUE);      // Fast data collection
                    chit("Rb;", QUEUE);       // Reset battery states
                    chit("Pa;", QUEUE);       // Print all for record
                    if ( INT_in == 9 )  // Xp9:  silent tweak test
                    {
                      chit("Xs0.1;", QUEUE);  // t_sat scaled to detect saturation in short dwell with Xf, sclr (0.1)
                      chit("Xf0.02;", QUEUE); // Frequency, Hz (0.02)
                      chit("XW5;", QUEUE);    // Wait time before starting to cycle (5)
                      chit("XT5;", QUEUE);    // Wait time after cycle to print (5)
                      chit("Xa-6000;", QUEUE);// Amplitude, A (-6000)
                      chit("XC20;", QUEUE);   // Number of injection cycles (20)
                      chit("v0;", QUEUE);     // Silent
                    }
                    else if ( INT_in == 10 )  // Xp10:  rapid tweak
                    {
                      chit("Xf0.02;", QUEUE); // Frequency 0.02 Hz
                      chit("Xa-2000;", QUEUE);// Amplitude -2000 A
                      chit("XW5;", QUEUE);    // Wait time before starting to cycle
                      chit("XT5;", QUEUE);    // Wait time after cycle to print
                      chit("XC3;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v2;", QUEUE);    // Data collection
                    }
                    else if ( INT_in == 11 )  // Xp11:  slow tweak
                    {
                      chit("Xf0.002;", QUEUE);// Frequency 0.002 Hz
                      chit("Xa-60;", QUEUE);  // Amplitude -60 A
                      chit("XW60;", QUEUE);   // Wait time before starting to cycle
                      chit("XT600;", QUEUE);  // Wait time after cycle to print
                      chit("XC1;", QUEUE);    // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v2;", QUEUE);    // Data collection
                    }
                    else if ( INT_in == 12 )  // Xp12:  slow half tweak
                    {
                      chit("Xf0.0002;", QUEUE); // Frequency 0.002 Hz
                      chit("Xa-6;", QUEUE);   // Amplitude -60 A
                      chit("XW60;", QUEUE);   // Wait time before starting to cycle
                      chit("XT2400;", QUEUE); // Wait time after cycle to print
                      chit("XC0.5;", QUEUE);  // Number of injection cycles
                      chit("W2;", QUEUE);     // Wait
                      chit("v2;", QUEUE);    // Data collection
                    }
                    chit("W2;", QUEUE);       // Wait
                    chit("XR;", QUEUE);       // Run cycle
                    break;

                  case( 20 ): case ( 21 ):    // Xp20:  Xp21:  20= 0.5 s sample/2.0s print, 21= 2 s sample/8 s print
                    chit("v0;", QUEUE);       // Turn off debug temporarily so not snowed by data dumps
                    chit("Pa;", QUEUE);       // Print all for record
                    if ( INT_in == 20 )
                    {
                      chit("Dr500;", QUEUE);  // 5x sample time, > RANDLES_T_MAX.  Randles dynamics disabled in Photon
                      chit("DP4;", QUEUE);    // 4x data collection, > RANDLES_T_MAX.  Randles dynamics disabled in Python
                      chit("v2;", QUEUE);    // Large data set
                    }
                    else if ( INT_in == 21 )
                    {
                      chit("DP20;", QUEUE);    // 20x data collection
                      chit("v2;", QUEUE);     // Slow data collection
                    }
                    chit("Rb;", QUEUE);       // Large data set
                    break;

                  default:
                    Serial.printf("Xp=%d unk.  see 'h'\n", INT_in);
                }
                break;

              case ( 'C' ): // XC:  injection number of cycles
                Sen->cycles_inj = max(min(cp.input_string.substring(2).toFloat(), 10000.), 0);
                Serial.printf("#inj cycles to%7.3f\n", Sen->cycles_inj);
                break;

              case ( 'R' ): // XR:  Start injection now
                if ( Sen->now>TEMP_INIT_DELAY )
                {
                  Sen->start_inj = Sen->wait_inj + Sen->now;
                  Sen->stop_inj = Sen->wait_inj + (Sen->now + min((unsigned long int)(Sen->cycles_inj / max(rp.freq/(2.*PI), 1e-6) *1000.), ULLONG_MAX));
                  Sen->end_inj = Sen->stop_inj + Sen->tail_inj;
                  Serial.printf("**\n*** RUN: at %ld,%7.3f cycles%7.3f to%7.3f with%7.3f wait and%7.3f tail ****\n**\n",
                    Sen->now, Sen->cycles_inj, float(Sen->start_inj)/1000., float(Sen->stop_inj)/1000., float(Sen->wait_inj)/1000.,
                    float(Sen->tail_inj)/1000.);
                }
                else Serial.printf("Wait%5.1fs for init\n", float(TEMP_INIT_DELAY-Sen->now)/1000.);
                break;

              case ( 'S' ): // XS:  Stop injection now
                Sen->start_inj = 0UL;
                Sen->stop_inj = 0UL;
                Sen->end_inj = 0UL;
                Sen->elapsed_inj = 0;
                Serial.printf("STOP\n");
                break;

              case ( 's' ): // Xs:  scale T_SAT
                FP_in = cp.input_string.substring(2).toFloat();
                Serial.printf("s_t_sat%7.1f s to\n", cp.s_t_sat);
                cp.s_t_sat = max(FP_in, 0.);
                Serial.printf("%7.1f\n", cp.s_t_sat);
                break;

              case ( 'W' ):  // XW<>:  Wait beginning of programmed transient
                FP_in = cp.input_string.substring(2).toFloat();
                Sen->wait_inj = (unsigned long int)(max(FP_in, 0.))*1000;
                Serial.printf("Wait%7.1f s to inj\n", FP_in);
                break;

              case ( 'T' ):  // XT<>:  Tail
                FP_in = cp.input_string.substring(2).toFloat();
                Sen->tail_inj = (unsigned long int)(max(FP_in, 0.))*1000;
                Serial.printf("Wait%7.1f s tail after\n", FP_in);
                break;

              case ( 'u' ):  // Xu<>:  Tb, fail it
                INT_in = cp.input_string.substring(2).toInt();
                Serial.printf("Fail tb %d to ", Sen->Flt->fail_tb());
                Sen->Flt->fail_tb(INT_in);
                Serial.printf("%d\n", Sen->Flt->fail_tb());
                break;

              case ( 'v' ):  // Xv<>:  Tb stale time scalar
                FP_in = cp.input_string.substring(2).toFloat();
                Serial.printf("Stale tb time sclr%9.4f to", Sen->Flt->tb_stale_time_sclr());
                Sen->Flt->tb_stale_time_sclr(FP_in);
                Serial.printf("%9.4f\n", Sen->Flt->tb_stale_time_sclr());
                break;

              default:
                Serial.print(cp.input_string.charAt(1)); Serial.println(" ? 'h'");
            }
            break;

          case ( 'h' ):  // h: help
            talkH(Mon, Sen);
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
void talkH(BatteryMonitor *Mon, Sensors *Sen)
{
  Serial.printf("\n\nHelp menu.  End entry with ';'.  SRAM='*'.  May omit '='\n");

  Serial.printf("\nb<?>   Manage fault buffer\n");
  Serial.printf("  bd= "); Serial.printf("dump fault buffer\n");
  Serial.printf("  bR= "); Serial.printf("reset fault buffer\n");

  Serial.printf("\nB<?> Battery e.g.:\n");
  Serial.printf(" *Bm=  %d.  Mon chem 0='BB', 1='LI' [%d]\n", rp.mon_mod, MON_CHEM); 
  Serial.printf(" *Bs=  %d.  Sim chem 0='BB', 1='LI' [%d]\n", rp.sim_mod, SIM_CHEM); 
  Serial.printf(" *BP=  %4.2f.  parallel in bank [%4.2f]'\n", rp.nP, NP); 
  Serial.printf(" *BS=  %4.2f.  series in bank [%4.2f]'\n", rp.nS, NS); 

  Serial.printf("\nc  clear talk, esp '-c;'\n");

  Serial.printf("\nC<?> Chg SOC e.g.:\n");
  Serial.printf("  Ca=  all - '(0-1.1)'\n"); 
  Serial.printf("  Cm=  model (& ekf if mod)- '(0-1.1)'\n"); 

  Serial.printf("\nD/S<?> Adj e.g.:\n");
  Serial.printf(" *DA= "); Serial.printf("%6.3f", rp.ib_bias_amp); Serial.printf(": delta amp, A [%6.3f]\n", CURR_BIAS_AMP); 
  Serial.printf(" *DB= "); Serial.printf("%6.3f", rp.ib_bias_noa); Serial.printf(": delta noa, A [%6.3f]\n", CURR_BIAS_NOA); 
  Serial.printf(" *Di= "); Serial.printf("%6.3f", rp.ib_bias_all); Serial.printf(": delta all, A [%6.3f]\n", CURR_BIAS_ALL); 
  Serial.printf(" *Dc= "); Serial.printf("%6.3f", rp.Vb_bias_hdwe); Serial.printf(": delta, V [%6.3f]\n", VOLT_BIAS); 
  Serial.printf("  DE= "); Serial.printf("%d", cp.eframe_mult); Serial.printf(": eframe mult Dr [20]\n"); 
  Serial.printf("  Du=  "); Serial.print(Sen->Sim->coul_eff()); Serial.println(": coul eff"); 
  Serial.printf("  Dm= "); Serial.printf("%6.3f", Sen->ShuntAmp->add()); Serial.printf(": delta amp inj, A [0]\n"); 
  Serial.printf("  Dn= "); Serial.printf("%6.3f", Sen->ShuntNoAmp->add()); Serial.printf(": delta noa inj, A [0]\n"); 
  Serial.printf("  DP=  "); Serial.print(cp.print_mult); Serial.println(": print mult Dr [4]"); 
  Serial.printf("  Dr=  "); Serial.print(Sen->ReadSensors->delay()); Serial.println(": minor frame, ms [100]"); 
  Serial.printf(" *Dt= "); Serial.printf("%6.3f", rp.Tb_bias_hdwe); Serial.printf(": delta hdwe, deg C [%6.3f]\n", TEMP_BIAS); 
  Serial.printf("  D^= "); Serial.printf("%6.3f", cp.Tb_bias_model); Serial.printf(": delta model, deg C [%6.3f]\n", TEMP_BIAS); 
  Serial.printf("  Dv=  "); Serial.print(Sen->vb_add()); Serial.println(": volt fault inj, V [0]"); 
  Serial.printf("  Dw=  "); Serial.print(Sen->Sim->Dv()); Serial.println(": Tab adj, V [0.01]"); 
  Serial.printf("  DT= "); Serial.printf("%6.3f", Sen->Tb_noise_amp()); Serial.printf(": noise mod, deg C pk-pk [%6.3f]\n", TB_NOISE); 
  Serial.printf("  DV= "); Serial.printf("%6.3f", Sen->Vb_noise_amp()); Serial.printf(": noise mod, V pk-pk [%6.3f]\n", VB_NOISE); 
  Serial.printf("  DM= "); Serial.printf("%6.3f", Sen->Ib_amp_noise_amp()); Serial.printf(": amp noise mod, A pk-pk [%6.3f]\n", IB_AMP_NOISE); 
  Serial.printf("  DN= "); Serial.printf("%6.3f", Sen->Ib_noa_noise_amp()); Serial.printf(": noa noise mod, A pk-pk [%6.3f]\n", IB_NOA_NOISE); 
  Serial.printf(" *SA= "); Serial.printf("%6.3f", rp.Ib_scale_amp); Serial.printf(": scale amp [%6.3f]\n", CURR_SCALE_AMP); 
  Serial.printf(" *SB= "); Serial.printf("%6.3f", rp.Ib_scale_noa); Serial.printf(": scale noa [%6.3f]\n", CURR_SCALE_NOA); 
  Serial.printf(" *Sc=  "); Serial.print(Sen->Sim->q_capacity()/Mon->q_capacity()); Serial.println(": rp. Scalar cap"); 
  Serial.printf(" *SG= "); Serial.printf("%6.3f/%6.3f", Sen->ShuntAmp->rp_shunt_gain_sclr(), Sen->ShuntAmp->rp_shunt_gain_sclr());
  Serial.printf(": rp. scale shunt gains [1]\n"); 
  Serial.printf("  Sh= "); Serial.printf("%6.3f", rp.hys_scale); Serial.printf(": hys sclr [%5.2f]\n", HYS_SCALE);
  Serial.printf("  SH= "); Serial.printf("%6.3f", Sen->Sim->hys_state()); Serial.printf(": hys states [0]\n");
  Serial.printf("  Sm= "); Serial.printf("%6.3f", Sen->ShuntAmp->sclr()); Serial.printf(": sclr amp, [1]\n"); 
  Serial.printf("  Sn= "); Serial.printf("%6.3f", Sen->ShuntNoAmp->sclr()); Serial.printf(": sclr noa [1]\n"); 
  Serial.printf("  Sr=  "); Serial.print(Sen->Sim->Sr()); Serial.println(": Scalar res sim"); 
  Serial.printf(" *Sk=  "); Serial.print(rp.cutback_gain_scalar); Serial.println(": Sat mod ctbk sclr"); 
  Serial.printf(" *SV= "); Serial.printf("%6.3f", rp.Vb_scale); Serial.printf(": scale vb sen [%6.3f]\n", VB_SCALE); 

  Serial.printf("\nF<?>   Faults\n");
  Serial.printf("  Fc= "); Serial.printf("%6.3f", Sen->Flt->cc_diff_sclr()); Serial.printf(": sclr cc_diff thr ^ [1]\n"); 
  Serial.printf("  Fd= "); Serial.printf("%6.3f", Sen->Flt->ib_diff_sclr()); Serial.printf(": sclr ib_diff thr ^ [1]\n"); 
  Serial.printf("  Ff= "); Serial.printf("%d", cp.fake_faults); Serial.printf(": faults faked (ignored)[%d]\n", FAKE_FAULTS); 
  Serial.printf("  Fi= "); Serial.printf("%6.3f", Sen->Flt->ewhi_sclr()); Serial.printf(": sclr e_wrap_hi thr ^ [1]\n"); 
  Serial.printf("  Fo= "); Serial.printf("%6.3f", Sen->Flt->ewlo_sclr()); Serial.printf(": sclr e_wrap_lo thr ^ [1]\n"); 
  Serial.printf("  Fq= "); Serial.printf("%6.3f", Sen->Flt->ib_quiet_sclr()); Serial.printf(": sclr ib_quiet thr v [1]\n"); 
  Serial.printf("  FI=  "); Serial.print(Sen->Flt->disab_ib_fa()); Serial.println(": disab Ib rng"); 
  Serial.printf("  FT=  "); Serial.print(Sen->Flt->disab_tb_fa()); Serial.println(": disab Tb rng"); 
  Serial.printf("  FV=  "); Serial.print(Sen->Flt->disab_vb_fa()); Serial.println(": disab Vb rng"); 

  Serial.printf("\nH<?>   Manage history\n");
  Serial.printf("  Hd= "); Serial.printf("dump summ log\n");
  Serial.printf("  HR= "); Serial.printf("reset summ log\n");
  Serial.printf("  Hs= "); Serial.printf("save and print log\n");

  Serial.printf("\nM<?> Amp tweaks on Coulombic Eff (CE)\n");
  Serial.printf("  MC= "); Serial.printf("%6.3f", Sen->ShuntAmp->max_change()); Serial.printf(": max step dCE [%6.4f]\n", TWEAK_MAX_CHANGE); 
  Serial.printf(" *Mk= "); Serial.printf("%6.3f", Sen->ShuntAmp->tweak_sclr()); Serial.println(": rp. CE sclr [1]"); 
  Serial.printf("  Mw= "); Serial.printf("%6.3f", Sen->ShuntAmp->time_to_wait()); Serial.printf(": next tweak, hr [%4.1f]\n", TWEAK_WAIT); 
  Serial.printf("  Mx= "); Serial.printf("%6.3f", Sen->ShuntAmp->max_tweak()); Serial.printf(": abs max dCE [%6.4f]\n", TWEAK_MAX); 
  Serial.printf("  Mz= "); Serial.printf("%6.3f", Sen->ShuntAmp->time_sat_past()); Serial.println(": mem of last twk, hr"); 

  Serial.printf("N<?> No-amp tweaks same parameters as Amp\n");
  Serial.printf("  NC= "); Serial.printf("%6.3f\n", Sen->ShuntNoAmp->max_change());
  Serial.printf(" *Nk= "); Serial.printf("%6.3f\n", Sen->ShuntNoAmp->tweak_sclr());
  Serial.printf("  Nw= "); Serial.printf("%6.3f\n", Sen->ShuntNoAmp->time_to_wait());
  Serial.printf("  Nx= "); Serial.printf("%6.3f\n", Sen->ShuntNoAmp->max_tweak());
  Serial.printf("  Nz= "); Serial.printf("%6.3f\n", Sen->ShuntNoAmp->time_sat_past());

  Serial.printf("\nP<?>   Print values\n");
  Serial.printf("  Pa= "); Serial.printf("all\n");
  Serial.printf("  Pe= "); Serial.printf("ekf\n");
  Serial.printf("  Pf= "); Serial.printf("faults\n");
  Serial.printf("  Pm= "); Serial.printf("Mon\n");
  Serial.printf("  PM= "); Serial.printf("amp shunt\n");
  Serial.printf("  PN= "); Serial.printf("noa shunt\n");
  Serial.printf("  Pr= "); Serial.printf("retained and command\n");
  Serial.printf("  Ps= "); Serial.printf("Sim\n");
  Serial.printf("  Px= "); Serial.printf("ib select\n");
  Serial.printf("  Pv= "); Serial.printf("vb details\n");

  Serial.printf("\nQ      vital stats\n");

  Serial.printf("\nR<?>   Reset\n");
  Serial.printf("  Rb= "); Serial.printf("batteries to present inputs\n");
  Serial.printf("  Rf= "); Serial.printf("fault logic latches\n");
  Serial.printf("  Ri= "); Serial.printf("all delta_q_inf\n");
  Serial.printf("  Rr= "); Serial.printf("saturate Mon and equalize Sim & Mon\n");
  Serial.printf("  RR= "); Serial.printf("DEPLOY\n");
  Serial.printf("  Rs= "); Serial.printf("small.  Reinitialize filters\n");

  Serial.printf("\ns  curr select mode (-1=noa, 0=auto, 1=amp) = "); Serial.println(rp.ib_select);

  Serial.printf("\nv= "); Serial.print(rp.debug); Serial.println(": verbosity, -128 - +128. [4]");
  Serial.printf("  -<>: Negative - Arduino plot compatible\n");
  // Serial.printf("  v-1: Debug\n");
  Serial.printf("   v1: GP\n");
  Serial.printf("   v2: GP, Sim & Sel\n");
  Serial.printf("   v3: EKF\n");
  // Serial.printf("   v5: OLED display\n");
  // Serial.printf("   v7: EKF solver init\n");
  // Serial.printf("   v8: Randles SS init\n");
  Serial.printf("  v12: EKF\n");
//  Serial.printf(" v-13: ib_dscn\n");
  // Serial.printf("  v14: vshunt and Ib raw\n");
  // Serial.printf("  v15: vb raw\n");
  // Serial.printf("  v16: Tb\n");
  // Serial.printf("  v34: EKF detail\n");
  // Serial.printf("  v35: Randles balance\n");
  // Serial.printf("  v37: EKF short\n");
  // Serial.printf("  v75: voc_low check mod\n");
  // Serial.printf("  v76: vb model\n");
  // Serial.printf("  v78: Batt model sat\n");
  // Serial.printf("  v79: sat_ib model\n");
  // Serial.printf("  v96: CC sat\n");

  Serial.printf("\nV<?> - VOC(SOC) curve deltas\n");
  Serial.printf(" VM= "); Serial.printf("%6.3f", Mon->dv_voc_soc()); Serial.println(": Mon vsoc out, V [0]"); 
  Serial.printf(" VS= "); Serial.printf("%6.3f", Sen->Sim->dv_voc_soc()); Serial.println(": Sim vsoc out, V [0]"); 
  Serial.printf(" Vm= "); Serial.printf("%6.3f", Mon->ds_voc_soc()); Serial.println(": Mon soc in [0]"); 
  Serial.printf(" Vs= "); Serial.printf("%6.3f", Sen->Sim->ds_voc_soc()); Serial.println(": Sim soc in[0]"); 

  Serial.printf("\nW<?> - iters to wait\n");

  Serial.printf("\nX<?> - Test Mode.   For example:\n");
  Serial.printf(" Xd=  "); Serial.printf("%d,   dc-dc charger on [0]\n", cp.dc_dc_on);
  Serial.printf(" Xm=  "); Serial.printf("%d,   modeling bitmap [0b0000]\n", rp.modeling);
  Serial.printf("      0x8 tweak_test = %d\n", rp.tweak_test());
  Serial.printf("      0x4 current = %d\n", rp.mod_ib());
  Serial.printf("      0x2 voltage = %d\n", rp.mod_vb());
  Serial.printf("      0x1 temp = %d\n", rp.mod_tb());
  Serial.printf(" Xa= "); Serial.printf("%6.3f", rp.amp); Serial.println(": Inj amp A pk (0-18.3) [0]");
  Serial.printf(" Xb= "); Serial.printf("%6.3f", rp.inj_bias); Serial.println(": Inj bias A [0]");
  Serial.printf(" Xf= "); Serial.printf("%6.3f", rp.freq/2./PI); Serial.println(": Inj freq Hz (0-2) [0]");
  Serial.printf(" Xt=  "); Serial.printf("%d", rp.type); Serial.println(": Inj 's'=sin(1), 'q'=square(2), 't'=tri(3), biases(4,5,6), 'o'=cos(8))");
 Serial.printf(" Xo= "); Serial.printf("%6.3f", rp.inj_bias); Serial.println(": Inj inj_bias A (-18.3-18.3) [0]");
  Serial.printf(" Xp= <?>, scripted tests...\n"); 
  Serial.printf("    Xp-1:  Off, modeling false\n");
  Serial.printf("    Xp0:  reset tests\n");
  Serial.printf("    Xp1:  1 Hz sine\n");
  Serial.printf("    Xp2:  1 Hz square\n");
  Serial.printf("    Xp3:  1 Hz triangle\n");
  Serial.printf("    Xp4:  -1C soft disch, reset xp0 or Di0\n");
  Serial.printf("    Xp5:  +1C soft chg\n");
  Serial.printf("    Xp6:  +/-500 A pulse\n");
  Serial.printf("    Xp9:  silent tweak test\n");
  Serial.printf("    Xp10: tweak cycle test\n");
  Serial.printf("    Xp11: slow cycle test\n");
  Serial.printf("    Xp12: slow half cycle reg test\n");
  Serial.printf("    Xp20: collect fast data\n");
  Serial.printf("    Xp21: collect slow data\n");
  Serial.printf(" XC= "); Serial.printf("%6.3f cycles inj\n", Sen->cycles_inj);
  Serial.printf(" XR  "); Serial.printf("RUN inj\n");
  Serial.printf(" XS  "); Serial.printf("STOP inj\n");
  Serial.printf(" Xs= "); Serial.printf("%4.2f scalar on T_SAT\n", cp.s_t_sat);
  Serial.printf(" XW= "); Serial.printf("%7.2f s wait start inj\n", float(Sen->wait_inj)/1000.);
  Serial.printf(" XT= "); Serial.printf("%7.2f s tail end inj\n", float(Sen->tail_inj)/1000.);
  Serial.printf(" Xu= "); Serial.printf("%d T=ignore Tb read\n", Sen->Flt->fail_tb());
  Serial.printf(" Xv= "); Serial.printf("%4.2f scale Tb 1-wire stale persist\n", Sen->Flt->tb_stale_time_sclr());
  Serial.printf("\n*Xx<> = SRAM saved\n");
  Serial.printf("\nurgency of cmds: -=ASAP,*=SOON, '' or +=QUEUE\n"); 
  Serial.printf("\nh this\n");
}
