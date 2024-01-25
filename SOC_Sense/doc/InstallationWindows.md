# Windows Software Installation for Particle Devices State of Charge Monitoring

Particle Workbench extension to Visual Studio Code is used to develop and configure the real-time code for the different Particle devices and installations.   Header files set the differences.   Particle 'configure for device' chooses the device options that then interact with the headers.

A serial port tool is required to interact.  puTTY is the most effective and integrates best with the TestSOC.py tool on both Windows and Linux.

## Installing GitHub

Install the desktop app from [here](https://desktop.github.com/)

GitHub is tightly integrated with browser sessions.   Sign in to your GitHub first using the browser and browse to  [here](https://github.com/davegutz/myStateOfCharge)
 
- Code - Open with GitHub Desktop - Clone

## Installing Particle Workbench Integrated Development Environment (IDE)

- Download from [here](https://docs.particle.io/quickstart/workbench/#windows)
- Follow the instructions at that link
- It is not necessary to install the Azure IoT toolkit
- When done with installation open Visual Studio
  - IDE - File - New Window 
  - New Window - Particle Workbench 
    - Welcome to Particle Workbench
       - Create new Project - browse to parent folder.   e.g. if want BT-AT under GitHub/myStateOfCharge select 'myStateOfCharge' - Choose parent folder
                    -  Project name - e.g. 'BT-AT' - ok..........project will 'initialize and redraw the window with browser on side
      -  Configure for device  e.g. '3.1.0'  - 'Photon' - device name optional e.g. 'pro'

## Installing puTTY

- Download from [here](https://www.putty.org/)
- On Windows, it's easiest to configure graphically.   Open puTTY and '_Load_' the '_Default Settings_'.
- Configure non-default settings per [here](../dataReduction/putty/sessions/puTTY_Windows_setup_def.odt) and save as 'def'
- Configure the 'test' settings by changing the _logging_ location per [here](../dataReduction/putty/sessions/puTTY_Windows_setup_test.odt)
- Plug in a Particle device to a USB and set the proper _Serial line_ in both the _test_ and _def_ settings and resave them.
- You can find the device by plugging it in and heading to _Device Manager_ - _Ports (COM & LPT)_ and read the _USB Serial Device (COM#)_
- The Particle devices will not always be producing output.   Try entering _v1;_ into the puTTY terminal window manually to get it to produce a data stream.
- Enter _v0;_ to turn it back off.
- You should be ready to go.

