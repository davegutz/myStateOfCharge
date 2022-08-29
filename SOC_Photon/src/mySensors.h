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
  boolean tb_stale_flt() { return tb_stale_flt_; };
  float load(Sensors *Sen);
  float noise();
protected:
  SlidingDeadband *SdTb;
  boolean tb_stale_flt_;   // One-wire did not update last pass
};


// ADS1015-based shunt
class Shunt: public Tweak, Adafruit_ADS1015
{
public:
  Shunt();
  Shunt(const String name, const uint8_t port, float *rp_delta_q_cinf, float *rp_delta_q_dinf, float *rp_tweak_sclr,
    float *cp_ib_bias, float *cp_ib_scale, const float v2a_s, float *rp_shunt_gain_sclr);
  ~Shunt();
  // operators
  // functions
  float add() { return ( add_ ); };
  void add(const float add) { add_ = add; };
  boolean bare() { return ( bare_ ); };
  void bias(const float bias) { *cp_ib_bias_ = bias; };
  float bias() { return ( *cp_ib_bias_*sclr_ + add_ ); };
  float bias_any(const float Ib) { return ( Ib*sclr_ + add_ ); };
  float ishunt_cal() { return ( ishunt_cal_*sclr_ + add_ ); };
  void load();
  void pretty_print();
  void scale(const float sclr) { *cp_ib_scale_ = sclr; };
  float scale() { return ( *cp_ib_scale_ ); };
  void rp_shunt_gain_sclr(const float sclr) { *rp_shunt_gain_sclr_ = sclr; };
  float rp_shunt_gain_sclr() { return *rp_shunt_gain_sclr_; };
  float sclr() { return ( sclr_ ); };
  void sclr(const float sclr) { sclr_ = sclr; };
  float v2a_s() { return v2a_s_ ; };
  float vshunt() { return vshunt_; };
  int16_t vshunt_int() { return vshunt_int_; };
  int16_t vshunt_int_0() { return vshunt_int_0_; };
  int16_t vshunt_int_1() { return vshunt_int_1_; };
protected:
  String name_;         // For print statements, multiple instances
  uint8_t port_;        // Octal I2C port used by Acafruit_ADS1015
  boolean bare_;        // If ADS to be ignored
  float *cp_ib_bias_;   // Global bias, A
  float *cp_ib_scale_;  // Global scale, A
  float v2a_s_;         // Selected shunt conversion gain, A/V
  int16_t vshunt_int_;  // Sensed shunt voltage, count
  int16_t vshunt_int_0_;// Interim conversion, count
  int16_t vshunt_int_1_;// Interim conversion, count
  float vshunt_;        // Sensed shunt voltage, V
  float ishunt_cal_;    // Sensed, calibrated ADC, A
  float sclr_;           // Scalar for fault test
  float add_;           // Adder for fault test, A
  float *rp_shunt_gain_sclr_; // Scalar on shunt gain
};

// Fault word bits
#define TB_FLT        0   // Momentary isolation of Tb failure, T=faulted
#define VB_FLT        1   // Momentary isolation of Vb failure, T=faulted
#define IB_AMP_FLT    2   // Momentary isolation of Ib amp failure, T=faulted 
#define IB_NOA_FLT    3   // Momentary isolation of Ib no amp failure, T=faulted 
//                    4
#define WRAP_HI_FLT   5   // Wrap isolates to Ib high fault
#define WRAP_LO_FLT   6   // Wrap isolates to Ib low fault
#define IB_RED_LOSS      7   // Loss of current sensor redundancy, T = fault
#define IB_DIF_HI_FLT 8   // Faulted sensor difference error, T = fault
#define IB_DIF_LO_FLT 9   // Faulted sensor difference error, T = fault
#define IB_DSCN_FLT   10  // Dual faulted quiet error, T = disconnected shunt
#define IB_AMP_BARE   11  // Unconnected ib bus, T = bare bus
#define IB_NOA_BARE   12  // Unconnected ib bus, T = bare bus
#define NUM_FLT       13

