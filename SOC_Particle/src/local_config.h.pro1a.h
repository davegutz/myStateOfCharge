#ifndef local_config_h
#define local_config_h

#include "version.h"
const String unit = version + "_pro1a";

// Features config
#define CONFIG_ARGON
#define CONFIG_47L16
#define CONFIG_DISP_SKIP 5
#define CONFIG_DS18B20

// * = SRAM EEPROM adjustments, retained on power reset

// Miscellaneous
#define ASK_DURING_BOOT       1   // Flag to ask for application of this file to * retained adjustements
#define MODELING              247 // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model.  +240 for discn

// Sensor biases
#define CURR_BIAS_AMP         0.5   // Calibration of amplified shunt sensor (* 'DA'), A
#define CURR_SCALE_AMP        0.990 // Hardware to match data (* 'SA')
#define CURR_BIAS_NOA         0.5   // Calibration of non-amplified shunt sensor (* 'DB'), A
#define CURR_SCALE_NOA        0.980 // Hardware to match data (* 'SB')
#define SHUNT_GAIN            1333. // Shunt V2A gain (scale with * 'SA' and 'SB'), A/V (1333 is 100A/0.075V)
#define SHUNT_AMP_R1          5600.     // Amplifed shunt ADS resistance, ohms (5k6)  100/5.6  = 17.86
#define SHUNT_AMP_R2          100000.   // Amplifed shunt ADS resistance, ohms (100k) 0.075v  = 1.34 v => 3.3/2+1.34 = 2.99 < 3.3
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors (* 'Di'), A
#define VOLT_BIAS             8.0   // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS             0.0   // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SENSE_R_LO      4700      // Vb low sense resistor, ohm (4700)
#define VB_SENSE_R_HI      20000     // Vb high sense resistor, ohm (20000)
#define VB_SCALE              1.004  // Scale Vb sensor (* 'SV')
#define VTAB_BIAS             0.0    // Bias on voc_soc table (* 'Dw'), V

// Battery.  One 12 V 100 Ah battery bank would have NOM_UNIT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have NOM_UNIT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  NOM_UNIT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF_SCALE   1.0     // Scalar on Coulombic efficiency of battery, fraction of charge that gets used (1.0)
#define MON_CHEM              0       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=CHINS, 2=Spare
#define SIM_CHEM              0       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=CHINS, 2=Spare
#define NOM_UNIT_CAP          100.    // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah
#define HYS_SCALE             1.0     // Scalar on hysteresis (1.0)
#define NS                    1.0     // Number of series batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BS')
#define NP                    1.0     // Number of parallel batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BP')

// Faults
#define FAKE_FAULTS           false   // What to do with faults, T=detect and display them but don't change signals
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
#define CC_DIFF_SOC_DIS_THRESH  0.2   // Signal selection threshold for Coulomb counter EKF disagree test (0.2, 0.1 too small on truck)

#endif
