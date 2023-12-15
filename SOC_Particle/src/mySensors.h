//
// MIT License
//
// Copyright (C) 2023 - Dave Gutz
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
#include "talk/transcribe.h"
#include "command.h"
#include "mySync.h"
#include "parameters.h"

// Temp sensor
#include <OneWire.h>
#include <DS18B20.h>

// AD
#include "Adafruit/Adafruit_ADS1X15.h"

extern PublishPars pp;  // For publishing
extern CommandPars cp;  // Various parameters to be static at system level
extern SavedPars sp;    // Various parameters to be static at system level and saved through power cycle
extern VolatilePars ap; // Various adjustment parameters shared at system level
struct Pins;

// DS18-based temp sensor
class TempSensor: public DS18B20
{
public:
  TempSensor();
  TempSensor(const uint16_t pin, const bool parasitic, const uint16_t conversion_delay);
  ~TempSensor();
  // operators
  // functions
  boolean tb_stale_flt() { return tb_stale_flt_; };
  float sample(Sensors *Sen);
  float noise();
protected:
  SlidingDeadband *SdTb;
  boolean tb_stale_flt_;   // One-wire did not update last pass
};


// ADS1015-based shunt
class Shunt: public Adafruit_ADS1015
{
public:
  Shunt();
  Shunt(const String name, const uint8_t port, float *sp_ib_scale, float *sp_Ib_bias, const float v2a_s,
    const uint8_t vc_pin, const uint8_t vo_pin);
  Shunt(const String name, const uint8_t port, float *sp_ib_scale, float *sp_Ib_bias, const float v2a_s,
    const uint8_t vo_pin);
  ~Shunt();
  // operators
  // functions
  boolean bare_detected() { return ( bare_detected_ ); };
  void dscn_cmd(const boolean cmd) { dscn_cmd_ = cmd; };
  unsigned long int dt() { return sample_time_ - sample_time_z_; };
  void convert(const boolean disconnect);
  float Ishunt_cal() { return Ishunt_cal_; };
  void pretty_print();
  void sample(const boolean reset_loc, const float T);
  float scale() { return ( *sp_ib_scale_ ); };
  unsigned long int sample_time(void) { return sample_time_; };
  float v2a_s() { return v2a_s_ ; };
  float vshunt() { return vshunt_; };
  int16_t vshunt_int() { return vshunt_int_; };
  int16_t vshunt_int_0() { return vshunt_int_0_; };
  int16_t vshunt_int_1() { return vshunt_int_1_; };
  float Vc() { return Vc_; };
  float Vo() { return Vo_; };
  float Vo_Vc() { return Vo_Vc_; };
  float Vo_Vc_f() { return Vo_Vc_f_; };
protected:
  String name_;         // For print statements, multiple instances
  uint8_t port_;        // Octal I2C port used by Acafruit_ADS1015
  boolean bare_detected_;        // If ADS to be ignored
  float v2a_s_;         // Selected shunt conversion gain, A/V
  int16_t vshunt_int_;  // Sensed shunt voltage, count
  int16_t vshunt_int_0_;// Interim conversion, count
  int16_t vshunt_int_1_;// Interim conversion, count
  float vshunt_;        // Sensed shunt voltage, V
  float Ishunt_cal_;    // Sensed bank current, calibrated ADC, A
  float *sp_Ib_bias_;   // Global bias, A
  float *sp_ib_scale_;  // Global scale, A
  unsigned long int sample_time_;   // Exact moment of hardware sample
  unsigned long int sample_time_z_; // Exact moment of past hardware sample
  boolean dscn_cmd_;    // User command to ignore hardware, T=ignore
  uint8_t vc_pin_;      // Common voltage pin, for !CONFIG_ADS1013_OPAMP
  uint8_t vo_pin_;      // Output voltage pin, for !CONFIG_ADS1013_OPAMP
  int Vc_raw_;          // Raw analog read, integer       
  float Vc_;            // Sensed Vc, common op amp voltage ref, V
  int Vo_raw_;          // Raw analog read, integer       
  float Vo_;            // Sensed Vo, output of op amp, V
  float Vo_Vc_;         // Sensed Vo-Vc, difference in output of op amps, V
  float Vo_Vc_f_;       // Sensed, filtered Vo-Vc, difference in output of op amps, V
  boolean using_tsc2010_; // Using differential hardware amp
};

