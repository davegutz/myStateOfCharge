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

Tweak logic should be used to get rid of drift of small A measurement.

>13.7 V is decent approximation for SoC>99.7, correct for temperature.

Temperature correction in ambient range is about BATT_DVOC_DT=0.001875 V/deg C from 25 * number of cells = 4.  This is estimated from the Battleborn characterization model.

The ADS module is delicate (ESD and handling).   I burned one out by
accidentally touching terminals to back of OLED board.   I now mount the
OLED board carefully off to the side.   Will need a hobby box to contain the final device.

### Voltage regulator

I salvaged a prototype 12-->5 VDC regulator from OBDII project.   It is based on 7805CT device with capacitors.

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
  1-V 3v3 (+ [TODO]:  0.1uF to ground for transient power draws of the ADC)
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
  VIN = 5V Rail 1A maximum from 7805CT home-made 12-->5 V regulator
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

### Problem:  The EKF crashes to zero after some changes to operating conditions

  You may temporarily fix this by running software reset talk ('Rs').   Permanently - work on the EKF to make it more robust.

### Problem:  The application overflows APP_FLASH on compilation

  Too much text being stored by Serial.printf statements.

### Problem:  The application overflows BACKUPSRAM on compilation

  Smaller NSUM in constants.h or rp in retained.h or cp in command.h

### Problem:  . ? h

  CoolTerm - Options - Terminal:  Enter Key Emulation:  CR

### Problem:  cTime very long.  If look at year, it is 1999.

  The VBAT battery died or was disconnected.  Restore the battery.  Connect the photon to the network.   Use Particle app on phone to connect it to wifi at least once.

### Problem:  Tbh = Tbm in display 'Pf' (print faults)

