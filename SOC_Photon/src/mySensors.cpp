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
    if ( rp.debug==-103 ) Serial.printf("I:  t=%7.3f ct=%d\n", temp, count);
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
  vshunt_int_(0), vshunt_int_0_(0), vshunt_int_1_(0), vshunt_(0), ishunt_cal_(0)
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
Sensors::Sensors(double T, double T_temp, byte pin_1_wire, Sync *PublishSerial, Sync *ReadSensors)
{
    this->T = T;
    this->T_filt = T;
    this->T_temp = T_temp;
    this->ShuntAmp = new Shunt("Amp", 0x49, &rp.delta_q_cinf_amp, &rp.delta_q_dinf_amp, &rp.tweak_sclr_amp, &cp.ibatt_bias_amp,
        shunt_amp_v2a_s);
    if ( rp.debug>102 )
    {
        Serial.printf("New Shunt('Amp'):\n");
        this->ShuntAmp->pretty_print();
    }
    this->ShuntNoAmp = new Shunt("No Amp", 0x48, &rp.delta_q_cinf_noamp, &rp.delta_q_dinf_noamp, &rp.tweak_sclr_noamp,
        &cp.ibatt_bias_noamp, shunt_noamp_v2a_s);
    if ( rp.debug>102 )
    {
        Serial.printf("New Shunt('No Amp'):\n");
        this->ShuntNoAmp->pretty_print();
    }
    this->SensorTbatt = new TempSensor(pin_1_wire, TEMP_PARASITIC, TEMP_DELAY);
    this->TbattSenseFilt = new General2_Pole(double(READ_DELAY)/1000., F_W_T, F_Z_T, -20.0, 150.);
    this->Sim = new BatteryModel(&rp.delta_q_model, &rp.t_last_model, &rp.s_cap_model, &rp.nP, &rp.nS, &rp.sim_mod);
    this->elapsed_inj = 0UL;
    this->start_inj = 0UL;
    this->stop_inj = 0UL;
    this->end_inj = 0UL;
    this->cycles_inj = 0.;
    this->PublishSerial = PublishSerial;
    this->ReadSensors = ReadSensors;
    this->display = true;
    this->sclr_coul_eff = 1.;
}

// Bookkeep time
double Sensors::keep_time(const unsigned long now_)
{
    static unsigned long int past = now_;
    past = now_;
    now = now_;
    return (now_ - past)/1e3;
}

// Current bias.  Feeds into signal conversion
void Sensors::shunt_bias(void)
{
  if ( rp.mod_ib() )
  {
    ShuntAmp->bias( rp.ibatt_bias_all + rp.inj_bias );
    ShuntNoAmp->bias( rp.ibatt_bias_all + rp.inj_bias );
  }
  else
  {
    ShuntAmp->bias( rp.ibatt_bias_amp + rp.ibatt_bias_all + rp.inj_bias );
    ShuntNoAmp->bias( rp.ibatt_bias_noamp + rp.ibatt_bias_all + rp.inj_bias );
  }
}

// Read and convert shunt Sensors
void Sensors::shunt_load(void)
{
  ShuntAmp->load();
  ShuntNoAmp->load();
}

// Print Shunt selection data
void Sensors::shunt_print(const boolean reset_free, const double T)
{
    Serial.printf("reset_free,select,inj_bias,vs_int_a,Vshunt_a,Ibatt_hdwe_a,vs_int_na,Vshunt_na,Ibatt_hdwe_na,Ibatt_hdwe,T,sclr_coul_eff=,    %d,%d,%7.3f,    %d,%7.3f,%7.3f,    %d,%7.3f,%7.3f,    %7.3f,%7.3f,  %7.3f,\n",
        reset_free, rp.ibatt_sel_noamp, rp.inj_bias, ShuntAmp->vshunt_int(), ShuntAmp->vshunt(), ShuntAmp->ishunt_cal(),
        ShuntNoAmp->vshunt_int(), ShuntNoAmp->vshunt(), ShuntNoAmp->ishunt_cal(),
        Ibatt_hdwe, T, sclr_coul_eff);
}

// Shunt selection
void Sensors::shunt_select(void)
{
  // Current signal selection, based on if there or not.
  // Over-ride 'permanent' with Talk(rp.ibatt_sel_noamp) = Talk('s')
  float model_ibatt_bias = 0.;

  // Check for bare sensor
  if ( !rp.ibatt_sel_noamp && !ShuntAmp->bare() )
  {
    Vshunt = ShuntAmp->vshunt();
    Ibatt_hdwe = ShuntAmp->ishunt_cal();
    model_ibatt_bias = ShuntAmp->bias();
    sclr_coul_eff = rp.tweak_sclr_amp;
  }
  else if ( !ShuntNoAmp->bare() )
  {
    Vshunt = ShuntNoAmp->vshunt();
    Ibatt_hdwe = ShuntNoAmp->ishunt_cal();
    model_ibatt_bias = ShuntNoAmp->bias();
    sclr_coul_eff = rp.tweak_sclr_noamp;
  }
  else
  {
    Vshunt = 0.;
    Ibatt_hdwe = 0.;
    model_ibatt_bias = 0.;
    sclr_coul_eff = 1.;
  }

  // Check for modeling
  if ( rp.modeling )
    Ibatt_model_in = model_ibatt_bias;
  else
    Ibatt_model_in = Ibatt_hdwe;
}

// Filter temp
void Sensors::temp_filter(const int reset_loc, const float t_rlim, const float tbatt_bias, float *tbatt_bias_last)
{
  // Rate limit the temperature bias, 2x so not to interact with rate limits in logic that also use t_rlim
  if ( reset_loc ) *tbatt_bias_last = tbatt_bias;
  float t_bias_loc = max(min(tbatt_bias,  *tbatt_bias_last + t_rlim*2.*T_temp),
                                          *tbatt_bias_last - t_rlim*2.*T_temp);
  *tbatt_bias_last = t_bias_loc;

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
}

// Filter temp
void Sensors::temp_load_and_filter(Sensors *Sen, const int reset_loc, const float t_rlim, const float tbatt_bias,
    float *tbatt_bias_last)
{
    SensorTbatt->load(Sen);
    temp_filter(reset_loc, T_RLIM, rp.tbatt_bias, tbatt_bias_last);
}

// Load analog voltage
void Sensors::vbatt_load(const byte vbatt_pin)
{
    if ( !rp.mod_vb() ) Vbatt_raw = analogRead(vbatt_pin);
    Vbatt_hdwe =  float(Vbatt_raw)*vbatt_conv_gain + float(VBATT_A) + rp.vbatt_bias;
}

// Print analog voltage
void Sensors::vbatt_print()
{
  Serial.printf("Vbatt_raw, rp.vbatt_bias, Vbatt_hdwe=, %d, %7.3f,  %7.3f,\n",
    Vbatt_raw, rp.vbatt_bias, Vbatt_hdwe);
}
