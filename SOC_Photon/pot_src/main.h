/*
 * Project Vent_Photon
  * Description:
  * Combine digital pot output in parallel with manual pot
  * to control an ECMF-150 TerraBloom brushless DC servomotor fan.
  * 
  * By:  Dave Gutz January 2021
  * 07-Jan-2021   Tinker version
  * 
//
// MIT License
//
// Copyright (C) 2021 - Dave Gutz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

* Found MCP4151 POT how to at
  https://community.particle.io/t/photon-controlling-5v-output-using-mcp4151-pot-and-photon-spi-api/25001/2

* There is a POT library here.   Haven't used it yet
  https://github.com/jmalloc/arduino-mcp4xxx

****Note about ground
* For ICs to work, I believe ECMF B (ground) needs to be connected to
  Photon and IC ground.  Need to try this.   If doesn't work, need
  to revert to PWM scheme and even that may need isolation

* Pot Analog Connections
  POHa    ECMF 10 V supply
  POWa    ECMF Control Signal
  POLa    2-D of MOSFET and 6-POW of digipot

* Digipot Hardware Connections (MCP4151-103, 10k nom, to Photon).   Completely off when depowered at VDD
  1-CS   = D5 and 4k7 to 5V rail
  2-SCK  = D4
  3-MOSI = D2  (4k7 to D3 jumper)
  4-GND  = GND RAIL
  5-POA  = POHd = 10v POHa from ECMF
  6-POW  = Analog POT POLa
  7-POB  = POLd = ECMF GND
  8-VDD  = 10v POHa from ECMF

* MOSFET IRF530N  N-ch Normally Closed MOSFET (S-D NC switched open by G)
  1-G     D7
  2-D     POB of analog POT and POW of digipot
  3-S     ECMF GND and POB of digipot

* Honeywell temp/humidity Hardware Connections (Humidistat with temp SOIC  HIH6131-021-001)
  1-VCORE= 0.1uF jumper to GND
  2-VSS  = GND rail
  3-SCL  = D1
  4-SCA  = D0
  5-AL_H = NC
  6-AL_L = NC
  7-NC   = NC
  8-VDD  = 3v3

* Photon to Proto
  GND = to 2 GND rails
  D0  = 4-SCA of Honeywell and 4k7 3v3 jumper I2C pullup
  D1  = 3-SCL of Honeywell and 4k7 3v3 jumper I2C pullup
  D2  = to 3-SDI/SDO of digipot
  D3  = 4k7 D3 jumper to D2
  D4  = 2-SCK of digipot
  D5  = 1-CS of digipot
  D6  = Y-C of DS18 and 4k7 3v3 jumper pullup
  VIN = 5V rail
  3v3 = 3v3 rail 
  micro USB = Serial Monitor on PC (either Particle Workbench monitor or CoolTerm) 

* 1-wire Temp (MAXIM DS18B20)  library at https://github.com/particle-iot/OneWireLibrary
  Y-C   = D6
  R-VDD = 5V rail
  B-GND = GND rail

* Elego power module mounted to 5V and 3v3 and GND rails
  5V jumper = 5V RAIL on "A-side" of Photon
  Jumper "D-side" of Photon set to OFF
  Round power supply = round power supply plug 12 VDC x 1.0A Csec CS12b20100FUF
  
 * Author: Dave Gutz davegutz@alum.mit.edu  repository GITHUB myVentilator
 
  To get debug data
  1.  Set debug = 2 in constants.h
  2.  Rebuild and upload
  3.  Start CoolTerm_0.stc

  Requirements:
  1.  Wire digital POT in parallel with supplied 10K hardware POT.
  2.  When Elego power off, digital POT off and digital POT resistance = open circuit

*/

// For Photon
#if (PLATFORM_ID == 6)
#define PHOTON
#include "application.h" // Should not be needed if file ino or Arduino
SYSTEM_THREAD(ENABLED); // Make sure code always run regardless of network status
#include <Arduino.h>     // Used instead of Print.h - breaks Serial
#else
#undef PHOTON
using namespace std;
#undef max
#undef min
#endif

#include "constants.h"
#include <OneWire.h>
#include <DS18.h>
														   
// Global locals
char buffer[256];           // Serial print buffer
int hum = 0;                // Relative humidity integer value, %
int I2C_Status = 0;         // Bus status
double Tbatt_Sense = NOMSET;   // Sensed plenum temp, F
double updateTime = 0.0;    // Control law update time, sec
int numTimeouts = 0;        // Number of Particle.connect() needed to unfreeze
bool webHold = false;       // Web permanence request
int webDmd = 62;            // Web sched, F

