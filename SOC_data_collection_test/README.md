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

## Redo Loop

***********************
Ctrl-Shift-P - Particle:Clean Application and Device OS (local)
Ctrl-Shift-P - Particle:Compile Application (local) or Check button in Visual Studio upper rc when select a source file
Ctrl-Shift-P - Particle:Cloud Flash or Ctrl-Shift-P - Particle:Local Flash
Ctrl-Shift-P - Particle:Serial Monitor

Desktop settings
    .json has "particle.targetDevice": "proto"
    local_config.h has
        const   String    unit = "proto";
        //#define BARE

Laptop settings.json has  "particle.targetDevice": "proto"
    .json has "particle.targetDevice": "proto"
    local_config.h has
        const   String    unit = "proto";
        //#define BARE

On desktop (has curlParticleProto.py running in cygwin)
    after observing outputs from long cURL run, Modify main.h etc.   Make sure it compiles.
    flash to target 'proto' and test
    push into GitHub repository
    get address of curl from particle.io console - pick device - click on 'view events in terminal' --> edit curl??.py file
    cd /cygdrive/c/Users/Dave/Documents/GitHub/myStateOfCharge/SOC_Photon/dataReduction/
    python curlParticleProto.py
    (add to .bashrc the above two lines as [emacs .bashrc]:
    alias soc='cd ...; python cur...'
    )

On laptop (same as desktop?)
    pull from GitHub repository changes just made on desktop
    compile
    flash to target 'proto'

View results
    Prototype realtime using Blynk (SoC)
    Target post-process using SoC_data_reduce.sce in sciLab

## Hardware notes

Grounds all tied together to solar ground and also to chassis.
Typically operate for data with laptop plugged into inverter and connected to microusb on Photon
Can run 500 W discharge using flood light plugged into inverter
Can run 500 W charge from alternator DC-DC converter (breaker under hood; start engine)

### Calibrating mV - A

You need a clamping ammeter.  Basic.   Best way to get the slope of the
conversion.   A 100A/.075V shunt is nominally 1333 A/V.

Do a discharge-charge cycle to get a good practical value for the bias of the conversion.    Calculate integral of A over cycle and get endpoint to match start point.   This will also provide a good estimate for battery capacity to populate the model.  (R1 visible easily).

PI tracking filter should be used to get rid of drift of small A measurement.

>13.7 V is decent approximation for SoC>99.7

The ADS module is delicate (ESD and handling).   I burned one out by
accidentally touching terminals to back of OLED board.   I now mount the
OLED board carefully off to the side.   Will need a hobby box to contain the final device.

### ASD 1015 12-bit

- HiLetgo ADS1015 12 Bit Analog to Digital Development Board ADC Converter Module ADC Development Board for Arduino
  $8.29 in Aug 2021
  I2C used.
  Code from Adafruit ADS1X15 library.   Differential = A0-A1
  1-V 3v3
  2-G = Gnd
  3-SCL = Photon D1
  4-SDA  = Photon D0
  5-ADDR = NC
  6-ALERT = NC
  7-A0 = Green from shunt
  8-A1 = Yellow from shunt
  9-A2 = NC
  10-A3 = NC

### ASD 1015 12-bit Amplified with OPA333 my custom board.   Avoids using negative absolute voltages on inputs - centers on 3v3 / 2

- HiLetgo ADS1015 12 Bit Analog to Digital Development Board ADC Converter Module ADC Development Board for Arduino
  $8.29 in Aug 2021
  I2C used.
  Code from Adafruit ADS1X15 library.   Differential = A0-A1
  Ti OPA333 Used.   $11.00 for 5 Amazon OPA333AIDBVR SOT23-5 mounted on SOT23-6
  No special code for OPA.  Hardware only.   Pre-amp for ADC 5:1.
  1-V 3v3
  2-G = Gnd
  3-SCL = Photon D1
  4-SDA  = Photon D0
  5-ADDR = 3v3
  6-ALERT = NC
  7-A0 = Green from shunt
  8-A1 = Yellow from shunt
  9-A2 = NC
  10-A3 = NC

### Particle Photon 1A max

- Particle Photon boards have 9 PWM pins: D0, D1, D2, D3, A4, A5, WKP, RX, TX
  GND = to 2 GND rails
  A1  = L of 20k ohm from 12v and H of 4k7 ohm to ground
  D0  = SCA of ASD, SCA of OLED, and 4k7 3v3 jumper I2C pullup
  D1  = SCL of ASD, SCA of OLED, and 4k7 3v3 jumper I2C pullup
  D6  = Y-C of DS18 for Tbatt and 4k7 3v3 jumper pullup
  VIN = 5V Rail 1A maximum
  3v3 = 3v3 rail out
  micro USB = Serial Monitor on PC (either Particle Workbench monitor or CoolTerm)

### 1-wire Temp (MAXIM DS18B20)  library at "https://github.com/particle-iot/OneWireLibrary"

  Y-C   = Photon D6
  R-VDD = 5V Rail
  B-GND = GND Rail

### Used Elego power module mounted to 5V and 3v3 and GND rails

  5V jumper = 5V RAIL on "A-side" of Photon
  Jumper "D-side" of Photon set to OFF
  Round power supply = round power supply plug 12 VDC x 1.0A Csec CS12b20100FUF
  Use Photon to power 3v3 so can still use microusb to power everything from laptop

### Display SSD1306-compatible OLED 128x32

  Amazon:  5 Pieces I2C Display Module 0.91 Inch I2C OLED Display Module Blue I2C OLED Screen Driver DC 3.3V~5V(Blue Display Color) 
  Code from Adafruit SSD1306 library
  1-GND = Gnd
  2-VCC = 3v3
  3-SCL = Photon D1
  4-SDA = Photon D0

### Shunt 75mv = 100A

  Use custom box that contains Shunt as junction box to obtain 12v, Gnd, Vshunts
  1-12v
  2-Gnd
  3-Yellow shunt high
  4-Green shunt low

## FAQ

### Problem:  CLI starts acting funny:  cannot login, gives strange errors ("cannot find module semver")

    Solution:  Manually reset CLI installation.  https://community.particle.io/t/how-to-manually-reset-your-cli-installation/54018

### Problem:  Software loads but does nothing

  This can happen with a new Particle device first time.   This can happen with a feature added to code that requires certain
  OS configuration.

    Solution:  run  Particle: Flash Application and Device OS (local)

## Author: Dave Gutz davegutz@alum.mit.edu  repository GITHUB myStateOfCharge

## To get debug data

  1. Set debug = 2 in main.h
  2. Rebuild and upload
  3. Start CoolTerm_0.stc

## Requirements:
  1.  
