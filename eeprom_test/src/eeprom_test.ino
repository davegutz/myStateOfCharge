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
  SerialLogHandler logHandler;
#endif

#include "hardware/SerialRAM.h"
SerialRAM ram;

#if (PLATFORM_ID==12)  // Argon only
  // First parameter is the transmit buffer size, second parameter is the receive buffer size
  const unsigned long TRANSMIT_PERIOD_MS = 2000;
  unsigned long lastTransmit = 0;
  int counter = 0;
#endif


// Globals
#include "myTalk.h"
#include "command.h"
#include "local_config.h"
#include "constants.h"
#include "parameters.h"


extern CommandPars cp;            // Various parameters to be common at system level
// extern Flt_st mySum[NSUM];        // Summaries for saving charge history
// extern Flt_st myFlt[NFLT];        // Summaries for saving fault history
extern SavedPars sp;              // Various parameters to be common at system level
extern eSavedPars esp;              // Various parameters to be common at system level

// retained Flt_st mySum[NSUM];          // Summaries
// retained Flt_st myFlt[NFLT];          // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level
#if defined(CONFIG_PHOTON) // Photon
  retained SavedPars sp = SavedPars(&ram);           // Various parameters to be common at system level
#elif defined(CONFIG_ARGON)  // Argon
  SavedPars sp = SavedPars(&ram);           // Various parameters to be common at system level
  eSavedPars esp = eSavedPars();             // Various parameters to be common at system level
#endif

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Hi");

  // I2C
  Wire1.begin();


  ram.begin(0, 0);
  ram.setAutoStore(true);
  delay(1000);
  sp.load_all();
  Serial.printf("Check corruption\n");
  if ( sp.is_corrupt() ) 
  {
    sp.reset_pars();
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

  static uint16_t count = 0;
  if ( count==100 )
  {
    unsigned int num = 0;
    unsigned long int now = micros();
    num = sp.load_all();
    unsigned long int then = micros();
    // sp.assign_all();
    float all = ( (then - now) - (micros() - then) ) /1e6;
    Serial.printf("\nread each avg %7.6f s, all %7.6fs\n\n", all/float(num), all);
    sp.mem_print();
  }
  if ( count==120 )
  {
    Serial.printf("\n\nsizeof EEPROM %d\n", EEPROM.length());
    unsigned int num = 0;
    unsigned long int now = micros();
    num = esp.load_all();
    Serial.printf("num %d\n", num);
    unsigned long int then = micros();
    float all = ( (then - now) - (micros() - then) ) /1e6;
    Serial.printf("\neread each eavg %7.6f s, eall %7.6fs\n\n", all/float(num), all);
    esp.mem_print();
  }
  if (++count == 200) count = 0;

  delay(100);

}