#ifdef PHOTON
byte pin_1_wire = D6; //Blinks with each heartbeat
byte vdd_supply = D7;  // Power the MCP4151
#endif

// Utilities
void serial_print_inputs(unsigned long now, double run_time, double T);
void serial_print(void);
int pot_write(int step);
boolean load(int reset, double T, unsigned int time_ms);
DS18 sensor_tbatt(pin_1_wire);

#ifndef NO_CLOUD
int particleHold(String command)
{
  if (command == "HOLD")
  {
    webHold = true;
    return 1;
  }
  else
  {
     webHold = false;
     return 0;
  }
}


int particleSet(String command)
{
  int possibleSet = atoi(command);
  if (possibleSet >= MINSET && possibleSet <= MAXSET)
  {
      webDmd = possibleSet;
      return possibleSet;
  }
  else return -1;
}
#endif


// Setup
void setup()
{

#ifdef PHOTON
  // WiFi.disconnect();
  // delay(1000);
  // WiFi.off();
  // delay(1000);
#endif

  // Serial
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:
  Serial.flush();
  delay(1000);  // Ensures a clean display on Arduino Serial startup on CoolTerm

  // Peripherals
  if ( !bare )
  {

    // I2C
    Wire.setSpeed(CLOCK_SPEED_100KHZ);
    Wire.begin();

  }

  // Begin
  Particle.connect();
  #ifndef NO_CLOUD
//    Particle.function("HOLD", particleHold);
//    Particle.function("SET",  particleSet);
  #endif

  

#ifdef PHOTON
  if ( debug>1 ) { sprintf(buffer, "Particle Photon.  bare = %d,\n", bare); Serial.print(buffer); };
#else
  if ( debug>1 ) { sprintf(buffer, "Arduino Mega2560.  bare = %d,\n", bare); Serial.print(buffer); };
#endif

  // Header for debug print
  if ( debug>1 )
  { 
    Serial.print(F("flag,time_ms,run_time,T,I2C_Status,")); Serial.println("");
  }

  if ( debug>3 ) { Serial.print(F("End setup debug message=")); Serial.println(F(", "));};

} // setup


