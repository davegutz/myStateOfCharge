/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/daveg/Documents/GitHub/myStateOfCharge/eeprom_test/src/eeprom_test.ino"
#include "local_config.h"
#include "constants.h"

void setup();
void loop();
#line 4 "c:/Users/daveg/Documents/GitHub/myStateOfCharge/eeprom_test/src/eeprom_test.ino"
#if defined(CONFIG_ARGON)
  SerialLogHandler logHandler;
  #include "hardware/SerialRAM.h"
  SerialRAM ram;
  // First parameter is the transmit buffer size, second parameter is the receive buffer size
  const unsigned long TRANSMIT_PERIOD_MS = 2000;
  unsigned long lastTransmit = 0;
  int counter = 0;
#endif


// Globals
#include "myTalk.h"
#include "command.h"
#include "parameters.h"


extern CommandPars cp;            // Various parameters to be common at system level
extern Flt_st mySum[NSUM];        // Summaries for saving charge history
extern Flt_st myFlt[NFLT];        // Summaries for saving fault history
extern SavedPars sp;              // Various parameters to be common at system level
extern eSavedPars esp;              // Various parameters to be common at system level

retained Flt_st mySum[NSUM];          // Summaries
retained Flt_st myFlt[NFLT];          // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level
#if defined(CONFIG_PHOTON)
  SavedPars sp = SavedPars(mySum, NSUM, myFlt, NFLT);
#elif defined(CONFIG_ARGON)
  SavedPars sp = SavedPars(&ram);           // Various parameters to be common at system level
  eSavedPars esp = eSavedPars();             // Various parameters to be common at system level
#elif defined(CONFIG_PHOTON2)
  SavedPars sp = SavedPars(mySum, NSUM, myFlt, NFLT);
  eSavedPars esp = eSavedPars();             // Various parameters to be common at system level
#endif

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Hi");



  #if defined(CONFIG_ARGON)
    // I2C
    Wire1.begin();
    ram.begin(0, 0);
    ram.setAutoStore(true);
  #endif
  delay(1000);
  // #if defined(CONFIG_PHOTON)
  //   sp.reset_pars();
  #if defined(CONFIG_ARGON)
    sp.load_all();
  // #elif defined(CONFIG_PHOTON2)
  //   sp.reset_pars();
  #endif
  esp.load_all();
  // esp.reset_pars();
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

/*
  static uint16_t count = 0;
  if ( count==100 )
  {
    unsigned int num = 0;
    unsigned long int now = micros();
    // num = sp.load_all();
    unsigned long int then = micros();
    num = sp.large_reset();
    sp.mem_print();
    float all = float(then - now) /1e6;
    Serial.printf("\nnum %d read each avg %7.6f s, all %7.6fs\n\n", num, all/float(num), all);
  }
  if ( count==120 )
  {
    Serial.printf("\n\nsizeof EEPROM %d\n", EEPROM.length());
    unsigned int num = 0;
    unsigned long int now = micros();
    // num = esp.load_all();
    num = esp.large_reset();
    unsigned long int then = micros();
    esp.mem_print();
    float all = float(then - now) /1e6;
    Serial.printf("\nnum %d eread each eavg %7.6f s, eall %7.6fs\n\n", num, all/float(num), all);
  }
  if (++count == 1000) count = 0;
*/

  delay(100);

}
