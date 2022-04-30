#ifndef local_config_h
#define local_config_h

// version with Tweak on both current sensors
const   String    unit = "soc0_20220430";

//#define CURR_BIAS_AMP       -0.2  // 3/1/2022. Dapped to -0.8 on 2/28
//#define CURR_BIAS_NOAMP     -0.5  // 3/1/2022. Dapped to -1.1 on 2/28
//#define CURR_BIAS_ALL       -0.6  // 3/5/2022. Dapped to 0 on 2/28
//#define CURR_BIAS_AMP       -0.8  // 3/5/2022. Dapped to -0.93 on 3/1
//#define CURR_BIAS_NOAMP     -1.1  // 3/5/2022. Dapped to -1.23 on 3/1
//#define CURR_BIAS_AMP       -0.93 // 4/30/2022. Change to -1.03 from tweak
//#define CURR_BIAS_NOAMP     -1.23 // 4/30/2022. Change to -1.37 from tweak
#define CURR_BIAS_AMP       -1.03
#define CURR_BIAS_NOAMP     -1.37
#define CURR_BIAS_ALL       0.0
#define VOLT_BIAS            0.0
#define TEMP_BIAS            0.0

#endif
