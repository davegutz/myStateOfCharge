#ifndef local_config_h
#define local_config_h

const String unit = "soc0p_20230403";  // scale CHINS
#define CONFIG_PHOTON
#undef CONFIG_ARGON

// * = SRAM EEPROM adjustments, retained on power reset

// Miscellaneous
#define ASK_DURING_BOOT       1   // Flag to ask for application of this file to * retained adjustements
#define MODELING              0   // Nominal modeling bitmap (* 'Xm'), 0=all hdwe, 1+=Tb, 2+=Vb, 4+=Ib, 7=all model.  +240 for discn
// #define USE_BLE

// Sensor biases
#define USE_ADS                 // Instead of using ADS device to read amp by I2C, use analog pins
#ifdef USE_ADS
    #define CURR_BIAS_AMP         0.98  // Calibration of amplified shunt sensor (* 'DA'), A
    #define CURR_SCALE_AMP        1.006 // Hardware to match data (* 'SA')
    #define CURR_BIAS_NOA         0.15  // Calibration of non-amplified shunt sensor (* 'DB'), A
    #define CURR_SCALE_NOA        1.042 // Hardware to match data (* 'SB')
    #define SHUNT_GAIN            1333. // Shunt V2A gain (scale with * 'SG'), A/V (-1333 is -100A/0.075V)
#else
    #define CURR_BIAS_AMP         0.0   // Calibration of amplified shunt sensor (* 'DA'), A
    #define CURR_SCALE_AMP        1.0   // Hardware to match data (* 'SA')
    #define CURR_BIAS_NOA         0.0   // Calibration of amplified shunt sensor (* 'DB'), A
    #define CURR_SCALE_NOA        1.0   // Hardware to match data (* 'SB')
    #define SHUNT_GAIN            1333. // Shunt V2A gain (scale with * 'SG'), A/V (-1333 is -100A/0.075V)
#endif
#define SHUNT_AMP_R1          5600.     // Amplifed shunt ADS resistance, ohms
#define SHUNT_AMP_R2          27000.    // Amplifed shunt ADS resistance, ohms
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors (* 'Di'), A
#define VOLT_BIAS             0.03  // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS             0.0   // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SENSE_R_LO      4700      // Vb low sense resistor, ohm (4700)
#define VB_SENSE_R_HI      20000     // Vb high sense resistor, ohm (20000)
#define VB_SCALE              1.017  // Scale Vb sensor (* 'SV')

// Battery.  One 12 V 100 Ah battery bank would have NOM_UNIT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have NOM_UNIT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  NOM_UNIT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF_SCALE   1.0     // Scalar on Coulombic efficiency of battery, fraction of charge that gets used (1.0)
#define MON_CHEM              1       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=CHINS, 2=Spare
#define SIM_CHEM              1       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=CHINS, 2=Spare
#define NOM_UNIT_CAP          112.7   // Nominal battery unit capacity at RATED_TEMP.  (* 'Sc' or '*BS'/'*BP'), Ah
#define HYS_SCALE             1.0     // Scalar on hysteresis (1.0 CHINS)
#define NS                    1.0     // Number of series batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BS')
#define NP                    2.0     // Number of parallel batteries in bank.  Fractions scale and remember NOM_UNIT_CAP (* 'BP')

// Faults
#define FAKE_FAULTS           true    // What to do with faults, T=detect and display them but don't change signals
// #define DEBUG_INIT                    // Use this to debug initialization using 'v-1;'
#define CC_DIFF_SOC_DIS_THRESH  0.5   // Signal selection threshold for Coulomb counter EKF disagree test (0.2, 0.1 too small on truck.   0.5 CHINS)

#endif
