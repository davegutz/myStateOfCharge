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

#include "SerialRAM/SerialRAM.h"
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
extern CommandPars cp;            // Various parameters to be common at system level
extern RetainedPars rp;           // Various parameters to be static at system level
extern Sum_st mySum[NSUM];        // Summaries for saving charge history
extern Flt_st myFlt[NFLT];        // Summaries for saving fault history

retained RetainedPars rp;             // Various control parameters static at system level
retained Sum_st mySum[NSUM];          // Summaries
retained Flt_st myFlt[NFLT];          // Summaries
CommandPars cp = CommandPars();       // Various control parameters commanding at system level


void setup() {
  Serial.begin(115200);
  Serial.println("Hi");
  Serial.flush();
  ram.begin(0, 0);
}

void loop() {
  uint8_t buffer = 0x00;
  uint8_t randomByte = random(256);
  uint16_t randomAddress = random(0x0200);
  
  

  ram.write(randomAddress, randomByte);
  buffer = ram.read(randomAddress);

  Serial.printf("Wrote byte: 0x%d ", randomByte);
  Serial.printf(" at address 0x%d ", randomAddress);
  Serial.printf(" - Read back value: 0x%d ", buffer);

  if(randomByte == buffer){
    Serial.println(" - OK!");
  }
  else{
    Serial.println(" ERROR! Values do not match! Check your pullup resistors and wiring");
  }
  
  delay(1000);

}
