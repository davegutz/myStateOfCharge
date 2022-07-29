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
#include "mySensors.h"
#include "local_config.h"
#include <math.h>
#include "debug.h"

extern CommandPars cp;          // Various parameters shared at system level
extern PublishPars pp;            // For publishing
extern RetainedPars rp;         // Various parameters to be static at system level


// class TempSensor
// constructors
TempSensor::TempSensor(const uint16_t pin, const bool parasitic, const uint16_t conversion_delay)
: DS18(pin, parasitic, conversion_delay)
{
   SdTbatt = new SlidingDeadband(HDB_TBATT);
}
TempSensor::~TempSensor() {}
// operators
// functions
float TempSensor::load(Sensors *Sen)
{
  // Read Sensor
  // MAXIM conversion 1-wire Tp plenum temperature
  uint8_t count = 0;
  float temp = 0.;
  float Tbatt_hdwe = 0.;
  // Read hardware and check
  while ( ++count<MAX_TEMP_READS && temp==0 && !rp.mod_tb() )
  {
    if ( read() ) temp = celsius() + (TBATT_TEMPCAL);
    delay(1);
  }

  // Check success
  if ( count<MAX_TEMP_READS && TEMP_RANGE_CHECK<temp && temp<TEMP_RANGE_CHECK_MAX )
  {
    Tbatt_hdwe = SdTbatt->update(temp);
    if ( rp.debug==16 ) Serial.printf("I:  t=%7.3f ct=%d, Tbatt_hdwe=%7.3f,\n", temp, count, Tbatt_hdwe);
  }
  else
  {
    Serial.printf("E: DS18, t=%8.1f, ct=%d, using lgv\n", temp, count);
    // Using last-good-value:  no assignment
  }
  return ( Tbatt_hdwe );
}


// class Shunt
// constructors
Shunt::Shunt()
: Tweak(), Adafruit_ADS1015(), name_("None"), port_(0x00), bare_(false){}
Shunt::Shunt(const String name, const uint8_t port, float *rp_delta_q_cinf, float *rp_delta_q_dinf, float *rp_tweak_sclr,
  float *cp_ibatt_bias, const float v2a_s)
: Tweak(name, TWEAK_MAX_CHANGE, TWEAK_MAX, TWEAK_WAIT, rp_delta_q_cinf, rp_delta_q_dinf, rp_tweak_sclr, COULOMBIC_EFF),
  Adafruit_ADS1015(),
  name_(name), port_(port), bare_(false), cp_ibatt_bias_(cp_ibatt_bias), v2a_s_(v2a_s),
  vshunt_int_(0), vshunt_int_0_(0), vshunt_int_1_(0), vshunt_(0), ishunt_cal_(0), slr_(1.), add_(0.)
{
  if ( name_=="No Amp")
    setGain(GAIN_SIXTEEN, GAIN_SIXTEEN); // 16x gain differential and single-ended  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  else if ( name_=="Amp")
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  else
    setGain(GAIN_EIGHT, GAIN_TWO); // First argument is differential, second is single-ended.
  if (!begin(port_)) {
    Serial.printf("FAILED to initialize ADS SHUNT MONITOR %s\n", name_.c_str());
    bare_ = true;
  }
  else Serial.printf("SHUNT MONITOR %s initialized\n", name_.c_str());
}
Shunt::~Shunt() {}
// operators
// functions

void Shunt::pretty_print()
{
  Serial.printf("Shunt(%s)::\n", name_.c_str());
  Serial.printf("  port_ =                0x%X; // I2C port used by Acafruit_ADS1015\n", port_);
  Serial.printf("  bare_ =                   %d; // If ADS to be ignored\n", bare_);
  Serial.printf("  *cp_ibatt_bias_ =   %7.3f; // Global bias, A\n", *cp_ibatt_bias_);
  Serial.printf("  v2a_s_ =            %7.2f; // Selected shunt conversion gain, A/V\n", v2a_s_);
  Serial.printf("  vshunt_int_ =           %d; // Sensed shunt voltage, count\n", vshunt_int_);
  Serial.printf("  ishunt_cal_ =       %7.3f; // Sensed, calibrated ADC, A\n", ishunt_cal_);
  Serial.printf("Shunt(%s)::", name_.c_str()); Tweak::pretty_print();
  Serial.printf("Shunt(%s)::", name_.c_str()); Adafruit_ADS1015::pretty_print(name_);
}

