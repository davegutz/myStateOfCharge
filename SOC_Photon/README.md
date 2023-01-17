# Vent_Photon

A Particle project named Vent_Photon

## Welcome to your project

Every new Particle project is composed of 3 important elements that you'll see have been created in your project directory for Vent_Photon.

### ```/src``` folder

This is the source folder that contains the firmware files for your project. It should *not* be renamed.  Anything that is in this folder when you compile your project will be sent to our compile service and compiled into a firmware binary for the Particle device that you have targeted.

If your application contains multiple files, they should all be included in the `src` folder. If your firmware depends on Particle libraries, those dependencies are specified in the `project.properties` file referenced below.

### ```.ino``` file

This file is the firmware that will run as the primary application on your Particle device. It contains a `setup()` and `loop()` function, and can be written in Wiring or C/C++. For more information about using the Particle firmware API to create firmware for your Particle device, refer to the [Firmware Reference](https://docs.particle.io/reference/firmware/) section of the Particle documentation.

### ```project.properties``` file

This is the file that specifies the name and version number of the libraries that your project depends on. Dependencies are added automatically to your `project.properties` file when you add a library to a project using the `particle library add` command in the CLI or add a library in the Desktop IDE.

## Adding additional files to your project

### Projects with multiple sources

If you would like add additional files to your application, they should be added to the `/src` folder. All files in the `/src` folder will be sent to the Particle Cloud to produce a compiled binary.

### Projects with external libraries

If your project includes a library that has not been registered in the Particle libraries system, you should create a new folder named `/lib/<libraryname>/src` under `/<project dir>` and add the `.h`, `.cpp` & `library.properties` files for your library there. Read the [Firmware Libraries guide](https://docs.particle.io/guide/tools-and-features/libraries/) for more details on how to develop libraries. Note that all contents of the `/lib` folder and subfolders will also be sent to the Cloud for compilation.

## Compiling your project

When you're ready to compile your project, make sure you have the correct Particle device target selected and run `particle compile <platform>` in the CLI or click the Compile button in the Desktop IDE. The following files in your project folder will be sent to the compile service:

- Everything in the `/src` folder, including your `.ino` application file
- The `project.properties` file for your project
- Any libraries stored under `lib/<libraryname>/src`

## Powering your device

The system is designed to be powered completely either from USB hooked to phone or device or from 12 V dc connector.   Normally in service the battery bank supplies 12 V and no USB is used.   The device saves fault information (EERAM 47L16) for cases when the battery banks management system powers off.  If the battery bank is off you can power with phone or device to extract information using UART terminal.   There are two UART terminals:  USB and HC-06 bluetooth.

## Redo Loop

***********************
Ctrl-Shift-P - Particle:Clean Application and Device OS (local)
Ctrl-Shift-P - Particle:Compile Application (local) or Check button in Visual Studio upper rc when select a source file
  Ctrl-Shift-P - Particle:Compile Application and Device OS (local) first time
Ctrl-Shift-P - Particle:Cloud Flash or Ctrl-Shift-P - Particle:Local Flash
  Ctrl-Shift-P - Particle:Flash application and Device OS (local) first time
Ctrl-Shift-P - Particle:Serial Monitor or CoolTerm
  'Talk' function using CoolTerm.  Be sure to add line feed to end of Ctrl-T dialog of CoolTerm.
  Arduino plot function works with debug=

Desktop settings
    .json has "particle.targetDevice": "proto"
    local_config.h has
        const   String    unit = "proto";

Laptop settings.json has  "particle.targetDevice": "soc0"
    .json has "particle.targetDevice": "soc0"
    local_config.h has
        const   String    unit = "soc0";

On laptop (same as desktop?)
    pull from GitHub repository changes just made on desktop
    compile
    flash to target 'soc0'
    be sure to build OS and flash OS too the first time

View results
  pycharm
  'CompareRunRun'   run to run overplot
  'CompareRunSim'   run vs sim overplot
  'CompareHist'     history vs sim overplot
  'CompareFault'    fault vs sim overplot

### Particle Argon Device - assumed at least 1A max

  GND = to 2 GND rails
  A1  = L of 20k ohm from 12v and H of 4k7 ohm + 47uF to ground
  A3  = Filtered Vo of 'no amp' amp circuit
  A3  = Filtered Vc of both amp circuits (yes, single point of failure for both amps)
  A5  = Filtered Vo of 'amp' amp circuit
  D0  = SCA of ASD, SCA of OLED, and 4k7 3v3 jumper I2C pullup
  D1  = SCL of ASD, SCA of OLED, and 4k7 3v3 jumper I2C pullup
  D6  = Y-C of DS18 for Tbatt and 4k7 3v3 jumper pullup
  VIN = 5V Rail 1A maximum from 7805CT home-made 12-->5 V regulator
  3v3 = 3v3 rail out supply to common of amp circuits, supply of OPA333, and R-OLED
  micro USB = Serial Monitor on PC (either Particle Workbench monitor or CoolTerm).   There is sufficient power in Particle devices to power the peripherals of this project on USB only
  USB  = 5v supply to VC-HC-06, R-1-wire
  TX  = RX of HC-06
  RX  = TX of HC-06
  D0-SDA = SDA-47L16 EERAM, Y-OLED
  D1-SCL = SCL-47L16 EERAM, B-OLED

### Voltage regulator (LM7805)

LM7805CT device with capacitors and LPF for Vb and 5v.   Voltage divider series resistors (24k7) to ground combine
with 0.33 uF cap to make LPF filter same bandwidth as Ib shunt LPF.
Vc  = 12v, 20k/4k7 divider to Gnd rail, and 0.33uF to Gnd rail
GND = Gnd rail
Vo  = 5v rail

### Passive Ib shunt and Vb low pass filters (LPF)

1 Hz LPF built into board.
Use pSpice circuit model (SOC_photon/datasheets/opa333_asd1013_5beta.asc) to verify filters because
OPA333 10uF high cap interacts with 1uF filter cap.  The Vb filter is a little more straightforward and same goals.
Goal of filter design is 2*pi r/s = 1 hz -3dB bandwidth.  Large PWM inverter noise from system enters at 60 Hz.
SOC calculation is equivalently a very slow time constant (integrator) so filter is between noise and usage.

### Amp ciruit 'amp'

  Ti OPA333.  Vc formed by 2x 4k7 voltage divider on 3v3 rail to ground.  A4 to A3 and A5 with 106 10uF high cap.
  See notes about 'LPF'
  V+   = 3v3 rail
  V-   = Gnd rail
  Vo   = 8k2/1uF LPF to A5 of device, 98k to pin+
  pin- = 5k1 of G-Shunt
  pin+ = 98k of Vc, 98k of Vo, and 5k1 of Y-Shunt

### No Amp ciruit 'no amp'

  For Argon Beta config, identical to 'amp' Amp circuit except A3 instead of A5.   Vc common to both amps (single failure point)
  For Photon Alpha, direct feed to ADS-1013 and no OPA333

### EERAM for Argon (47L16)

  V+   = 5v rail
  Vcap = Cap 106 10uF to Gnd rail.  Storage that powers EEPROM save on power loss
  V-   = Gnd rail
  SDA  = D0-SDA of Device
  SCL  = D1-SCL of Device

### 1-wire Temp (MAXIM DS18B20)  library at "https://github.com/particle-iot/OneWireLibrary"

  Y-C   = Device D6
  R-VDD = 3V3 Rail
  B-GND = GND Rail

### Display SSD1306-compatible OLED 128x32

  Amazon:  5 Pieces I2C Display Module 0.91 Inch I2C OLED Display Module Blue I2C OLED Screen Driver DC 3.3V~5V(Blue Display Color)
  Code from Adafruit SSD1306 library
  1-GND = Gnd
  2-VCC = 3v3
  3-SCL = Device D1-SCL
  4-SDA = Device D0-SDA

### Shunt 75mv = 100A

  Use custom harness that contains Shunt as junction box to obtain 12v, Gnd, Vshunts
  R-12v
  B-Gnd
  Y-Yellow shunt high
  G-Green shunt low

### HC-06 Bluetooth Module

  Attach directly to 5V and TX/RX
  VC  = 5v rail
  GND = Gnd rail
  TX  = RX of Device
  RX  = TX of Device

### ASD 1013 12-bit PHOTON ALPHA ONLY *****Amplified with OPA333 my custom board.   Avoids using negative absolute voltages on inputs - centers on 3v3 / 2

- HiLetgo ADS1015 12 Bit Analog to Digital Development Board ADC Converter Module ADC Development Board for Arduino
  $8.29 in Aug 2021
  I2C used.
  Code from Adafruit ADS1X15 library.   Differential = A0-A1
  Ti OPA333 Used.   $11.00 for 5 Amazon OPA333AIDBVR SOT23-5 mounted on SOT23-6
  No special code for OPA.  Hardware only.   Pre-amp for ADC 5:1.
  1-V 3v3:  0.1uF to ground for transient power draws of the ADC
  2-G = Gnd
  3-SCL = Photon D1
  4-SDA  = Photon D0
  5-ADDR = 3v3
  6-ALERT = NC
  7-A0 = Green from shunt
  8-A1 = Yellow from shunt
  9-A2 = NC
  10-A3 = NC

### Particle Photon Device 1A max PHOTON ALPHA

- Particle Photon boards have 9 PWM pins: D0, D1, D2, D3, A4, A5, WKP, RX, TX
  GND = to 2 GND rails
  A1  = L of 20k ohm from 12v and H of 4k7 ohm + 47uF to ground
  A3  = H of 'no amp' amp circuit
  A3  = 8k2/1uF filter of of Vc of both amp circuits (yes, single point of failure for both amps)
  A5  = H of 'amp' amp circuit
  D0  = SCA of ASD, SCA of OLED, and 4k7 3v3 jumper I2C pullup
  D1  = SCL of ASD, SCA of OLED, and 4k7 3v3 jumper I2C pullup
  D6  = Y-C of DS18 for Tbatt and 4k7 3v3 jumper pullup
  VIN = 5V Rail 1A maximum from 7805CT home-made 12-->5 V regulator
  3v3 = 3v3 rail out supply to common of amp circuits, supply of OPA333, and R-OLED
  micro USB = Serial Monitor on PC (either Particle Workbench monitor or CoolTerm).   There is sufficient power in Particle devices to power the peripherals of this project on USB only
  5v  = supply to VC-HC-06, R-1-wire
  TX  = RX of HC-06
  RX  = TX of HC-06
  SDA = SDA-ADS-1013 amp, SDA-ADS-1013 no amp, Y-OLED
  SCL = SCL-ADS-1013 amp, SCL-ADS-1013 no amp, B-OLED

## FAQ

### Problem:  CLI starts acting funny:  cannot login, gives strange errors ("cannot find module semver")

    Solution:  Manually reset CLI installation.  https://community.particle.io/t/how-to-manually-reset-your-cli-installation/54018

### Problem:  Software loads but does nothing or doesn't work sensibly at all

  This can happen with a new Particle device first time.   This can happen with a feature added to code that requires certain
  OS configuration.   Its easy to be fooled by this because it appears that the application loads correctly and some stuff even works.

    Solution:  run  Particle: Flash Application and Device OS (local).   You may have to compile same before running this.  Once this is done the redo loop is Flash Application (local) only.

### Problem:  Local flash gives red message "Unable to connect to the device ((device name requested)). Make sure the device is connected to the host computer via USB"

  Find out what device you're flashing (particle.io).  In file .vscode/settings.json change "particle.targetDevice": "((device name actual))".  It may still flash correctly even with red warning message (always did for me).

### Problem:  Converted current wanders (sometimes after 10 minutes).   Studying using prototype without 150k/1uF LPF.   Multimeter used to  verify constant hardware volts input.  Also create solid mV input using 10k POT + 1M ohm resistor from 5v

  Investigation found two problems:
    - Solder joint at current sense connector failing
    - Ground of laptop  monitoring the Photon wildly floating, biasing ADS inputs probably tripping Schottky diodes.
  DO NOT PLUG IN LAPTOP TO ANYTHING WHILE MONITORING Photon

### Problem:  Messages from devices:  "HTTP error 401 from ... - The access token provided is invalid."

  This means you haven't signed into the particle cloud during this session.   May proceed anyway.   If want message to go away, go to "Welcome to Particle Workbench" and click on "LOGIN."   Enter username and password for particle.io.  Usually hit enter for the 6-digit code - unless you set one up.

### Problem:  Hiccups in Arduino plots

  These are caused by time slips.   You notice that the Arduino plot x-axis is not time rather sample number.   So if a sample is missed due to time slip in Photon then it appears to be a time slip on the plot.   Usually this is caused by running Wifi.   Turn off Wifi.

### Problem:  Experimentation with 'modeling' and running near saturation results in numerical lockup

  The saturation logic is a feedback loop that can overflow and/or go unstable.   To break the loop, set cutback_gain to 0 in retained.h for a re-compile.   When comfortable with settings, reset it to 1.

### Problem:  The EKF crashes to zero after some changes to operating conditions

  You may temporarily fix this by running software reset talk ('Rs').   Permanently - work on the EKF to make it more robust.  One long term fix found to be running it with slower update time.

### Problem:  The application overflows APP_FLASH on compilation

  Too much text being stored by Serial.printf statements.   May temporarily co-exist with the limit by reducinig NSUM summary memory size.

### Problem:  'Insufficient room for heap.' or 'Insufficient room for .data and .bss sections!' on compilation, or flashing red lights after flash

  You probaby added some code and overflowed PROM.  Smaller NSUM in constants.h or rp in retained.h or cp in command.h

### Problem:  The application overflows BACKUPSRAM on compilation

  Smaller NFLT/NSLT in constants.h or sp in retained.h

### Problem:  . ? h

  CoolTerm - Options - Terminal:  Enter Key Emulation:  CR

### Problem:  cTime very long.  If look at year, it is 1999.

  The Photon ALPHA VBAT battery died or was disconnected.  Restore the battery.  Connect the photon to the network.   Use Particle app on phone to connect it to wifi at least once.

  The Argon device hasn't synchronized since last power up.   Connect to wifi.  It is most convenient to setup the Argon devices' default wifi to be your phone running hotspot.   Then just do Particle setup from the phone - start and exit.   TODO:  may be possible to save time information to the Argon EERAM.

### Problem:  Tbh = Tbm in display 'Pf' (print faults)

This is normal for temperature.   Modeled Tb is very simple = to a constant + bias.   So I chose to display what is actually used in the model rather than what is actually used in signal selection.

## Author: Dave Gutz davegutz@alum.mit.edu  repository GITHUB myStateOfCharge

## To get debug data

  1. Set debug = 2 in main.h
  2. Rebuild and upload
  3. Start CoolTerm_0.stc

See 'State of Charge Monitor.odt' for full set of requirements, testing, discussion and recommendations for going forward.

## Changelog

20221220 Beta 
  - Functional
    a. hys_cap Hysteresis Capacitance decreased by factor of 10 to match cold charging (coldCharge v20221028 20221210.txt) (7 deg C).
    b. The voc-soc curve shifts low about 0.07 (7% soc) on cold days, causing cc_diff and e_wrap_lo faults.   As a result, an additional scalar was put on saturation scalar for those fault logics.

  - Argon Related.   Cannot get Photon anymore.   Also will not have Argon available but Photon 2 will be someday.   Argon very much like Photon 2 with no EERAM 47L16 built into the A/S (retained).   Retained on Argon lasts only for reset.   For power, bought EERAM 47L16.
    a. parameters.cpp/.h to manage EERAM 47L16 dump.   rp --> sp
    b. BLE logic.  Nominally disabled (#undef USE_BLE) because there is no good UART terminal for BLE.   Using HC-06 same as Photon

  - Simulation neatness
    a. Everything in python verification model is lower case for battery unit (12 VDC bank).
    b. nP and  nS done at hardware interfaces (mySensors.cpp/h and myCloud.cpp/h::assign_publist)
    c. load function defined
    d. add_stuff consolidated in one place
    e. Complete set of overplotting functions:   run/run and run/sim

