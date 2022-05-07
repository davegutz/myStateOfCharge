#ifndef local_config_h
#define local_config_h

const   String    unit = "pro_20220502";  // Gross configuration stamp, put on data points and matches GitHub tag
#define CURR_BIAS_AMP         -0.24 // Calibration of amplified shunt sensor, A
#define CURR_BIAS_NOAMP       -1.1  // Calibration of non-amplified shunt sensor, A
#define CURR_BIAS_ALL         0.0   // Bias on all shunt sensors, A
#define VOLT_BIAS             0.0   // Bias on Vbatt sensor, V
#define TEMP_BIAS             0.0   // Bias on Tbatt sensor, deg C
const String batt_mod = "Battleborn"; // Used various configuration parameters
#define RATED_BATT_CAP        100.  // Nominal battery unit capacity, Ah
#define NS                    1.0   // Number of series batteries in bank
#define NP                    1.0   // Number of parallel batteries in bank

#endif
