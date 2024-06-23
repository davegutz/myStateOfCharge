#ifndef soc4p2_h
#define soc4p2_h

#include "version.h"

// Features config
#define HDWE_UNIT               "soc4p2"
#define SOFT_SBAUD              460800      // Default Serial baud when able (don't think this does anything)
#define SOFT_S1BAUD             230400      // Default Serial1 baud when able to run AT to set it using AT+BAUD9 (don't think this does anything)
#define HDWE_PHOTON2
#define HDWE_IB_HI_LO
#define HDWE_2WIRE
// #define SOFT_DEBUG_QUEUE
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
// #define LOGHANDLE

// * = SRAM EEPROM adjustments, retained on power reset

// Miscellaneous
#define ASK_DURING_BOOT       1   // Flag to ask for application of this file to * retained adjustements
#define MODELING              0 // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model.  +240 for discn

// Sensor biases
#define CURR_BIAS_AMP        -0.10  // Calibration of amplified shunt sensor (* 'DA'), A
#define CURR_SCALE_AMP         1.0  // Hardware to match data (* 'SA')
#define CURR_BIAS_NOA        -0.25  // Calibration of non-amplified shunt sensor (* 'DB'), A
#define CURR_SCALE_NOA         1.0  // Hardware to match data (* 'SB')
#define CURR_SCALE_DISCH       1.0  // Scale discharge to account for asymetric inverter action only on discharge (* 'SD'), slr
#define SHUNT_GAIN            1333. // Shunt V2A gain (scale with * 'SA' and 'SB'), A/V (1333 is 100A/0.075V)
#define SHUNT_AMP_R1          5100. // Internal amp resistance 196x, ohms (5100)
#define SHUNT_AMP_R2       1000000. // Internal amp resistance 196x, ohms (1000000)
#define IB_ABS_MAX_AMP        12.0  // Hard range limit of sensor electrically impossible (=1.65 * SHUNT_GAIN * SHUNT_AMP_R1 / SHUNT_AMP_R2 *1.05) but saw -11.48 A
#define SHUNT_NOA_R1          5100. // Internal amp resistance 29.4x, ohms (5100)
#define SHUNT_NOA_R2        150000. // Internal amp resistance 29.4x, ohms (150000)
#define IB_ABS_MAX_NOA        78.5  // Hard range limit of sensor electrically impossible (=1.65 * SHUNT_GAIN * SHUNT_NOA_R1 / SHUNT_NOA_R2 *1.05)
#define HDWE_IB_HI_LO_NOA_LO   -11. // Full NOA discharge transition, A (-11)
#define HDWE_IB_HI_LO_AMP_LO   -10. // Full AMP discharge transition, A (-10)  
#define HDWE_IB_HI_LO_AMP_HI    10. // Full AMP charge transition, A (10)
#define HDWE_IB_HI_LO_NOA_HI    11. // Full NOA charge transition, A (11)
#define CURR_BIAS_ALL           0.0 // Bias on all shunt sensors (* 'DI'), A
#define VOLT_BIAS              0.05 // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS               0.0 // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SENSE_R_LO          4700 // Vb low sense resistor, ohm (4700)
#define VB_SENSE_R_HI         22000 // Vb high sense resistor, ohm (22000)
#define VB_SCALE                1.0 // Scale Vb sensor (* 'SV')
#define VTAB_BIAS              0.11 // Bias on voc_soc table (* 'Dw'), V

// Battery.  One 12 V 100 Ah battery bank would have NOM_UNIT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have NOM_UNIT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  NOM_UNIT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF_SCALE   1.0     // Scalar on Coulombic efficiency of battery, fraction of charge that gets used (1.0)
#define CHEM                  0       // Chemistry monitor code integer, 0=Battleborn, 1=CHINS-guest room, 2=CHINS-garage
#define NOM_UNIT_CAP          108.4   // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah
#define HYS_SCALE             1.0     // Scalar on hysteresis (1.0)
#define NS                    1.0     // Number of series batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BS')
#define NP                    1.0     // Number of parallel batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BP')

// Faults
#define FAKE_FAULTS           true    // What to do with faults, T=detect and display them but don't change signals
#define CC_DIFF_SOC_DIS_THRESH  0.5   // Signal selection threshold for Coulomb counter EKF disagree test (0.2, 0.1 too small on truck)

#endif
