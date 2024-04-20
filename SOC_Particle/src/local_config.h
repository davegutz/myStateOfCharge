#ifndef LOCAL_CONFIG_H
#define LOCAL_CONFIG_H

// #include "local_config.h.pro0p.h"
// #include "local_config.h.pro1a.h"
// #include "local_config.h.pro2p2.h"  // funny standalone breadboard like a DUE.  Avail. seldom used
#include "local_config.h.pro3p2.h"
// 
// #include "local_config.h.soc0p.h"
// #include "local_config.h.soc1a.h"
// //#include "local_config.h.soc3p2.h"   failed. shorted 3v3 to gnd with blob of solder
// #include "local_config.h.soc2p2.h"

#ifdef CONFIG_PRO0P
    #define _UNIT str("pro0p")
#elif defined(CONFIG_PRO1A)
    #define _UNIT str("pro1a")
#elif defined(CONFIG_PRO2P2)
    #define _UNIT str("pro2p2")
#elif defined(CONFIG_PRO3P2)
    #define _UNIT str("pro3p2")
#elif defined(CONFIG_SOC0P)
    #define _UNIT str("soc0p")
#elif defined(CONFIG_SOC1A)
    #define _UNIT str("soc1a")
#elif defined(CONFIG_SOC2P2)
    #define _UNIT str("soc2p2")
#elif defined(CONFIG_SOC3P2)
    #define _UNIT str("soc3p2")
#else
    #error "CONFIG_<UNIT> not defined"
#endif

const String unit = version + "_" + _UNIT;

#endif
