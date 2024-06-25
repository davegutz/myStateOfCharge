#ifndef pro4p2_h
#define pro4p2_h
// 0a10aced202194944a04c148
#include "version.h"

// Features config
#define HDWE_UNIT               "pro4p2"
#define SOFT_SBAUD              460800      // Default Serial baud when able (don't think this does anything)
#define SOFT_S1BAUD             230400      // Default Serial1 baud when able to run AT to set it using AT+BAUD9 (don't think this does anything)
#define HDWE_PHOTON2
#define HDWE_BARE
// #define SOFT_DEBUG_QUEUE
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
// #define LOGHANDLE

// * = SRAM EEPROM adjustments, retained on power reset

// Miscellaneous
#define ASK_DURING_BOOT       0   // Flag to ask for application of this file to * retained adjustements
#define MODELING              7   // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model.  +240 for discn

// Hardware

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
