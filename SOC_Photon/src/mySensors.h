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

#ifndef _MY_SENSORS_H
#define _MY_SENSORS_H

#include "myLibrary/myFilters.h"
#include "Battery.h"
#include "constants.h"
#include "myCloud.h"
#include "myTalk.h"
#include "retained.h"
#include "command.h"
#include "Tweak.h"
#include "mySync.h"

// Temp sensor
#include <hardware/OneWire.h>
#include <hardware/DS18.h>

// AD
#include <Adafruit/Adafruit_ADS1X15.h>

extern RetainedPars rp; // Various parameters to be static at system level
extern PublishPars pp;  // For publishing
extern CommandPars cp;  // Various parameters to be static at system level

// DS18-based temp sensor
class TempSensor: public DS18
{
public:
  TempSensor();
  TempSensor(const uint16_t pin, const bool parasitic, const uint16_t conversion_delay);
  ~TempSensor();
  // operators
  // functions
  float load(Sensors *Sen);
  float noise();
protected:
  SlidingDeadband *SdTbatt;
};


// ADS1015-based shunt
class Shunt: public Tweak, Adafruit_ADS1015
{
public:
  Shunt();
  Shunt(const String name, const uint8_t port, float *rp_delta_q_cinf, float *rp_delta_q_dinf, float *rp_tweak_sclr,
    float *cp_ibatt_bias, const float v2a_s);
  ~Shunt();
  // operators
  // functions
  float add() { return ( add_ ); };
  void add(const float add) { add_ = add; };
  boolean bare() { return ( bare_ ); };
  void bias(const float bias) { *cp_ibatt_bias_ = bias; };
  float bias() { return ( *cp_ibatt_bias_*slr_ + add_ ); };
  float bias_any(const float Ib) { return ( Ib*slr_ + add_ ); };
  float ishunt_cal() { return ( ishunt_cal_*slr_ + add_ ); };
  void load();
  void pretty_print();
  float slr() { return ( slr_ ); };
  void slr(const float slr) { slr_ = slr; };
  float v2a_s() { return ( v2a_s_ ); };
  float vshunt() { return ( vshunt_ ); };
  int16_t vshunt_int() { return ( vshunt_int_ ); };
  int16_t vshunt_int_0() { return ( vshunt_int_0_ ); };
  int16_t vshunt_int_1() { return ( vshunt_int_1_ ); };
protected:
  String name_;         // For print statements, multiple instances
  uint8_t port_;        // Octal I2C port used by Acafruit_ADS1015
  boolean bare_;        // If ADS to be ignored
  float *cp_ibatt_bias_;// Global bias, A
  float v2a_s_;         // Selected shunt conversion gain, A/V
  int16_t vshunt_int_;  // Sensed shunt voltage, count
  int16_t vshunt_int_0_;// Interim conversion, count
  int16_t vshunt_int_1_;// Interim conversion, count
  float vshunt_;        // Sensed shunt voltage, V
  float ishunt_cal_;    // Sensed, calibrated ADC, A
  float slr_;           // Scalar for fault test
  float add_;           // Adder for fault test, A
};


