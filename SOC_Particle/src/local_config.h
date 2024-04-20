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
    const String unit = version + "_pro0p";
#elif defined(CONFIG_PRO1A)
    const String unit = version + "_pro1a";
#elif defined(CONFIG_PRO2P2)
    const String unit = version + "_pro2p2";
#elif defined(CONFIG_PRO3P2)
    const String unit = version + "_pro3p2";
#elif defined(CONFIG_SOC0P)
    const String unit = version + "_soc0p";
#elif defined(CONFIG_SOC1A)
    const String unit = version + "_soc1a";
#elif defined(CONFIG_SOC2P2)
    const String unit = version + "_soc2p2";
#elif defined(CONFIG_SOC3P2)
    const String unit = version + "_soc3p2";
#els
    #error "CONFIG_<UNIT> not defined"
#endif


#endif