// Fault word bits.   All faults heal
#define TB_FLT        0   // Momentary isolation of Tb failure, T=faulted
#define VB_FLT        1   // Momentary isolation of Vb failure, T=faulted
#define IB_AMP_FLT    2   // Momentary isolation of Ib amp failure, T=faulted 
#define IB_NOA_FLT    3   // Momentary isolation of Ib no amp failure, T=faulted 
//                    4
#define WRAP_HI_FLT   5   // Wrap isolates to Ib high fault
#define WRAP_LO_FLT   6   // Wrap isolates to Ib low fault
#define RED_LOSS      7   // Loss of current knowledge redundancy, T = fault
#define IB_DIFF_HI_FLT 8   // Faulted sensor difference error, T = fault
#define IB_DIFF_LO_FLT 9   // Faulted sensor difference error, T = fault
#define IB_DSCN_FLT   10  // Dual faulted quiet error, T = disconnected shunt
#define IB_AMP_BARE   11  // Unconnected ib bus, T = bare bus
#define IB_NOA_BARE   12  // Unconnected ib bus, T = bare bus
#define NUM_FLT       13  // Number of these

// Fail word bits.   A couple don't latch because single sensor fail in dual sensor system
#define TB_FA         0   // Peristed, latched isolation of Tb failure, heals because soft type, T=failed
#define VB_FA         1   // Peristed, latched isolation of Vb failure, latches because hard type, T=failed
#define IB_AMP_FA     2   // Amp sensor selection memory, latches because hard type, T = amp failed
#define IB_NOA_FA     3   // Noamp sensor selection memory, latches because hard type, T = no amp failed
#define CC_DIFF_FA    4   // Accumulated Coulomb Counter difference used to isolate IB differences, heals functional type, T = faulted=failed 
#define WRAP_HI_FA    5   // Wrap isolates to Ib high fail, heals because dual sensor (no latch)
#define WRAP_LO_FA    6   // Wrap isolates to Ib low fail, heals because dual sensor (no latch)
#define WRAP_VB_FA    7   // Wrap isolates to Vb fail, latches because single sensor (latch)
#define IB_DIFF_HI_FA 8   // Persisted sensor difference error, latches because hard type, T = fail
#define IB_DIFF_LO_FA 9   // Persisted sensor difference error, latches because hard type, T = fail
#define IB_DSCN_FA    10  // Dual persisted quiet error, heals functional type, T = disconnected shunt
#define NUM_FA        11  // Number of these

#define faultSet(bit) (bitSet(fltw_, bit) )
#define failSet(bit) (bitSet(falw_, bit) )
#define faultRead(bit) (bitRead(fltw_, bit) )
#define failRead(bit) (bitRead(falw_, bit) )
#define faultAssign(bval, bit) if (bval) bitSet(fltw_, bit); else bitClear(fltw_, bit)
#define failAssign(bval, bit) if (bval) bitSet(falw_, bit); else bitClear(falw_, bit)

void bitMapPrint(char *buf, const int16_t fw, const uint8_t num);