// load
void Shunt::load()
{
  if ( !bare_ && !rp.mod_ib() )
  {
    if ( rp.debug>102 ) Serial.printf("begin %s->readADC_Differential_0_1 at %ld...", name_.c_str(), millis());

    vshunt_int_ = readADC_Differential_0_1();
    
    if ( rp.debug>102 ) Serial.printf("done at %ld\n", millis());
    if ( rp.debug==-14 ) { vshunt_int_0_ = readADC_SingleEnded(0);  vshunt_int_1_ = readADC_SingleEnded(1); }
                    else { vshunt_int_0_ = 0;                       vshunt_int_1_ = 0; }
  }
  else
  {
    vshunt_int_0_ = 0; vshunt_int_1_ = 0; vshunt_int_ = 0;
  }
  vshunt_ = computeVolts(vshunt_int_);
  ishunt_cal_ = vshunt_*v2a_s_*float(!rp.modeling) + *cp_ibatt_bias_;
}


// Class Sensors
Sensors::Sensors(double T, double T_temp, byte pin_1_wire, Sync *PublishSerial, Sync *ReadSensors):
  Ibatt_amp_fa_(false), Ibatt_noamp_fa_(false), Vbatt_fa_(false), Vbatt_flt_(false),
  rp_tbatt_bias_(&rp.tbatt_bias), tbatt_bias_last_(0.)
{
  this->T = T;
  this->T_filt = T;
  this->T_temp = T_temp;
  this->ShuntAmp = new Shunt("Amp", 0x49, &rp.delta_q_cinf_amp, &rp.delta_q_dinf_amp, &rp.tweak_sclr_amp,
    &cp.ibatt_tot_bias_amp, shunt_amp_v2a_s);
  if ( rp.debug>102 )
  {
      Serial.printf("New Shunt('Amp'):\n");
      this->ShuntAmp->pretty_print();
  }
  this->ShuntNoAmp = new Shunt("No Amp", 0x48, &rp.delta_q_cinf_noamp, &rp.delta_q_dinf_noamp, &rp.tweak_sclr_noamp,
    &cp.ibatt_tot_bias_noamp, shunt_noamp_v2a_s);
  if ( rp.debug>102 )
  {
      Serial.printf("New Shunt('No Amp'):\n");
      this->ShuntNoAmp->pretty_print();
  }
  this->SensorTbatt = new TempSensor(pin_1_wire, TEMP_PARASITIC, TEMP_DELAY);
  this->TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);
  this->Sim = new BatteryModel(&rp.delta_q_model, &rp.t_last_model, &rp.s_cap_model, &rp.nP, &rp.nS, &rp.sim_mod);
  this->IbattErrFilt = new LagTustin(0.1, TAU_ERR_FILT, -MAX_ERR_FILT, MAX_ERR_FILT);  // actual update time provided run time
  this->IbattErrPersist = new TFDelay();
  this->IbattAmpHardFail  = new TFDelay();
  this->IbattNoAmpHardFail  = new TFDelay();
  this->VbattHardFail  = new TFDelay();
  this->elapsed_inj = 0UL;
  this->start_inj = 0UL;
  this->stop_inj = 0UL;
  this->end_inj = 0UL;
  this->cycles_inj = 0.;
  this->PublishSerial = PublishSerial;
  this->ReadSensors = ReadSensors;
  this->display = true;
  this->sclr_coul_eff = 1.;
  this->Ibatt_hdwe_model = 0.;
  this->ib_sel_stat_ = 1;     // Default is amp
}

// Bias model outputs for sensor fault injection
void Sensors::bias_all_model()
{
  Ibatt_amp_model = ShuntAmp->bias_any( Ibatt_model );
  Ibatt_noamp_model = ShuntNoAmp->bias_any( Ibatt_model );
}


// Deliberate choice based on results and inputs
// Inputs:  ib_sel_stat_, Ibatt_amp_hdwe, Ibatt_noamp_hdwe, Ibatt_amp_model, Ibatt_noamp_model
// Outputs:  Ibatt_hdwe_model, Ibatt_hdwe, sclr_coul_eff, Vshunt
void Sensors::choose_()
{
  if ( ib_sel_stat_>0 )
  {
      Vshunt = ShuntAmp->vshunt();
      Ibatt_hdwe = Ibatt_amp_hdwe;
      Ibatt_hdwe_model = Ibatt_amp_model;
      sclr_coul_eff = rp.tweak_sclr_amp;
  }
  else if ( ib_sel_stat_<0 )
  {
      Vshunt = ShuntNoAmp->vshunt();
      Ibatt_hdwe = Ibatt_noamp_hdwe;
      Ibatt_hdwe_model = Ibatt_noamp_model;
      sclr_coul_eff = rp.tweak_sclr_noamp;
  }
  else
  {
      Vshunt = 0.;
      Ibatt_hdwe = 0.;
      Ibatt_hdwe_model = 0.;
      sclr_coul_eff = 1.;
  }
}

