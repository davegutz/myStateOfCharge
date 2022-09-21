#ifndef local_config_h
#define local_config_h

const String unit = "pro_20220917b";  // dt fix for ekf, e_wrap_lo thr

#define ASK_DURING_BOOT         1   // Flag to ask for application of this file to * retained adjustements
// * = SRAM EEPROM adjustments, retained on power reset

// Sensor biases
#define SHUNT_GAIN            -1333.// Shunt V2A gain (scale with * 'SG'), A/V (-1333 is -100A/0.075V)
#define CURR_BIAS_AMP         0.    // Calibration of amplified shunt sensor (* 'DA'), A
#define CURR_BIAS_NOA         0.    // Calibration of non-amplified shunt sensor (* 'DB'), A
#define CURR_SCALE_NOA        1.    // Hardware to match data (* 'SA')
#define CURR_SCALE_AMP        1.    // Hardware to match data (* 'SB')
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors (* 'Di'), A
#define VOLT_BIAS             1.8   // Bias on Vb sensor (* 'Dc'), V
#define TEMP_BIAS             0.0   // Bias on Tb sensor (* 'Dt'), deg C
#define VB_SCALE              1.0   // Scale Vb sensor

// Battery.  One 12 V 100 Ah battery bank would have RATED_BATT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have RATED_BATT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  RATED_BATT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF         0.9985  // Coulombic efficiency of battery (Perm scale * 'Mk' for amp, * 'Nk' for noa), fraction of charge that gets used
#define MON_CHEM              0       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=LION 
#define SIM_CHEM              0       // Chemistry code integer (* 'Bm' for mon, * 'Bs' for sim), 0=Battleborn, 1=LION 
#define RATED_BATT_CAP        100.    // Nominal battery unit capacity.  (* 'Sc' or '*BS'/'*BP'), Ah
#define NS                    1.0     // Number of series batteries in bank.  Fractions scale and remember RATED_BATT_CAP (* 'BS')
#define NP                    1.0     // Number of parallel batteries in bank.  Fractions scale and remember RATED_BATT_CAP (* 'BP')


#endif
