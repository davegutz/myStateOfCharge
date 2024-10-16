#ifndef local_config_h
#define local_config_h

#undef CONFIG_PHOTON
// #define CONFIG_ARGON
// #undef CONFIG_PHOTON2
#undef CONFIG_ARGON
#define CONFIG_PHOTON2
#include "application.h"  // Should not be needed if file ino or Arduino
SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status
#include <Arduino.h>      // Used instead of Print.h - breaks Serial
const String unit = "pro1a_20221130b";  // tune hys

// * = SRAM EEPROM adjustments, retained on power reset

// Miscelaneous
#define ASK_DURING_BOOT       1   // Flag to ask for application of this file to * retained adjustements
#define MODELING              7   // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model    
#define HYS_SCALE             1.0     // Scalar on hysteresis (1.0)

// Sensor biases
#define SHUNT_GAIN            -1333.// Shunt V2A gain (scale with * 'SG'), A/V (-1333 is -100A/0.075V)
#define CURR_BIAS_AMP         9.7   // Calibration of amplified shunt sensor (* 'DA'), A
#define CURR_BIAS_NOA         0.    // Calibration of non-amplified shunt sensor (* 'DB'), A
#define CURR_SCALE_NOA        1.    // Hardware to match data (* 'SA')
#define CURR_SCALE_AMP        1.    // Hardware to match data (* 'SB')
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors (* 'Di'), A
#define VOLT_BIAS             1.8   // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS             0.0   // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SCALE              1.0   // Scale Vb sensor (* 'SV')
#define SHUNT_AMP_R1          5600.     // Amplifed shunt ADS resistance, ohms (5k6)  100/5.6  = 17.86
#define SHUNT_AMP_R2          100000.   // Amplifed shunt ADS resistance, ohms (100k) 0.075v  = 1.34 v => 3.3/2+1.34 = 2.99 < 3.3
#define VB_SENSE_R_LO      4700      // Vb low sense resistor, ohm (4700)
#define VB_SENSE_R_HI      20000     // Vb high sense resistor, ohm (20000)
#define NOM_UNIT_CAP          100.    // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah

// Battery.  One 12 V 100 Ah battery bank would have RATED_BATT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have RATED_BATT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  RATED_BATT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF         0.9985  // Coulombic efficiency of battery, fraction of charge that gets used
#define MON_CHEM              0       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=LION 
#define SIM_CHEM              0       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=LION 
#define RATED_BATT_CAP        100.    // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah
#define NS                    1.0     // Number of series batteries in bank.  Fractions scale and remember RATED_BATT_CAP (* 'BS')
#define NP                    1.0     // Number of parallel batteries in bank.  Fractions scale and remember RATED_BATT_CAP (* 'BP')

// Faults
#define FAKE_FAULTS           false   // What to do with faults, T=detect and display them but don't change signals

#endif