// Calculate selection for choice
// Use model instead of sensors when running tests as user
// Equivalent to using voc(soc) as voter between two hardware currrents
// Over-ride sensed Ib, Vb and Tb with model when running tests
// TODO:  fix this nomenclature
// Inputs:  Sen->Ibatt_model, Sen->Ibatt_hdwe,
//          Sen->Vbatt_model, Sen->Vbatt_hdwe,
//          ----------------, Sen->Tbatt_hdwe, Sen->Tbatt_hdwe_filt
// Outputs: Ibatt,
//          Vbatt,
//          Tbatt, Tbatt_filt
void Sensors::select_all(BatteryMonitor *Mon)
{
  // EKF error test - failure conditions track poorly
  cc_diff_ = Mon->soc_ekf() - Mon->soc();  // These are filtered in their construction (EKF is a dynamic filter and 
                                                  // Coulomb counter is a big integrator)
  cc_flt_ = abs(cc_diff_) >= SOC_DISAGREE_THRESH;

  // Compare current sensors - failure conditions large difference
  // Difference error, filter, check, persist
  if ( rp.mod_ib() ) ib_diff_ = Ibatt_amp_model - Ibatt_noamp_model;
  else ib_diff_ = Ibatt_amp_hdwe - Ibatt_noamp_hdwe;
  ib_diff_f_ = IbattErrFilt->calculate(ib_diff_, reset, min(T, MAX_ERR_T));
  ib_diff_flt_ = abs(ib_diff_f_) >= IBATT_DISAGREE_THRESH;
  ib_diff_fa_ = IbattErrPersist->calculate(ib_diff_flt_, IBATT_DISAGREE_SET, IBATT_DISAGREE_RESET, T, reset);

  // Truth table
  static int8_t ib_sel_stat_last = ib_sel_stat_;
  if ( ShuntAmp->bare() && ShuntNoAmp->bare() )
  {
    ib_sel_stat_ = 0;
  }
  else if ( rp.ibatt_select>0 && !ShuntAmp->bare() )
  {
    ib_sel_stat_ = 1;
  }
  else if ( rp.ibatt_select<0 && !ShuntNoAmp->bare() )
  {
    ib_sel_stat_ = -1;
  }
  else if ( rp.ibatt_select==0 )
  {
    if ( ShuntAmp->bare() && !ShuntNoAmp->bare() )
    {
      ib_sel_stat_ = -1;
    }
    else if ( ShuntAmp->bare() || (ib_diff_fa_ && cc_flt_) )
    {
      ib_sel_stat_ = -1;
    }
    else
    {
      ib_sel_stat_ = 1;
    }
  }
  if ( ib_sel_stat_ != ib_sel_stat_last )
      Serial.printf("Select change:  ShuntAmp->bare=%d, ShuntNoAmp->bare=%d, ibatt_err_fail=%d, cc_flt_=%d, rp.ibatt_select=%d, ibatt_sel_status=%d,\n",
        ShuntAmp->bare(), ShuntNoAmp->bare(), ib_diff_fa_, cc_flt_, rp.ibatt_select, ib_sel_stat_);

  // Reselect since may be changed
  // Inputs:  ib_sel_stat_, Ibatt_amp_hdwe, Ibatt_noamp_hdwe, Ibatt_amp_model, Ibatt_noamp_model
  // Outputs:  Ibatt_hdwe_model, Ibatt_hdwe, sclr_coul_eff, Vshunt
  choose_();

  // Final assignments
  if ( rp.mod_ib() )  Ibatt = Ibatt_hdwe_model;
  else Ibatt = Ibatt_hdwe;

  if ( rp.mod_vb() )  Vbatt = Vbatt_model;
  else Vbatt = Vbatt_hdwe;
  
  if ( rp.mod_tb() )
  {
    Tbatt = RATED_TEMP;
    Tbatt_filt = Tbatt;
  }
  else
  {
    Tbatt = Tbatt_hdwe;
    Tbatt_filt = Tbatt_hdwe_filt;
  }
  if ( rp.debug==26 ) // print_signal_select
  {
      double cTime;
      if ( rp.tweak_test() ) cTime = double(now)/1000.;
      else cTime = control_time;
      sprintf(cp.buffer, "unit_sel,%13.3f, %d, %d,    %d, %d,      %9.6f, %d,   %7.5f,%7.5f,%7.5f,%7.5f,%7.5f,   %7.5f, %d, %d,     %d, %7.5f,%7.5f, %d, %7.5f,    %7.5f,%7.5f, %d, %7.5f,   %5.2f,%5.2f, %d, %5.2f, %c,",
          cTime, reset, rp.ibatt_select,
          ShuntAmp->bare(), ShuntNoAmp->bare(),
          cc_diff_, cc_flt_,
          Ibatt_amp_hdwe, Ibatt_noamp_hdwe, Ibatt_amp_model, Ibatt_noamp_model, Ibatt_model, 
          ib_diff_, ib_diff_flt_, ib_diff_fa_,
          ib_sel_stat_, Ibatt_hdwe, Ibatt_hdwe_model, rp.mod_ib(), Ibatt,
          Vbatt_hdwe, Vbatt_model, rp.mod_vb(), Vbatt,
          Tbatt_hdwe, Tbatt, rp.mod_tb(), Tbatt_filt,
          '\0');
      Serial.println(cp.buffer);
  }
  ib_sel_stat_last = ib_sel_stat_;
}

