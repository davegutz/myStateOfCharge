// For Photon
#if (PLATFORM_ID==6 || PLATFORM_ID==12)  // Photon, Argon
  #include "application.h"  // Should not be needed if file ino or Arduino
  SYSTEM_THREAD(ENABLED);   // Make sure code always run regardless of network status
  #include <Arduino.h>      // Used instead of Print.h - breaks Serial
#else
  using namespace std;
  #undef max
  #undef min
#endif

#if (PLATFORM_ID==12)  // Argon only
  #include "hardware/BleSerialPeripheralRK.h"
  SerialLogHandler logHandler;
#endif

#include "hardware/SerialRAM.h"
SerialRAM ram;

#if (PLATFORM_ID==12)  // Argon only
  // First parameter is the transmit buffer size, second parameter is the receive buffer size
  BleSerialPeripheralStatic<32, 256> bleSerial;
  const unsigned long TRANSMIT_PERIOD_MS = 2000;
  unsigned long lastTransmit = 0;
  int counter = 0;
#endif


// Globals
#include "myTalk.h"
#include "mySummary.h"
#include "retained.h"
#include "command.h"
#include "local_config.h"
#include "constants.h"
#include "parameters.h"

extern CommandPars cp;            // Various parameters to be common at system level
extern RetainedPars rp;           // Various parameters to be static at system level
extern Sum_st mySum[NSUM];        // Summaries for saving charge history
extern Flt_st myFlt[NFLT];        // Summaries for saving fault history
extern SavedPars sp;             // Various parameters to be common at system level

retained RetainedPars rp;             // Various control parameters static at system level
retained Sum_st mySum[NSUM];          // Summaries
retained Flt_st myFlt[NFLT];          // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level
SavedPars sp = SavedPars(&ram);           // Various parameters to be common at system level

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Hi");
  ram.begin(0, 0);
  ram.setAutoStore(true);
  sp.load_all();
  Serial.printf("Check corruption\n");
  if ( sp.is_corrupt() ) 
  {
    sp.nominal();
    Serial.printf("Fixed corruption\n");
  }
  sp.pretty_print(true);
}

void loop()
{
 
    // Chit-chat requires 'read' timing so 'DP' and 'Dr' can manage sequencing
  asap();
  chat();   // Work on internal chit-chat
  talk();   // Collect user inputs

  unsigned int num = 0;
  unsigned long int now = micros();
  num = sp.read_all();
  unsigned long int then = micros();
  sp.assign_all();
  float all = ( (then - now) - (micros() - then) ) /1e6;
  Serial.printf("read each avg %7.6f s, all %7.6fs\n", all/float(num), all);

  delay(100);

}
