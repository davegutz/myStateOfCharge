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
        //#define BARE

Laptop settings.json has  "particle.targetDevice": "soc0"
    .json has "particle.targetDevice": "soc0"
    local_config.h has
        const   String    unit = "soc0";
        //#define BARE

On desktop (has curlParticleProto.py running in cygwin)
    after observing outputs from long cURL run, Modify main.h etc.   Make sure it compiles.
    flash to target 'proto' and test
    be sure to build OS and flash OS too the first time
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
    flash to target 'soc0'
    be sure to build OS and flash OS too the first time

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

>13.7 V is decent approximation for SoC>99.7, correct for temperature.

Temperature correction in ambient range is about BATT_DVOC_DT=0.001875 V/deg C from 25 * number of cells = 4.  This is estimated from the Battleborn characterization model.

The ADS module is delicate (ESD and handling).   I burned one out by
accidentally touching terminals to back of OLED board.   I now mount the
OLED board carefully off to the side.   Will need a hobby box to contain the final device.

### Passive shunt low pass filter (LPF)

1 - Green = 150K resistor from shunt high side
1 - Green = 1uF cap to 2 - yellow
2 - Yellow = from shunt low side

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
  7-A0 = Green from shunt lpf via 10k Resistor
  8-A1 = Yellow from shunt lpf via 10k Resistor
  9-A2 = NC
  10-A3 = NC

### ASD 1015 12-bit Amplified with OPA333 my custom board.   Avoids using negative absolute voltages on inputs - centers on 3v3 / 2

- HiLetgo ADS1015 12 Bit Analog to Digital Development Board ADC Converter Module ADC Development Board for Arduino
  $8.29 in Aug 2021
  I2C used.
  Code from Adafruit ADS1X15 library.   Differential = A0-A1
  Ti OPA333 Used.   $11.00 for 5 Amazon OPA333AIDBVR SOT23-5 mounted on SOT23-6
  No special code for OPA.  Hardware only.   Pre-amp for ADC 5:1.
  1-V 3v3 (+ TODO:  0.1uF to ground for transient power draws of the ADC)
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
  A1  = L of 20k ohm from 12v and H of 4k7 ohm + 47uF to ground
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

  This means you haven't signed into the particle cloud during this session.   Go to "Welcome to Particle Workbench" and click on "LOGIN."   Enter username and password for particle.io.  Usually hit enter for the 6-digit code - unless you set one up.

### Problem:  Hiccups in Arduino plots

  These are caused by time slips.   You notice that the Arduino plot x-axis is not time rather sample number.   So if a sample is missed due to time slip in Photon then it appears to be a time slip on the plot.   Usually this is caused by running Wifi.   Turn off Wifi.

### Problem:  Experimentation with 'modeling' and running near saturation results in numerical lockup

  The saturation logic is a feedback loop that can overflow and/or go unstable.   To break the loop, set cutback_gain to 0 in retained.h for a re-compile.   When comfortable with settings, reset it to 1.

## Author: Dave Gutz davegutz@alum.mit.edu  repository GITHUB myStateOfCharge

## To get debug data

  1. Set debug = 2 in main.h
  2. Rebuild and upload
  3. Start CoolTerm_0.stc

## Requirements

  1. Calculate state of charge of 100 Ah Battleborn LiFePO4 battery.   The state of charge is percentage of 100 Ah available.
    a.  Nominally the battery may have more than 100 Ah capacity.   This should be confirmed.
    b.  Displayed SOC may exceed 100 but should not go below 0.
  2. Displayed values when sitting either charging slightly or discharging DC ciruits (no inverter) should appear constant after filter rise.
    a.  Estimate and display hours to 100 SOC. (charge is positive).   This is the main use of this device:  to assure sleeper that CPAP, once turned on, will last longer than expected sleep time.
    b.  Measure and display shunt current (charge is positive), A.
    c.  Measure and display battery voltage, V.
    d.  Measure and display battery temperature, F.
  3. Implement an adjustment 'talk' function.  Along with general debugging it must be capable of
    a.  Setting SOC state for those times that the
  4. CPU hard and soft resets must not change the state of operation, either SOC, display, or serial bus.
  5. Serial streams shall have an absolute Julien time for easy plotting and comparison.
  6. Built-in test vector function, engaged using 'talk' function.
  7. Load software using USB.  Wifi to truck will not be reliable.
  8. Likewise, monitor USB using laptop or phone.  'Talk' function should change serial monitor for debugging.
  9. Device shall have no effect on system operation.   Monitor function only.
  10. Bluetooth serial interface required.  ***This did not work due to age of Android phone (6 yrs) not compatible with the latest bluetooth devices.
  11. Keep as much summary as possible of SOC every half hour:  Tbatt, Vbatt, SOC.   Save as SRAM battery backup memory. Print out to serial automatically on boot and as requested by 'Talk.'  This will tell users charging history.
  12. Adjustments to model using talk function should preserve delta_q between models to preserve change from
  saturated situation.  The one 'constant' in this device is that it may be reset to reality whenever fully
  charged.   Test it the same way.  The will be separate adjustment to bias the model away from this ('n' and 'N')

## Implementation Notes

  1. An EKF is no more accurate than the open loop OCV-SOC curves.
  2. A Coulomb Counter implementation is very accurate but needs to calibrate every couple cycles.   This should happen naturally as the battery charges fully each day.
  3. Blynk phone monitor implemented, as well as Particle Cloud, but found to be impractical because
    a.  Seldom near wifi when camping.
    b.  OLED display works well.
    c.  'Talk' interface works well.   Can use with phone while on the go.  Need to buy dongle for the phone (Android only): Micro USB 2.0 OTG Cable ($6 on Amazon). 
  4. Shunt monitor seems to have 0 V bias at 0 A current so this is assumed when scaling inputs.
  5. Current calibrated using clamping ammeter turning large loads off and on.
  6. Battleborn nominal capacity determined by load tests.
  7. An iterative solver for SOC-VOC model works extremely well given calculatable derivative for the equations from reference.  PI observer removed in favor of solver.  The solver could be used to initialize SOC for the Coulomb Counter but probaly not worth the trouble.   Leave this device in more future possible usage.  An EKF
  then replaced the solver because it essentially does the same thing and was much quieter.
  8. Note that there is no effect of the device on system operation so debugging via serial can be more extensive than usual.
  9. Two-pole filters and update time framing, determined experimentally and by experience to provide best possible Coulommb Counter and display behavior.
  10. Wifi turned off immediately.  Can be turned on by 'Talk('w')'
  11. Battery temperature changes very slowly, well within capabilities of DS18B sensor.
  12. 12-bit AD sufficiently accurate for Coulomb Counting.  Precision is what matters and it is fine, even with bit flipping.
  13. All constants in header files (Battery.h and constants.h and retained.h and command.h).
  14. System can become confused with retained parameters getting off-nominal values and no way to
  reset except by forcing a full compile reload.   So the 'Talk('A')' feature was added to re-nominalize
  the rp structure.   You have to reset to force them to take effect.