// Sensors (like a big struct with public access)
class Sensors
{
public:
    Sensors();
    Sensors(double T, double T_temp, byte pin_1_wire, Sync *PublishSerial, Sync *ReadSensors);
    ~Sensors();
    int Vbatt_raw;                  // Raw analog read, integer
    float Vbatt;                    // Selected battery bank voltage, V
    float Vbatt_hdwe;               // Sensed battery bank voltage, V
    float Vbatt_model;              // Modeled battery bank voltage, V
    float Tbatt;                    // Selected battery bank temp, C
    float Tbatt_filt;               // Selected filtered battery bank temp, C
    float Tbatt_hdwe;               // Sensed battery temp, C
    float Tbatt_hdwe_filt;          // Filtered, sensed battery temp, C
    float Tbatt_model;              // Temperature used for battery bank temp in model, C
    float Tbatt_model_filt;         // Filtered, modeled battery bank temp, C
    float Vshunt;                   // Sensed shunt voltage, V
    float Ibatt;                    // Selected battery bank current, A
    float Ibatt_amp_hdwe;           // Sensed amp battery bank current, A
    float Ibatt_amp_model;          // Modeled amp battery bank current, A
    float Ibatt_noamp_hdwe;         // Sensed noamp battery bank current, A
    float Ibatt_noamp_model;        // Modeled noamp battery bank current, A
    float Ibatt_hdwe;               // Sensed battery bank current, A
    float Ibatt_hdwe_model;         // Selected model hardware signal, A
    float Ibatt_model;              // Modeled battery bank current, A
    float Ibatt_model_in;           // Battery bank current input to model (modified by cutback), A
    float Wbatt;                    // Sensed battery bank power, use to compare to other shunts, W
    unsigned long int now;          // Time at sample, ms
    double T;                       // Update time, s
    boolean reset;                  // Reset flag, T = reset
    double T_filt;                  // Filter update time, s
    double T_temp;                  // Temperature update time, s
    Sync *PublishSerial;            // Handle to debug print time
    Sync *ReadSensors;              // Handle to debug read time
    boolean saturated;              // Battery saturation status based on Temp and VOC
    Shunt *ShuntAmp;                // Ib sense amplified
    Shunt *ShuntNoAmp;              // Ib sense non-amplified
    TempSensor* SensorTbatt;        // Tb sense
    General2_Pole* TbattSenseFilt;  // Linear filter for Tb. There are 1 Hz AAFs in hardware for Vb and Ib
    SlidingDeadband *SdTbatt;       // Non-linear filter for Tb
    BatteryModel *Sim;              // Used to model Vb and Ib.   Use Talk 'Xp?' to toggle model on/off
    LagTustin *IbattErrFilt;        // Noise filter for signal selection
    TFDelay *IbattErrPersist;          // Persistence current error for signal selection 
    TFDelay *IbattAmpHardFail;      // Persistence voltage range check for signall selection 
    TFDelay *IbattNoAmpHardFail;    // Persistence voltage range check for signall selection 
    TFDelay *VbattHardFail;         // Persistence voltage range check for signall selection 
    unsigned long int elapsed_inj;  // Injection elapsed time, ms
    unsigned long int start_inj;    // Start of calculated injection, ms
    unsigned long int stop_inj;     // Stop of calculated injection, ms
    unsigned long int wait_inj;     // Wait before start injection, ms
    unsigned long int end_inj;      // End of print injection, ms
    unsigned long int tail_inj;     // Tail after end injection, ms
    float cycles_inj;               // Number of injection cycles
    double control_time;            // Decimal time, seconds since 1/1/2021
    boolean display;                // Use display
    double sclr_coul_eff;           // Scalar on Coulombic Efficiency
    void bias_all_model();          // Bias model outputs for sensor fault injection
    void select_all(BatteryMonitor *Mon, const boolean reset);  // Final choices
    void shunt_bias(void);          // Load biases into Shunt objects
    void shunt_check(BatteryMonitor *Mon);  // Range check Ibatt signals
    void shunt_load(void);          // Load ADS015 protocol
    void shunt_print(); // Print selection result
    void shunt_select_initial();   // Choose between shunts for model
    void temp_filter(const boolean reset_loc, const float t_rlim);
    void temp_load_and_filter(Sensors *Sen, const boolean reset_loc, const float t_rlim);
    void vbatt_check(BatteryMonitor *Mon, const float _Vbatt_min, const float _Vbatt_max);  // Range check Vbatt
    void vbatt_load(const byte vbatt_pin);  // Analog read of Vbatt
    float Tbatt_noise();
    float Tbatt_noise_amp() { return ( Tbatt_noise_amp_ ); };
    void Tbatt_noise_amp(const float noise) { Tbatt_noise_amp_ = noise; };
    float Vbatt_noise();
    float Vbatt_noise_amp() { return ( Vbatt_noise_amp_ ); };
    void Vbatt_noise_amp(const float noise) { Vbatt_noise_amp_ = noise; };
    float Ibatt_noise();
    float Ibatt_noise_amp() { return ( Ibatt_noise_amp_ ); };
    void Ibatt_noise_amp(const float noise) { Ibatt_noise_amp_ = noise; };
    void vbatt_print(void);         // Print Vbatt result
    boolean Ibatt_amp_fail() { return Ibatt_amp_fa_; };
    boolean Ibatt_noamp_fail() { return Ibatt_noamp_fa_; };
    boolean Vbatt_fail() { return Vbatt_fa_; };
    int8_t ibatt_sel_status() { return ib_sel_stat_; };
protected:
    boolean cc_flt_;          // EKF tested disagree, T = error
    float cc_diff_;           // EKF tracking error, C
    boolean Ibatt_amp_fa_;    // Amp sensor selection memory, T = amp failed
    boolean Ibatt_amp_flt_;   // Momentary isolation of Ibatt failure, T=faulted 
    float ib_diff_;           // Current sensor difference error, A
    float ib_diff_f_;         // Filtered sensor difference error, A
    boolean ib_diff_fa_;      // Persisted sensor difference error, T = fail
    boolean ib_diff_flt_;     // Faulted sensor difference error, T = fault
    boolean Ibatt_noamp_fa_;  // Noamp sensor selection memory, T = no amp failed
    boolean Ibatt_noamp_flt_; // Momentary isolation of Ibatt failure, T=faulted 
    boolean Vbatt_fa_;        // Peristed, latched isolation of Vbatt failure, T=failed
    boolean Vbatt_flt_;       // Momentary isolation of Vbatt failure, T=faulted
    int8_t ib_sel_stat_;      // Memory of Ibatt signal selection, -1=noamp, 0=none, 1=amp
    float *rp_tbatt_bias_;    // Location of retained bias, deg C
    float tbatt_bias_last_;   // Last value of bias for rate limit, deg C
    void choose_(void);       // Deliberate choice based on inputs and results
    PRBS_7 *Prbn_Tbatt_;      // Tb noise generator model only
    PRBS_7 *Prbn_Vbatt_;      // Vb noise generator model only
    PRBS_7 *Prbn_Ibatt_;      // Ib noise generator model only
    float Tbatt_noise_amp_;   // Tb noise amplitude model only, deg C pk-pk
    float Vbatt_noise_amp_;   // Vb noise amplitude model only, V pk-pk
    float Ibatt_noise_amp_;   // Ib noise amplitude model only, A pk-pk
};


#endif