// Fail word bits
#define TB_FA         0   // Peristed, latched isolation of Tb failure, T=failed
#define VB_FA         1   // Peristed, latched isolation of Vb failure, T=failed
#define IB_AMP_FA     2   // Amp sensor selection memory, T = amp failed
#define IB_NOA_FA     3   // Noamp sensor selection memory, T = no amp failed
#define CCD_FA        4   // Coulomb Counter difference accumulated, T = faulted=faile
#define WRAP_HI_FA    5   // Wrap isolates to Ib high fail
#define WRAP_LO_FA    6   // Wrap isolates to Ib low fail
#define WRAP_VB_FA    7   // Wrap isolates to Vb fail
#define IB_DIF_HI_FA  8   // Persisted sensor difference error, T = fail
#define IB_DIF_LO_FA  9   // Persisted sensor difference error, T = fail
#define IB_DSCN_FA    10  // Dual persisted quiet error, T = disconnected shunt
#define NUM_FA        11

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
  void bitMapPrint(char *buf, const int16_t fw, const uint8_t num);
  float cc_diff() { return cc_diff_; };
  void cc_diff_sclr(const float sclr) { cc_diff_sclr_ = sclr; };
  float cc_diff_sclr() { return cc_diff_sclr_; };
  boolean cc_diff_fa() { return failRead(CCD_FA); };
  void disab_ib_fa(const boolean dis) { disab_ib_fa_ = dis; };
  boolean disab_ib_fa() { return disab_ib_fa_; };
  void disab_tb_fa(const boolean dis) { disab_tb_fa_ = dis; };
  boolean disab_tb_fa() { return disab_tb_fa_; };
  void disab_vb_fa(const boolean dis) { disab_vb_fa_ = dis; };
  boolean disab_vb_fa() { return disab_vb_fa_; };
  boolean dscn_fa() { return failRead(IB_DSCN_FA); };
  boolean dscn_flt() { return faultRead(IB_DSCN_FLT); };
  void ewhi_sclr(const float sclr) { ewhi_sclr_ = sclr; };
  float ewhi_sclr() { return ewhi_sclr_; };
  void ewlo_sclr(const float sclr) { ewlo_sclr_ = sclr; };
  float ewlo_sclr() { return ewlo_sclr_; };
  float e_wrap() { return e_wrap_; };
  float e_wrap_filt() { return e_wrap_filt_; };
  void fail_tb(const boolean fail) { fail_tb_ = fail; };
  boolean fail_tb() { return fail_tb_; };
  uint16_t fltw() { return fltw_; };
  uint16_t falw() { return falw_; };
  void ib_diff_sclr(const float sclr) { ib_diff_sclr_ = sclr; };
  float ib_diff_sclr() { return ib_diff_sclr_; };
  void ib_quiet_sclr(const float sclr) { ib_quiet_sclr_ = sclr; };
  float ib_quiet_sclr() { return ib_quiet_sclr_; };
  boolean ib_amp_fa() { return failRead(IB_AMP_FA); };
  boolean ib_amp_flt() { return faultRead(IB_AMP_FLT);  };
  boolean ib_noa_fa() { return failRead(IB_NOA_FA); };
  boolean ib_noa_flt() { return faultRead(IB_NOA_FLT); };
  boolean ib_red_loss() { return faultRead(IB_RED_LOSS); };
  boolean ib_red_loss_calc() { return (ib_sel_stat_!=1 || rp.ib_select!=0 || ib_dif_fa()); };
  boolean vb_fail() { return ( vb_fa() || vb_sel_stat_==0 ); };
  int8_t ib_sel_stat() { return ib_sel_stat_; };
  float ib_diff() { return ( ib_diff_ ); };
  float ib_diff_f() { return ( ib_diff_f_ ); };
  boolean ib_dif_fa() { return ( failRead(IB_DIF_HI_FA) || failRead(IB_DIF_LO_FA) ); };
  boolean ib_dif_hi_fa() { return failRead(IB_DIF_HI_FA); };
  boolean ib_dif_hi_flt() { return faultRead(IB_DIF_HI_FLT); };
  boolean ib_dif_lo_fa() { return failRead(IB_DIF_LO_FA); };
  boolean ib_dif_lo_flt() { return faultRead(IB_DIF_LO_FLT); };
  boolean ib_dscn_fa() { return failRead(IB_DSCN_FA); };
  boolean ib_dscn_flt() { return faultRead(IB_DSCN_FLT); };
  void ib_quiet(const boolean reset, Sensors *Sen);
  float ib_quiet() { return ib_quiet_; };
  float ib_rate() { return ib_rate_; };
  void ib_wrap(const boolean reset, Sensors *Sen, BatteryMonitor *Mon);
  void pretty_print(Sensors *Sen, BatteryMonitor *Mon);
  void pretty_print1(Sensors *Sen, BatteryMonitor *Mon);
  void reset_all_faults() { reset_all_faults_ = true; };
  void select_all(Sensors *Sen, BatteryMonitor *Mon, const boolean reset);
  void shunt_check(Sensors *Sen, BatteryMonitor *Mon, const boolean reset);  // Range check Ib signals
  void shunt_select_initial();   // Choose between shunts for model
  boolean tb_fa() { return failRead(TB_FA); };
  boolean tb_flt() { return faultRead(TB_FLT); };
  int8_t tb_sel_status() { return tb_sel_stat_; };
  void tb_stale_time_sclr(const float sclr) { tb_stale_time_sclr_ = sclr; };
  float tb_stale_time_sclr() { return tb_stale_time_sclr_; };
  void vb_check(Sensors *Sen, BatteryMonitor *Mon, const float _Vb_min, const float _Vb_max, const boolean reset);  // Range check Vb
  int8_t vb_sel_stat() { return vb_sel_stat_; };
  boolean vb_fa() { return failRead(VB_FA); };
  boolean vb_flt() { return faultRead(VB_FLT); };
  boolean wrap_fa() { return ( failRead(WRAP_HI_FA) || failRead(WRAP_LO_FA) ); };
  boolean wrap_hi_fa() { return failRead(WRAP_HI_FA); };
  boolean wrap_hi_flt() { return faultRead(WRAP_HI_FLT); };
  boolean wrap_lo_fa() { return failRead(WRAP_LO_FA); };
  boolean wrap_lo_flt() { return faultRead(WRAP_LO_FLT);  };
  boolean wrap_vb_fa() { return failRead(WRAP_VB_FA); };
