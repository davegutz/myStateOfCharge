#ifndef local_config_h
#define local_config_h

// version with Tweak on both current sensors
const String unit = "soc0_20220609";  // new float types, battery types, nomenclature, coulombic efficiency

// Sensor biases
#define CURR_BIAS_AMP         -0.23 // Calibration of amplified shunt sensor ('Da=#.#;'), A
#define CURR_BIAS_NOAMP       -0.57 // Calibration of non-amplified shunt sensor ('Db=#.#;'), A
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors ('Di=#.#;'), A
#define VOLT_BIAS             0.0   // Bias on Vbatt sensor ('Dc=#.#;'), V
#define TEMP_BIAS             0.0   // Bias on Tbatt sensor ('Dt=#.#;'  +reset), deg C

// Battery.  One 12 V 100 Ah battery bank would have RATED_BATT_CAP 100, NS 1, and NP 1
// Two 12 V 100 Ah series battery bank would have RATED_BATT_CAP 100, NS 2, and NP 1
// Four 12 V 200 Ah with two in parallel joined with two more in series
//   would have  RATED_BATT_CAP 200, NS 2, and NP 2
#define COULOMBIC_EFF         0.9985  // Coulombic efficiency of battery, fraction of charge that gets used
#define MOD_CODE              0       // Chemistry code integer ('Bm=#; Bs=#'), 0=Battleborn, 1=LION 
#define RATED_BATT_CAP        100.    // Nominal battery unit capacity (Scale with 'Sc=#.#;'), Ah
#define RATED_TEMP            25.     // Temperature at RATED_BATT_CAP (TODO:  talk input), deg C
#define NS                    1.0     // Number of series batteries in bank ('BS=#')
#define NP                    1.0     // Number of parallel batteries in bank ('BP=#;')


#endif
