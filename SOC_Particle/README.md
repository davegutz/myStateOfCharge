# State of Charge Monitor

Use temperature, voltage and bipolar current measurements with a programmable logic controller to reliably monitor the state of charge of a domestic LifeP04 battery bank that is charged by various sources.


<!-- TOC -->
* [State of Charge Monitor](#state-of-charge-monitor)
  * [Abstract:](#abstract)
  * [Off-the-Shelf Hardware Description](#off-the-shelf-hardware-description)
  * [Requirements](#requirements)
  * [Assumptions](#assumptions)
  * [Implementation Notes](#implementation-notes)
  * [Battery Unit Concept](#battery-unit-concept)
  * [Repository](#repository)
  * [Prototypes](#prototypes)
  * [User Interface](#user-interface)
  * [Synchronization](#synchronization)
  * [Post Process Monitoring](#post-process-monitoring)
  * [Cost](#cost)
  * [Battery Heating](#battery-heating)
  * [Hardware notes](#hardware-notes)
    * [Calibrating mV - A](#calibrating-mv---a)
  * [Accuracy](#accuracy)
  * [Calibration checklist](#calibration-checklist)
  * [Boot checklist - after new software load](#boot-checklist---after-new-software-load)
  * [Throughput](#throughput)
  * [Calibration](#calibration)
  * [Powering your device](#powering-your-device)
  * [Redo Loop](#redo-loop)
  * [Device Interfaces](#device-interfaces)
  * [FAQ](#faq)
  * [To get debug data](#to-get-debug-data)
  * [Changelog](#changelog)
<!-- TOC -->

## Abstract
Users of rechargeable battery banks need to know how much charge remains.  This becomes important for estimating the range of travel for an electric car, for example – the 'gas gauge replacement.'   In my case, when truck camping with my CPAP machine I need to know if there is enough charge in the RV battery bank to power the CPAP overnight.   I've never woke up gasping and I want to minimize that possibility.   Old technology batteries, e.g. Lead-acid, have a steep curve relationship between terminal voltage and state-of-charge (SoC).   This makes it easy to use a voltmeter as a SoC gauge.  Newer batteries developed for electric cars are different.  The flat voltage-SoC characteristic of modern LiFeP04 batteries – safe to install in sleeping quarters and efficient charge handlers – makes it nearly impossible to guess SoC from voltage measurement.   Further complicating the task, the electrical hysteresis where voltage depends on direction of charging/discharging and time history is large compared to the SoC characteristic.   Hysteresis uncertainty approaches the entire SoC curve from fully discharged to fully charged.   A 'smart,' reliable monitor is needed.  Smart would be capabilities that keep track of the time history to as accurately as possible predict time remaining for current usage.   Reliable would be features that keep the system operational in the presence of common failures, allowing the user to repair the system at their convenience with no downtime.  At present, my system is designed for a couple, one-off prototypes and relies on calibration of inexpensive components for precision.  The known condition of full saturation is easily detected and used to re-calibrate the device on the fly.  Hardware RC filtering is needed to clean up the sensor signals from noise injected by AC inverter devices.  A 1 Hz low-pass anti-alias filter in hardware is all that is needed.   Since SoC is a long term, e.g. 24 hour integration process – a very slow low-pass filter in itself, no precision is lost using the hardware RC filtering.   Therefore, advanced software filtering is not required for the main task of counting Coulomb charge.   An Extended Kalman Filter (EKF) advanced filtering is useful, however, to detect component failures and establish reliability.   I used Mathworks' EKF prototype.  I created my own 'reservoir' charge model to track hysteresis.  I found that reasonable reliability is achieved with simplex temperature sensing, simplex voltage sensing, and dual current sensing.   The combination of two current signals, one voltage sensor, the EKF, and known SoC characteristic (voltage-currrent-time history) enables the equivalent of triplex current sensing.   Quiet signal detection logic detects that a current sensor is disconnected either by wiring or failure to help isolate.  For strictly hardware reliability reasons, the 1-wire temperature sensor and the mini-OLED display are line replaceable.   Faults and history are recorded in EERAM for later retrieval.   A standalone Python data reduction program (DRP) allows comparing history with a model to understand operation.   The DRP also serves as regression machine to compare software changes with past changes, to develop component maps for characterizing a system, and to investigate problems.  Some learnings:   The system needs to run at about 0.1 seconds update to accurately capture by integration the peaks and valleys of battery bank usage.  The EKF needs to run double precision as well as slower update rate, about 0.5 seconds, to handle the numerics of the system.   It is possible to self-calibrate by comparing total charge history to total discharge history between known charge states – full charge.   By triangulation of data history, the charging efficiency of the batteries needs to be estimated to complement the history data.  System uncertainty for this home-made system is large so that this self-calibration adds no value and was omitted from the published version.    Simple scalar-adder calibration of the installed current sensor using a clamping current meter is sufficient to establish monitor precision within about a half hour of time remaining estimate.  To calibrate the system, the user needs to be intentionally discharged and recharged at expected operating temperatures to characterize the SoC-voltage (voc(soc)) map and hysteresis model.  Triangulation of data, IE. repeated calibration runs from full charge to full discharge, allows estimation of the battery capacity versus rated while calibrating the current sensors (full charge condition is repeatable).  In the case of a CPAP system, I used a couple household fans to simulate actual load and ran data collection overnight in the driveway with the fans while I slept in the house with  my CPAP.  Battery life may be monitored by repeat calibration.   Most of my findings are preliminary based on my observations of prototypes and a lot of experience in the development of computer controlled systems.   I took advantage of my 'skunk works' arrangement in retirement.    Rigorous study is needed to establish the findings as fact for some product device.


## Off-the-Shelf Hardware Description
I used prototype boards to connect various off-the-shelf devices into a reliable, maintainable box.  The heart of the system is the Particle programmable logic controller (PLC).   I've incorporated both the Particle Photon device gen 1 device and the Particle Argon gen 3 device.   Figure board layout shows the latest Argon schematic using Stripboard, a prototyping 0.1 inch spaced board that underneath connects vertical elements with layered copper conductors and over the top and bottom connects signals into bus bars and horizontal elements that connect v+ and v- into bus bars.   The Photon is no longer available, so I didn't bother to show a schematic.  It is similar without the 47L16 device but with a battery connected to VBAT.   The Argon will disappear soon too probably in 2023.   The recommended replacement is Photon 2.  In the future I suggest getting the Photon 2 developer kit that has headers and other peripherals such as USB installed.
![Figure board layout](./doc/board layout.png)  Figure:  State of Charge Wiring Diagram Board Layout

It doesn't matter which of the old generation boards are used.   The Photon uses an external CR2040 battery to energy EERAM while the Argon uses a peripheral 47L16 EERAM device that saves to EEPROM when it detects loss of power, using an external capacitor to power itself while doing so.   The SOC_Particle application uses #define to configure for either board.   When the use 'Configure for device' on the Particle Workbench Visual Studio IDE and select the Particle device, Argon and give it a name, it automatically configures the global #defines to allow you to select the correct #define locally for your own use.   Giving it a name checks that you're loading the proper device that you configured for.  

Device names are given using the Particle App with the device in Setup mode (push left button for 3 seconds to get blinking blue light).

The 47L16 EERAM device provides memory to the Argon through power loss, sometimes caused by the battery management system (BMS) of the battery itself as well as user events.

OPA333 op-amp circuits, two for reliability, convert the bipolar +/- 0.075 volt Bayite shunt signal, current range 100 Amps, into full range 0-3.3v A/D signal.   The current signal uses a dedicated, shielded sense line to feed high impedance op-amp circuits to avoid line effects.

A voltage divider circuit measures battery voltage straight from the device power supply.   Caution:  the Particle devices blow if A/D voltage exceeds 3.3v.   If the prototype is used on a two-battery series 2S system, the voltage divider circuit needs modification.

A 12v – 5v converter circuit feeds power to the Particle device and the 1-wire temperature sensor.   The Particle device in turn converts 3.3 v for other devices – op-amp, EERAM and OLED display.   The Particle device also has a USB.  The power demands do not exceed the capacity of the Particle so USB-only usage is possible for user off-line testing.   Measurement of 12v supply is missing in this mode.

All the hardware if the least expensive possible.   Battery uncertainty drives accuracy.   And low rate of production make use of user calibration feasible.

The initial battery used to test the system, and use on RV trips, was the Battleborn 100 A-h LiFeP04.   It is the most expensive battery available and comes with an industry-leading BMS that prevents damage from misuse.  I thought that was important for the first prototype.    Chins batteries are available at 1/3 the price.


## Requirements
In the spirit of Software Engineering principles, I document perceived requirements.   Many of these arose out of testing.

    1. Calculate state of charge of 100 Ah Battleborn LiFePO4 battery.  The state of charge is percentage of 100 Ah available.
       a. Nominally the battery may have more than 100 Ah capacity.  This should be confirmed.
       b. Displayed SoC may exceed 100 but should not go below 0.
    2. Displayed values when sitting either charging slightly or discharging DC circuits (no inverter) should appear constant after filter rise.
       a. Estimate and display hours to 1 soc. (charge is positive).  This is the main use of this device: to assure sleeper that CPAP, once turned on, will last longer than expected sleep time.
       b. Measure and display shunt current (charge is positive), A.
       c. Measure and display battery voltage, V.
       d. Measure and display battery temperature, F.
    3. Implement an adjustment 'talk' function. Along with general debugging it must be capable of
       a. Setting soc state
       b. Separately setting the model state
       c. Resetting pretty much anything
       d. Injecting test signals
    4. CPU hard and soft resets must not change the state of operation, either soc, display, or serial bus.
    5. Serial streams shall have an absolute Julian-type time for easy plotting and comparison.
    6. Built-in test function, engaged using 'talk' function.
    7. Load software using USB. WiFi to truck or phone (hotspot) may not be reliable.  Related requirement: provide holes to press Particle Device buttons: sometimes setup long-press needed or manual flash request needed with these devices.
    8. Likewise, monitor USB using laptop or phone. 'Talk' function should change serial monitor and inject signals for debugging.
    9. Device shall have no effect on system operation.  Monitor function only.
    10. Bluetooth serial interface required. ***This did not work due to age of Android phone (6 yrs) not compatible with the latest Bluetooth devices.
    11. Keep as much summary as possible of soc every half hour: Tb, Vb, soc.  Save as SRAM battery backup memory. Print out to serial automatically on boot and as requested by 'Talk.' This will tell users charging history.
    12. Adjustments to model using talk function should preserve delta_q between models to preserve change from saturated situation. The one 'constant' in this device is that it may be reset to reality whenever fully charged.  Test it the same way. The will be separate adjustment to bias the model away from this ('n' and 'W')
    13. The 'Talk' function should let the user test a lot of stuff.  The biggest short coming is that it is a little quirky. For example, resetting the Particle device or doing any number of things sometimes requires the filters to be initialized differently, e.g. initialize to input rather than tending toward initializing to 0.  The initialization was optimized for installed use. So the user needs to get used to rationalizing the initialization behavior they see when testing. They can wait for the system to settle, sometimes 5 minutes.  They can run Talk('Rs') to attempt a software filter reset - won't help if the test draws a steady current after reset/reboot.
    14. Inject hardware current signal for testing purposes.  This may be implemented by reconnecting the shunt input wires to a PWM signal from the Particle device.
    15. [deleted] The PWM signal for injecting test signals should run at 60 Hz to better mimic the 60 Hz behavior of the inverter when installed and test the hardware AAF filters, (RC=2*pi, 1 Hz -3dB bandwidth).
    16. There shall be a built-in model to test logic and perform regression tests. The EKF needs a realistic response on current to voltage to work properly (H and hx assume a system). The built-in model must properly represent behavior onto and off of limits of saturation and 0.
    17. The Battery Monitor will not properly predict saturation and low shutoff.  It doesn't need to.  The prime requirement is to count Coulombs and reset the counter when saturated.  The detection of saturation may be crude but always work - this will result in saturation being declared at too low voltage which is conservative, in that the logic will display a state of charge that is lower than actual.
    18. The Battery Model used for testing shall implement a current cutback as saturation is neared, to roughly model the BMS in the actual system.
    19. The Battery Monitor shall implement a predicted loss of capacity with temperature.  The Battery Model does not need to implement this but should anyway.
    20. The Battery Model shall be scale-able using Talk for capacity.  The Battery Monitor will have a nominal size and it is OK to recompile for different sizes.
    21. The Battery Model Coulomb Counting algorithm shall be implemented with separate logic to avoid the problem of logic changes confounding regression test.
    22. The Talk function shall have a method Talk('RR') that resets all logic to installed configuration.
    23. The default values of sensor signal biases should remain as default values in logic.
    24. Power loss and resets (both hard and soft) must not affect long term Coulomb counting.  This may result in adding a button-cell battery to the VBAT terminal of Particle device and using 'retained' SRAM data for critical states in the logic.  It would be nice if loss and resets did not affect Battery Model and testing too.
    25. 'soc' shall be current charge 'q' divided by current capacity 'q_capacity' that reflects changes with temperature.
    26. Coulomb counting shall simultaneously track temperature changes to keep aligned with capacity estimates.
    27. 'SoC' shall be current charge 'q' at the instant temperature divided by rated capacity at rated temperature.
    28. The monitor logic must detect and be benign that the DC-DC charger has come on setting Vb while the BMS in the battery has shutoff current.  This is to prevent falsely declaring saturation from DC-DC charger on.
    29. Only Battleborn will be implemented at first but structure will support other suppliers. For now, recompilation is needed to run another supplier and #define switches between them.
    30. The nominal unit of configuration shall be a 12 v battery with a characteristic(soc) and rated Ah capacity. Use with multiple batteries shall include running an arbitrary number of batteries in parallel and then series (nPnS). The configuration shall be in local_config.h and in retained.h.  This is called the 'chemistry' of the configuration.
    31. The configuration shall be fully adjustable on the fly using Talk. If somebody has Battleborn or a LION they will not need to ever recompile and reflash a Particle device.
    32. Maximize system availability in presence of loss of sensor signals. Soft or hard resets cause signal fault detection and selection to reset.  Flash display to communicate signal status: every fourth update of screen indicates a minor fault. Every other update is major fault where action needed.  Print signal faults in the 'Q' talk.  Add ability to mask faults to a retained parameter rp.


## Assumptions

    1. Randles 2-state RC+RC linear dynamic battery model
    2. RC 1-state dynamic hysteresis lag with variable resistance, R, constant capacitance, C, and limited authority
    3. voc(soc) industry standard state-of-charge to voltage characteristic
    4. Battery has Coulombic charging efficiency 
    5. Coulombic efficiency related to Randles resistances
    6. All current entering series Randles/Hysteresis model ends up as charge.   Transient Randles delta voltage and transient Hysteresis delta voltage store charge but since current is passed along to the Coulomb Counter the Coulomb Counter contains all the charge information if the battery is allowed to settle which it always is.
    7. Perfect sensor calibration.   Book-keep error as an accuracy statement along with modeling errors
    8. +0.01 fractional change of soc with each degree C bank temperature change
    9. New batteries have capacity to 105% of rating


## Implementation Notes

    1. An EKF is no more accurate than the open loop voc(soc) curves.  As a solver, it does seem to follow through troughs without divergence – a pleasant surprise.
    2. A Coulomb Counter implementation is very accurate but needs to calibrate every couple cycles to avoid 'infinite wander.'  This should happen naturally as the battery charges fully each day.
    3. Blynk phone monitor implemented, as well as Particle Cloud, but found to be impractical because
       a. Seldom near WiFi when camping.
       b. OLED display works well.
       c. 'Talk' interface works well. Can use with phone while on the go. Need to buy dongle for the phone (Android only): Micro USB 2.0 OTG Cable ($6 on Amazon).
    4. Shunt monitor seems to have 0 V bias at 0 A current so this is assumed when scaling inputs.
    5. Current calibrated using clamping ammeter turning large loads off and on.
    6. Battleborn nominal capacity determined by load tests.
    7. An iterative solver for voc(soc) model works extremely well given calculable derivative for the equations from reference. PI observer removed in favor of solver. The solver could be used to initialize soc for the Coulomb Counter but probably not worth the trouble.  Leave this device in more future possible usage. An EKF then replaced the solver because it essentially does the same thing and was much quieter and could be used for fault detection / isolation.
    8. Note that there is no effect of the device on system operation so debugging via serial can be more extensive than usual.
    9. Two-pole filters and update time framing, determined experimentally and by experience to provide best possible Coulomb Counter and display behavior.
    10. WiFi turned off immediately. Can be turned on by 'Talk('w')'
    11. Battery temperature changes very slowly, well within capabilities of DS18B sensor.
    12. 12-bit AD sufficiently accurate for Coulomb Counting. Precision is what matters and it is fine, even with bit flipping.
    13. All constants in header files (Battery.h and constants.h and retained.h and command.h).
    14. System can become confused with retained parameters getting off-nominal values and no way to reset except by forcing a full compile reload.  So the 'Talk('A')' feature was added to re-nominalize the rp structure.  You have to reset to force them to take effect.
    15. The minor frame time (READ_DELAY) could be run as fast as 5.  The application runs in 0.005 seconds. The anti-alias filters in hardware need to run at 1 Hz -3dB bandwidth (tau = 0.159 s) to filter out the PWM-like activity of the inverter that is trying to regulate 60 Hz power.  With that kind of hardware filtering, there is no value added to running the logic any faster than 10 Hz (READ_DELAY = 100).  There's lots of throughput margin available for adding more EKF logic, etc.
    16. The hardware AAF filters effectively smooth out the PWM test input to look analog.  This is desired behavior: the system must filter 60 Hz inverter activity.  The testing is done with PWM signal to give us an opportunity to test the hardware A/D behavior. The user must remember to move the internal jumper wire from 3-5 (testing) to 4-5 (installed).
    17. Regression test:  For installed with real signal, could disconnect solar panels and inject using Talk('Di<>') or Talk('Xp5') and Talk('Xp6'). Uninstalled, should run through them all: Talk('Xp<>,) 0-6. Uninstalled should runs onto and off of limits.
    18. In modeling mode the Battery Model passes all sensed signals on to the Battery Monitor.  The Model does things like cutting back current near saturation, and injecting test signals both hard and soft. The Signal Sense logic needs to perform some injection especially soft so Model not needed for some regression.
    19. All logic uses a counting scheme that debits Coulombs since last known saturation.  The prime requirement of using saturation to periodically reset logic is reflected in use of change since saturation.
    20. The easiest way to confirm that the EKF is working correctly is to set 'modeling' using Talk('Xm7') and verify that soc_ekf equals soc_mod.
    21. Lessons-learned from installation in truck. Worked fine driving both A/D with D2 from main board.  But when installed in truck all hell broke loose.  The root cause was grounding the Vlow fuse side of shunt legs of the A/D converters.  In theory the fuse side is the same as ground of power supply.  But the wires are gauge 20-22 and I detected a 75 mA current in the Vh and Vl legs which is enough to put about 50% error on detection.  Very sensitive.  But if float both legs and avoid ground looping it works fine.  And as long as ground loops avoided, there is no need to beef up the sense wire gauge because there will be no current to speak of. I revised the schematics.  There are now two pin-outs: one for installed in truck and another when driving with the D2 pin and PWM into the RC circuits.
    22. 'Talk' refers to using CoolTerm to transmit commands through the myTalk.cpp functions. Talk is not threaded so can only send off a barrage of commands open loop and hope for the best.
    23. I had to add persistence to the 'log on boot' function.  When in a cold shutoff, the BMS of the battery periodically 'looks' at the state of the battery by turning it on for a few seconds. The summary filled up quickly with these useless logs. I used a 60 second persistence.
    24. Use Tb<8 deg C to turn off monitoring for saturation.  This prevents false saturation trips under normal conditions.
    25. Had a failed attempt to heat the battery before settling on what Battleborn does here: <https://battlebornbatteries.com/faq-how-to-use-a-heat-pad-with-battle-born-batteries/>. I thought the pads I bought here: <https://smile.amazon.com/gp/product/B0794V5J5H/ref=ppx_yo_dt_b_asin_title_o05_s00?ie=UTF8&psc=1> would gently heat the batter. I had the right wattage (36 W vs 30 W that Battleborn uses) but I put them all on the bottom. And the temp sensor is on top.  And the controller is on/off.  That set up a situation where the heat would be on full blast and the entire battery had to heat up before the wave of heat would hit the sensor to shut it off.  The wave continued to create about 5 degree C overshoot of temp then 5 degree C undershoot on recovery.  I measured up to 180 F at the bottom of the battery using an oven thermometer. There was some localized melting of the case. A model accurately predicted it.  See GitHub\myStateOfCharge\SOC_Particle\Battery State\EKF\sandbox\GP_battery_warm_2022a.py. I made a jacket, covered whole thing with R1 camping pad (because BB suggests putting temp sensors inside 'blanket') and put the temp sensor for the SoC monitor and the heater together at one corner about 2 inches down from the top.  I set the on/off to run between 40 and 50F (4.4 - 7.2C) compared to BB 35 - 45 F. I wanted tighter control for better data.  Maybe after I get more data I'll loosen it back up to 35 - 45 F.
    26. Calibrated the t_soc voc tables at a 1-4 A discharge rate.  Then they charged 5 - 30 A.  The hysteresis would have been used some at the 1-4 A rate.  May have to adjust tables +0.01 V or so after adding the hysteresis. Measure it: 0.007 V.
    27. An issue that only show up when using the 'talk' function is an overload of resources that causes a busy waiting of current input reading. I added to readADC_Differential_0_1 count_max logic to prevent forever waiting.  There is an optimum because if wait too long it creates cascade wait race in rest of application. 50: inf events. 100:0-4 events. 200: 5 events.  800: 25 events. You can reproduce the problem by sending "Hd" wile running the tweak test of previous item. Apparently exercising the Serial port while reading can cause the read to crash. The actual read statement does not forever wait, but the writer of readADC_Differential_0_1() put a 'while forever' loop around it.  I modified the Adafruit code to time out the loop.
    28. Running the tweak test will cause voc to wander low and after about 5 cycles will no longer saturate.  The hysteresis model causes this and is an artifact of running a huge, fast cosine input to the monitor. Will not happen in real world.
    29. A clean way to initialize the EKF is an iterative solver called whenever initialization is needed. The times this is needed are hard bootup and adjustment of SoC state using Talk function.  The latter is initiated using cp.soft_reset. The convergence test persistence is initialized false as desired by TFDelay(false,...) instantiation.  Want it to begin false because of the potentially severe consequences of using the EKF to re-initialize the Coulomb Counter.
    30. The Coulomb Counter is reset to the EKF after 30 seconds with more than 5% error from EKF.  Initialization takes care of this automatically. The EKF must be converged within threshold for time(EKF_CONV & EKF_T_CONV).
    31. To hedge on errors, the displayed amp hours remaining is a weighted average of the EKF and the Coulomb Counter. Weighted to EKF as error increases from DF1 to DF2 error.  Set to EKF when error > DF2. Set to average when error < DF1.
    32. Some Randles dynamics approximation was added to the models, both simulated and EKF embedded to better match reality.  I did this in response to poor behavior of the electrical circuits in presence of 60 Hz pulse noise introduced by my system's pure-sine A/C inverter.  Eventually I added 1 Hz time constant RC circuits to the A/D analog inputs to smooth things out.  I'm not sure the electrical circuit models add any value now.  This is because the objective of this device is to measure long term energy drain, time averaged by integration over periods of hours.  So much filtering inherent in integration would swamp most of the dynamics captured by the Randles models.  It averages out.  Detailed study is needed to justify either leaving it in or removing it. An easy study would be to run the simulated version, turning off the Randles model in the Monitor object only. Run the accelerated age test - 'Tweak test' - and observe changes in the Monitor's tweak behavior upon turning off its Randles model.  Then and only then it may be sensible to embark on improving the Randles models.
    33. Given the existing Randles models, note that current is constrained to be the same through series arrangements of battery units.  The constraint comes from external loads that have much higher impedance than battery cells.  Those cells are nearly pure capacitance.  With identical current and mostly linear dynamics, series batteries would have the same current and divide the voltage perfectly.  Parallel batteries would have the same voltage and divide the current perfectly.  Even the non-linear hysteresis is driven by that same identical current forcing a perfect division of that voltage drop too.  So multiple battery banks may be managed by scalars nP and nS on the output of single battery models.
    34. Throughput test
        v1;Dr1;
         look at T print, estimate X-2s value (0.049s, ~50% margin)
        Dr100;
         confirm T restored to 0.100s
    35. Android running of data collection.  The very best I could accomplish is to run BLESerial App and save to file. You can use BLE transmit to change anything, echoed still on USB so flying a little blind.  Best for changing 'v0' to 'v1' and vice-versa. The script 'DataOverModel.py' WILL RUN ON ANDROID using PyDroid and a bunch of stupid setup work. But live plots I couldn't get to work because USB support is buggy. The author of best tools usbserial4 said using Python 2 was best - forget that! Easiest to collect data using BLESerial and move to PC for analysis. Could use laptop PC to collect either BLE or USB and that should work really well.  Plug laptop into truck cab inverter.
    36. Capitalized parameters, violation of coding standards, are "Bank" values, e.g. for '2P3S' parallel-serial banks of batteries while lower case are per 12V battery unit.
    37. Signal injection examples:
       Ca0.5;Xts;Xa100;Xf0.1;XW5;XC5;v1;XR;
       Expected anomalies:
       - real world collection sometimes run sample times longer than RANDLES_T_MAX.  When that happens the modeled simulation of Randles system will oscillate so it is bypassed.  Real data will appear to have first order response and simulation in python will appear to be step.
    38. Manual tests to check initialization of real world.  Set 'Xm=4;' to over-ride current sensor. Set 'Dc<>' to place Vb where you want it.  Press hard reset button to force re-initialization to the EKF. If get stuck saturated or not, remember to add a little bit of current 'Di<>' positive to engage saturation and negative to disengage saturation.
    39. Bluetooth. In the logic it is visible as Serial1.  All the Serial1 API are non-blocking, so if Bluetooth not connected or has failed then nothing seems to happen. Another likely reason is baud rate mismatch between the HC-06 device (I couldn't get HC-05 to work) and Serial.begin(baud).  There is a project above this SOC_Particle project called BT-AT that runs the AT on HC-06. You need to set baud rate using that.  You should use 115200 or higher.  Lower rates caused the Serial API to be the slowest routines in the chain of call.  At 9600 the fastest READ time 'Dr<>' was 100 - 200 ms depending on the print interval 'Dp<>'.   It’s hard to understand why it was difficult to do the initial setup.   I think it was a combination of three things (maybe 4):
        a) Either came configured 115200 or set it inadvertently using Arduino attempts to configure.    So try 115200 in BT-AT/main.h first for Serial1.   If that doesn’t work, try 38400.
        b) The AT function does not echo anything if a setting is already in place!   It’s easy to miss the echos in all the confusion so a successful adjustment was not observed then subsequent attempts appear to fail.
        c) When testing with phone, make sure to use the SOC_Particle app not the BT-AT app. And wait longer than you think is reasonable for the phone to find during ‘Pair new device’ activities.   Maybe wait up to 1 minute.
        d) A Hail Mary would be to try configuring with Arduino then return here and repeat using /src/.ino.  (see 	BT-AT/arduino/sketch_mar30a_oneshot.ino)

    40. Current is a critical signal for availability.  If lose current also lose knowledge of instantaneous voc_stat because do not know how to adjust for rapid changes in Vb without current.  So the EKF useful only for steady state use. If add redundant current sensor then If current is available, it creates a triplex signal selection process where current sensors may be compared to each other and Coulomb counter may be compared to EKF to provide enough information to sort out the correct signals. For example, if the currents disagree and CC and EKF agree then the standby current sensor has faulted.  For that same situation and the CC and EKF disagree then either the active current sensor has likely failed. If the currents agree and CC and EKF disagree then the voltage sensor has likely failed.  All this consistent with proper and same calibration of current sensors' gains and setting biases so indicated currents are zero when actual current is zero. Need to cover small long duration current differences as well as large fast current difference failures.  These tend to compete in any logic. So two mechanisms are created to deal with them separately (ccd_fa and wrap_fa).  The Coulomb Counter Difference logic (ccd) detects slow small failures as the precise Coulomb Counters drift apart between the two current sensors.  The Wrap logic detects rapid failures as the sensed current is used to predict vb and then trips when compared to actual vb. Maximum currents are about 1C for a single rated capacity unit, e.g. 1P1S.  The trip points are sized to detect stuff no faster than that and as slow as C0.16.  The logic works better than this design range.
    41. Other fault notes:
       a. Every fail fault must change something on the display. The goal is that will prompt the user to run 'Pf' to see cause.
       b. Wrap logic 0.2 v=16 A. There is an inflection in voc(soc) that requires forgiveness during saturation. 0.25 v = 20 A for wrap hi.  Sized so wrap_lo_fa trips before false saturating with delta I=-100 with soc=.95
       c. If Tb is never read on boot up it should fail to NOMINAL_TB.
    42. Fault injection testing
    43. Tuning EKF (R & Q):
        - Depends on update time.  Presently designed for 2.0 sec.
        - If update time changes (combo of Dr and DE), need to retune R and Q
        - Requirements:
           'AmpHiFailSlow' 6 min to cc_diff = 0.004
           'rapidTweakRegression40C' abs(soc_ekf-soc)/soc <0.25
           0.1 < R < 1 (Q will be <<1)
           Behavior goes with R/Q.  Could retune R 2x and Q 2x and get same result.
    44. Shunt sampling:
        a) The ADS devices are really ugly. Their main advantage is differential input for low noise, good resolution.  But they're expensive ($30 total), take up a lot of room, and use ALL the throughput.  We're not limited by throughput fortunately but just barely. An equivalent solution is to increase the OpAmp333 gain by 5x then use the A-pins on the particle devices to sample the output of the OpAmp and the reference voltages separately. (A0 and A1 of the former ADS - but remove the ADS.)  The main disadvantage of the Op Amps are noise so over-sample and filter them digitally.  The performance of the filtered high gain signals should be equal to the differential ADS devices but I didn't do a study.  There is virtually no sample time hit for A-pin calls (<1 ms total for 2).
        b) A good idea to combine the analog commons into one feedback at A4 bit the dust because circuit analysis using LTSpice showed frequency response of shared common rolls off much more quickly due to 2x current in shared circuit. Also makes a single point of failure.  These two reasons led to 2 analog inputs used for each current feedback, to create a differential. The difference is done in logic because the Particle devices do not have differential hardware inputs. A better simplification idea is to buy a couple TSC2010-IDT 20x differential amplifier chips (https://www.digikey.com/en/products/detail/stmicroelectronics/TSC2011IYDT/13244059).  These will eliminate two wires, free up two A/Ds and best of all provide differential accuracy. I did what I did because a. I didn't know the TSC2010-IDT chip existed and b. the +/- nature of the shunt sensor, and 0 - 3.3 v nature of A/D converters, require a dc offset on a bipolar signal.  I began this project with ADS1013 differential A/D cards.  When I realized I could get rid of them because the Particle devices have plenty of A/D I didn't realize I could go to a differential amplifier instead.  Anyway they weren't available until 6/2023 estimate.

## Battery Unit Concept

For convenience all modeling is done in relation to a 12v battery of a certain manufacturer.  The user specifies the rated capacity.  The user has to develop, beg, borrow, and steal these characteristics and ultimately test their own battery through a full discharge/charge cycle.   But once this is set, battery banks are built by scaling the number of series and parallel units, (nS and nP).   Those numbers are floating point for maximum flexibility.   A 2S3P bank would consist of 4 batteries.  The voltage output would be 2x a unit (24v) and the current output would be 3x a unit.   This scaling may be done at the sensor interface and at the display interface.   Then the computations – modeling and counting are done with the single battery unit.

Caution
The voltage sensor is a 3.3v A/D on the PLC to measure 15v.   Exceeding 3.3v will blow the A/D and make the monitor mostly useless.   It would be difficult to know saturation, a critical sensing element for basic accuracy.   A voltage divider is soldered to do this.   If more batteries are added in series, then the low voltage leg of the divider will need less resistance.   See the 4k7 resistor on the right hand side of the main board diagram at top – feeding A1.  It's easy to add a parallel resistor to achieve this whereas the high voltage leg would require removing solder.   There is a way to add this resistor to the back of the main PLC board accessed by removing the cover of the device.   Remember to rescale and recalibrate the voltage measurement in the monitor application.

Current Sensor Design
Shunt return a small voltage for a current passing through it.   The voltage is small to avoid affecting system performance and generating heat.   The voltage is bipolar because current may either charge or discharge.   So an amplifier is required to generate a high voltage for a 3.3v unipolar A/D that does not change sign.   This is easy to do with a single op-amp. See Figure 3.  For a shunt that puts out 0.075v at rated current a gain of about 20 is needed.  The RC filtering (R1*C106) is performed on the output to also serve as anti-aliasing of op-amp and other circuit noise.

A disadvantage of this design is that 4 A/D are needed for two sensors.   Each sensor needs the output voltage and also the common voltage.   There is a chip available, TSC2010-IDT, that does differential amplification using three built-in op-amps.   The base chip is TSC201.   The trailing "0" denotes 20 amplification.   There are others: the next size up, "1" is 60.  The IDT is the standard quality control for an SOIC-8 surface mount.   These sensors use high quality internal resistors and cost about $5.00 each but were not available until 6 months after building the Beta prototype.

## Repository

All information including code, data sheets, scripts, the source for this document is organized in the open GitHub repository https://github.com/davegutz/myStateOfCharge.   The MIT license is applied to make all this information open.
Most information is in the primary application folder SOC_Particle, named after the first prototype:  a Particle Photon PLC running a 'state of charge' counting algorithm.
Moving alphabetically, the first folder 'Battery State' is a record regarding the theory of LiFeP04 battery state of charge (SoC) monitoring.   The subfolder 'EKF' is a record of theory of Extended Kalman Filter as applied to SoC.   Inside the 'sandbox' folder are Python models of this topic.
The second folder 'dataReduction' collects raw CoolTerm data capture files '.txt.'   The README.md file at the top level has a listing of the different types of scripts run using the 'talk' function through CoolTerm UART transmit/receive interface to generate these files.   That is normally done as regression testing to understand code change.   Those are sometimes renamed as '.xls', '.xlsx', or '.ods' to plot by hand using Microsoft Excel or Open Document Spreadsheet.   By renaming them the hand work is remembered in the repository and not over-written by the next regression test runs through the script series.   The '.stc' files are CoolTerm configuration files that are useful to save.  The subfolder 'figures' is a permanent collection of data reduction runs, see the 'py' folder of the data reduction Python scripts.  The subfolder 'temp' is a scratch folder used by the Python scripts.   The '.csv' files in there are useful for debugging the Python scripts.
The folder 'datasheets' has hardware datasheets as well as snapshots of any hand-drawn schematics in 'Schematics' and a folder 'pSpice' with LTSpice modeling of the circuits.
The folder 'lib' is created by Visual Studio Particle Workbench to import Particle-specific code libraries.
The folder 'py' was mentioned before.   It is the Python scripts used for data reduction.   It was handy to run the same scripts to overlay data on predicted model results.   So any design work could be performed by iterating on a particular script run that has a problem, modifying the Python model of the application to find solutions.   Inside 'py' is a folder 'pyDAGx' where I store my own Python libraries.   The 'venv' folder is maintained by pyCharm IDE.
The folder 'src' has the application source.  The app is 'SOC_Particle.ino.'   Particle follows the Arduino naming convention.  The '.sav' files are my record of a concept that 'tweaked' the charging efficiency to match Coulomb Counting history to incidence of saturation.    The concept assumes the current sensors are calibrated perfectly.   Inside this folder are 'Adafruit' libraries imported by hand instead of the Particle process, 'hardware' libraries for miscellaneous devices, and 'myLibrary' with my stuff I've accumulated over time – mainly dynamic filters used for application logic.
The folder 'target' is generated by the Particle Workbench to hold application .elf files.
There is a gsheet in Appendix 3.   Links called 'Truck Camping ALL STUFF' that has a manifest of the prototype hardware.   See the tabs 'SoC Device Alpha' and 'SoC Device Beta.'   Alpha refers to the Photon device design with ADS cards feeding I2C.   Beta refers to the Argon design with pure OpAmp devices.
Design Process Philosophy
For this project I followed a streamlined method that I hope 'self documents.'   Data reduction centers on an 'overplot' idea.   The run regression, I would overplot the result of old runs on new runs looking for and verifying differences.   To run verification, I would overplot the result of new runs on the new model, looking for differences.  In verification, there should be a perfect overplot.   There are some very rare instances where the data does not overlay perfectly.   The presumption is that in achieving a perfect overlay that design meets intent and I avoided a painfully detailed explanation of plot artifacts.   Design documentation is done at a higher level with a rough description, not too much detail that would detract from understanding.

## Prototypes
Two situations drove the prototypes and sometimes choice of design.

First, sampling of a low-voltage bipolar current shunt is the most challenging hardware design in this project.   At first, I tried to leverage a device described in the literature.   It sampled the low voltage using the differential inputs into an A/D converter (ADS1013) sent along an I2C to the Particle devices.    It was later improved by adding an op-amp (OPA333) as suggested by Texas Instruments engineers in the datasheets for the ADS1013.  This got me started and lives on in the Particle device prototypes.

The biggest problem with the initial ADS I2C solution is the throughput.   Every call to the I2C device uses between 1 and 100 ms of dead wait time.   This sets the throughput for the application.   To capture current spikes properly a 0.100 second (100 ms) update time is needed.   

Eventually I realized that the Particle devices have plenty of fast A/D converters and that the real challenge was to make a high accuracy bipolar instrumentation amplifier.  Microelectronics make a 20:1 chip that matches the 3.3v and 0.075v ranges of the Particle A/D and shunt output range called TSC2010-IDT that is perfect for the task and costs less than $4.   It was not available due to COVID pandemic shortages, so instead I used two one-sided A/D converters to manage primary and common voltages of the one-sided OPA333 op-amp circuits to calculate sensed current.   This is less accurate but workable since there is plenty of A/D interfaces.

The second situation that drove design is that Particle devices are morphing over time.   The Photon device is their first / second generation device with Wi-Fi interface and built-in EEPROM to handle power loss.   I interfaced to TX/RX UART an HC-06 standard Bluetooth (as opposed to the newer low-energy BLE) for monitoring by the user while moving.   When those ran out Particle offers up the third-generation Argon also with Wi-Fi that does not have built-in EEPROM but does have built-in BLE.  I bought 47L16 EERAM I2C modules to replace the EEPROM function.  This required considerable application programming to support.   I tried the BLE function but found that UART terminal apps to support it are poor.    So I continued on with the HC-06 since it was a ready solution.   The future of Particle is the Photon 2.   I would recommend the Photon 2 Development Kit.   They are not yet available.   It will require the EERAM and HC-06, most likely, unless UART BLE terminals magically appear.

An almost third situation was software morphing.   Particle requires software versions to match hardware devices and those change over time.   Fortunately this application is simple enough that basic firmware could handle it and firmware bugs did not affect it.    I simply kept up with Particle's releases, always using the most recent as they came out.


## User Interface

It’s all done using a system of two-letter codes.   The user connects a computer to the PLC and fires up some serial interface program such as puTTY.   Callbacks in the application watch for the codes, decode them, and make changes to the program.

I used Serial transmit / receive to communicate with the PLC.   Particle provides an API to instantly transmit serial information to the application.  There are built-in callback functions ('serialEvent()' and 'serialEvent1()')  for the two serial lines that the user populates with whatever commands they want.   The user is expected to pair this function with 'Serial.available()' function called within the callback to parse out user input. The callback function executes each minor frame – call of 'loop().'   Particle devices work like Arduino, so calls to 'loop()' happen as fast as the application is able to.   The user is responsible for managing time frames.

Below [\ref {f2}] is a functional block diagram (FBD) of the user interface.

[\label {f2}] ![Figure 2](doc/fbd.png)  Figure:  Functional Block Diagram of User Interface to Application SOC_Particle

## Synchronization
This refers to real time latencies, not clock time.  I chose to implement a 'loose synchronization' method.   Frames are formed by timers called each pass of 'loop().'   If time is up for a frame, a boolean is set and any logic associated with the frame is then enables.   It becomes 'loose' when events and user input introduce slippage in the frame.   I do this for flexibility and to run the application as fast as possible.   This application is not time critical.   By that there is no feedback control that requires high fidelity dynamic calculation.   But I reuse and ecosystem of synchronization used on time critical applications where running as fast as possible is desirable.
The key technology to enable loose synchronization are dynamic digital signal processing algorithms that handle variable update time.  My library of functions calculate difference equation coefficients each update for the measured update time.
Dynamic difference equations are aliased – unstable – if they are called too infrequently for the dynamic eigenvalues specified for the algorithm.   The library assumes that whatever time slippage occurs is a rare occurrence.   The user determines the stable update time for each instance and limits the numerical value of update time supplied to the algorithm call to the stable range.   This technique limits the destabilizing spikes that occur with coefficients recalculated with unstable update time.  The occurrence is rare.   By the next 'loop()' call, maybe even a few calls, the algorithm has a chance to resume normal operation with a slight glitch.
The glitches tend to be stabilizing, quieting, because update time is less than actual, so they are difficult to spot.
Data saved from the application drives the over-plot model.  The glitches appear in the over-plot response in an identical way as in the application, so they are impossible to see in a plot.
Initialization
Dynamic algorithms require a happy initial condition for the states.   On power-up, the initial first few passes are reserved for sensing the initial conditions and iterating the various 'use-before-calculate' situations.   To speed this up, the application has a 'initialize_all()' function that attempts to perform this in one pass.

The one-wire temperature sensor sometimes requires up to a minute to provide converted values.   A special 'reset_temp' flag manages this on power up, extending the initialization period until the first good value appears.   After initialization, there are rate limits on the converted temperature values to prevent latency from spiking the logic and confusing filters.   Fortunately, the battery monitoring algorithms are not very sensitive to temperature changes at the occurrence rate they occur.

It is possible to re-initialize on the fly.   When the user requests the monitor to be at a different charge state, for some off-line testing of the logic for example, the synchronization ecosystem will turn on the 'reset' flags to force this to happen as on a normal power-up.

Because 'sat' is a UBC, when calling 'count_coulombs()' we have to tell it that we expect to call it again, and it should set the 'resetting_' sticky bit upon exit.   For that, we set the 'resetting' (no underscore) input flag.

A significant simplification is possible to initialize the off-line over-plot model.  The data drives the model and thus must initialize to the same condition to be useful.   Rather than develop a separate initialization ecosystem for the model, it simply initializes to the incoming data.   The incoming data carries flags names 'reset' or 'reset_temp' that tell the model when to do this.   The re-initialization events occur seamlessly too.
Reliability Concept
There is dual current sense hardware.   With flat battery voc(soc), current sensing and associated Coulomb Counting integration are critically important to the proper and accurate functioning of this device.   This would be true no matter how sophisticated the modeling and filtering employed.

Third current signal constructed using voltage feedback, and EKF and voc(soc).   The EKF's first function is to solve VOC from SoC.   An on-line solver was initially used and replaced by the EKF because the EKF has features that buy it's way on.  The EKF has two probability inputs:  one is the confidence in the voltage sensor and the second is confidence in the process model SoC.   By favoring the SoC (integration of doubly-redundant current a.k.a. Coulomb Counting) the EKF automatically switches and rejects a failed voltage (or bad voc(soc) curve or bad hysteresis model).   If the doubly redundant currents disagree, the EKF can then moderate to isolate the bad current signal.   Triplex current sensing is manufactured by system design.

The shunt device and wiring are prime reliable.   This is a declaration that the system cannot detect or replace them.    However, failure is tested and the failure modes are graceful.   For example, there is 'quiet' logic that detects disconnected wiring and/or shunt failure and displays a 'conn' status on the OLED.

Note that an important feature of this scheme is annunciation and user intervention.  Once the triplex logic isolates a failure it is in a mode where further isolation is not much better than a coin toss.   How do you choose between two different wristwatches telling different time?  If the user repairs a failure soon after annunciation then exposure to multiple failure scenario is negligible.

Power loss is a normal event.   They are caused by user action and by the BMS.   To protect LiFeP04 chemistry, the BMS shuts off charging below 0 C and shuts off usage below 10v.   Features are added to Count Coulombs and monitor faults in presence of normal power loss occurrence.

Except for shutting off, the Battleborn batteries are sensitive to temperature.   Accuracy is improved with sensor but not terribly.  The most likely consequence of temperature sensor loss would be an unexpected BMS shutoff for low temperature while the device is predicting good time remaining.  Temperature sensor failures are monitored and displayed so the user can take action to reduce exposure.
Real-Time Monitoring
There are three possible ways:
    • Connect CoolTerm to the USB
    • Connect phone UART terminal to the USB
    • Monitor HC-06 standard Bluetooth Serial Output
All use the 'Serial' and 'Serial1' APIs in the Particle Workbench tools.   'Serial' is connected to the USB TX/RX.   'Serial' is used for all heavy troubleshooting and testing.   Various levels of verbosity print various styles of monitoring.   Verbosity level 0, 'v0', transmits nothing on 'Serial.'   Level 1, 'v1' is the most basic data stream.   So on.   Type 'h' to get a list of options.  'Serial1' content is a subset of 'Serial' content connected to Bluetooth TX/RX.   'Serial1' is useful for daily quick check.   The level 0, 'v0', form reproduces the OLED display on the Bluetooth monitor.
Appendix 2:  Regression Suite has lots of examples.

## Post Process Monitoring
High sample rate is needed to properly book-keep current integration for spikes.  Once that is verified and built in the device build the monitoring process may have as long as 30 minutes between data points.   Sensor failures are on the order of 10 seconds between data points.   Two circular buffers manage this data collection.    Seven points are captured for any critical sensor failure and that buffer frozen until manually reset.   It may be downloaded using any of the three monitoring methods.  Any and all excess memory is used for the history.   The Argon PLCs have some extra EERAM beyond what's needed for sensor failures.   And the Argon PLCs have extra PRAM to access as long as the PLC is not depowered.   All these are available using any of the three monitoring methods.
Resistor Quality
The temperature range for this device is about 0C for a heated battery up to about 40C on hot summer day.   This is a small swing and 5% resistors would show about 1% effect.   And since the battery hysteresis, variation, life and the need to calibrate for installation effects drive accuracy, the resistor quality could be poor.   This is unnecessary because the high quality resistors are negligibly more expensive.   In the future, 1% 1/4 watt resistors should be procured.

## Cost

## Battery Heating

For some expenditure of charge, the battery can remain available for full functioning charge and discharge even when temperature outside, or inside the unheated RV, drop below the level where the battery management system (BMS) would normally shut off the terminals.  The BMS, present in all LiFeP04 batteries, does this to prevent damage to the chemistry.   This is all independent of sunshine, as many RVs, including mine, charge from the engine too.

The limit of this feature is somewhere below -5F.   Any colder and the battery expend all its energy overnight keeping warm leaving nothing left to run a CPAP machine.

Heating pads are wrapped around the sides of the battery and another temperature sensor, as well as the monitor's temperature sensor, are inserted underneath the pad between the heaters and the battery.  An old foam camping pad is wrapped around it all.  A special circuit, usually used to heat chicken brooders, monitors this other sensor and routes battery power to the heaters.  The circuit has an adjustable on-off set-point and hysteresis.  I found that 40 – 45 F (4.4 – 7.2 deg C) works well as a practical matter.   The spreadsheet 'Truck Camping ALL STUFF.gsheet' tab 'Settings'.    There is a link to this in Appendix 3.   Links. 

Heating is done only from the sides




## Hardware notes

Grounds all tied together to solar ground and also to chassis.
Typically operate for data with laptop plugged into inverter and connected to microUSB on Particle device
Can run 500 W discharge using floodlight plugged into inverter
Can run 500 W charge from alternator DC-DC converter (breaker under hood; start engine)

### Calibrating mV - A

Before installation, use a power supply to calibrate the current sensor gain and offset

You need a clamping ammeter. Basic.  Best way to get the slope of the conversion.  A 100A/.075V shunt is nominally 1333 A/V.

Do a discharge-charge cycle to get a good practical value for the bias of the conversion.  Calculate integral of A over cycle and get endpoint to match start point.  This will also provide a good estimate for battery capacity to populate the model. (R1 visible easily).

Tweak logic could be used to get rid of drift of small A measurement.  This integrates each freely counted current input and compares endpoints after long cycle.  Should be no difference.  If there is, tweak.  Not implemented - not worth complexity.

>13.7 V is decent approximation for SoC>99.7, correct for temperature.

Temperature correction in ambient range is about BATT_DVOC_DT=0.001875 V/deg C from 25 * number of cells = 4. This is estimated from the Battleborn characterization model I performed.

The ADS module is delicate (ESD and handling).  I burned one out by
accidentally touching terminals to back of OLED board.  I now mount the
OLED board carefully off to the side.  Will need a hobby box to contain the final device

## Accuracy

1. Current Sensor 
   1. Gain - component calibration at install 
   2. Bias - component calibration at install 
   3. Drift - Tweak test (not implemented)
   4. Amplifiers.  There is a chip (TSC2010-IDT, https://www.digikey.com/en/products/detail/stmicroelectronics/TSC2011IYDT/13244059, 20x for 3.3v and 0.075 shunt) that creates one voltage signal for differential action.  That will reduce complexity, use fewer A/D, and be more accurate.
2. Voltage Sensor
3. Temperature Sensor
4. Hysteresis Model
5. Coulombic Efficiency and Drift
6. Coulomb Counter 
   1. Temperature derivative on counting is a new concept. I believe I am pioneering this idea of technology.  That the temperature effects are large enough to fundamentally increase the order of Coulomb Counting. 
   2. My notion seems to be backed up by high sensitivity
7. Dynamic Model 
   1. Not sure this is even needed to due to low bandwidth of daily charge cycle.  Average out. 
   2. Leave this in design for now for study.  Able to disable
8. EKF 
   1. Failure isolation 
   2. A-hr displayed on OLED for reference - quick check on EKF.
9. Redundancy 
   - Virtual triplex Ib_Amp, Ib_No-amp, Vb

## Calibration checklist
1. Delta adjustment for Vb.
2. Nothing to do for Tb.  If heater kit, make sure Tb is inside the jacket next to the battery case.
3. Collect extreme ranges of data
   1. 0, +/- 0.3C (30% of max A-h capacity in A)
   2. S/W: Ib amp, Ib no amp, Vb 
   3. Hardware: clamp multi-meter next to shunt, multi meter on Vb
4. Use spreadsheet to estimate first order polynomial fit to current data.  Checks bias and gain for linearity. These devices are linear so if that's not what is seen on plots, check data.
5. To date, my work has not been precise enough to see temperature dependence on shunt calibration.
6. Real runs using battery heater to establish VOC(SoC, Tb) and determine capacity, which should be > rating.

## Boot checklist - after new software load
1. Synchronize time if necessary. Use hotspot on phone.  Press left button and hold 3 sec to get blink blue. Use particle app to '+' device. Reset using right button to complete the process.  Time is UTC.  Unfortunately for device soc1a, the bar code on the Argon is hidden inside the case.   Then you must use the talk('U') feature of the interface programs.   This will work over Bluetooth for Argon.   Use Unix Epoch website and subtract (hours from Zulu)* 3600 and paste onto U.
2. Update the version in local_config.h for 'unit ='.
3. Start CoolTerm record. Record Hd, Pf, Pa, brief v1 burst for the previous load.
4. On restart after load, check the retained parameter list (SRAM battery backed up).  The list is displayed on startup for convenience.  Go slowly with this if you've been tuning.
5. Record Hd, Pf, Pa, brief v1 burst.  Confirm the 'unit =' is for the intended build install.
6. Check Xm=0 before walk away from installed system. 

## Throughput
1. Driven by ADC read of ADS1013 device (current AD).  For Argon with EERAM 47L16 it is ADC write of parameters at 0.001 each. Manage this with blink logic in put_all_dynamic() of 'SavedPars' object, called with display update time DISPLAY_USER_DELAY (1.2 sec) in ino file.
2. There is a 6×1.2 delay between some transient events and when it is remembered by the EERAM.  If you're pushing buttons rapidly and repeating scripts you may run into stale data issue especially remembered charge states 'delta_q...' --> soc...
3. Probably wiring quality drives the conversion count for (busy wait for I2C comm). Or could be flaky ADS devices.
4. Per unit
 pro0p: amp 62 cts, 0.0320 sec; noa 200 cts, 0.0460 sec
 soc0p:
 pro1a: amp 46 cts, 0.0330 sec; noa 1 cts, 0.0030 sec
 soc1a: delete ADS:  ~0.0015 sec


Dynamic Randles Model
Dynamic Hysteresis Model
Coulombic Efficiency


## Calibration

The goal of calibration is to accurately predict battery charge and current.   Knowing current it is convenient to also display the time remaining either to discharge or to charge.

Tuning the Temperature Sensor
Tuning the Voltage Sensors
Tuning the Current Sensors
Tuning the Coulombic Efficiency
Tuning the Model
An observer of the raw activity on voltage and current measurement will notice that battery terminal voltage Vb will bounce dramatically as the input current Ib changes.   The characteristic of interest for charge is the relationship between the open circuit voltage Voc and the State-of-Charge (SoC) when nothing is changing at the terminals.   Knowing that, and having an accurate model of the things that change when current changes, allows predicting Voc(SoC) when current changes.   Then the State-of-Charge is known.

What's its used for is up to the designer.   This application uses it as a surrogate for a third current sensor to isolate current sensor failures.
There are no intermediate sensors in the battery chemistry to help with that process.   That would be nice.

The two main model pieces are the charging dynamics and the hysteresis.   These may be represented by a dynamic linear model usually called the Randles model after a man named Randles and a dynamic non-linear hysteresis model of some sort.    I relied on industry literature to discover this and to provide me with a starting point on their design.   The construction I discuss elsewhere.

The method I have chosen is to embed the models in the application.   Therefore I replicate the models off-line and tune using data to drive the models and compare the result to data.   Assuming the sensors themselves are already calibrated, then using known Ib to drive the models I look to match Vb.  The application generates a ton of data.  To collect good fidelity of the Randles model, at least every 0.3 seconds is required to prevent aliasing the fast time constant.   I use 0.2 seconds.   The default execution speed of the application is 0.1 seconds (Dr=100 ms) for best fidelity during operation.  I leave that and simply collect the data slower than it is created (DP=2).   The parameter DP sets how often to print the application data out UART while it runs at 0.1 s (100 ms)  seconds update.  Then the off-line model may run its dynamics at 0.2 s without aliasing.   If aliasing is observed by divergence of the Randles model, it has been mis-tuned.   So long runs at 0.2 s are collected and drive the model. 

There are two processes performed simultaneously.   Because for the model tuning process to work, the off-line model must agree with the on-line application.   The Baseline of Figure 5 is declared as such because the model may be seen to agree with the application by overlaying data.   Then with that agreement assured, and the same 'Ib' input to both then the model can be tuned to make the 'Vb' agree.

The off-line data is marked as '_ver' for 'verification.'   In Figure 5you can see that 'ib_charge_s_ver' overlays 'ib_charge_s' which intern overlay 'ib_in_s' and 'ib_sel.'   That last variable, 'ib_sel' is the final, selected value of current that is driving everything.

This verifies the integrity of the models required for the matching process to be valid.
A glossary is included in Appendix 1.   Nomenclature to help.  Figure 4  Is a Functional Block Diagram of the data match process.   There are many modes to run this match.   The one illustrated is one that inputs current to all paths and outputs voltages from each block.   It is useful for troubleshooting to sometimes drive one path with a voltage from another path but the simplest one shown here is the most straightforward for data match discussed in this tuning section.

Before beginning tuning I check the following matches for the four Mon/Sim legs of Figure 4:
    • 'dv_hys'	'dv_hys_s'	'dy_hys_ver'		'dv_hys_s_ver'
    • 'dv_dyn'	'dv_dyn_s'	'dv_dyn_ver'		'dv_dyn_s_ver'
    • 'soc'		'soc_s'		'soc_ver'		'soc_s_ver' 
    • 'vb'		'vb_s'		'vb_ver' 		'vb_s_ver'

The characteristics most readily available to tune a match between off-line vb, 'vb_s_ver' and on-line vb, 'vb' are
    • Resistance values in the off-line Randles models: r_ct, r_dif, r0
    • Time constant values in the off-line Randles models:  tau_ct, tau_dif
    • Hysteresis tables in the off-line models: x_dv, y_soc, t_r, t_dv_max, t_dv_min
    • voc(soc) table in off-line models:  t_x_soc, t_y_t, t_voc
    • Diffusion time constant in the off-line models: tau_dif 
    • Battery capacity, NOM_UNIT_CAP
    • Coulombic charge efficiency, COULOMBIC_EFF

Quick studies are possible using simple scaling parameters.   The most useful are
	Resistance scaling of all the resistance values in the off-line Randles models: 'sres_in'
	Time constant scaling in the off-line Randles models:  stauct, staudif
	Hysteresis scaling of the off-line models: 'scale_hys_mon_in' and 'scale_hys_sim_in'

The initial generation of these model elements using a Battleborn prototype system on my RV truck was complicated, time consuming, and full of guess work until data congealed in a match.   Eventually, small changes in test parameters and special testing to isolate parameter effects won out.
The approach of confirming all four legs of this paradigm match, drives quality into the process.
This CHINS installation does not yet know what the Coulombic Efficiency and capacity are.   Several runs and recharges to full discharge are required to triangulate those values.    For now, the Battleborn observations are used:  0.985 and 105 A-h.

Black Box Paradigm
To review and add perspective:  the tuning process may be considered a black-box process.

There are three signal measurements.  'b' stands for 'bank' consolidation of multiple batteries in a bank
    • Current into the battery terminals, Ib
    • Voltage at the battery terminals, Vb
    • Temperature of the battery chemistry, Tb

There are four models each with two components
    • Application BatteryMonitor Randles
    • Application BatteryMonitor Hysteresis
    • Application BatterySim Randles
    • Application BatterySim Hysteresis
    • Off-line Python BatteryMonitor Randles
    • Off-line Python BatteryMonitor Hysteresis
    • Off-line Python BatterySim Randles
    • Off-line Python BatterySim Hysteresis
      


There are many characteristics to observe
    • Capacity, Coulombs from counting – observe exhaustion and saturation for various conditions to triangulate
    • Current step response – initial step and subsequent settling times to extract time constants.   Hold at least 3x longer than hysteresis time constant (~45 minutes)
    • Predicted voc(soc) – hysteresis time constant and resistance values
    • Predicted voc(soc) – mismatch between discharge-charge
    • Long-term settling – hysteresis time constant (=1/RC)

There are an infinite number of data conditions to observe, such as
    • Moment of saturation – accumulated current measurements should be zero for a perfect model match result.  Measures Coulombic Efficiency and confirms sensor calibrations
    • Moment of exhaustion – need to know minimum Coulomb state at all operating temperatures
    • Slow discharge-charge – schedule development because hysteresis small
    • Rapid discharge-charge
    • Step current changes – detection of gains especially Randles resistance
    • Difference between discharge-charge of the voc(soc) curve.   It should be zero and flat for a perfect model match result.  Adjust dynamics, scalars, and schedules.
    • Leverage the fact that the Randles model always passes output current equal to input current
    • Leverage the fact that the linear Randles model is linear and therefore symmetrical for discharge-charge
    • Leverage the fact that the hysteresis will be near zero for very slow discharge rates
      
The task is to match the predicted Vb of the off-line Sim model to the measured Vb under all operating conditions.  And ensure that there remains a perfect match between the four component models at all times.   Then we can use the models for anything.

A perfect match won't be possible.   The difference between discharge-charge of voc(soc), Volts at State-of-charge,  is a reasonable measure of overall success.

Initial Tuning
The very first step is to calibrate the sensors.   After that we assume they are valid.   It will become obvious if they are incorrect, unfortunately after a lot of work.   So do that first, carefully.

The next first step is to make sure that all the model components are consistent with each other.    The real world data does not have to match at this point.   There are four versions of the Randles model and four versions of the hysteresis model.   The application has two of each, one set called in the BatteryMonitor object and the other set called in the BatterySim object.

The off-line model has two of each and every effort was made to duplicate the C++ code of the application in Python code of the off-line model.   The plot sets carefully overlay all four models to allow them to be validated easily.

Driving the models with data is a time tested way to drive model verification.   Collect data from the application and drive the models and view matches.   So the basic verification is to collect Ib, Vb, and Tb.  Drive the off-line model with the Ib and Tb data and over-plot Vb.
Data Collecting
There are two ways to gather data: transient verbose continuous streaming into CoolTerm at 0.1 or slower intervals and steady-state fault logging at 30 minute intervals.   Interestingly, the slow fault logging is sufficient for most tuning except Randles model.

The Randles model is also the one that makes most sense to tune first, since it is linear and symmetrical.   After it is tuned, assuming it doesn't vary with operating condition, it is relatively easy to extract the hysteresis.

To generate enough degrees of freedom for tuning we will need some charging, some discharging, rapid and slow, all state-of-charge. 

Tuning Step 1
The results of initial tuning are shown in Figure 5, Figure 6, and Figure 7 for transients soc=0.9, transients soc=0.8, and steady-state fault log from soc=0.9-1.0.   For these I've made following initial changes:
    • schedule tune to center about steady-state
    • Randles tune all resistances by 1.6x Battleborn
    • Randles tune tau_dif to be 0.3x Battleborn
    • Hysteresis variable
The hysteresis transfer function is of the form:

	tau = Rh * gain_scalar / (Rh * Ch * tau_scalar * s + 1)

So we need to make two adjustments to match data.   These I call s_hys, and s_cap.   The first is the gain_scalar and the second is the tau_scalar.    Baseline is Battleborn:

```
y_soc_bb  = [0.0,   0.5,  1.0],  soc brkpt
x_dv_bb   = [-0.7, -0.5,  -0.3,  0.0,   0.15,  0.3,   0.7], dv_hys brkpt
t_r_bb    = [0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
		 0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
		 0.016, 0.016, 0.016, 0.005, 0.010, 0.010, 0.010]; Rh values
t_dv_max_bb  = [0.7, 0.3,  0.15]
t_dv_min_bb  = [-0.7, -0.5, -0.3]
hys_cap =  3.6e3				tau_null = 3.6e3 * 0.005 = 18 s;  Ch value

Condition
Randles s_res
Randles
s_tau_dif
Hysteresis
s_cap_chg
Hysteresis
s_hys_chg
Hysteresis
s_cap_dis
Hysteresis
s_hys_dis
0.9-1.0 Flt
1.6
0.3
90
8
10
2
0.9 Trans
1.6
0.3
90
4
10
0.7
0.8 Trans
1.6
0.3
4
1.5
40
2
```
Table 1: CHINS Battery Initial Tuning #1 – Non-schedule data

The Randles values in the table apply to all the Randles resistances simultaneously and to the diffusion time constant.  The hysteresis values in the Table apply to t_r and hys_cap, (Rh and Ch).







The end result of this tuning step is a difference between discharge-charge voc(soc) of +/-0.05 volts – see the red and green lines voc_stat_chg_ver and voc_stat_dis_ver.   This is very good and would cause an uncertainty of about 20% soc.   This should make a useful reference model.

EKF differences, not shown, should sort themselves out after re-running the application with these changes.  Anyway, you should not spend much time looking at EKF until the rest of the application is tuned and re-run.
Tuning Step 2
The challenge is to turn these results into hysteresis schedules.  This is denoted as myCH_Tuner=3 in the Python code.  The CHINS values are:

y_soc_ch  = [0.8,   0.86,  0.92,  0.95];                          soc brkpt
x_dv_ch   = [-0.1,    -.05,  -.04,  0.0,  0.06,  0.07,  0.1];  dv_hys brkpt
t_r_ch    = [0.004,  0.004,  0.4,  0.4,  0.4,   0.010, 0.008,
		 0.003,   0.003,  0.4,  0.4,  0.003, 0.003, 0.003,
		 0.003,   0.003,  0.4,  0.4,  0.003, 0.003, 0.003,
		 0.03,    0.03,   0.4,  0.4,  0.08,  0.08,  0.08];     Rh values
t_dv_max_ch  = [0.3,  0.3,  0.3,  0.3]
t_dv_min_ch  = [-0.3, -0.3, -0.3, -0.3]
hys_cap =  1e4				tau_null = 1e4 * 0.4 = 4000 s;  Ch value
Table 2: CHINS Initial Hysteresis Tuning - Changed Later




and the results results are shown in Figure 8, Figure 9, and Figure 10.









Now the end result of this tuning step is a difference between discharge-charge voc(soc) of between 0.05 and 0.10 volts – see the red and green lines voc_stat_chg_ver and voc_stat_dis_ver.   This is nearly as good as tuning attempt 1 and would cause an uncertainty of about +/- 20% soc.  This should make a useful reference model.

But it could be improved.  We can never match charging transients because the hysteresis model lacks a restriction on the supply current into the hysteresis capacitor.   See the red ellipses on the figures.   This should be added in a future version.   Also the 0.9-1.0 tuning lacks transient data to properly tune.   This should be gotten later.    Also we don't have the full state-of-charge range.   The modeling and scheduling is especially important near zero charge to alert the user that they're running low.   This should be gotten later.  Finally the hysteresis capacitance is 10000 f.  The Battleborn was 3600 and the maximum value to allow 15 A of the 30 A charge controller to charge at 0.0014 v/s observed is about 10000 f.   So next time around I would tune everything with 3600 just to be foolishly consistent.

When the restriction on hysteresis supply is added later, there are a couple other improvements needed:
    • Set tau_ct to the present tau_dif.  The present tau_ct is too fast to be useful and doesn't seem to exist anyway.
    • The present tau_dif is probably the 400 second hysteresis time constant.   So delete it and use the non-linear hysteresis model instead.   Since that is non-linear than tau_dif becomes undefined
    • This allows minimum update time to be as long as 10 seconds (tau_ct ~ 20 for CHINS).   This further allows much more prolonged data capture and faster design times.   I would still execute the monitor at 0.1 seconds to capture all the 1 Hz bandwidth sensed current changes.  If we miss peaks the accumulated Coulomb counting errors would be large.

EKF differences, not shown, should sort themselves out after re-running the application with these changes.  Anyway, you should not spend much time looking at EKF until the rest of the application is tuned and re-run.   We will need to keep an eye on the EKF during these early times because the voc(soc) schedule for CHINS is flat at high soc.  This EKF's ability to solve to a flat line is TBD.   A pure numerical solver would not be able to.   The EKF has a soc input to favor it 'probably' close to the Coulomb counter.


A couple tips on tuning hysteresis:
    • The resistance is high around null to create a hysteresis band.   Tau = R*C so check that Tau is about 2.5 hours, or 7500.   This will have to be tuned again to match the real world battery relaxation time period 
    • use the right side to tune the shape of increasing dv / current and the left for decreasing
    • Set s_hys_chg/dis = 0 to see what the scripts think the ideal R is
    • Need 45 minute transients to see the slow ~500 second hysteresis time constant.
    • You can calculate requirements for the hysteresis capacitance, C.   When running with s_hys_chg/dis = 0 as described a couple lines previous, the required rate of change of voltage is available.  The maximum supply current is known.   Therefore the maximum C that allows charging to keep up with the required (check =observed) voltage is known.


Let's revisit the purpose of this tuning.  The accuracy of the things we're tuning affect how well the EKF tracks reality for purposes of detecting sensor faults.   The thresholds of faults have been tuned out to +/-0.2 soc and beyond due to unmodeled variations.   So the <0.05 V tuning accuracy that we observe above are within the needs of the fault detection.  This level of effort is needed to achieve the +/-0.2 result on thresholds, however.

It's fair to ask if the sensor redundancy is worth the effort.   I am considering simplifying to one current sensor and simply throwing a generic, displayed fault light when we're sure there's something wrong.

Schedule Tuning – at the end
The original process used .xls/.ods spreadsheets.   Lately I used the plots from tuning hysteresis.   When the red and green curves above were aligned after tuning hysteresis, I would tune voc(soc) to run the schedule line (black) through the middle of the red and green.   Spreadsheets not required.
Yet More Tuning
I was able to eliminate the charging supply current problem mentioned with the 'ellipses' in the previous section.  I scheduled a scalar on the ib input to the hysteresis model.   The scalar is a function of dv_hys like the resistance model is and works well.    Here's the final resistance / scalar schedules, Table 4, where t_r is the resistance schedule and t_s  is the scalar schedule.   The suffix '1' is for the CHINS battery.   '0' is for the Battleborn battery that did not need additional scaling.  The t_s schedule for Battleborn is unity 1.

And the final tuned transients appear below in Figure 11, Figure 12, and Figure 13.   Notice that the 0.9 – 1.0 fault run map is missing and a low content transient is added.   The reconstruction of transient behavior from 30 minute fault data using CompareFault.py does not do a good job of capturing the Randles dynamics so the hysteresis modeling is poor.   Eventually I modified the  Python model to simply run to minimum or maximum hysteresis depending on negative or positive sign of charge current.  The Randles model in the reconstructed mode is ignored.   So don't use it to tune hysteresis logic or schedules.   But it can be used to estimate voc_soc scheduling.










Final accuracy.


Condition
Randles r_0
Randles
r_ct
Randles
tau_ct
all
0.0138
0.0059
24.9
Table 3: CHINS Battery Final Tuning

Schedule is flat.

self.cap1 = 1e4  # scaled later
t_soc1 = [.47, .75, .80, .86]
t_dv1 = [-.10,   -.05,   -.04, 0.0,  .02,  .04, .05,   .06,   .07,   .10]
schp4 = [0.003,  0.003,  0.4,  0.4,  0.4,  0.4, 0.010, 0.010, 0.010, 0.010]
schp8 = [0.004,  0.004,  0.4,  0.4,  0.4,  0.4, 0.4,   0.4,   0.014, 0.012]
schp9 = [0.004,  0.004,  0.4,  0.4,  .2,  .09,  0.04,  0.006, 0.006, 0.006]
t_r1 = schp4 + schp8 + schp8 + schp9
t_dv_min1 = [-0.3, -0.3, -0.3, -0.3]
t_dv_max1 = [0.3, 0.3, 0.3, 0.3]
SRs1p4 = [1.,    1.,    .2,   .2,   .2,   .2,  1.,    1.,    1.,   1.]
SRs1p8 = [ 1.,    1.,    .2,   .2,   .2,   1.,  1.,    1.,    1.,   1.]
SRs1p9 = [ 1.,    1.,    .1,   .1,   .2,   1.,  1.,    1.,    1.,   1.]
t_s1 =  SRs1p4 + SRs1p8 + SRs1p8 + SRs1p9
Table 4: CHINS Final Hysteresis Scheduling
t_x_soc1 = [-0.100,  -0.060,  -0.035,   0.000,   0.050,   0.100,   0.140,   0.170,   0.200,   0.250,   0.300,   0.400,   0.500,   0.600,   0.700,   0.800,   0.900,   0.980,   0.990,   1.000]
t_y_t1 = [5.,  11.1,   11.2,  40.]
t_voc1 = [4.000,   4.000,   4.000,   4.000,   4.000,   4.000,   4.000,   5.370,  10.042,  12.683,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700,
          4.000,   4.000,   4.000,   4.000,   4.000,   4.000,   6.963,  10.292,  12.971,  13.025,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700,
          4.000,   4.000,   4.000,   9.000,  12.453,  12.746,  12.869,  12.931,  12.971,  13.025,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700,
          4.000,   4.000,   4.000,   9.000,  12.453,  12.746,  12.869,  12.931,  12.971,  13.025,  13.059,  13.107,  13.152,  13.205,  13.243,  13.284,  13.299,  13.310,  13.486,  14.700]

Table 5: CHINS Final voc(soc)

Finally
Make sure to test the logic after making these extensive tuning changes.    Following are some of the problems I found and how I fixed them.
    • soc_ekf != soc steady-state.   In the model with tight constraints, there's no reason the EKF should differ at steady-state.  When I entered the new voc_soc schedule it had a long flat section.   Normally, such flat sections, or valleys even, cause solvers to be indeterminate. The EKF does a fair job of finding a solution because it has soc_ekf as a reference, an independent Coulomb counter, in addition to the voc_stat error.   But it's not perfect.   I added a 0.01 change at either end of the flat:  -0.01 at 0.80 and +0.01 at 0.98, where 0.00 at 0.90.
    • The Bs and Bm talk adjustments did not have the desired effect.   When compiled, the proper chemistry behavior occurred.   But changing the chemistry with adjustment did not engage the desired chemistry.  The class structure of the C++ code was missing the proper inheritance between the stored chemistry and the point of use.
Retune Battleborn
After learning more with the CHINS tuning it was worth another look at the Battleborn tuning.

I ran a 15 A discharge and 30 A charge around soc=0.8 on the truck with the newly installed Argon Beta configuration.   Warning:  these transients have excessive noise because I forgot a 47 micro-Farad capacitor on the Vb sensor input.   This was corrected later and illustrates another need for testing to verify proper implementation both hardware and software.

As-installed are shown in Figure 14.  You see that the vb_s_ver traces all trend below vb.   It appears that the initial calibration curves have shifted 0.11 volts higher.   I double and triple-checked the Vb calibration to be sure it is correct now.   To investigate I did not reinstall the Photon soc0 device to recheck because that device is hard-wired into the house and also was re-calibrated at that new install.  So I don’t know if an error was made in initial calibration or the battery has shifted over time.


   I adjusted the voc(soc) curve output biases and re-ran the Python scripts:  Figure 15,  Figure 16, and Figure 17 where now I also focused on the discharge and charge phases of the transients – the short initial minutes when Randles dynamics are active.





The overall view now show a match of vb_s_ver and vb.   Moving on to the focused views we can now observe that the Randles resistance values need tuning to capture the initial jump better.  I increased r_0 to from 0.0046 Ohms to 0.0113 Ohms to capture the initial step and then reduced r_ct from 0.0077 Ohms to 0.001 Ohms to preserve the overall resistance sum of 0.0123 Ohms.  It is apparent by inspection that this will match up the discharge traces but there is a non-linear trend obvious in the charge trace caused by something else.

The something else is the hysteresis model that I proceeded to tweak by reducing resistance in the hysteresis to allow voltage to rise faster.   The final results are shown in Figure 18, Figure 19, and Figure 20.   The hysteresis tweak was small and arbitrary.   Mainly my goal was to make the smallest possible tweak because the dataset is so small.   I pulled the soc=1 curve down to 0.8 and reduced the resistance slightly while increasing the maximum limit on modeled hysteresis to 0.2 volts.





There is an artifact in the tuned transients just after 5800 seconds.   This is caused by the data reduction program interpolating incoming data and I ignore it for now.

The final schedules are below.   The highlights indicate what changed from the original.  I saw no reason the change tau_ct with this data set.

Condition
Randles r_0
Randles
r_ct
Randles
tau_ct
all
0.0113
0.0010
83
Table 6: Battleborn Battery Final Tuning

self.cap1 = 1e4  # scaled later
t_dv0 = [-0.7,   -0.5,  -0.3,  0.0,   0.15,   0.3,   0.7]
t_soc0 = [0, .5, .7]
t_s0 = [1., 1., 1., 1., 1., 1., 1.,
        1., 1., 1., 1., 1., 1., 1.,
        1., 1., 1., 1., 1., 1., 1.]
t_dv_min0 = [-0.7, -0.5, -0.3]
t_dv_max0 = [0.7,  0.3,  0.2]
t_r0 = [0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
        0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
        0.016, 0.016, 0.013, 0.005, 0.007, 0.010, 0.010]
Table 7: Battleborn Final Hysteresis Scheduling
t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.99, 0.995,
            1.00]
t_y_t0 = [5., 11.1, 20., 30., 40.]
t_voc0 = [4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
          4.00, 4.00, 4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
          4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
          4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
          4.00, 4.00, 4.00,  4.00,  10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19, 13.20, 13.72, 14.50]
                 + 0.11
Table 8: Battleborn Final voc(soc) Scheduling.  ‘+ 0.11’ Indicates Permanent Adjustment to Output 
Values of Table (VM=VS=0.11)

There are a couple curiosities in the final tuning results.
    • Very little hysteresis in Battleborn after relaxing to 0 current.   There was no need to build flats into the schedules as in the CHINS (see the resistance = 0.4 values around 0 current in Table 4.)   We may yet need that with more data. 
    • Double losses in CHINS:   r_0 and r_ct.  They have purposely designed for low cost – are increased losses a result?
    • Shift of voc(soc) for Battleborn?   Time will tell.
    • Future Battleborn results will demonstrate that adding the missing 47 micro-Farad capacitor fixed the Vb noise.

Again, the hysteresis accuracy contributes to better fault detection and isolation by improving the accuracy of the EKF model.   Coulomb counting of accurately measured current is what drives accuracy of Amp-Hours display on the device.   The calibration performed here does not verify the accuracy of Coulomb Counting.

Since voltage sensing resets Coulomb Counters at saturation, memory of accumulated current measurement error gets lost.    For this reason, an ‘infinity counter’ is being added that does not get reset.  It will provide feedback to a user willing to dive deep and monitor that counter.   Errors in counting accumulated over many discharge / charge cycles indicate either
    • Current sensing error – CURR_BIAS_*, CURR_SCALE_*, SHUNT_GAIN (DA, DB, SB, SA, SG)
    • Coulombic efficiency error – coul_eff ( not adjustable)
I believe that errors in the calibration are so large and variable of the battery so frequent that trying to tune the Coulombic efficiency more than the crude attempts contained in the code are a waste of time.   The ‘game’ is to satisfactorily tune the current sensors to predict when the battery may die overnight withing about a half hour.   And this will be difficult to prove, further supporting the claim that the fine tuning of Coulombic efficiency is a waste of effort.   Despite the difficulty, it must be attempted.   Amp-Hours remaining will be added to data collection and for history monitoring.
VBAT
It seemed like a good idea to monitor the VBAT EEPROM backup power battery voltage to alert the user when the battery dies.   This applies to the Photon only.   Argon uses EERAM device with a capacitor.   The VBAT monitor did not work because the ADC converters appear to discharge the battery when connected to VBAT.   The documentation for Particle device devices say the converters draw about 1/10 the power as the VBAT input.   For some reason, this appears to be incorrect.   The Photon is used inside the house while the Argon is used in the truck RV.   The need for VBAT failure detection inside the house is unimportant.
Nomenclature
Capital letters always denote 'bank' parameters, what may be observed in the installation with several 100 A-h batteries in series-parallel configuration.   'nP' and 'nS' refer to the number of parallel and serial batteries respectively.   If the bank is something other than 100 A-h batteries nP and nS may be scalar values.  EG one 200 A-h battery would be nP=2, nS=1.   Also, capacity may be scaled as well though it's most convenient to study these systems in 'units' of 100 A-h battery.  My convention is the use the capacity scalar to manage the life effects.   EG a new battery at 105% capacity would use scalar 'NOM_UNIT_CAP' and 'Sc' ('NOM_UNIT_CAP' and 'scale_in' in off-line).   If these conventions are followed rigidly, then any data collected on the installed application may be converted into lower-case 'unit' parameters by simply multiplying and dividing by nS and nP making sure the scaling parameters are duplicated in the off-line model as parameter inputs input by hand.


Recommendations
    1. Procure TSC2010-IDT chips going forward.
    2. Procure 1% 1/4 watt resistors going forward.

Appendix 1.   Nomenclature

Application
Off-line
Description









tau_ct
tau_ct
Battery chemistry charge transfer time constant, s
tau_dif
tau_dif
Battery chemistry diffusion time constant, s









r_ct
r_ct
Battery chemistry charge transfer resistance, Ohm
r_dif
r_dif
Battery chemistry diffusion resistance, Ohm
r_sd
r_sd
Battery chemistry lumped parameter Randles model equivalent resistance, Ohm
r0
r0
Battery chemistry direct resistance, Ohm







sres_in
Scalar applied to all Randles model resistances for off-line tuning
































Appendix 2:  Regression Suite
    • All these must self initialize – correct the application until they do
    • You have to start and stop data recording.  Use ctr-R and shift-ctr-R in CoolTerm.   You may use Arduino Serial but it is a little messier because it doesn't support multiple windows.
    • Open two talk windows (ctr-T in CoolTerm) and copy-paste the two lines in the talk windows (start and reset in items below)
    • Initiate by starting a recording (ctr-R) and running start line in first talk. Those without a second line self-terminate. You still have to stop recording (shift-ctr-R).
    • Terminate by watching for reset of failure condition (or wait long enough) and running second line in second talk.
    • Move to pyCharm and run 'CompareRunSim' after proper comment/uncomment in 'data_file_old_txt' assignment.
    • Running with a bare device, need to set the modeling dscn bits 16, 32, 64, and 128. (Add 240 to whatever Xm you normally run with, e.g. 7 becomes 247 for fully bare and modeled.)
    • The second lines below are to reset.  I recommend opening a second transmit window if using CoolTerm to have these ready to go.
    • Copy and paste the lines into CoolTerm 'transmit' windows (ctr-T).  Or run in line mode (Options – Terminal – Line Mode) and use communication bar like in Arduino Serial App still copy and paste.
    • Example set of plots, from offSitHysBms begin here:   Figure 21.
       
       ampHiFail:
           start:  	Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;
           reset: Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
            ▪ Should detect and switch amp current failure (reset when current display changes from '50/diff' back to normal '0' and wait for CoolTerm to stop streaming.)
            ▪ 'diff' will be displayed. After a bit more, current display will change to 0.
            ▪ To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).

rapidTweakRegression:
      start:  Ff0;HR;Xp10;
        ◦ Should run three very large current discharge/recharge cycles without fault
        ◦ Best test for seeing time skews and checking fault logic for false trips
rapidTweakRegression vA CH: Bm1;Bs1;SQ1.127;Sq1.127;Ff0;HR;Xp10;  for CompareRunRun.py Argon vs Photon builds.   This is the only test for that.
      
	offSitHysBms: operate around BMS off, starting above from about 11v, go below, come back up.  EXAMPLE PLOTS START HERE:  Figure 21.
		start:  Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;
     		reset:  XS;v0;Hd;Xp0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;
            ▪ Best test for seeing Randles model differences.  No faults
            ▪ Only test to confirm on/off behavior.  Make sure comes back on.
            ▪ It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shuttoff.
            ▪ reset_s_ver because device reset on power up
          offSitHysBms CH:  Ca0.014;
          
	triTweakDisch:
		start:  Ff0;D^0;Xp0;v0;Xm15;Xtt;Ca1.;Ri;Mw0;Nw0;MC0.004;Mx0.04;NC0.004;Nx0.04;Mk1;Nk1;-Dm1;-Dn1;DP1;Rb;Pa;Xf0.02;Xa-29500;XW5;XT5;XC3;W2;HR;Pf;v2;W2;Fi1000;Fo1000;Fc1000;Fd1000;FV1;FI1;FT1;XR;
		reset:  v0;Hd;XS;Dm0;Dn0;Fi1;Fo1;Fc1;Fd1;FV0;FI0;FT0;Xp0;Ca1.;Pf;DP4;
            ▪ Should run three very large triangle wave current discharge/recharge cycles without fault.  
          
	coldStart:
		start:  Ff0;D^-18;Xp0;Xm247;Fi1000;Fo1000;Ca0.93;Ds0.06;Sk0.5;Rb;Rf;Dr100;DP1;v2;W100;DI40;Fi1;Fo1;
     		reset:  DI0;W10;v0;W5;Pf;Rf;Pf;v0;DP4;D^0;Ds0;Sk1;Fi1;Fo1;Ca0.93;
            ▪ Should charge for a bit after hitting cutback on BMS.   Should see current gradually reduce.   Run until 'SAT' is displayed.   Could take ½ hour.
            ▪ The Ds term biases voc(soc) by delta x and makes a different type of saturation experience, to accelerate the test.
            ▪ Look at chart 'DOM 1' and verify that e_wrap misses ewlo_thr (thresholds moved as result of previous failures in this transient)
            ▪ Don't remember why this problem on cold day only.

 	ampHiFailFf:
		start:  Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;
        		reset:  Hs;Hs;Hs;Hs;Pf;Hd;Ff0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
    • Should detect but not switch amp current failure. (See 'diff' and current!=0 on OLED).
    • Run about 60s. Start by looking at 'DOM 1' fig 3. No fault record (keeps recording).  Verify that on Fig 3 the e_wrap goes through a threshold ~0.4 without tripping faults.  

 	ampLoFail:
		start:  Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm-50;Dn0.0001;
         		reset: Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
    • Should detect and switch amp current failure.
    • Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).

	ampHiFailNoise:
		start: Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;DT.05;DV0.05;DM.2;DN2;W50;Dm50;Dn0.0001;Ff0;
         		reset:  Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;
    • Noisy ampHiFail.  Should detect and switch amp current failure.
    • Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).

	rapidTweakRegression40C: 
		start:  Ff0;HR;D^15;Xp10;
               	reset:  D^0;
    • Should run three very large current discharge/recharge cycles without fault

 	slowTweakRegression:
		start:  Ff0;HR;Xp11;
		reset:  self terminates
    • Should run one very large slow (~15 min) current discharge/recharge cycle without fault.   It will take 60 seconds to start changing current.

	satSit: 
		start:  Ff0;D^0;Xp0;Xm247;Ca0.993;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10;XT10;XC1;W2;HR;Pf;v2;W5;XR;
		reset:  XS;v0;Hd;Xp0;Ca.9962;W5;Pf;Rf;Pf;v0;DP4;
    • Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.
    • operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat
    • satSit CH:  Ca0.993

	flatSitHys:
    		start:  Ff0;D^0;Xp0;Xm247;Ca0.9;Rb;Rf;Dr100;DP1;Xts;Xa-81;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;
     		reset:  XS;v0;Hd;Xp0;Ca.9;W5;Pf;Rf;Pf;v0;DP4;
    • Operate around 0.9.  For CHINS, will check EKF with flat voc(soc).   Takes about 10 minutes.
    • Make sure EKF soc (soc_ekf) tracks actual soc without wandering.

	offSitHysBmsNoise:
  		start:  Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;v2;W5;XR;
     		reset:  XS;v0;Hd;Xp0;DT0;DV0;DM0;DN0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;
    • Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.
    • operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset 
    • Make sure comes back on.
    • It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shuttoff.

	ampHiFailSlow:
		start:  Ff0;D^0;Xm247;Ca0.5;Pf;v2;W2;Dr100;DP1;HR;Dm6;Dn0.0001;Fc0.02;Fd0.5;
         		reset:  Hd;Xp0;Pf;Rf;W2;+v0;Dr100;DE20;Fc1;Fd1;Rf;Pf;
    • Should detect and switch amp current failure. Will be slow (~6 min) detection as it waits for the EKF to wind up to produce a cc_diff fault.
    • Will display “diff” on OLED due to 6 A difference before switch (not cc_diff).
    • EKF should tend to follow voltage while soc wanders away.
  
 	vHiFail:
		start:  Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;
      		reset:  Dv0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;
    • Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.
    • To diagnose, begin with DOM 1 fig. 2 or 3.   Look for e_wrap to go through ewl_thr.
    • You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.

 	vHiFailFf:    
		start: Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;
         		reset: Dv0;Ff0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;
        ◦ Run for about 1 minute.
        ◦ Should detect voltage failure (see DOM1 fig 2 or 3) but not display anything on OLED.

 	pulseEKF: Xp6 # TODO: doesn't work now.

 	pulseSS: Xp7
        ◦ Should generate a very short <10 sec data burst with a pulse.  Look at plots for good overlay. e_wrap will have a delay.
        ◦ This is the shortest of all tests.  Useful for quick checks.
  
 	tbFailMod:
start:Ff0;D^0;Ca.5;Xp0;W4;Xm247;DP1;Dr100;W2;HR;Pf;v2;Xv.002;Xu1;W200;Xu0;Xv1;W100;v0;Pf;
        		reset:Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;
        ◦  Run for 60 sec.   Plots DOM 1 Fig 2 or 3 should show Tb was detected as fault but not failed.
          
tbFailHdwe:  This script sometimes doesn't work but test performs fine manually
start:Ff0;D^0;Ca.5;Xp0;W4;Xm246;DP1;Dr100;W2;HR;Pf;v2;Xv.002;W50;Xu1;W200;Xu0;Xv1;W100;v0;Pf;
		reset: Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;
        ◦ Should momentary flash '***' then clear itself.  All within 60 seconds.
        ◦ 'Xp0' in reset puts Xm back to 247.
        ◦ 



Special Testing (e.g. Gorilla Testing)

Scale battery
App:  NOM_UNIT_CAP in local_config.h
Python:  scale_in in CompareRunSim.py or CompareRunRun.py (+Battery. NOM_UNIT_CAP)
Notes:  talk 'Sc' scales the on-board model in BatterySim only, not BatteryMonitor	
 	dwell noise Ca.5:
  Ff0;HR;Di.1;Dv0;Dr100;DP1;v4;Ca.5;DT.05;DV0.05;DM.2;DN2;
    Xp0;v0;Hd;DT0;DV0;DM0;DN0;Di0;Dv0;DP4;
  # Dwell for >5 minutes. This operating condition found EKF failure one time.  Led to multi-framing the EKF (update time = 2.0).  Before change, update time of EKF was so short (0.1) that internal filter parameters were truncating. When plotted shows EKF staying on point, not wandering off.  Beware, a dwell has most parameters changing slowly.  Auto-scaling in python plots combined with truncation in data stream (but not python) makes some interesting artifacts.

 dwell Ca.5:
  Ff0;HR;Di.1;Dr100;DP1;v4;Ca.5;
    Xp0;v0;Hd;DT0;DV0;DM0;DN0;Di0;Dv0;DP4;
  # Dwell for >5 minutes. This operating condition found EKF failure one time.  Led to multi-framing the EKF (update time = 2.0).  Before change, update time of EKF was so short (0.1) that internal filter parameters were truncating. When plotted shows EKF staying on point, not wandering off. Auto-scaling in python plots combined with truncation in data stream (but not python) makes some interesting artifacts.








Appendix 3.   Links
'Truck Camping ALL STUFF.gsheet'	Multiple tabbed spreadsheet of parts manifest, settings, hardware schematics, and what all. https://docs.google.com/spreadsheets/d/19PZ2GmKIlw14doqwePp9xxWH96coLm8WmXdIwlzdxjE/edit#gid=167481175




## Powering your device

The system is designed to be powered completely either from USB hooked to phone or device or from 12 V dc connector.   Normally in service the battery bank supplies 12 V and no USB is used.   The device saves fault information (EERAM 47L16) for cases when the battery banks management system powers off.  If the battery bank is off you can power with phone or device to extract information using UART terminal.   There are two UART terminals:  USB and HC-06 bluetooth.

## Redo Loop

***********************
Simple production mode:
Welcome to Particle Workbench - Configure for device - (pick appropriate OS) - (pick device name set using Particle app)
Make edits
Press 'check' symbol when a .h or .cpp file is open and highlighted
Press 'lightning' symbol
'Talk' using CoolTerm

More complex to deal with issues (always flashes despite device name difference)
Ctrl-Shift-P - Particle:Clean Application and Device OS (local)
Ctrl-Shift-P - Particle:Compile Application (local) or Check button in Visual Studio upper rc when select a source file
  Ctrl-Shift-P - Particle:Compile Application and Device OS (local) first time
Ctrl-Shift-P - Particle:Cloud Flash or Ctrl-Shift-P - Particle:Local Flash
  Ctrl-Shift-P - Particle:Flash application and Device OS (local) first time
Ctrl-Shift-P - Particle:Serial Monitor or CoolTerm(saves data)
  'Talk' function using Monitor or CoolTerm(saves data)

Desktop settings
    .json has "particle.targetDevice": "proto"
    local_config.h has
        const   String    unit = "proto";

Laptop settings.json has  "particle.targetDevice": "soc0"
    .json has "particle.targetDevice": "soc0"
    local_config.h has
        const   String    unit = "soc0";

On laptop (same as desktop)
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

## Device Interfaces

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

### Amp circuit 'amp'

  Ti OPA333.  Vc formed by 2x 4k7 voltage divider on 3v3 rail to ground.  A4 to A3 and A5 with 106 10uF high cap.
  See notes about 'LPF'
  V+   = 3v3 rail
  V-   = Gnd rail
  Vo   = 8k2/1uF LPF to A5 of device, 98k to pin+
  pin- = 5k1 of G-Shunt
  pin+ = 98k of Vc, 98k of Vo, and 5k1 of Y-Shunt

### No Amp circuit 'no amp'

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

### Problem:  The application overflows SRAM on compilation (Argon only?)

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