protected:
  TFDelay *IbdHiPer;        // Persistence ib diff hi
  TFDelay *IbdLoPer;        // Persistence ib diff lo
  TFDelay *IbAmpHardFail;   // Persistence ib hard fail amp
  TFDelay *IbNoAmpHardFail; // Persistence ib hard fail noa
  TFDelay *TbStaleFail;     // Persistence stale tb one-wire data
  TFDelay *VbHardFail;      // Persistence vb hard fail amp
  TFDelay *QuietPer;        // Persistence ib quiet disconnect detection
  LagTustin *IbErrFilt;     // Noise filter for signal selection
  LagTustin *WrapErrFilt;   // Noise filter for voltage wrap
  General2_Pole *QuietFilt; // Linear filter to test for quiet
  RateLagExp *QuietRate;    // Linear filter to calculate rate for quiet
  boolean cc_diff_fa_;      // EKF tested disagree, T = error
  float cc_diff_;           // EKF tracking error, C
  float cc_diff_sclr_;      // Scale cc_diff detection thresh, scalar
  boolean disab_ib_fa_;     // Disable hard fault range failures for ib
  boolean disab_tb_fa_;     // Disable hard fault range failures for tb
  boolean disab_vb_fa_;     // Disable hard fault range failures for vb
  float ewhi_sclr_;         // Scale wrap hi detection thresh, scalar
  float ewlo_sclr_;         // Scale wrap lo detection thresh, scalar
  float ewsat_sclr_;        // Scale wrap detection thresh when voc(soc) saturated, scalar
  float e_wrap_;            // Wrap error, V
  float e_wrap_filt_;       // Wrap error, V
  boolean fail_tb_;         // Make hardware bus read ignore Tb and fail it
  float ib_diff_sclr_;      // Scale ib_diff detection thresh, scalar
  float ib_quiet_sclr_;     // Scale ib_quiet detection thresh, scalar
  float ib_diff_;           // Current sensor difference error, A
  float ib_diff_f_;         // Filtered sensor difference error, A
  float ib_quiet_;          // ib hardware noise, A/s
  float ib_rate_;           // ib rate, A/s
  TFDelay *WrapHi;          // Time high wrap fail persistence
  TFDelay *WrapLo;          // Time low wrap fail persistence
  int8_t tb_sel_stat_;      // Memory of Tb signal selection, 0=none, 1=sensor
  float tb_stale_time_sclr_; // Scalar on persistences of Tb hardware stale chec, (1)
  int8_t vb_sel_stat_;      // Memory of Vb signal selection, 0=none, 1=sensor
  int8_t ib_sel_stat_;      // Memory of Ib signal selection, -1=noa, 0=none, 1=a
  boolean reset_all_faults_;// Reset all fault logic
  int8_t tb_sel_stat_last_; // past value
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
  Sensors(double T, double T_temp, byte pin_1_wire, Sync *ReadSensors);
  ~Sensors();
  int Vb_raw;                 // Raw analog read, integer
  float Vb;                   // Selected battery bank voltage, V
  float Vb_hdwe;              // Sensed battery bank voltage, V
  float Vb_model;             // Modeled battery bank voltage, V
  float Tb;                   // Selected battery bank temp, C
  float Tb_filt;              // Selected filtered battery bank temp, C
  float Tb_hdwe;              // Sensed battery temp, C
  float Tb_hdwe_filt;         // Filtered, sensed battery temp, C
  float Tb_model;             // Temperature used for battery bank temp in model, C
  float Tb_model_filt;        // Filtered, modeled battery bank temp, C
  float Vshunt;               // Sensed shunt voltage, V
  float Ib;                   // Selected battery bank current, A
  float Ib_amp_hdwe;          // Sensed amp battery bank current, A
  float Ib_amp_model;         // Modeled amp battery bank current, A
  float Ib_noa_hdwe;          // Sensed noa battery bank current, A
  float Ib_noa_model;         // Modeled noa battery bank current, A
  float Ib_hdwe;              // Sensed battery bank current, A
  float Ib_hdwe_model;        // Selected model hardware signal, A
  float Ib_model;             // Modeled battery bank current, A
  float Ib_model_in;          // Battery bank current input to model (modified by cutback), A
  float Wb;                   // Sensed battery bank power, use to compare to other shunts, W
  unsigned long int now;      // Time at sample, ms
  double T;                   // Update time, s
  boolean reset;              // Reset flag, T = reset
  double T_filt;              // Filter update time, s
  double T_temp;              // Temperature update time, s
  Sync *ReadSensors;          // Handle to debug read time
  boolean saturated;          // Battery saturation status based on Temp and VOC
  Shunt *ShuntAmp;            // Ib sense amplified
  Shunt *ShuntNoAmp;          // Ib sense non-amplified
  TempSensor* SensorTb;       // Tb sense
  General2_Pole* TbSenseFilt; // Linear filter for Tb. There are 1 Hz AAFs in hardware for Vb and Ib
  SlidingDeadband *SdTb;      // Non-linear filter for Tb
  BatteryModel *Sim;          // Used to model Vb and Ib.   Use Talk 'Xp?' to toggle model on/off
  unsigned long int elapsed_inj;  // Injection elapsed time, ms
  unsigned long int start_inj;// Start of calculated injection, ms
  unsigned long int stop_inj; // Stop of calculated injection, ms
  unsigned long int wait_inj; // Wait before start injection, ms
  unsigned long int end_inj;  // End of print injection, ms
  unsigned long int tail_inj; // Tail after end injection, ms
  float cycles_inj;           // Number of injection cycles
  double control_time;        // Decimal time, seconds since 1/1/2021
  boolean display;            // Use display
  double sclr_coul_eff;       // Scalar on Coulombic Efficiency
  void bias_all_model();      // Bias model outputs for sensor fault injection
  void final_assignments(BatteryMonitor *Mon);   // Make final signal selection
  void shunt_bias(void);      // Load biases into Shunt objects
  void shunt_load(void);      // Load ADS015 protocol
  void shunt_print();         // Print selection result
  void shunt_scale(void);     // Load scalars into Shunt objects
  void shunt_select_initial();   // Choose between shunts for model
  void temp_filter(const boolean reset_loc, const float t_rlim);
  void temp_load_and_filter(Sensors *Sen, const boolean reset_loc, const float t_rlim);
  void vb_load(const byte vb_pin);  // Analog read of Vb
  float Tb_noise();
  float Tb_noise_amp() { return ( Tb_noise_amp_ ); };
  void Tb_noise_amp(const float noise) { Tb_noise_amp_ = noise; };
  float Vb_noise();
  float Vb_noise_amp() { return ( Vb_noise_amp_ ); };
  void Vb_noise_amp(const float noise) { Vb_noise_amp_ = noise; };
  float Ib_noise();
  float Ib_amp_noise();
  float Ib_amp_noise_amp() { return ( Ib_amp_noise_amp_ ); };
  void Ib_amp_noise_amp(const float noise) { Ib_amp_noise_amp_ = noise; };
  float Ib_noa_noise();
  float Ib_noa_noise_amp() { return ( Ib_noa_noise_amp_ ); };
  void Ib_noa_noise_amp(const float noise) { Ib_noa_noise_amp_ = noise; };
  void vb_print(void);     // Print Vb result
  float vb_add() { return ( vb_add_ ); };
  void vb_add(const float add) { vb_add_ = add; };
  float Vb_add() { return ( vb_add_ * rp.nS ); };
  Fault *Flt;
protected:
  float *rp_tb_bias_;    // Location of retained bias, deg C
  float tb_bias_last_;   // Last value of bias for rate limit, deg C
  void choose_(void);    // Deliberate choice based on inputs and results
  PRBS_7 *Prbn_Tb_;      // Tb noise generator model only
  PRBS_7 *Prbn_Vb_;      // Vb noise generator model only
  PRBS_7 *Prbn_Ib_amp_;  // Ib amplified sensor noise generator model only
  PRBS_7 *Prbn_Ib_noa_;  // Ib non-amplified sensor noise generator model only
  float Tb_noise_amp_;   // Tb noise amplitude model only, deg C pk-pk
  float Vb_noise_amp_;   // Vb noise amplitude model only, V pk-pk
  float Ib_amp_noise_amp_;  // Ib noise on amplified sensor, amplitude model only, A pk-pk
  float Ib_noa_noise_amp_;  // Ib noise on non-amplified sensor, amplitude model only, A pk-pk
  float vb_add_;         // Fault injection bias, V
};


#endif