// Current bias.  Feeds into signal conversion
void Sensors::shunt_bias(void)
{
  if ( !rp.mod_ib() )
  {
    ShuntAmp->bias( rp.ibatt_bias_amp + rp.ibatt_bias_all + rp.inj_bias );
    ShuntNoAmp->bias( rp.ibatt_bias_noamp + rp.ibatt_bias_all + rp.inj_bias );
  }
}

// Checks analog current.  Latches
void Sensors::shunt_check(BatteryMonitor *Mon)
{
    if ( reset )
    {
        Ibatt_amp_fa_ = false;
        Ibatt_noamp_fa_ = false;
    }
    float current_max = RATED_BATT_CAP * Mon->nP();
    Ibatt_amp_flt_ = ShuntAmp->ishunt_cal()  <= -current_max || ShuntAmp->ishunt_cal() >= current_max;
    Ibatt_noamp_flt_ = ShuntNoAmp->ishunt_cal()  <= -current_max || ShuntNoAmp->ishunt_cal() >= current_max;
    Ibatt_amp_fa_ = Ibatt_amp_fa_ || IbattAmpHardFail->calculate(Ibatt_amp_flt_, IBATT_HARD_SET, IBATT_HARD_RESET, T, reset);
    Ibatt_noamp_fa_ = Ibatt_noamp_fa_ || IbattNoAmpHardFail->calculate(Ibatt_noamp_flt_, IBATT_HARD_SET, IBATT_HARD_RESET, T, reset);
}

// Read and convert shunt Sensors
void Sensors::shunt_load(void)
{
    ShuntAmp->load();
    ShuntNoAmp->load();
}

// Print Shunt selection data
void Sensors::shunt_print()
{
    Serial.printf("reset,T,select,inj_bias,vs_int_a,Vshunt_a,Ibatt_hdwe_a,vs_int_na,Vshunt_na,Ibatt_hdwe_na,Ibatt_hdwe,T,sclr_coul_eff,Ibatt_amp_fault,Ibatt_amp_fail,Ibatt_noamp_fault,Ibatt_noamp_fail,=,    %d,%7.3f,%d,%7.3f,    %d,%7.3f,%7.3f,    %d,%7.3f,%7.3f,    %7.3f,%7.3f,  %7.3f, %d,%d,  %d,%d,\n",
        reset, T, rp.ibatt_select, rp.inj_bias, ShuntAmp->vshunt_int(), ShuntAmp->vshunt(), ShuntAmp->ishunt_cal(),
        ShuntNoAmp->vshunt_int(), ShuntNoAmp->vshunt(), ShuntNoAmp->ishunt_cal(),
        Ibatt_hdwe, T, sclr_coul_eff,
        Ibatt_amp_flt_, Ibatt_amp_fa_, Ibatt_noamp_flt_, Ibatt_noamp_fa_);
}

