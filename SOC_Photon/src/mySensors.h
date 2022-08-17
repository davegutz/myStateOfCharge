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

// Fault word bits
#define TB_FLT        0   // Momentary isolation of Tb failure, T=faulted
#define VB_FLT        1   // Momentary isolation of Vb failure, T=faulted
#define IB_AMP_FLT    2   // Momentary isolation of Ib amp failure, T=faulted 
#define IB_NOA_FLT    3   // Momentary isolation of Ib no amp failure, T=faulted 
#define CCD_FLT       4
#define WRAP_HI_FLT   5   // Wrap isolates to Ib high fault
#define WRAP_LO_FLT   6   // Wrap isolates to Ib low fault
#define IB_DIF_HI_FLT 8   // Faulted sensor difference error, T = fault
#define IB_DIF_LO_FLT 9   // Faulted sensor difference error, T = fault
#define IB_DSCN_FLT   10  // Dual faulted quiet error, T = disconnected shunt
#define IB_AMP_BARE   11  // Unconnected ib bus, T = bare bus
#define IB_NOA_BARE   12  // Unconnected ib bus, T = bare bus

// Fail word bits
#define TB_FA         0   // Peristed, latched isolation of Tb failure, T=failed
#define VB_FA         1   // Peristed, latched isolation of Vb failure, T=failed
#define IB_AMP_FA     2   // Amp sensor selection memory, T = amp failed
#define IB_NOA_FA     3   // Noamp sensor selection memory, T = no amp failed
#define CCD_FA        4
#define WRAP_HI_FA    5   // Wrap isolates to Ib high fail
#define WRAP_LO_FA    6   // Wrap isolates to Ib low fail
#define WRAP_VB_FA    7   // Wrap isolates to Vb fail
#define IB_DIF_HI_FA  8   // Persisted sensor difference error, T = fail
#define IB_DIF_LO_FA  9   // Persisted sensor difference error, T = fail
#define IB_DSCN_FA    10  // Dual persisted quiet error, T = disconnected shunt

#define faultSet(bit) (bitSet(fltw_, bit) )
#define failSet(bit) (bitSet(falw_, bit) )
#define faultRead(bit) (bitRead(fltw_, bit) )
#define failRead(bit) (bitRead(falw_, bit) )
#define faultAssign(bval, bit) if (bval) bitSet(fltw_, bit); else bitClear(fltw_, bit)
#define failAssign(bval, bit) if (bval) bitSet(falw_, bit); else bitClear(falw_, bit)