// Detect faults and manage selection
class Fault
{
public:
  Fault();
  Fault(const double T, uint8_t *sp_preserving);
  ~Fault();
  float cc_diff() { return cc_diff_; };
  void cc_diff(Sensors *Sen, BatteryMonitor *Mon);
  boolean cc_diff_fa() { return failRead(CC_DIFF_FA); };
  float cc_diff_thr_;     // Threshold Coulomb Counters difference faults, soc fraction
  float cc_diff_thr() { return cc_diff_thr_; };
  boolean dscn_fa() { return failRead(IB_DSCN_FA); };
  boolean dscn_flt() { return faultRead(IB_DSCN_FLT); };
  float ewhi_thr_;      // Threshold e_wrap failed high, V
  float ewhi_thr() { return ewhi_thr_; };
  float ewlo_thr_;      // Threshold e_wrap failed low, V
  float ewlo_thr() { return ewlo_thr_; };
  float e_wrap() { return e_wrap_; };
  float e_wrap_filt() { return e_wrap_filt_; };
  uint16_t fltw() { return fltw_; };
  uint16_t falw() { return falw_; };
  boolean ib_amp_bare() { return faultRead(IB_AMP_BARE);  };
  boolean ib_amp_fa() { return failRead(IB_AMP_FA); };
  boolean ib_amp_flt() { return faultRead(IB_AMP_FLT);  };
  void ib_diff(const boolean reset, Sensors *Sen, BatteryMonitor *Mon);
  float ib_diff_thr_;     // Threshold current difference faults, A
  float ib_diff_thr() { return ib_diff_thr_; };
  boolean ib_dscn_fa() { return failRead(IB_DSCN_FA); };
  boolean ib_dscn_flt() { return faultRead(IB_DSCN_FLT); };
  boolean ib_noa_bare() { return faultRead(IB_NOA_BARE); };
  boolean ib_noa_fa() { return failRead(IB_NOA_FA); };
  boolean ib_noa_flt() { return faultRead(IB_NOA_FLT); };
  float ib_quiet_thr_;     // Threshold below which ib is quiet, A pk
  float ib_quiet_thr() { return ib_quiet_thr_; };
  int8_t ib_sel_stat() { return ib_sel_stat_; };
  void ib_sel_stat(const boolean cmd) { ib_sel_stat_ = cmd; };
  float ib_diff() { return ( ib_diff_ ); };
  float ib_diff_f() { return ( ib_diff_f_ ); };
  boolean ib_diff_fa() { return ( failRead(IB_DIFF_HI_FA) || failRead(IB_DIFF_LO_FA) ); };
  boolean ib_diff_hi_fa() { return failRead(IB_DIFF_HI_FA); };
  boolean ib_diff_hi_flt() { return faultRead(IB_DIFF_HI_FLT); };
  boolean ib_diff_lo_fa() { return failRead(IB_DIFF_LO_FA); };
  boolean ib_diff_lo_flt() { return faultRead(IB_DIFF_LO_FLT); };
  void ib_quiet(const boolean reset, Sensors *Sen);
  float ib_quiet() { return ib_quiet_; };
  float ib_rate() { return ib_rate_; };
  void ib_wrap(const boolean reset, Sensors *Sen, BatteryMonitor *Mon);
  int8_t latched_fail() { return latched_fail_; };
  void latched_fail(const boolean cmd) { latched_fail_ = cmd; };
  int8_t latched_fail_fake() { return latched_fail_fake_; };
  void latched_fail_fake(const boolean cmd) { latched_fail_fake_ = cmd; };
  boolean no_fails() { return !latched_fail_; };
  boolean no_fails_fake() { return !latched_fail_fake_; };
  void preserving(const boolean cmd) {  sp.put_Preserving(cmd); }; // TODO:  Parameter class with = operator --> put. Then *sp_preserving = cmd
  boolean preserving() { return *sp_preserving_; };
  void pretty_print(Sensors *Sen, BatteryMonitor *Mon);
  void pretty_print1(Sensors *Sen, BatteryMonitor *Mon);
  boolean record() { if ( ap.fake_faults ) return no_fails_fake(); else return no_fails(); };
  boolean red_loss() { return faultRead(RED_LOSS); };
  boolean red_loss_calc();
  void reset_all_faults(const boolean cmd) { reset_all_faults_ = cmd; };
  boolean reset_all_faults() { return reset_all_faults_; };
  void select_all(Sensors *Sen, BatteryMonitor *Mon, const boolean reset);
  void shunt_check(Sensors *Sen, BatteryMonitor *Mon, const boolean reset);  // Range check Ib signals
  void shunt_select_initial(const boolean reset);   // Choose between shunts for model
  boolean tb_fa() { return failRead(TB_FA); };
  boolean tb_flt() { return faultRead(TB_FLT); };
  int8_t tb_sel_status() { return tb_sel_stat_; };
  void tb_stale(const boolean reset, Sensors *Sen);
  void vb_check(Sensors *Sen, BatteryMonitor *Mon, const float _vb_min, const float _vb_max, const boolean reset);  // Range check Vb
  boolean vb_fail() { return ( vb_fa() || vb_sel_stat_==0 ); };
  int8_t vb_sel_stat() { return vb_sel_stat_; };
  boolean vb_fa() { return failRead(VB_FA); };
  boolean vb_flt() { return faultRead(VB_FLT); };
  boolean wrap_fa() { return ( failRead(WRAP_HI_FA) || failRead(WRAP_LO_FA) ); };
  boolean wrap_hi_fa() { return failRead(WRAP_HI_FA); };
  boolean wrap_hi_flt() { return faultRead(WRAP_HI_FLT); };
  boolean wrap_lo_fa() { return failRead(WRAP_LO_FA); };
  boolean wrap_lo_flt() { return faultRead(WRAP_LO_FLT);  };
  boolean wrap_vb_fa() { return failRead(WRAP_VB_FA); };
  void wrap_err_filt_state(const float in) { WrapErrFilt->state(in); }
protected:
  TFDelay *IbAmpHardFail;   // Persistence ib hard fail amp
  TFDelay *IbdHiPer;        // Persistence ib diff hi
  TFDelay *IbdLoPer;        // Persistence ib diff lo
  LagTustin *IbErrFilt;     // Noise filter for signal selection
  TFDelay *IbNoAmpHardFail; // Persistence ib hard fail noa
  General2_Pole *QuietFilt; // Linear filter to test for quiet
  TFDelay *QuietPer;        // Persistence ib quiet disconnect detection
  RateLagExp *QuietRate;    // Linear filter to calculate rate for quiet
  TFDelay *TbStaleFail;     // Persistence stale tb one-wire data
  TFDelay *VbHardFail;      // Persistence vb hard fail amp
  LagTustin *WrapErrFilt;   // Noise filter for voltage wrap
  boolean cc_diff_fa_;      // EKF tested disagree, T = error
  float cc_diff_;           // EKF tracking error, C
  float cc_diff_empty_slr_;  // Scale cc_diff when soc low, scalar
  float ewmin_slr_;        // Scale wrap detection thresh when voc(soc) less than min, scalar
  float ewsat_slr_;        // Scale wrap detection thresh when voc(soc) saturated, scalar
  float e_wrap_;            // Wrap error, V
  float e_wrap_filt_;       // Wrap error, V
  float ib_diff_;           // Current sensor difference error, A
  float ib_diff_f_;         // Filtered sensor difference error, A
  float ib_quiet_;          // ib hardware noise, A/s
  float ib_rate_;           // ib rate, A/s
  boolean latched_fail_;    // There is a latched fail, T=latched fail
  boolean latched_fail_fake_;  // There would be a latched fail if not faking, T=latched fail
  int8_t tb_sel_stat_;      // Memory of Tb signal selection, 0=none, 1=sensor
  int8_t vb_sel_stat_;      // Memory of Vb signal selection, 0=none, 1=sensor
  int8_t ib_sel_stat_;      // Memory of Ib signal selection, -1=noa, 0=none, 1=a
  boolean reset_all_faults_;// Reset all fault logic
  int8_t tb_sel_stat_last_; // past value
  int8_t vb_sel_stat_last_; // past value
  int8_t ib_sel_stat_last_; // past value
  uint16_t fltw_;           // Bitmapped faults
  uint16_t falw_;           // Bitmapped fails
  TFDelay *WrapHi;          // Time high wrap fail persistence
  TFDelay *WrapLo;          // Time low wrap fail persistence
  uint8_t *sp_preserving_;  // Saving fault buffer.   Stopped recording.  T=preserve
};