This is normal for temperature.   Modeled Tb is very simple = to a constant + bias.   So I chose to display what is actually used in the model rather than what is actually used in signal selection.

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
    a.  Estimate and display hours to 1 soc. (charge is positive).   This is the main use of this device:  to assure sleeper that CPAP, once turned on, will last longer than expected sleep time.
    b.  Measure and display shunt current (charge is positive), A.
    c.  Measure and display battery voltage, V.
    d.  Measure and display battery temperature, F.
  3. Implement an adjustment 'talk' function.  Along with general debugging it must be capable of
    a.  Setting soc state
    b.  Separately setting the model state
    c.  Resetting pretty much anything
    d.  Injecting test signals
  4. CPU hard and soft resets must not change the state of operation, either soc, display, or serial bus.
  5. Serial streams shall have an absolute Julien-type time for easy plotting and comparison.
  6. Built-in test function, engaged using 'talk' function.
  7. Load software using USB.  Wifi to truck will not be reliable.
  8. Likewise, monitor USB using laptop or phone.  'Talk' function should change serial monitor and inject signals for debugging.
  9. Device shall have no effect on system operation.   Monitor function only.
  10. Bluetooth serial interface required.  ***This did not work due to age of Android phone (6 yrs) not compatible with the latest bluetooth devices.
  11. Keep as much summary as possible of soc every half hour:  Tbatt, Vbatt, soc.   Save as SRAM battery backup memory. Print out to serial automatically on boot and as requested by 'Talk.'  This will tell users charging history.
  12. Adjustments to model using talk function should preserve delta_q between models to preserve change from
  saturated situation.  The one 'constant' in this device is that it may be reset to reality whenever fully
  charged.   Test it the same way.  The will be separate adjustment to bias the model away from this ('n' and 'W')
  13. The 'Talk' function should let the user test a lot of stuff.   The biggest short coming is that it is a little quirky.  For example, resetting the Photon or doing any number of things sometimes requires the filters to be initialized differently, e.g. initialize to input rather than tending toward initializing to 0.   The initialization was optimized for installed use.  So the user needs to get used to rationalizing the initialization behavior they see when testing.  They can wait for the system to settle, sometimes 5 minutes.   They can run Talk('Rs') to attempt a software filter reset - won't help if the test draws a steady current after reset/reboot.
  14. Inject hardware current signal for testing purposes.   This may be implemented by reconnecting the shunt input wires to a PWM signal from the Photon.
  15. The PWM signal for injecting test signals should run at 60 Hz to better mimic the 60 Hz behavior of the inverter when installed and test the hardware AAF filters, (RC=2*pi, 1 Hz -3dB bandwidth).
  16. There shall be a built-in model to test logic and perform regression tests.  The EKF in MyBatt needs a realistic response on current to voltage to work properly (H and hx assume a system).  The built-in model must properly represent behavior onto and off of limits of saturation and 0.
  17. The Battery Monitor will not properly predict saturation and low shutoff.   It doesn't need to.   The prime requirement is to count Coulombs and reset the counter when saturated.   The detection of saturation may be crude but always work - this will result in saturation being declared at too low voltage which is conservative, in that the logic will display a state of charge that is lower than actual.
  18. The Battery Model used for testing shall implement a current cutback as saturation is neared, to rougly model the BMS in the actual system.
  19. The Battery Monitor shall implement a predicted loss of capacity with temperature.   The Battery Model does not need to implement this but should anyway.
  20. The Battery Model shall be scaleable using Talk for capacity.   The Battery Monitor will have a nominal size and it is ok to recompile for different sizes.
  21. The Battery Model Coulomb Counting algorithm shall be implemented with separate logic to avoid the problem of logic changes confounding regression test.
  22. The Talk function shall have a method Talk('RR') that resets all logic to installed configuration.
  23. The default values of sensor signal biases should remain as default values in logic.
  24. Power loss and resets (both hard and soft) must not affect long term Coulomb counting.   This may result in adding a button-cell battery to the VBAT terminal of Photon and using 'retained' SRAM data for critical states in the logic.   It would be nice if loss and resets did not affect Battery Model and testing too.
  25. 'soc' shall be current charge 'q' divided by current capacity 'q_capacity' that reflects changes with temperature.
  26. Coulomb counting shall simultaneously track temperature changes to keep aligned with capacity estimates.
  27. 'SOC' shall be current charge 'q' at the instant temperature divided by rated capacity at rated temperature.
  28. The monitor logic must detect and be benign that the DC-DC charger has come on setting Vb while the BMS in the battery has shutoff current.   This is to prevent falsely declaring saturation from DC-DC charger on.
  29. Only Battleborn will be implemented at first but structure will support other suppliers.  For now, recompilation is needed to run another supplier and #define switches between them.
  30. The nominal unit of configuration shall be a 12 v battery with a characteristic soc-voc and rated Ah capacity.  Use with multiple batteries shall include running an arbitrary number of batteries in parallel and then series (nPmS).  The configuration shall be in local_config.h and in retained.h.   This is called the 'chemistry' of the configuration.
  31. The configuration shall be fully adjustable on the fly using Talk.  If somebody has Battleborn or a LION they will not need to ever recompile and reflash a Photon.
  32. Maximize system availability in presence of loss of sensor signals.  Soft or hard resets cause signal fault detection and selection to reset.   Flash display to communicate signal status:  every fourth update of screen indicates a minor fault.  Every other update is major fault where action needed.   Print signal faults in the 'Q' talk.   Add ability to mask faults to a retained parameter rp.

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
  7. An iterative solver for SOC-VOC model works extremely well given calculatable derivative for the equations from reference.  PI observer removed in favor of solver.  The solver could be used to initialize soc for the Coulomb Counter but probaly not worth the trouble.   Leave this device in more future possible usage.  An EKF
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
  15. The minor frame time (READ_DELAY) could be run as fast as 5.   The application runs in 0.005 seconds.  The anti-alias filters in hardware need to run at 1 Hz -3dB bandwidth (tau = 0.159 s) to filter out the PWM-like activity of the inverter that is trying to regulate 60 Hz power.   With that kind of hardware filtering, there is no value added to running the logic any faster than 10 Hz (READ_DELAY = 100).   There's lots of throughput margin available for adding more EKF logic, etc.
  16. The hardware AAF filters effectively smooth out the PWM test input to look analog.   This is desired behavior:  the system must filter 60 Hz inverter activity.   The testing is done with PWM signal to give us an opportunity to test the hardware A/D behavior.  The user must remember to move the internal jumper wire from 3-5 (testing) to 4-5 (installed).
  17. Regression test:   For installed with real signal, could disconnect solar panels and inject using Talk('Di<>') or Talk('Xp5') and Talk('Xp6').  Uninstalled, should run through them all:  Talk('Xp<>,) 0-6.  Uninstalled should runs onto and off of limits.
  18. In modeling mode the Battery Model passes all sensed signals on to the Battery Monitor.   The Model does things like cutting back current near saturation, and injecting test signals both hard and soft.  The Signal Sense logic needs to perform some injection especially soft so Model not needed for some regression.
  19. All logic uses a counting scheme that debits Coulombs since last known saturation.   The prime requirement of using saturation to periodically reset logic is reflected in use of change since saturation.
  20. The easiest way to confirm that the EKF is working correctly is to set 'modeling' using Talk('Xm7') and verify that soc_ekf equals soc_mod.
  21. Lessons-learned from installation in truck.  Worked fine driving both A/D with D2 from main board.    But when installed in truck all hell broke loose.   The root cause was grounding the Vlow fuse side of shunt legs of the A/D converters.   In theory the fuse side is the same as ground of power supply.   But the wires are gage 20-22 and I detected a 75 mA current in the Vh and Vl legs which is enough to put about 50% error on detection.   Very sensitive.   But if float both legs and avoid ground looping it works fine.   And as long as ground loops avoided, there is no need to beef up the sense wire gage because there will be no current to speak of.  I revised the schematics.   There are now two pin-outs:  one for installed in truck and another when driving with the D2 pin and PWM into the RC circuits.
  22. Regression testing:  
    a. Saturation test.  Run Talk('Xp7').   This will initialize monitor at 0.5 and model near saturation then drive toward saturation.   Watch voc vs v_sat and sat in the v1 debug display that gets started.  Reset with 'Xp-1'.  If starting up Xp7 it initializes saturated just enter 'm< val >' with lower 'val' than what 'Xp7' started it with until it initializes without saturation.  Should saturate soon and reset soc of monitor to 1.0.  Then setting 'Di-1000' you should see it reset after a very little time. Again, reset the whole mess with 'Xp-1'.
    b.  
  23. 'Talk' refers to using CoolTerm to transmit commands through the myTalk.cpp functions. Talk is not threaded so can only send off a barage of commands open loop and hope for the best.
  24. I had to add persistence to the 'log on boot' function.   When in a cold shutoff, the BMS of the battery periodically 'looks' at the state of the battery by turning it on for a few seconds. The summary filled up quickly with these useless logs.  I used a 60 second persistence.
  25. Use Tb<8 deg C to turn off monitoring for saturation.   This prevents false saturation trips under normal conditions.
  26. Had a failed attempt to heat the battery before settling on what Battleborn does here:  <https://battlebornbatteries.com/faq-how-to-use-a-heat-pad-with-battle-born-batteries/>.  I thought the pads I bought here:  <https://smile.amazon.com/gp/product/B0794V5J5H/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1> would gently heat the batter.  I had the right wattage (36 W vs 30 W that Battleborn uses) but I put them all on the bottom.  And the temp sensor is on top.   And the controller is on/off.   That set up a situation where the heat would be on full blast and the entire battery had to heat up before the wave of heat would hit the sensor to shut it off.   The wave continued to create about 5 degree C overshoot of temp then 5 degree C undershoot on recovery.   I measured up to 180 F at the bottom of the battery using an oven thermometer.  There was some localized melting of the case.  A model accurately predicted it.   See GitHub\myStateOfCharge\SOC_Photon\Battery State\EKF\sandbox\GP_battery_warm_2022a.py.  I made a jacket, covered whole thing with R1 camping pad (because BB suggests putting temp sensors inside 'blanket') and put the temp sensor for the SOC monitor and the heater together at one corner about 2 inches down from the top.   I set the on/off to run between 40 and 50F (4.4 - 7.2C) compared to BB 35 - 45 F.  I wanted tighter control for better data.   Maybe after I get more data I'll loosen it back up to 35 - 45 F.
  27. Calibrated the t_soc voc tables at a 1-4 A discharge rate.    Then they charged 5 - 30 A.   The hysteresis would have been used some at the 1-4 A rate.   May have to adjust tables +0.01 V or so after adding the hysteresis.  Measure it:  0.007 V.
  28. An issue that only show up when using the 'talk' function is an overload of resources that causes a busy waiting of current input reading.  I added to readADC_Differential_0_1 count_max logic to prevent forever waiting.   There is an optimum because if wait too long it creates cascade wait race in rest of application.  50: inf events.  100:0-4 events.  200:  5 events.   800:  25 events.  You can reproduce the problem by sending "Hd" wile running the tweak test of previous item.  Apparently exercising the Serial port while reading can cause the read to crash. The actual read statement does not forever wait, but the writer of readADC_Differential_0_1 put a 'while forever' loop around it.   I modified the Adafruit code to time out the loop.
  29. Running the tweak test will cause voc to wander low and after about 5 cycles will no longer saturate.   The hysteresis model causes this and is an artifact of running a huge, fast cosine input to the monitor.  Will not happen in real world.
  30. A clean way to initialize the EKF is an iterative solver called whenever initialization is needed.  The times this is needed are hard bootup and adjustment of SOC state using Talk function.   The latter is initiated using cp.soft_reset.  The convergence test persistence is initialized false as desired by TFDelay(false,...) instantiation.   Want it to begin false because of the potentially severe consequences of using the EKF to re-initialize the Coulomb Counter.
  31. The Coulomb Counter is reset to the EKF after 30 seconds with more than 5% error from EKF.   Initialization takes care of this automatically.  The EKF must be converged within threshold for time(EKF_CONV & EKF_T_CONV).
  32. To hedge on errors, the displayed amp hours remaining is a weighted average of the EKF and the Coulomb Counter.  Weighted to EKF as error increases from DF1 to DF2 error.   Set to EKF when error > DF2.  Set to average when error < DF1.
  33. Some Randles dynamics approximation was added to the models, both simulated and EKF embedded to better match reality.   I did this in response to poor behavior of the electrical circuits in presence of 60 Hz pulse noise introduced by my system's pure-sine A/C inverter.   Eventually I added 1 Hz time constant RC circuits to the A/D analog inputs to smooth things out.   I'm not sure the electrical circuit models add any value now.   This is because the objective of this device is to measure long term energy drain, time averaged by integration over periods of hours.   So much fitering inherent in integration would swamp most of the dynamics captured by the Randles models.   It averages out.   Detailed study is needed to justify either leaving it in or removing it.  An easy study would be to run the simulated version, turning off the Randles model in the Monitor object only.  Run the accelerated age test - 'Tweak test' - and observe changes in the Monitor's tweak behavior upon turning off its Randles model.   Then and only then it may be sensible to embark on improving the Randles models.
  34. Given the existing Randles models, note that current is constrained to be the same through series arranagements of battery units.   The constraint comes from external loads that have much higher impedance than battery cells.   Those cells are nearly pure capacitance.   With identical current and mostly linear dynamics, series batteries would have the same current and divide the voltage perfectly.   Parallel batteries would have the same voltage and divide the current perfectly.   Even the non-linear hysteresis is driven by that same identical current forcing a perfect division of that voltage drop too.   So multiple battery banks may be managed by scalars nP and nS on the output of single battery models.

  ........................................................................................
  35. Regression tests:

Rapid tweak test 1 min using models Xm15 'tweakMod' to test tweak onlya_ (no data collection, v0)
  Xp9;
    to end prematurely
  XS; Dn0.9985; Ca1; Mk0; Nk0;
    expected result (may have small values of abs(Di)<0.01 ):
       Tweak(Amp)::adjust:, past=       1.1, pres=       1.1, error=      -0.0, gain= -0.039993, delta_hrs=  0.013891, Di=  0.000, new_Di=  0.000,
       Tweak(No Amp)::adjust:, past=       1.1, pres=       1.1, error=      -0.0, gain= -0.039993, delta_hrs=  0.013891, Di=  0.000, new_Di=  0.000,

Rapid tweak test 02:30 min using models 'tweakMod'
    start recording, save to ../dataReduction/< name >.txt
  Xp10;
    to end prematurely
  XS; Dn0.9985; Ca1; Mk0; Nk0;
    run py script DataOverModel.py, using suggestions at bottom of file to recognize the new data
  Slow cycle test 10:00 min using models 'cycleMod'
    start recording, save to ../dataReduction/< name >.txt
  Xp11;
    to end prematurely
  XS; Dn0.9985; Ca1; Mk0; Nk0;
    run py script DataOverModel.py, using suggestions at bottom of file to recognize the new data

Throughput test
  v1;Dr1;
    look at T print, estimate X-2s value (0.049s, ~50% margin)
  Dr100;
    confirm T restored to 0.100s
    .
37. Android running of data collection.   The very best I could accompish is to run BLESerial App and save to file. You can use BLE transmit to change anything, echoed still on USB so flying a little blind.   Best for changing 'v0' to 'v1' and vice-versa.  The script 'DataOverModel.py' WILL RUN ON ANDROID using PyDroid and a bunch of stupid setup work.  But live plots I couldn't get to work because USB support is buggy.  The author of best tools usbserial4 said using Python 2 was best - forget that!  Easiest to collect data using BLESerial and move to PC for analysis.  Could use laptop PC to collect either BLE or USB and that should work really well.   Plug laptop into truck cab inverter.
38. Capitalized parameters, violation of coding standards, are "Bank" values, e.g. for '2P3S' parallelSerial banks of batteries while lower case are per 12V battery unit.
39. Signal injection examples:
 Ca0.5;Xts;Xa100;Xf0.1;XW5;XC5;v1;XR;
40.  Expected anomolies:
  a.  real world collection sometimes run sample times longer than RANDLES_T_MAX.   When that happens the modeled simulation of Randles system will oscillate so it is bypassed.   Real data will appear to have first order response and simulation in python will appear to be step.
41.  Manual tests to check initialzation of real world.   Set 'Xm=4;' to over-ride current sensor.  Set 'Dc<>' to place Vb where you want it.   Press hard reset button to force reinitialization to the EKF.  If get stuck saturated or not, remember to add a little bit of current 'Di<>' positive to engage saturation and negative to disengage saturation.
42.  Bluetooth (BLE).  In the logic it is visible as Serial1.   All the Serial1 api are non-blocking, so if BLE not connected or has failed then nothing seems to happen.  Another likely reason is baud rate mismatch between the HC-06 device (I couldn't get HC-05 to work) and Serial.begin(baud).   There is a project above this SOC_Photon project called BT-AT that runs the AT on HC-06.  You need to set baud rate using that.   You should use 115200 or higher.   Lower rates caused the Serial api to be the slowest routines in the chain of call.   At 9600 the fastest READ time 'Dr<>' was 100 - 200 ms depending on the print interval 'Dp<>'.
43.  Current is a critical signal for availability.   If lose current also lose knowledge of instantaneous voc_stat because do not know how to adjust for rapid changes in Vb without current.   So the EKF useful only for steady state use.  If add redundant current sensor then If current is available, it creates a triplex signal selection process where current sensors may be compared to each other and Coulomb counter may be compared to EKF to provide enough information to sort out the correct signals.  For example, if the currents disagree and CC and EKF agree then the standby current sensor has faulted.   For that same situation and the CC and EKF disagree then either the active current sensor has likely failed.  If the currents agree and CC and EKF disagree then the voltage sensor has likely failed.  The amplified current sensor is most accurate and is the first choice default.  The non amplified sensor is ok - observing long cycling 'Xp9' see that for a long history, counted coulombs are the same and sensor errors average out.   All this consistent with proper and same calibration of current sensors' gains and setting biases so indicated currents are zero when actual current is zero.  Need to cover small long duration current differnces as well as large fast current difference failures.   These tend to compete in any logic.  So two mechanisms are created to deal with them separately (ccd_fa and wrap_fa).   The Coulomb Counter Difference logic (ccd) detects slow small failures as the precise Coulomb Counters drift apart between the two current sensors.   The Wrap logic detects rapid failures as the sensed current is used to predict vb and then trips when compared to actual vb.  Maximum currents are about 1C for a single rated capacity unit, e.g. 1P1S.   The trip points are sized to detect stuff no faster than that and as slow as C0.16.   The logic works better than this design range.
44. Other fault notes:
  Every fail fault must change something on the display.  Goal is to make user run 'Pf' to see cause
  Wrap logic 0.2 v=16 A. There is an inflection in voc(soc) that requires forgiveness during saturation.  0.25 v = 20 A for wrap hi.   Sized so wrap_lo_fa trips before false saturating with delta I=-100 with soc=.95
  The tweak logic is small and limited and does not significantly interact with fault logic.
  If Tb is never read on boot up it should fail to NOMINAL_TB.
45. Tweak shall be disabled for small <20% charge swings
45. Fault injection testing

Full regression suite:
# All these must self initialize
# You have to start and stop data recording.   Use ctrl-R and shift-ctrl-R in CoolTerm
# Open two talk windows (ctrl-T in CoolTerm) and put the two lines in the talk windows
	# Initiate by running first line in first talk.  Those without second line self-terminate.  Still have to stop recording.
	# Terminate by watching for reset of failure condition (or wait long enough) and running second line in second talk.
# Move to pyCharm and run 'CompareRun' after proper comment/uncomment in 'data_file_old_txt' assignment

  ampHiFail:      Ff0;D^0;Xm7;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dm50;Dn0.0001;
                  Hs;Hs;Hs;Hs;Hd;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
		# Should detect and switch amp current failure.  Start looking at 'DOM 1' fig 3.  Fault record (frozen)

  ampHiFailFf:    Ff1;D^0;Xm7;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dm50;Dn0.0001;
				  Hs;Hs;Hs;Hs;Pf;Hd;Ff0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
		# Should detect but not switch amp current failure.  Start looking at 'DOM 1' fig 3.  No fault record (keeps recording)

  ampLoFail:      Ff0;D^0;Xm7;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dm-50;Dn0.0001;
                  Hs;Hs;Hs;Hs;Hd;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
		# Should detect and switch amp current failure.  Start looking at 'DOM 1' fig 3.  Fault record (frozen)

  ampHiFailNoise: Ff0;D^0;Xm7;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;DT.05;DV0.05;DM.2;DN2;W50;Dm50;Dn0.0001;Ff0;
                  Hs;Hs;Hs;Hs;Hd;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
		# Noisy ampHiFail

  rapidTweakRegression:  Ff0;HR;Xp10;
		# Should run three very large current discharge/recharge cycles without fault
		# Best test for seeing time skews

  rapidTweakRegression40C_:   Ff0;HR;D^15;Xp10;
                              D^0;

  slowTweakRegression:  Ff0;HR;Xp11;

  triTweakDisch:  Ff0;D^0;Xp0;v0;Bm0;Bs0;Xm15;Xtt;Ca1.;Ri;Mw0;Nw0;MC0.004;Mx0.04;NC0.004;Nx0.04;Mk1;Nk1;-Dm1;-Dn1;DP1;Rb;Pa;Xf0.02;Xa-29500;XW5;XT5;XC3;W2;HR;Pf;v2;W2;Fi1000;Fo1000;Fc1000;Fd1000;FV1;FI1;FT1;XR;
                  v0;Hd;XS;Dm0;Dn0;Fi1;Fo1;Fc1;Fd1;FV0;FI0;FT0;Xp0;Ca1.;Pf;DP4;

  satSit:  operate around saturation, starting above, go below, come back up.  Tune Ca to start just above vsat
         Ff0;D^0;Xp0;Xm15;Ca0.9962;Rb;Rf;Dr100;DP1;Xts;Xa-17;Xf0.002;XW10;XT10;XC1;W2;HR;Pf;v2;W5;XR;
          XS;v0;Hd;Xp0;Ca.9962;W5;Pf;Rf;Pf;v0;DP4;
	

  satSitHys:  operate around saturation, starting above, go below, come back up.  Tune Ca to start just above vsat.  Go low enough to exercise hys reset
         Ff0;D^0;Xp0;Xm15;Ca0.9962;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;
          XS;v0;Hd;Xp0;Ca.9962;W5;Pf;Rf;Pf;v0;DP4;

	offSitHysBms:  operate around saturation, starting above, go below, come back up.  Tune Ca to start just above vsat.  Go low enough to exercise hys reset		
		Ff0;D^0;Xp0;Xm7;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;
          XS;v0;Hd;Xp0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;
		# Best test for seeing Randles model differences
		# Only test to see on/off behavior

	offSitHysBmsNoise:  operate around saturation, starting above, go below, come back up.  Tune Ca to start just above vsat.  Go low enough to exercise hys reset		
		Ff0;D^0;Xp0;Xm7;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;DT.05;DV0.05;DM.2;DN2;Ph;HR;Pf;v2;W5;XR;
          XS;v0;Hd;Xp0;DT0;DV0;DM0;DN0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;

  ampHiFailSlow:  Ff0;D^0;Xm7;Ca0.5;Pf;v2;W2;Dr1000;DP1;HR;Dm6;Dn0.0001;Fc.02;Fd.5;
                  Hd;Xp0;Pf;Rf;W2;+v0;Dr100;Fc1;Fd1;Rf;Pf;

  vHiFail:        Ff0;D^0;Xm7;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.25;
                  Hd;Xp0;Rf;W200;+v0;Dr100;Rf;Pf;DP4;

  pulseEKF:  Xp6  # TODO: doesn't work now.

  pulseSS:  Xp7

  tbFailMod:    Ff0;D^0;Ca.5;Xp0;W4;Xm7;DP1;Dr100;W2;HR;Pf;v2;Xv.002;W50;Xu1;W200;Xu0;Xv1;W100;v0;Pf;
                Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;
  tbFailHdwe:   This script sometimes doesn't work but test performs fine manually
                Ff0;D^0;Ca.5;Xp0;W4;Xm6;DP1;Dr100;W2;HR;Pf;v2;Xv.002;W50;Xu1;W200;Xu0;Xv1;W100;v0;Pf;
                Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;
  EKF_Track:  investigate EKF tracking
         Ff0;D^0;Xp0;Xm15;Ca0.5;Rb;Rf;Xts;Xa2000;Xf0.02;XW6;XT6;XC1;Dr100;DP1;HR;Pf;v3;XR;
          XS;v0;Hd;Xp0;Ca.5;W5;Pf;Rf;Pf;v0;DP4;
  EKF_Track Dr2000:  investigate EKF tracking.   Confirm proper operation with Dr!=100
         Ff0;D^0;Xp0;Xm15;Ca0.5;Rb;Rf;Xts;Xa2000;Xf0.02;XW6;XT6;XC1;Dr2000;DP1;HR;Pf;v3;XR;
          XS;v0;Hd;Xp0;Ca.5;W5;Pf;Rf;Pf;v0;DP4;
  on_off_on:  using voltage
          Ff0;Xm5;v2;Ca.09;W10;SV.9;W10;SV.8;W10;SV.7;W10;SV.6;W10;SV1;W50;v0;HR;Pf;
          XS;v0;Hd;Xp0;Ca.5;W5;Pf;Rf;Pf;v0;DP4;
  start_up:  using current
          Ff0;D^0;Xp0;Xm7;Ca0.05;Rb;Rf;Xts;Xa-500;Xf0.02;XW6;XT6;XC1;Dr100;DP1;HR;Pf;v2;XR;
          XS;v0;Hd;Xp0;Ca.5;W5;Pf;Rf;Pf;v0;DP4;
  
  dwell Ca.5:
		Ff0;HR;v3;Ca.5;DT.05;DV0.05;DM.2;DN2;Di.1;Dv0;DP1;
        Xp0;v0;Hd;DT0;DV0;DM0;DN0;Di0;Dv0;DP4;

Bucket list (optional. Used to debug bucket shaped VOC_SOC that wasn't real):
  Bucket:  Ff0;HR;Bm2;Bs1;D^15;
           Bm0;Bs0;D^0;
           use Bsim=1 and Bmon=2 in MonSim
  triTweakDischBucketMix: set Xm7;D^<>; or Xm0;Dt<>; +reset
                  Ff0;Xp0;v0;Bm2;Bs1;Xm0;Fi1000;Fo1000;Fc1000;Fd1000;FV1;FI1;FHd;T1;SA0;SB0;DA0;DB0;Xtt;Ca1.;Ri;Mw0;Nw0;MC0.004;Mx0.04;NC0.004;Nx0.04;Mk1;Nk1;-Dm1;-Dn1;DP1;Rb;Pa;Xf0.02;Xa-33000;XW5;XT5;XC3;W2;v2;W3;XR;
                  v0;XS;SA1;SB1;Dm0;Dn0;Bm0;Bs0;FV0;FI1;FT0;Xp0;Ca1.;HR;Pf;DP4;
  slowTweakRegressionBucket: Ff0;Bm2;Bs1;D^15;Xp11


## Accuracy

1. Current Sensor
  a. Gain - component calibration at install
  b. Bias - component calibration at install
  c. Drift - Tweak test
  d. Amplifier vs non-amplifier.  Observations of tweak function show over long averaging time the non-amplified sensor provides equivalent results.  Could probably get by with two non-amplified sensors.
2. Voltage Sensor
3. Temperature Sensor
4. Hysteresis Model
5. Coulombic Efficiency and Tweaking and Drift
6. Coulomb Counter
  a.  Temperature derivative on counting is a new concept.  I believe I am pioneering this idea of technology.   That the temperature effects are large enough to fundamentally increase the order of Coulomb Counting.
  b. My notion seems to be backed up by high sensitiviy
7. Dynamic Model
  a. Not sure this is even needed to due to low bandwidth of daily charge cycle.   Average out.
  b. Leave this in design for now for study.   Able to disable
8. EKF
  a. Failure isolation
  b. A-hr displayed on oled for reference - quick check on EKF.
9. Redundancy
  a. Virtual triplex Ib_Amp, Ib_No-amp, Vb


## Calibration checklist
1. Delta adjustment for Vb.
2. Nothing to do for Tb.   If heater kit, make sure Tb is inside the jacket next to the battery case.
3. Collect extreme ranges of data.
  a. 0, +/- 0.3C (30% of max A-h capacity in A)
  b. S/W:  Ib amp, Ib no amp, Vb
  c. Hardware: clamp multimeter next to shunt, multimeter on Vb
4. Use spreadsheet to estimate first order polynomial fit to current data.   Checks bias and gain for linearity.  These devices are linear so if that's not what is seen on plots, check data.
5. To date, my work has not been precise enough to see temperature dependence on shunt calibration.
6. Real runs using battery heater to establish VOC(SOC, Tb) and determine capacity, which should be > rating.
7. To disable tweak logic:  Mk=Nk=1  , MC=NC=0 (max tweak = 0)

## Boot checklist - after new software load
1. Update the version in local_conig.h for 'unit ='.
2. Start CoolTerm record.  Record Hd, Pf, Pa, brief v1 burst for the previous load.
3. On restart after load, check the retained parameter list (SRAM battery backed up).   The list is displayed on startup for convenience.   Go slowly with this if you've been tuning.
4. Record Hd, Pf, Pa, brief v1 burst.   Confirm the 'unit =' is for the intended build install.
5. Check Xm=0 before walk away from installed system.
6. Enable tweak logic if desired (default values in [] in 'h' printout.)