// Shunt selection.  Use Coulomb counter and EKF to sort three signals:  amp current, non-amp current, voltage
// Initial selection to charge the Sim for modeling currents on BMS cutback
// Inputs: rp.ibatt_select (user override), Mon (EKF status)
// States:  Ibatt_fail_noamp_
// Outputs:  Ibatt_hdwe, Ibatt_model_in, Vbatt_sel_status_
void Sensors::shunt_select_initial()
{
    // Current signal selection, based on if there or not.
    // Over-ride 'permanent' with Talk(rp.ibatt_select) = Talk('s')

    // Retrieve values.   Return values include scalar/adder for fault
    Ibatt_amp_model = ShuntAmp->bias_any( Ibatt_model );      // uses past Ib
    Ibatt_noamp_model = ShuntNoAmp->bias_any( Ibatt_model );  // uses pst Ib
    Ibatt_amp_hdwe = ShuntAmp->ishunt_cal();
    Ibatt_noamp_hdwe = ShuntNoAmp->ishunt_cal();

    // Initial choice
    // Inputs:  ib_sel_stat_, Ibatt_amp_hdwe, Ibatt_noamp_hdwe, Ibatt_amp_model(past), Ibatt_noamp_model(past)
    // Outputs:  Ibatt_hdwe_model, Ibatt_hdwe, sclr_coul_eff, Vshunt
    choose_();

    // Check for modeling
    if ( rp.mod_ib() )
        Ibatt_model_in = rp.inj_bias + rp.ibatt_bias_all;
    else
        Ibatt_model_in = Ibatt_hdwe;
}

// Filter temp
void Sensors::temp_filter(const boolean reset_loc, const float t_rlim)
{
    // Rate limit the temperature bias, 2x so not to interact with rate limits in logic that also use t_rlim
    if ( reset_loc ) tbatt_bias_last_ = *rp_tbatt_bias_;
    float t_bias_loc = max(min(*rp_tbatt_bias_,  tbatt_bias_last_ + t_rlim*2.*T_temp),
                                            tbatt_bias_last_ - t_rlim*2.*T_temp);
    tbatt_bias_last_ = t_bias_loc;

    // Filter and add rate limited bias
    if ( reset_loc && Tbatt>40. )  // Bootup T=85.5 C
    {
        Tbatt_hdwe = RATED_TEMP + t_bias_loc;
        Tbatt_hdwe_filt = TbattSenseFilt->calculate(RATED_TEMP, reset_loc,  min(T_temp, F_MAX_T_TEMP)) + t_bias_loc;
    }
    else
    {
        Tbatt_hdwe_filt = TbattSenseFilt->calculate(Tbatt_hdwe, reset_loc,  min(T_temp, F_MAX_T_TEMP)) + t_bias_loc;
        Tbatt_hdwe += t_bias_loc;
    }
    if ( rp.debug==16 ) Serial.printf("reset_loc,t_bias_loc, RATED_TEMP, Tbatt_hdwe, Tbatt_hdwe_filt, %d, %7.3f, %7.3f, %7.3f, %7.3f,\n",
      reset_loc,t_bias_loc, RATED_TEMP, Tbatt_hdwe, Tbatt_hdwe_filt );
}

// Filter temp
void Sensors::temp_load_and_filter(Sensors *Sen, const boolean reset_loc, const float t_rlim)
{
    Tbatt_hdwe = SensorTbatt->load(Sen);
    temp_filter(reset_loc, T_RLIM);
}

// Check analog voltage.  Latches
void Sensors::vbatt_check(BatteryMonitor *Mon, const float _Vbatt_min, const float _Vbatt_max)
{
    if ( reset ) Vbatt_fa_ = false;
    Vbatt_flt_ = Vbatt_hdwe <= _Vbatt_min*Mon->nS() || Vbatt_hdwe >= _Vbatt_max*Mon->nS();
    Vbatt_fa_ = Vbatt_fa_ || VbattHardFail->calculate(Vbatt_flt_, VBATT_HARD_SET, VBATT_HARD_RESET, T, reset);
}

// Load analog voltage
void Sensors::vbatt_load(const byte vbatt_pin)
{
    Vbatt_raw = analogRead(vbatt_pin);
    Vbatt_hdwe =  float(Vbatt_raw)*vbatt_conv_gain + float(VBATT_A) + rp.vbatt_bias;
}

// Print analog voltage
void Sensors::vbatt_print()
{
  Serial.printf("reset, T, Vbatt_raw, rp.vbatt_bias, Vbatt_hdwe, Vbatt_fault, Vbatt_fail=, %d, %7.3f, %d, %7.3f,  %7.3f, %d, %d,\n",
    reset, T, Vbatt_raw, rp.vbatt_bias, Vbatt_hdwe, Vbatt_flt_, Vbatt_fa_);
}