// Sensors (like a big struct with public access)
class Sensors
{
public:
  Sensors();
  Sensors(double T, double T_temp, Pins *pins, Sync *ReadSensors, Sync *Talk, Sync *Summarize);
  ~Sensors();
  int Vb_raw;                 // Raw analog read, integer
  float Vb;                   // Selected battery bank voltage, V
  float Vb_hdwe;              // Sensed battery bank voltage, V
  float Vb_hdwe_f;            // Sensed, filtered battery bank voltage, V
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
  float Ib_amp_hdwe_f;        // Sensed, filtered amp battery bank current, A
  float Ib_amp_model;         // Modeled amp battery bank current, A
  float Ib_noa_hdwe;          // Sensed noa battery bank current, A
  float Ib_noa_hdwe_f;        // Sensed, filtered noa battery bank current, A
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
  Sync *Summarize;            // Handle to debug read time
  Sync *Talk;                 // Handle to debug talk time
  General2_Pole* TbSenseFilt; // Linear filter for Tb. There are 1 Hz AAFs in hardware for Vb and Ib
  SlidingDeadband *SdTb;      // Non-linear filter for Tb
  BatterySim *Sim;            // Used to model Vb and Ib.   Use Talk 'Xp?' to toggle model on/off
  unsigned long int elapsed_inj;  // Injection elapsed time, ms
  unsigned long int start_inj;// Start of calculated injection, ms
  unsigned long int stop_inj; // Stop of calculated injection, ms
  unsigned long int end_inj;  // End of print injection, ms
  double control_time;        // Decimal time, seconds since 1/1/2021
  boolean display;            // Use display
  boolean bms_off;            // Calculated by BatteryMonitor, battery off, low voltage, switched by battery management system?
  unsigned long int dt_ib(void) { return dt_ib_; };
  void final_assignments(BatteryMonitor *Mon);  // Make final signal selection
  float ib() { return Ib / sp.nP(); };                            // Battery unit current, A
  float ib_amp_hdwe() { return Ib_amp_hdwe / sp.nP(); };          // Battery amp unit current, A
  float ib_amp_model() { return Ib_amp_model / sp.nP(); };        // Battery amp model unit current, A
  float ib_hdwe() { return Ib_hdwe / sp.nP(); };                  // Battery select hardware unit current, A
  float ib_hdwe_model() { return Ib_hdwe_model / sp.nP(); };      // Battery select hardware model unit current, A
  float ib_model() { return Ib_model / sp.nP(); };                // Battery select model unit current, A
  float ib_model_in() { return Ib_model_in / sp.nP(); };          // Battery select model input unit current, A
  float ib_noa_hdwe() { return Ib_noa_hdwe / sp.nP(); };          // Battery no amp unit current, A
  float ib_noa_model() { return Ib_noa_model / sp.nP(); };        // Battery no amp model unit current, A
  float Ib_amp_add();
  float Ib_noa_add();
  float Ib_amp_noise();
  float Ib_noa_noise();
  float Ib_noise();
  void reset_temp(const boolean reset) { reset_temp_ = reset; };
  boolean reset_temp() { return ( reset_temp_ ); };
  unsigned long int sample_time_ib(void) { return sample_time_ib_; };
  unsigned long int sample_time_vb(void) { return sample_time_vb_; };
  void shunt_print();         // Print selection result
  void shunt_select_initial(const boolean reset);   // Choose between shunts for model
  void temp_load_and_filter(Sensors *Sen, const boolean reset_temp);
  float Tb_noise();
  float vb() { return Vb / sp.nS(); };                            // Battery select unit voltage, V
  float vb_hdwe() { return Vb_hdwe / sp.nS(); };                  // Battery select hardware unit voltage, V
  void vb_load(const uint16_t vb_pin, const boolean reset);       // Analog read of Vb
  float vb_model() { return (Vb_model / sp.nS()); };              // Battery select model unit voltage, V
  float Vb_add();
  float Vb_noise();
  void vb_print(void);                  // Print Vb result
  Fault *Flt;
protected:
  LagExp *AmpFilt;      // Noise filter for calibration
  LagExp *NoaFilt;      // Noise filter for calibration
  LagExp *VbFilt;       // Noise filter for calibration
  void choose_(void);   // Deliberate choice based on inputs and results
  PRBS_7 *Prbn_Tb_;     // Tb noise generator model only
  PRBS_7 *Prbn_Vb_;     // Vb noise generator model only
  PRBS_7 *Prbn_Ib_amp_; // Ib amplified sensor noise generator model only
  PRBS_7 *Prbn_Ib_noa_; // Ib non-amplified sensor noise generator model only
  boolean reset_temp_;  // Keep track of temperature reset, stored for plotting, T=reset
  unsigned long int sample_time_ib_;          // Exact moment of selected Ib sample, ms
  unsigned long int sample_time_vb_;          // Exact moment of selected Vb sample, ms
  unsigned long int sample_time_ib_hdwe_;     // Exact moment of Ib sample, ms
  unsigned long int sample_time_vb_hdwe_;     // Exact moment of Vb sample, ms
  unsigned long int dt_ib_hdwe_;          // Delta update of Ib sample, ms
  unsigned long int dt_ib_;               // Delta update of selected Ib sample, ms
};


#endif
