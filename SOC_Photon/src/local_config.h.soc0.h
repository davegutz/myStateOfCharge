#ifndef local_config_h
#define local_config_h

// version with Tweak on both current sensors
const   String    unit = "soc0_20220502";  // Gross configuration stamp, put on data points and matches GitHub tag

//#define CURR_BIAS_AMP       -0.2  // 3/1/2022. Dapped to -0.8 on 2/28
//#define CURR_BIAS_NOAMP     -0.5  // 3/1/2022. Dapped to -1.1 on 2/28
//#define CURR_BIAS_ALL       -0.6  // 3/5/2022. Dapped to 0 on 2/28
//#define CURR_BIAS_AMP       -0.8  // 3/5/2022. Dapped to -0.93 on 3/1
//#define CURR_BIAS_NOAMP     -1.1  // 3/5/2022. Dapped to -1.23 on 3/1
//#define CURR_BIAS_AMP       -0.93 // 4/30/2022. Change to -1.03 from tweak
//#define CURR_BIAS_NOAMP     -1.23 // 4/30/2022. Change to -1.37 from tweak
#define CURR_BIAS_AMP         -1.03 // Calibration of amplified shunt sensor, A
#define CURR_BIAS_NOAMP       -1.37  // Calibration of non-amplified shunt sensor, A
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors, A
#define VOLT_BIAS             0.0   // Bias on Vbatt sensor, V
#define TEMP_BIAS             0.0   // Bias on Tbatt sensor, deg C

// const String chemistry = "Battleborn"; // Used various configuration parameters
#define MOD_CODE              0     // Chemistry code integer, 0=Battleborn, 1=LION
#define RATED_BATT_CAP        100.  // Nominal battery unit capacity, Ah
#define RATED_TEMP            25.   // Temperature at RATED_BATT_CAP, deg C
#define NS                    1.0   // Number of series batteries in bank
#define NP                    1.0   // Number of parallel batteries in bank

#endif
