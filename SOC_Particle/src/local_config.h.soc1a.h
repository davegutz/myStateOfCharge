#ifndef local_config_h
#define local_config_h

#include "version.h"
#define CONFIG_SOC1A

// Features config
#define CONFIG_SBAUD               230400      // Default Serial baud
#define CONFIG_S1BAUD              230400      // Default Serial1 baud when able to run AT to set it using AT+BAUD9
#define CONFIG_ARGON
#define CONFIG_SSD1306_OLED
#define CONFIG_47L16_EERAM
#define CONFIG_DS18B20_SWIRE

// #define DEBUG_QUEUE
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
// #define LOGHANDLE

// * = SRAM EEPROM adjustments, retained on power reset

// Miscellaneous
#define ASK_DURING_BOOT       1   // Flag to ask for application of this file to * retained adjustements
#define MODELING              0   // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model.  +240 for discn    

// Sensor biases
#define CURR_BIAS_AMP        -0.13  // Calibration of amplified shunt sensor (* 'DA'), A
#define CURR_SCALE_AMP        1.0   // Hardware to match data (* 'SA')
#define CURR_BIAS_NOA         0.44  // Calibration of non-amplified shunt sensor (* 'DB'), A
#define CURR_SCALE_NOA        1.0   // Hardware to match data (* 'SB')
#define CURR_SCALE_DISCH      1.0   // Scale discharge to account for asymetric inverter action only on discharge (* 'SD'), slr
#define SHUNT_GAIN            1333. // Shunt V2A gain (scale with * 'SA' and 'SB'), A/V (1333 is 100A/0.075V)
#define SHUNT_AMP_R1          5100.     // Amplifed shunt ADS resistance, ohms (5k1)  100/5.1  = 19.61
#define SHUNT_AMP_R2          100000.   // Amplifed shunt ADS resistance, ohms (100k) 0.075v  = 1.47 v => 3.3/2+1.47 = 3.12 < 3.3
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors (* 'DI'), A
#define VOLT_BIAS             0.0   // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS             0.0   // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SENSE_R_LO      4700      // Vb low sense resistor, ohm (4700)
#define VB_SENSE_R_HI      20000     // Vb high sense resistor, ohm (20000)
#define VB_SCALE              1.0125 // Scale Vb sensor (* 'SV')
#define VTAB_BIAS             0.11   // Bias on voc_soc table (* 'Dw'), V

// Battery.  One 12 V 100 Ah battery bank would have NOM_UNIT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have NOM_UNIT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  NOM_UNIT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF_SCALE   1.0     // Scalar on Coulombic efficiency of battery, fraction of charge that gets used (1.0)
#define MON_CHEM              0       // Chemistry monitor code integer, 0=Battleborn, 1=CHINS
#define SIM_CHEM              0       // Chemistry simulate code integer, 0=Battleborn, 1=CHINS
#define NOM_UNIT_CAP          108.4   // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah
#define HYS_SCALE             1.0     // Scalar on hysteresis (1.0)
#define NS                    1.0     // Number of series batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BS')
#define NP                    1.0     // Number of parallel batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BP')

// Faults
#define FAKE_FAULTS           true   // What to do with faults, T=detect and display them but don't change signals
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
#define CC_DIFF_SOC_DIS_THRESH  0.2   // Signal selection threshold for Coulomb counter EKF disagree test (0.2, 0.1 too small on truck)

#endif