// Loop
void loop()
{
  static unsigned long now = millis();      // Keep track of time
  static unsigned long past = millis();     // Keep track of time
  static boolean toggle = false;            // Generate heartbeat
  static double run_time = 0;               // Time, seconds
  static int reset = 1;                     // Dynamic reset
  // static boolean was_testing = true;        // Memory of testing, used to perform a logic reset on transition
  double T = 0;                             // Present update time, s
  boolean testing = true;                   // Initial startup is calibration mode to 60 bpm, 99% spo2, 2% PI
  const int bare_wait = int(1000.0);        // To simulate peripherals sample time
  static int cmd = 0;
  bool control;            // Control sequence, T/F
  bool display;            // LED display sequence, T/F
  bool filter;             // Filter for temperature, T/F
  bool publishAny;         // Publish, T/F
  bool publish1;           // Publish, T/F
  bool publish2;           // Publish, T/F
  bool publish3;           // Publish, T/F
  bool publish4;           // Publish, T/F
  bool query;              // Query schedule and OAT, T/F
  bool read;               // Read, T/F
  static unsigned long    lastControl  = 0UL; // Last control law time, ms
  static unsigned long    lastDisplay  = 0UL; // Las display time, ms
  static unsigned long    lastFilter   = 0UL; // Last filter time, ms
  static unsigned long    lastModel    = 0UL; // Las model time, ms
  static unsigned long    lastPublish1 = 0UL; // Last publish time, ms
  static unsigned long    lastPublish2 = 0UL; // Last publish time, ms
  static unsigned long    lastPublish3 = 0UL; // Last publish time, ms
  static unsigned long    lastPublish4 = 0UL; // Last publish time, ms
  static unsigned long    lastQuery    = 0UL; // Last read time, ms
  static unsigned long    lastRead     = 0UL; // Last read time, ms
  static double           tFilter;            // Modeled temp, F

  // Sequencing
  filter = ((now-lastFilter)>=FILTER_DELAY) || reset>0;
  if ( filter )
  {
    tFilter     = float(now-lastFilter)/1000.0;
    if ( debug > 3 ) Serial.printf("Filter update=%7.3f\n", tFilter);
    lastFilter    = now;
  }

  publish1  = ((now-lastPublish1) >= PUBLISH_DELAY*4);
  if ( publish1 ) lastPublish1  = now;
  publish2  = ((now-lastPublish2) >= PUBLISH_DELAY*4)  && ((now-lastPublish1) >= PUBLISH_DELAY);
  if ( publish2 ) lastPublish2  = now;
  publish3  = ((now-lastPublish3) >= PUBLISH_DELAY*4)  && ((now-lastPublish1) >= PUBLISH_DELAY*2);
  if ( publish3 ) lastPublish3  = now;
  publish4  = ((now-lastPublish4) >= PUBLISH_DELAY*4)  && ((now-lastPublish1) >= PUBLISH_DELAY*3);
  if ( publish4 ) lastPublish4  = now;
  publishAny  = publish1 || publish2 || publish3 || publish4;

  read    = ((now-lastRead) >= READ_DELAY || reset>0) && !publishAny;
  if ( read     ) lastRead      = now;

  query   = ((now-lastQuery)>= QUERY_DELAY) && !read;
  if ( query    ) lastQuery     = now;

  display   = ((now-lastDisplay) >= DISPLAY_DELAY) && !query;
  if ( display ) lastDisplay    = now;


  // Sample inputs
  past = now;
  now = millis();
  T = (now - past)/1e3;
  unsigned long deltaT = now - lastControl;
//  control = (deltaT>=CONTROL_DELAY) && !display;
  control = (deltaT>=CONTROL_DELAY) || reset;
  if ( control  )
  {
    updateTime    = float(deltaT)/1000.0 + float(numTimeouts)/100.0;
    lastControl   = now;
  }

  // was_testing = testing;
  testing = load(reset, T, now);
  testing = testing;
  digitalWrite(vdd_supply, HIGH);
  delay(2000);

  if ( bare )
  {
    delay ( bare_wait );
  }
  run_time += T;
  if ( debug>3 ) { Serial.print(F("debug loop message here=")); Serial.println(F(", ")); };

  // Outputs
  if ( control )
  {
    if ( !reset )
    {
      cmd += 32;
      if ( cmd>255 ) cmd = 255;
    }
    pot_write(cmd);
    Serial.println(cmd, DEC);
    toggle = !toggle;
    if ( cmd==255 ) cmd = 0;
  }

  // Read sensors
  if ( read )
  {
    if ( !bare )
    {
      if ( debug>3 ) Serial.println(F("read"));
      Wire.beginTransmission(TA_SENSOR);
      Wire.endTransmission();
      delay(40);
      Wire.requestFrom(TA_SENSOR, 4);
      Wire.write(byte(0));
      uint8_t b = Wire.read();
      I2C_Status = b >> 6;

      // Honeywell conversion
      int rawHum  = (b << 8) & 0x3f00;
      rawHum |=Wire.read();
      hum = roundf(rawHum / 163.83) + HW_HUMCAL;
      int rawTemp = (Wire.read() << 6) & 0x3fc0;
      rawTemp |=Wire.read() >> 2;

      // MAXIM conversion
      if (sensor_tbatt.read()) Tbatt_Sense = sensor_tbatt.fahrenheit() + TP_TEMPCAL;

    }
    else
    {
      delay(41); // Usual I2C time
    }
  }


  // Publish
  if ( publishAny && debug>3 )
  {
    if ( debug>3 ) Serial.println(F("publish"));
  }

  // Monitor
  if ( debug>1 )
  {
    serial_print_inputs(now, run_time, T);
    serial_print();
  }

  // Initialize complete
  reset = 0;

} // loop


// Inputs serial print
void serial_print_inputs(unsigned long now, double run_time, double T)
{
  Serial.print(F("0,")); Serial.print(now, DEC); Serial.print(", ");
  Serial.print(run_time, 3); Serial.print(", ");
  Serial.print(T, 6); Serial.print(", ");  
  Serial.print(I2C_Status, DEC); Serial.print(", ");
  Serial.print(Tbatt_Sense, 1); Serial.print(", ");
  Serial.print(hum, 1); Serial.print(", ");
}

// Normal serial print
void serial_print()
{
  if ( debug>0 )
  {
    Serial.print(0, DEC); Serial.print(F(", "));   Serial.println(F(""));
  }
  else
  {
  }
  
}

// Load and filter
// TODO:   move 'read' stuff here
boolean load(int reset, double T, unsigned int time_ms)
{
  static boolean done_testing = false;

  // Read Sensor
  if ( !bare )
  {
  }
  else
  {
  }

  // Built-in-test logic.   Run until finger detected
  if ( true && !done_testing )
  {
    done_testing = true;
  }
  else                    // Possible finger detected
  {
    done_testing = false;
  }

  // Built-in-test signal replaces sensor
  if ( !done_testing )
  {
  }

  return ( !done_testing );
}

int pot_write(int step)
{
    digitalWrite(D5, LOW);
    SPI1.transfer(0);
    SPI1.transfer(step);
    digitalWrite(D5, HIGH);
    return step;
}