// Detect faults and manage selection
class Fault
{
public:
  Fault();
  Fault(const double T);
  ~Fault();
  void bitMapPrint(char *buf, const int16_t fw);
  float cc_diff() { return cc_diff_; };
  boolean cc_flt() { return cc_flt_; };
  boolean dscn_fa() { return failRead(IB_DSCN_FA); };
  boolean dscn_flt() { return faultRead(IB_DSCN_FLT); };
  float e_wrap() { return e_wrap_; };
  float e_wrap_filt() { return e_wrap_filt_; };
  uint16_t fltw() { return fltw_; };
  uint16_t falw() { return falw_; };
  boolean ib_amp_fa() { return failRead(IB_AMP_FA); };
  boolean ib_amp_flt() { return faultRead(IB_AMP_FLT);  };
  boolean ib_noa_fa() { return failRead(IB_NOA_FA); };
  boolean ib_noa_flt() { return faultRead(IB_NOA_FLT); };
  boolean ib_red_loss() { return ib_sel_stat_<0; };
  boolean Vbatt_fail() { return ( vb_fa() || vb_sel_stat_==0 ); };
  int8_t ib_sel_stat() { return ib_sel_stat_; };
  float ib_diff() { return ( ib_diff_ ); };
  float ib_diff_f() { return ( ib_diff_f_ ); };
  boolean ib_dif_hi_fa() { return failRead(IB_DIF_HI_FA); };
  boolean ib_dif_hi_flt() { return faultRead(IB_DIF_HI_FLT); };
  boolean ib_dif_lo_fa() { return failRead(IB_DIF_LO_FA); };
  boolean ib_dif_lo_flt() { return faultRead(IB_DIF_LO_FLT); };
  boolean ib_dscn_fa() { return failRead(IB_DSCN_FA); };
  boolean ib_dscn_flt() { return faultRead(IB_DSCN_FLT); };
  void ib_quiet(const boolean reset, Sensors *Sen);
  void ib_wrap(const boolean reset, Sensors *Sen, BatteryMonitor *Mon);
  void pretty_print(Sensors *Sen, BatteryMonitor *Mon);
  void reset_all_faults() { reset_all_faults_ = true; };
  void select_all(Sensors *Sen, BatteryMonitor *Mon, const boolean reset);
  void shunt_check(Sensors *Sen, BatteryMonitor *Mon, const boolean reset);  // Range check Ibatt signals
  void shunt_select_initial();   // Choose between shunts for model
  boolean tb_fa() { return failRead(TB_FA); };
  boolean tb_flt() { return faultRead(TB_FLT); };
  int8_t tbatt_sel_status() { return tb_sel_stat_; }
  void vbatt_check(Sensors *Sen, BatteryMonitor *Mon, const float _Vbatt_min, const float _Vbatt_max, const boolean reset);  // Range check Vbatt
  int8_t vb_sel_stat() { return vb_sel_stat_; };
  boolean vb_fa() { return failRead(VB_FA); };
  boolean vb_flt() { return faultRead(VB_FLT); };
  boolean wrap_hi_fa() { return failRead(WRAP_HI_FA); };
  boolean wrap_hi_flt() { return faultRead(WRAP_HI_FLT); };
  boolean wrap_lo_fa() { return failRead(WRAP_LO_FA); };
  boolean wrap_lo_flt() { return faultRead(WRAP_LO_FLT);  };
  boolean wrap_vb_fa() { return failRead(WRAP_LO_FA); };
protected:
  TFDelay *IbdHiPersist;
  TFDelay *IbdLoPersist;
  TFDelay *IbattAmpHardFail;
  TFDelay *IbattNoAmpHardFail;
  TFDelay *VbattHardFail;
  TFDelay *QuietPer;
  LagTustin *IbattErrFilt;  // Noise filter for signal selection
  LagTustin *WrapErrFilt;   // Noise filter for voltage wrap
  General2_Pole *QuietFilt; // Linear filter to test for quiet (disconnected)
  boolean cc_flt_;          // EKF tested disagree, T = error
  float cc_diff_;           // EKF tracking error, C
  float e_wrap_;            // Wrap error, V
  float e_wrap_filt_;       // Wrap error, V
  float ib_diff_;           // Current sensor difference error, A
  float ib_diff_f_;         // Filtered sensor difference error, A
  float ib_quiet_;          // ib hardware noise, A
  TFDelay *WrapHi;          // Time high wrap fail persistence
  TFDelay *WrapLo;          // Time low wrap fail persistence
  int8_t tb_sel_stat_;      // Memory of Tbatt signal selection, 0=none, 1=sensor
  int8_t vb_sel_stat_;      // Memory of Vbatt signal selection, 0=none, 1=sensor
  int8_t ib_sel_stat_;      // Memory of Ibatt signal selection, -1=noamp, 0=none, 1=a
  boolean reset_all_faults_; // Reset all fault logic
  int8_t vb_sel_stat_last_; // past value
  int8_t ib_sel_stat_last_; // past value
  uint16_t fltw_;           // Bitmapped faults
  uint16_t falw_;           // Bitmapped fails
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
  void final_assignments();       // Make final signal selection
  void shunt_bias(void);          // Load biases into Shunt objects
  void shunt_load(void);          // Load ADS015 protocol
  void shunt_print(); // Print selection result
  void shunt_select_initial();   // Choose between shunts for model
  void temp_filter(const boolean reset_loc, const float t_rlim);
  void temp_load_and_filter(Sensors *Sen, const boolean reset_loc, const float t_rlim);
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
  float vbatt_add() { return ( vbatt_add_ ); };
  void vbatt_add(const float add) { vbatt_add_ = add; };
  float Vbatt_add() { return ( vbatt_add_ * rp.nS ); };
  Fault *Flt;
protected:
  float *rp_tbatt_bias_;    // Location of retained bias, deg C
  float tbatt_bias_last_;   // Last value of bias for rate limit, deg C
  void choose_(void);       // Deliberate choice based on inputs and results
  PRBS_7 *Prbn_Tbatt_;      // Tb noise generator model only
  PRBS_7 *Prbn_Vbatt_;      // Vb noise generator model only
  PRBS_7 *Prbn_Ibatt_;      // Ib noise generator model only
  float Tbatt_noise_amp_;   // Tb noise amplitude model only, deg C pk-pk
  float Vbatt_noise_amp_;   // Vb noise amplitude model only, V pk-pk
  float Ibatt_noise_amp_;   // Ib noise amplitude model only, A pk-pk
  float vbatt_add_;         // Fault injection bias, V
};


#endif
