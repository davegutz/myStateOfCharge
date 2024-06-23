#ifndef soc2p2_h
#define soc2p2_h

#include "version.h"

// Features config
#define HDWE_UNIT               "soc2p2"
#define SOFT_SBAUD              460800      // Default Serial baud when able
#define SOFT_S1BAUD             230400      // Default Serial1 baud when able to run AT to set it using AT+BAUD9
#define HDWE_PHOTON2
#define HDWE_IB_HI_LO
#define HDWE_DS2482_1WIRE
// #define SOFT_DEBUG_QUEUE
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
// #define LOGHANDLE

// * = SRAM EEPROM adjustments, retained on power reset

// Miscellaneous
#define ASK_DURING_BOOT       1   // Flag to ask for application of this file to * retained adjustements
#define MODELING              0 // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model.  +240 for discn

// Sensor biases
#define CURR_BIAS_AMP         0.02  // Calibration of amplified shunt sensor (* 'DA'), A
#define CURR_SCALE_AMP         1.0  // Hardware to match data (* 'SA')
#define CURR_BIAS_NOA        -0.1   // Calibration of non-amplified shunt sensor (* 'DB'), A
#define CURR_SCALE_NOA         1.0  // Hardware to match data (* 'SB')
#define CURR_SCALE_DISCH       1.0  // Scale discharge to account for asymetric inverter action only on discharge (* 'SD'), slr
#define SHUNT_GAIN            2666. // Shunt V2A gain (scale with * 'SA' and 'SB'), A/V (2666 is 200A/0.075V)
#define SHUNT_AMP_R1          5100. // Internal amp resistance 196x, ohms (5100)
#define SHUNT_AMP_R2       1000000. // Internal amp resistance 196x, ohms (1000000)
#define IB_ABS_MAX_AMP        12.0  // Hard range limit of sensor electrically impossible (=1.65 * SHUNT_GAIN * SHUNT_AMP_R1 / SHUNT_AMP_R2 *1.05) but saw -11.48 A
#define SHUNT_NOA_R1          5100. // Internal amp resistance 29.4x, ohms (5100)
#define SHUNT_NOA_R2         75000. // Internal amp resistance 29.4x, ohms (150000)
#define IB_ABS_MAX_NOA        157.  // Hard range limit of sensor electrically impossible (=1.65 * SHUNT_GAIN * SHUNT_NOA_R1 / SHUNT_NOA_R2 *1.05)
#define HDWE_IB_HI_LO_NOA_LO   -11. // Full NOA discharge transition, A (-11)
#define HDWE_IB_HI_LO_AMP_LO   -10. // Full AMP discharge transition, A (-10)  
#define HDWE_IB_HI_LO_AMP_HI    10. // Full AMP charge transition, A (10)
#define HDWE_IB_HI_LO_NOA_HI    11. // Full NOA charge transition, A (11)
#define CURR_BIAS_ALL           0.0 // Bias on all shunt sensors (* 'DI'), A
#define VOLT_BIAS             -0.05 // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS               0.0 // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SENSE_R_LO          4700 // Vb low sense resistor, ohm (4700)
#define VB_SENSE_R_HI         47000 // Vb high sense resistor, ohm (47000)
#define VB_SCALE                1.0 // Scale Vb sensor (* 'SV')
#define VTAB_BIAS              -0.1 // Bias on voc_soc table (* 'Dw'), V

// Battery.  One 12 V 100 Ah battery bank would have NOM_UNIT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have NOM_UNIT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  NOM_UNIT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF_SCALE   1.0     // Scalar on Coulombic efficiency of battery, fraction of charge that gets used (1.0)
#define CHEM                  2       // Chemistry monitor code integer, 0=Battleborn, 1=CHINS-guest room, 2=CHINS-garage
#define NOM_UNIT_CAP          102.9   // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah
#define HYS_SCALE             1.0     // Scalar on hysteresis (1.0)
#define NS                    2.0     // Number of series batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BS')
#define NP                    2.0     // Number of parallel batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BP')

// Faults
#define FAKE_FAULTS           true    // What to do with faults, T=detect and display them but don't change signals
#define CC_DIFF_SOC_DIS_THRESH  0.5   // Signal selection threshold for Coulomb counter EKF disagree test (0.2, 0.1 too small on truck)

#endif
