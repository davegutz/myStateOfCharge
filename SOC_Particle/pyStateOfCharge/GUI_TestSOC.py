#! /bin/sh
"exec" "`dirname $0`/venv/bin/python3" "$0" "$@"
#  #! /Users/daveg/Documents/GitHub/myStateOfCharge/SOC_Particle/py/venv/bin/python
# The #! operates for macOS only. 'Python Launcher' (Python Script Preferences) option for 'Allow override with #! in script' is checked.
#  Graphical interface to Test State of Charge application
#  Run in PyCharm
#     or
#  python3 GUI_TestSOC.py
#
#  2023-Jun-15  Dave Gutz   Create
# Copyright (C) 2024 Dave Gutz
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

"""Define a class to manage configuration using files for memory (poor man's database)"""
import sys
import time
from configparser import ConfigParser
import re
from tkinter import ttk, filedialog
import tkinter.simpledialog
import tkinter.messagebox
from CompareHistHist import compare_hist_hist
from CompareHistSim import compare_hist_sim
from CompareRunSim import compare_run_sim
from CompareRunRun import compare_run_run
from CompareRunHist import compare_run_hist
from CountdownTimer import CountdownTimer
from threading import Thread
import shutil
import pyperclip
import subprocess
import pyautogui
import datetime
import platform
from Colors import Colors
from test_soc_util import run_shell_cmd
if platform.system() == 'Darwin':
    # noinspection PyUnresolvedReferences
    from ttwidgets import TTButton as myButton  # Need this for  macOS - ignore warnings
else:
    from tkinter import Button as myButton
bg_color = 'lightgray'

# sys.stdout = open('logs.txt', 'w')
# sys.stderr = open('logse.txt', 'w')

plat = sys.platform
if plat == 'linux':
    default_dr = '/home/daveg/google-drive/GitHubArchive/SOC_Particle/dataReduction'
elif plat == 'darwin':
    default_dr = '/Users/daveg/Library/CloudStorage/GoogleDrive-davegutz2006@gmail.com/My Drive/GitHubArchive/SOC_Particle/dataReduction'
else:
    default_dr = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction'

# Configuration for entire folder selection read with filepaths
def_dict = {
    'test': {
        "version": "g20240331",
        "unit": "pro2p2",
        "battery": "chg",
        'dataReduction_folder': default_dr,
    },
    'ref': {
        "version": "g20240331",
        "unit": "pro0p",
        "battery": "chg",
        'dataReduction_folder': default_dr,
    },
    'others': {
        "option": "custom",
        'macro': 'end_early',
        'mod_in_app': "247",
        'modeling': True,
    },
    }

# Transient string
unit_list = [
    'pro0p', 'pro1a', 'pro2p2', 'pro2p2_hi_lo', 'pro3p2', 'pro3p2_hi_lo', 'pro4p2', 'soc0p', 'soc1a', 'soc2p2_hi_lo',
    'soc3p2_hi_lo', 'soc4p2_hi_lo',
    ]
battery_list = ['bb', 'ch', 'chg']
sel_list = [
    'custom', 'init1', 'saveAdjusts', 'ampHiFail', 'noaHiFail', 'rapidTweakRegression', 'allIn', 'allInBB', 'allInCH',
    'allInCHG', 'allProto', 'pulseSS', 'rapidTweakRegressionH0', 'offSitHysBmsBB', 'offSitHysBmsCH', 'offSitHysBmsCHG',
    'triTweakDisch', 'ampHiFailFf', 'ampLoFail', 'noaLoFail', 'ampHiFailNoise', 'noaHiFailNoise',
    'rapidTweakRegression40C', 'slowTweakRegression', 'satSitBB', 'satSitCH', 'satSitCHG',
    ]
sel_list1 = [
    'flatSitHys', 'offSitHysBmsNoiseBB', 'offSitHysBmsNoiseCH', 'offSitHysBmsNoiseCHG', 'ampHiFailSlow',
    'noaHiFailSlow', 'vHiFail', 'vHiFailNoise', 'vHiFailH', 'vHiFailFf', 'pulseSSH', 'tbFailMod1W', 'tbFailHdwe1W',
    'tLoFailMod', 'tLoFailHdwe', 'DvMon', 'DvSim', 'faultParade', 'allInBBn', 'allInCHn', 'allInCHGn', 'stepDown',
    'stepUp',
    ]
macro_sel_list = [
    'end_early', 'modMidInit', 'modMidInitNoCc', 'modLowInitBB', 'modLowInitCH', 'modLowInitCHG',
    'noisePackage', 'silentPackage', 'quiet', 'quietwait', 'cleanup', 'tempCleanup', 'tranPrep', 'slow',
    'slowTwitchDef', 'fastTwitchDef', 'c06', 'd06', 'c08', 'd08', 'c18', 'd18', 'c50', 'cm50', 'c00',
    'twitch', 'time_stamp', 's00', 'sd50', 'sc50',
    ]

# Macro
satInit = 'Dh;*W;*vv0;*XS;*Ca1;BZ;Ff0;DP1;HR;Rf;XD;'
modMidInit = 'Xm247;Ca0.50;BZ;Ff0;DP1;HR;Rf;XD;'
modMidInitNoCc = 'Xm247;Ca0.50;BZ;Ff0;DP1;HR;Rf;XD;'
modLowInitBB = 'Xm247;Ca0.050;BZ;Ff0;DP1;HR;Rf;XD;'
modLowInitCH = 'Xm247;Ca0.103;BZ;Ff0;DP1;HR;Rf;XD;'
modLowInitCHG = 'Xm247;Ca-0.004;BZ;Ff0;DP1;HR;Rf;XD;'
noisePackage = 'DT.05;DV0.3;DM.75;DN6;'
silentPackage = 'DT0;DV0;DM0;DN0;'
slow = 'Dr400;D>400;DP1;'
quiet = 'vv0;Dr100;DP4;D>313;Dh;'
quietwait = '<vv0;Dr100;DP4;D>313;Dh;'
cleanup = 'Hd;Pf;<HR;<Rf;<XD;'
tempCleanup = 'Rf;XD; '
time_stamp = 'XY;'
tranPrep = 'HR;vv4;Dh1000;W2;Rs;W17;'
slowTranPrep = 'HR;vv4;W2;Rs;' + slow + 'W5;'
slowTwitchDef = 'Rb;Rf;Xts;Xf0.004;Mm1000;Mn-1000;Nm1000;Nn-1000;XW10000;XT10;XC2;'
fastTwitchDef = 'Rb;Rf;Xts;Xf0.002;XW10000;XT10;XC1;'
c18 = time_stamp + 'Dm18;Dn0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
d18 = time_stamp + 'Dn18;Dm0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
c06 = time_stamp + 'Dm6;Dn0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
d06 = time_stamp + 'Dn6;Dm0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
c08 = time_stamp + 'Dm8;Dn0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
d08 = time_stamp + 'Dn8;Dm0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
c50 = time_stamp + 'Dm50;Dn0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
d50 = time_stamp + 'Dn50;Dm0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
cm50 = time_stamp + 'Dm-50;Dn0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation
dm50 = time_stamp + 'Dn-50;Dm0.0001;'  # 0.0001 helps saturation logic behave correctly in a quiet simulation50
sc50 = time_stamp + 'DI50;'  # 50 amp discharge
sd50 = time_stamp + 'DI-50;'  # 50 amp discharge
c00 = 'Dm0;Dn0;Rf;W50;'
s00 = 'DI0;Rf;W100;'
twitch = time_stamp + 'XR;'

# Note:  Photon 2 is throughput limited on the Serial buses.  The *tweak* transients are sensitive to differences
# caused by over-runs and slip and set Dr400 before Xp* then resets to Dr100 (nominal).
lookup = {
        'satInit': (22, 'Y;' + quiet + 'cc;Dh;*W;*vv0;*XS;*Ca1;BZ;Ff0;DP1;<HR;<Rf;<XD;', ('',)),
        'initMid': (22, 'Y;' + quiet + 'cc;Dh1800000;*W;*vv0;*XS;*Ca.5;BZ;Ff0;<HR;<Rf;<XD;', ('',)),
        'saveAdjusts': (60, 'vv4;Dh1000;PR;PV;Pr;Pr;BP2;Pr;BP1;Pr;BS2;Pr;BS1;Pr;Pr;Pr;DA5;Pr;DB-5;Pr;RS;Pr;Dc0.2;Pr;Dc0;DI-10;Pr;DI0;Pr;Dt5;Pr;Dt0;Pr;SA2;Pr;SA1;Pr;SB2;Pr;SB1;Pr;si-1;Pr;RS;Pr;Sk2;Pr;Sk1;Pr;SQ2;Pr;SQ1;Pr;Sq3;Pr;Sq1;Pr;SV1.1;Pr;SV1;Pr;Xb10;Pr;Xb0;Pr;Xa1000;Pr;Xa0;Pr;Xf1;Pr;RS;Pr;Xm10;Pr;RS;Pr;W3;vv0;XQ3;PR;PV;XQ60000;Dh;', ("For testing out the adjustments and memory", "Read through output and witness set and reset of all", "The DS2482 moderate headroom should not exceed limit printed.  EG 11 of 12 is ok.")),
        'custom': (72, 'XQ60000;', ("For general purpose data collection", "'save data' will present a choice of file name", "")),
        'allIn':   (3790,
                    slow + 'Dh4000;' +
                    'cc;' + modMidInit + slowTranPrep + c50 + 'XQ25000;' + c00 + tempCleanup +      # 1 ampHiFail 0
                    '  Xp10;  ' +                                                           # 2 rapidTweakRegression 62
                    '  Xp7;  ' +                                                            # 3 pulseSS  251
                    '  Xp13;  ' +                                                           # 4 triTweakDisch  269
                    modMidInit + slowTranPrep + 'Ff1;' + c50 + 'XQ40000;' + c00 + tempCleanup +     # 5 ampHiFailFf 465
                    modMidInit + slowTranPrep + cm50 + 'XQ50000;' + c00 + tempCleanup +     # 6 ampLoFail    543
                    '  D^15;Xp10; ' +                                                       # 7 rapidTweakRegression40C  630
                    '  Xp11;  ' +                                                           # 8 slowTweakRegression  880
                    'Xm247;Ca0.9;Rb;Rf;Xts;Xa-81;Xf0.004;XW10000;XT10;XC2;W1;HR;vv4;W;Rs;XR;XQ580000;Xa0;Xb0;' + tempCleanup +  # 9 flatSitHys   1466
                    modMidInit + slowTranPrep + c08 + 'Fc0.001;Fd0.5;XQ400000;' + c00 + tempCleanup +    # 10 ampHiFailSlow  2043
                    modMidInit + slowTranPrep + 'XY;Dv0.82;XQ60000;' + 'Dv0;' + tempCleanup +     # 11 vHiFail  2480
                    modMidInit + slowTranPrep + 'Xv.002;XY;Xu1;XQ80000;Xu0;Xv1;W50;' + tempCleanup +       # 12 tbFailMod  2560
                    modMidInit + 'Xm246;' + slowTranPrep + 'Xv.002;W10;XY;Xu1;XQ80000;Xu0;Xv1;W50;' + tempCleanup +  # 13 tbFailHdwe   2684
                    modMidInit + slowTranPrep + 'XY;Dw-0.8;Dn0.0001;XQ120000;Dw0;' + tempCleanup +  # 14 DvMon  2798
                    modMidInit + slowTranPrep + 'XY;Dy-0.8;Dn0.0001;XQ120000;Dy0;' +  # 15 DvSim  2936
                    modMidInit + slowTranPrep + d50 + 'XQ25000;' + c00 + tempCleanup +      # 16 noaHiFail 3006
                    modMidInit + slowTranPrep + d06 + 'Fc0.004;Fd0.5;XQ400000;' + c00 + tempCleanup +    # 17 noaHiFailSlow  3476
                    modMidInit + slowTranPrep + cm50 + 'XQ50000;' + c00 + tempCleanup +     # 18 ampLoFail     3553
                    quiet + cleanup,
                    ('All the best transients', "Must have same 'vv*' throughout", "")),
        'allInBB': (1200,
                    slow + 'Dh4000;' +
                    modLowInitBB + slowTwitchDef + 'Xa-162;' + slowTranPrep + twitch + 'XQ568000;' + 'Xa0;' + tempCleanup +  # offSitHysBmsBB
                    'Xm247;Ca0.9962;' + fastTwitchDef + 'Xa17;' + slowTranPrep + 'XR;XQ600000;' + 'Xa0;' +  # satSitBB
                    quiet + cleanup,
                    ('All the best transients BB', "Must have same 'vv*' throughout", "")),
        'allInCH': (1200,
                    slow + 'Dh4000;' +
                    modLowInitCH + slowTwitchDef + 'Xa-324;' + slowTranPrep + twitch + 'XQ568000;' + 'Xa0;' +  # offSitHysBmsCH
                    'Xm247;Ca0.9920;' + fastTwitchDef + 'Xa17;' + slowTranPrep + 'XR;XQ600000;' + 'Xa0;' +  # satSitCH
                    quiet + cleanup,
                    ('All the best transients CH', "Must have same 'vv*' throughout", "")),
        'allInCHG': (1200,
                     slow + 'Dh4000;' +
                     modLowInitCHG + slowTwitchDef + 'Xa-324;' + slowTranPrep + twitch + 'XQ568000;' + 'Xa0;' +  # offSitHysBmsCHG
                     'Xm247;Ca0.9920;' + fastTwitchDef + 'Xa17;' + slowTranPrep + 'XR;XQ600000;' + 'Xa0;' +  # satSitCHG
                     quiet + cleanup,
                     ('All the best transients CHG', "Must have same 'vv*' throughout", "")),
        'allInBBn': (690,
                     slow + 'Dh4000;' +
                     modMidInit + slowTranPrep + noisePackage + c50 + 'XQ25000;' + c00 + silentPackage + tempCleanup +  # 1 ampHiFailNoise 0
                     modLowInitBB + slowTwitchDef + 'Xa-162;' + noisePackage + tranPrep + 'XR;XQ568000;' + 'Xa0;' + silentPackage +  # 1 offSitHysBmsNoiseCHG 70
                     quiet + cleanup,
                     ('All the best transients CHG noise', "Must have same 'vv*' throughout", "")),
        'allInCHn': (690,
                     slow + 'Dh4000;' +
                     modMidInit + slowTranPrep + noisePackage + c50 + 'XQ25000;' + c00 + silentPackage + tempCleanup +  # 1 ampHiFailNoise 0
                     modLowInitCH + slowTwitchDef + 'Xa-324;' + noisePackage + tranPrep + 'XR;XQ568000;' + 'Xa0;' + silentPackage +  # 1 offSitHysBmsNoiseCHG 70
                     quiet + cleanup,
                     ('All the best transients CHG noise', "Must have same 'vv*' throughout", "")),
        'allInCHGn': (690,
                      slow + 'Dh4000;' +
                      modMidInit + slowTranPrep + noisePackage + c50 + 'XQ25000;' + c00 + silentPackage + tempCleanup +  # 1 ampHiFailNoise 0
                      modLowInitCHG + slowTwitchDef + 'Xa-324;' + noisePackage + tranPrep + 'XR;XQ568000;' + 'Xa0;' + silentPackage +  # 1 offSitHysBmsNoiseCHG 70
                      quiet + cleanup,
                      ('All the best transients CHG noise', "Must have same 'vv*' throughout", "")),
        'ampHiFail': (85, modMidInit + tranPrep + c50 + 'XQ25000;' + c00 + quiet + cleanup, ("Should detect and switch amp current failure (reset when current display changes from '50/diff' back to normal '0' and wait for CoolTerm to stop streaming.)", "'diff' will be displayed. After a bit more, current display will change to 0.", "To evaluate plots, start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'noaHiFail': (85, modMidInit + tranPrep + d50 + 'XQ25000;' + c00 + quiet + cleanup, ("Should detect and switch amp current failure (reset when current display changes from '50/diff' back to normal '0' and wait for CoolTerm to stop streaming.)", "'diff' will be displayed. After a bit more, current display will change to 0.", "To evaluate plots, start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'rapidTweakRegression': (200, 'Xp10;' + quiet + cleanup, ('Should run three very large current discharge/recharge cycles without fault', 'Best test for seeing time skews and checking fault logic for false trips')),
        'allProto': (552, modMidInit + tranPrep + c50 + 'XQ25000;' + c00 + tempCleanup + '  Xp10;  Xp13;  ' + modMidInitNoCc + tranPrep + cm50 + 'XQ50000;' + c00 + quiet + cleanup, ('Proto multi', "Must have same 'vv*' throughout", "No 'HR' either")),
        'pulseSS': (20, slow + 'Xp7;' + quiet + cleanup, ("Should generate a very short <10 sec data burst with a hw pulse.  Look at plots for good overlay. e_wrap should be flat.", "This is the shortest of all tests.  Useful for quick checks.", "ib_diff_flt will take time beyond event to reset running Hi-Lo.")),
        'rapidTweakRegressionH0': (200, 'Sh0;' + slow + 'Xp10;' + quiet + cleanup, ('Should run three very large current discharge/recharge cycles without fault', 'No hysteresis. Best test for seeing time skews and checking fault logic for false trips', 'Tease out cause of e_wrap faults.  e_wrap MUST be flat!')),
        'offSitHysBmsBB': (625, modLowInitBB + slowTwitchDef + 'Xa-162;' + tranPrep + twitch + 'XQ568000;' + 'Xa0;' + quiet + cleanup, ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',)),
        'offSitHysBmsCH': (625, modLowInitCH + slowTwitchDef + 'Xa-324;' + tranPrep + twitch + 'XQ568000;' + 'Xa0;' + quiet + cleanup, ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',)),
        'offSitHysBmsCHG': (625, modLowInitCHG + slowTwitchDef + 'Xa-324;' + tranPrep + twitch + 'XQ568000;' + 'Xa0;' + quiet + cleanup, ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',)),
        'triTweakDisch': (200, slow + 'Xp13;' + quiet + cleanup, ('Should run three very large current discharge/recharge cycles without fault', 'Best test for seeing time skews and checking fault logic for false trips')),
        'ampHiFailFf': (92, modMidInit + tranPrep + 'Ff1;' + c50 + 'XQ40000;' + c00 + quiet + cleanup, ("Should detect but not switch amp current failure. (See 'diff' and current!=0 on OLED).", "Run about 60s. Start by looking at 'Ult 1' fig 4. No fault record (keeps recording).  Verify that on Fig 3 the e_wrap goes through a threshold ~0.4 without change of 'ib_sel_stat'", "This show when deploy with Fake Faults (Ff) don't throw false trips (it happened)", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'ampLoFail': (110, modMidInit + tranPrep + cm50 + 'XQ50000;' + c00 + quiet + cleanup, ("Should detect and switch amp current failure.", "Start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'noaLoFail': (110, modMidInit + tranPrep + dm50 + 'XQ50000;' + c00 + quiet + cleanup, ("Should detect and switch amp current failure.", "Start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'ampHiFailNoise': (85, modMidInit + tranPrep + noisePackage + c50 + 'XQ25000;' + c00 + silentPackage + quiet + cleanup, ("Noisy ampHiFail.  Should detect and switch amp current failure.", "Start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'noaHiFailNoise': (85, modMidInit + tranPrep + noisePackage + d50 + 'XQ25000;' + c00 + silentPackage + quiet + cleanup, ("Noisy ampHiFail.  Should detect and switch amp current failure.", "Start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change")),
        'rapidTweakRegression40C': (200, 'D^15;' + slow + 'Xp10;' + quiet + cleanup, ("Should run three very large current discharge/recharge cycles without fault", "Self-terminates")),
        'slowTweakRegression': (682, 'Xp11' + quiet + cleanup, ("Should run one very large slow (~15 min) current discharge/recharge cycle without fault.   It will take 60 seconds to start changing current.",)),
        'satSitBB': (656, 'Xm247;Ca0.9962;' + fastTwitchDef + 'Xa17;' + tranPrep + 'XR;XQ600000;' + 'Xa0;' + quiet + cleanup, ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",)),
        'satSitCH': (656, 'Xm247;Ca0.9920;' + fastTwitchDef + 'Xa17;' + tranPrep + 'XR;XQ600000;' + 'Xa0;' + quiet + cleanup, ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",)),
        'satSitCHG': (656, 'Xm247;Ca0.986;' + fastTwitchDef + 'Xa17;' + tranPrep + 'XR;XQ600000;' + 'Xa0;' + quiet + cleanup, ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",)),
        'flatSitHys': (620, 'Xm247;Ca0.9;Rb;Rf;Xts;Xa-81;Xf0.004;XW10000;XT10;XC2;W1;vv4;Dh1000;W;Rs;XR;XQ580000;Xa0;Xb0;' + quiet + cleanup, ("Operate around 0.9.  For CHINS, will check EKF with flat voc(soc).   Takes about 10 minutes.", "Make sure EKF soc (soc_ekf) tracks actual soc without wandering.")),
        'offSitHysBmsNoiseBB': (620, modLowInitBB + slowTwitchDef + 'Xa-162;' + noisePackage + tranPrep + 'XR;XQ568000;' + 'Xa0;' + silentPackage + quiet + cleanup, ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff.")),
        'offSitHysBmsNoiseCH': (620, modLowInitCH + slowTwitchDef + 'Xa-324;' + noisePackage + tranPrep + 'XR;XQ568000;' + 'Xa0;' + silentPackage + quiet + cleanup, ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff.")),
        'offSitHysBmsNoiseCHG': (620, modLowInitCHG + slowTwitchDef + 'Xa-324;' + noisePackage + tranPrep + 'XR;XQ568000;' + 'Xa0;' + silentPackage + quiet + cleanup, ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff.")),
        'ampHiFailSlow': (470, modMidInit + tranPrep + c08 + 'Fc0.001;Fd0.5;XQ400000;' + c00 + quiet + cleanup, ("Should detect and switch amp current failure. Will be slow (~6 min) detection as it waits for the EKF to wind up to produce a cc_diff fault.", "Will display “diff” on OLED due to 6 A difference before switch (not cc_diff).", "EKF should tend to follow voltage while soc wanders away.", "Run for 6  minutes to see cc_diff_fa")),
        'noaHiFailSlow': (470, modMidInit + tranPrep + d08 + 'Fc0.001;Fd0.5;XQ400000;' + c00 + quiet + cleanup, ("Should detect and switch amp current failure. Will be slow (~6 min) detection as it waits for the EKF to wind up to produce a cc_diff fault.", "Will display “diff” on OLED due to 6 A difference before switch (not cc_diff).", "EKF should tend to follow voltage while soc wanders away.", "Run for 6  minutes to see cc_diff_fa")),
        'vHiFail': (90, modMidInit + tranPrep + 'XY;Dv0.82;XQ60000;' + 'Dv0;' + quiet + cleanup, ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with 'Ult 1' fig 4.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION")),
        'vHiFailNoise': (90, modMidInit + noisePackage + tranPrep + 'XY;Dv0.82;XQ60000;' + 'Dv0;' + quiet + cleanup, ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with 'Ult 1' fig 4.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION")),
        'vHiFailH': (66, modMidInit + tranPrep + 'SH.3;W10;' + 'XY;Dv0.82;XQ30000;' + 'Dv0;' + quiet + cleanup, ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with 'Ult 1' fig 4.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION.  Initial BB shift will be limited by hys table")),
        'vHiFailFf': (90, modMidInit + tranPrep + 'Ff1;XY;Dv0.8;XQ60000;' + 'Dv0;' + quiet + cleanup, ("Run for about 1 minute.", "Should detect voltage failure (see DOM1 fig 2 or 3) but not display anything on OLED.", "Usually shows SAT.")),
        'pulseSSH': (25, slow + 'Xp8;' + quiet + cleanup, ("Should generate a very short <10 sec data burst with a hw pulse.  Look at plots for good overlay. e_wrap should be flat.", "This is the shortest of all tests.  Useful for quick checks.", "ib_diff_flt will take time beyond event to reset running Hi-Lo.")),
        'tbFailMod1W': (136, modMidInit + tranPrep + 'Xv.002;XY;Xu1;XQ80000;Xu0;Xv1;W50;' + quiet + cleanup, ("Run for 80 sec.   Plot Ult 1 Fig 4 should show Tb was detected as fault but not failed.",)),
        'tbFailHdwe1W': (136, modMidInit + 'Xm246;' + tranPrep + 'Xv.002;W10;XY;Xu1;XQ80000;Xu0;Xv1;W50;' + quiet + cleanup, ("Run for 80 sec.   Plot Ult 1 Fig 4 should show Tb was detected as failed.", "")),
        'tLoFailMod': (185, modMidInit + tranPrep + 'XY;D^-113;XQ120000;' + 'D^0;' + cleanup + '<W50;' + quietwait + '<Pf;', ("Simulates open thermistor.", "To diagnose, begin with 'Ult 1' fig 4.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION")),
        'tLoFailHdwe': (185, modMidInit + 'Xm230;' + tranPrep + 'XY;Dt-113;XQ120000;' + 'Dt0;' + cleanup + '<W50;' + quietwait + '<Pf;', ("Simulates open thermistor.", "To diagnose, begin with 'Ult 1' fig 4.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION")),
        'DvMon': (152, modMidInit + tranPrep + 'XY;Dw-0.8;Dn0.0001;XQ120000;Dw0;' + quiet + cleanup, ("Should detect and switch voltage failure and use vb_model", "'*fail' will be displayed.", "To evaluate plots, start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'redl' flashing on OLED even after fault cleared automatically (lost redundancy).", "Run for 2 min to confirm no cc_diff_fa")),
        'DvSim': (152, modMidInit + tranPrep + 'XY;Dy-0.8;Dn0.0001;XQ120000;Dy0;' + quiet + cleanup, ("Should detect and switch voltage failure and use vb_model", "'*fail' will be displayed.", "To evaluate plots, start looking at 'Ult 1' fig 4. Fault record (frozen). Will see 'redl' flashing on OLED even after fault cleared automatically (lost redundancy).", "Run for 2 min to confirm no cc_diff_fa")),
        'faultParade': (320, modMidInit + 'Dh1000;vv4;XY;Dm50;Dn0.0001;W200;Dm0;Dn0;W20;Rf;XQ240000;' + quiet + cleanup, ("Check fault, history, and summary logging", "Should flag faults but take no action", "", "", "")),
        'stepDown': (103, modMidInit + tranPrep + sd50 + 'XQ25000;' + s00 + quiet + cleanup, ("Should be normal hard discharge step", "", "", "")),
        'stepUp': (103, modMidInit + tranPrep + sc50 + 'XQ25000;' + s00 + quiet + cleanup, ("Should be normal hard charge step", "", "", "")),
}

macro_lookup = {
        'end_early': (22, 'Y;cc;Dh1800000;*W;*vv0;*XS;*Ca1;<Hd;<Pf;', ('', '', '', '')),
        'modMidInit': (5, modMidInit, ('', '', '', '')),
        'modLowInitBB': (5, modLowInitBB, ('', '', '', '')),
        'modLowInitCH': (5, modLowInitCH, ('', '', '', '')),
        'noisePackage': (5, noisePackage, ('', '', '', '')),
        'silentPackage': (5, silentPackage, ('', '', '', '')),
        'quiet': (5, quiet, ('', '', '', '')),
        'cleanup': (5, cleanup, ('', '', '', '')),
        'tempCleanup': (5, tempCleanup, ('', '', '', '')),
        'tranPrep': (5, tranPrep, ('', '', '', '')),
        'time_stamp': (5, time_stamp, ('', '', '', '')),
        'slow': (5, slow, ('', '', '', '')),
        'slowTwitchDef': (5, slowTwitchDef, ('', '', '', '')),
        'fastTwitchDef': (5, fastTwitchDef, ('', '', '', '')),
        'c06': (5, c06, ('', '', '', '')),
        'd06': (5, d06, ('', '', '', '')),
        'c08': (5, c08, ('', '', '', '')),
        'd08': (5, d08, ('', '', '', '')),
        'c18': (5, c18, ('', '', '', '')),
        'd18': (5, d18, ('', '', '', '')),
        'c50': (5, c50, ('', '', '', '')),
        'd50': (5, d50, ('', '', '', '')),
        'cm50': (5, cm50, ('', '', '', '')),
        'c00': (5, c00, ('', '', '', '')),
        'twitch': (5, twitch, ('', '', '', '')),
        }

putty_connection = {'': 'test',
                    'soc0p': 'testsoc0p',
                    'soc1a': 'testsoc1a',
                    'pro0p': 'testpro0p',
                    'pro1a': 'testpro1a',
                    'pro2p2': 'testpro2p2',
                    'pro2p2_hi_lo': 'testpro2p2',
                    'pro3p2': 'testpro3p2',
                    'pro3p2_hi_lo': 'testpro3p2',
                    'pro4p2': 'testpro4p2',
                    'soc2p2': 'testsoc2p2',
                    'soc2p2_hi_lo': 'testsoc2p2',
                    'soc3p2': 'testsoc3p2',
                    'soc3p2_hi_lo': 'testsoc3p2',
                    'soc4p2': 'testsoc4p2',
                    'soc4p2_hi_lo': 'testsoc4p2',
                    }


# Begini - configuration class using .ini files
class Begini(ConfigParser):

    def __init__(self, name, def_dict_):
        ConfigParser.__init__(self)

        (config_path, config_basename) = os.path.split(name)
        if platform.system() == 'Linux':
            config_txt = os.path.splitext(config_basename)[0] + '_linux.ini'
            self.config_file_path = os.path.join('/home/daveg/.local/', config_txt)
        elif platform.system() == 'Darwin':
            config_txt = os.path.splitext(config_basename)[0] + '_macos.ini'
            self.config_file_path = os.path.join('/Users/daveg/.local/', config_txt)
        else:
            config_txt = os.path.splitext(config_basename)[0] + '.ini'
            self.config_file_path = os.path.join(os.getenv('LOCALAPPDATA'), config_txt)
        print('config file', self.config_file_path)
        if os.path.isfile(self.config_file_path):
            self.read(self.config_file_path)
        else:
            with open(self.config_file_path, 'w') as cfg_file:
                self.read_dict(def_dict_)
                self.write(cfg_file)
            print('wrote', self.config_file_path)

    # Get an item
    def get_item(self, ind, item):
        return self[ind][item]

    # Put an item
    def put_item(self, ind, item, value):
        self[ind][item] = value
        self.save_to_file()

    # Save again
    def save_to_file(self):
        with open(self.config_file_path, 'w') as cfg_file:
            self.write(cfg_file)
        print('wrote', self.config_file_path)


# Executive class to control the global variables
class ExRoot:
    def __init__(self):
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.version = None
        self.root_config = None
        self.load_root_config(self.config_path)

    def load_root_config(self, config_file_path):
        self.root_config = ConfigParser()
        if os.path.isfile(config_file_path):
            self.root_config.read(config_file_path)
        else:
            with open(config_file_path, 'w') as cfg_file:
                self.root_config.add_section('Root Preferences')
                rec_folder_path = os.path.expanduser('~') + '/Documents/Recordings'
                if not os.path.exists(rec_folder_path):
                    os.makedirs(rec_folder_path)
                self.root_config.set('Root Preferences', 'recordings path', rec_folder_path)
                self.root_config.write(cfg_file)
        return self.root_config

    def save_root_config(self, config_path_):
        if os.path.isfile(config_path_):
            with open(config_path_, 'w') as cfg_file:
                self.root_config.write(cfg_file)
            print('Saved', config_path_)
        return self.root_config


# Executive class to control the global variables
class Exec:
    def __init__(self, cf_=None, ind=None, level=None, path_disp_len_=25):
        self.cf = cf_
        self.ind = ind
        self.level = level
        self.path_disp_len = path_disp_len_
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        # self.root_config = None
        self.load_root_config(self.config_path)
        self.dataReduction_folder = self.cf[self.ind]['dataReduction_folder']
        self.version = self.cf[self.ind]['version']
        self.battery = self.cf[self.ind]['battery']
        self.unit = self.cf[self.ind]['unit']
        if self.version is None:
            self.version = 'undefined'
        self.version_path = str(os.path.join(self.dataReduction_folder, self.version))
        try:
            os.makedirs(self.version_path, exist_ok=True)
        except OSError:
            tk.messagebox.showerror(title="Error", message="check google-drive available")
        # Following need explicit shallow copy lines
        self.folder_button = myButton(master, text=self.dataReduction_folder[-20:],
                                      command=self.enter_data_reduction_folder, fg="blue", bg=bg_color)
        self.version_button = None
        self.unit_button = None
        self.battery_button = None
        self.key_label = None
        self.file_txt = None
        self.file_path = None
        self.file_exists = None
        self.dataReduction_folder_exists = None
        self.key_exists_in_file = None
        self.label = None
        self.key = None

    def __copy__(self):
        """Shallow copy function"""
        instance = object.__new__(Exec)
        vars(instance).update(vars(self))
        return instance

    def create_file_path_and_key(self, name_override=None):
        if name_override is None:
            self.file_txt = create_file_txt(self.cf['others']['option'], self.unit, self.battery)
            self.key = create_file_key(self.version, self.unit, self.battery)
            print('version', self.version, 'key', self.key)
        else:
            self.file_txt = create_file_txt(name_override, self.unit, self.battery)
            self.key = create_file_key(self.version, self.unit, self.battery)
        self.file_path = os.path.join(self.version_path, self.file_txt)
        self.update_file_label()
        self.file_exists = os.path.isfile(self.file_path)
        self.update_file_label()
        self.update_key_label()
        self.update_folder_button()

    def enter_battery(self):
        answer = tk.simpledialog.askstring(title=self.level,
                                           prompt="Enter battery e.g. 'bb for Battleborn', 'ch' or 'chg' for CHINS:")
        if answer is None or answer == ():
            print('enter operation cancelled')
            return
        self.battery = answer
        self.cf[self.ind]['battery'] = self.battery
        self.cf.save_to_file()
        self.battery_button.config(text=self.battery)
        self.create_file_path_and_key()
        self.update_key_label()

    def enter_data_reduction_folder(self):
        answer = tk.filedialog.askdirectory(title="Select a destination (i.e. Library) dataReduction folder",
                                            initialdir=self.dataReduction_folder)
        if answer is None or answer == ():
            print('enter operation cancelled')
            return
        self.dataReduction_folder = answer
        self.cf[self.ind]['dataReduction_folder'] = self.dataReduction_folder
        self.cf.save_to_file()
        self.folder_button.config(text=self.dataReduction_folder[self.path_disp_len:])
        self.update_folder_button()

    def enter_unit(self):
        answer = tk.simpledialog.askstring(title=self.level, initialvalue=self.unit,
                                           prompt="Enter unit e.g. 'pro0p', 'pro1a', 'pro2p2'"
                                                  "'pro2p2_hi_lo', 'pro3p2', 'pro3p2_hi_lo', 'pro4p2', 'soc0p', 'soc1a',"
                                                  "'soc2p2_hi_lo', 'soc3p2_hi_lo', 'soc4p2_hi_lo':")
        if answer is None or answer == ():
            print('enter operation cancelled')
            return
        self.unit = answer
        self.cf[self.ind]['unit'] = self.unit
        self.cf.save_to_file()
        self.unit_button.config(text=self.unit)
        self.create_file_path_and_key()
        self.update_key_label()
        self.update_file_label()

    def enter_version(self):
        answer = tk.simpledialog.askstring(title=__file__, prompt="Enter version <vYYYYMMDD>:",
                                           initialvalue=self.version)
        if answer is None or answer == ():
            print('enter operation cancelled')
            return
        self.version = answer
        self.cf[self.ind]['version'] = self.version
        self.cf.save_to_file()
        self.version_button.config(text=self.version)
        self.version_path = os.path.join(self.dataReduction_folder, self.version)
        os.makedirs(self.version_path, exist_ok=True)
        self.create_file_path_and_key()
        self.update_key_label()
        self.label.config(text=self.file_txt)

    def load_root_config(self, config_file_path):
        self.root_config = ConfigParser()
        if os.path.isfile(config_file_path):
            self.root_config.read(config_file_path)
        else:
            with open(config_file_path, 'w') as cfg_file:
                self.root_config.add_section('Root Preferences')
                rec_folder_path = os.path.expanduser('~') + '/Documents/Recordings'
                if not os.path.exists(rec_folder_path):
                    os.makedirs(rec_folder_path)
                self.root_config.set('Root Preferences', 'recordings path', rec_folder_path)
                self.root_config.write(cfg_file)
        return self.root_config

    def save_root_config(self, config_path_):
        if os.path.isfile(config_path_):
            with open(config_path_, 'w') as cfg_file:
                self.root_config.write(cfg_file)
            print('Saved', config_path_)
        return self.root_config

    def super_shallow_copy(self, other):
        self.level = other.level
        self.path_disp_len = other.path_disp_len
        self.script_loc = other.script_loc
        self.config_path = other.config_path
        self.root_config = other.root_config
        self.dataReduction_folder = other.dataReduction_folder
        self.version = other.version
        self.battery = other.battery
        self.unit = other.unit
        self.version_path = other.version_path
        self.file_txt = other.file_txt
        self.file_path = other.file_path
        self.file_exists = other.file_exists
        self.dataReduction_folder_exists = other.dataReduction_folder_exists
        self.key_exists_in_file = other.key_exists_in_file
        self.key = other.key

    def update_battery_stuff(self):
        self.cf[self.ind]['version'] = self.version
        self.cf[self.ind]['unit'] = self.unit
        self.cf[self.ind]['battery'] = self.battery
        self.cf[self.ind]['dataReduction_folder'] = self.dataReduction_folder
        self.cf.save_to_file()
        self.create_file_path_and_key()
        self.update_folder_button()
        self.update_version_button()
        self.update_unit_button()
        self.update_battery_button()
        self.update_key_label()
        self.update_file_label()

    def update_file_label(self):
        self.label.config(text=self.file_txt)
        if self.file_exists:
            self.label.config(bg='lightgreen')
        else:
            self.label.config(bg='pink')

    def update_battery_button(self):
        self.battery_button.config(text=self.battery)

    def update_folder_button(self):
        if os.path.exists(self.dataReduction_folder):
            self.dataReduction_folder_exists = True
        else:
            self.dataReduction_folder_exists = False
        self.folder_button.config(text=self.dataReduction_folder[-self.path_disp_len:])
        if self.dataReduction_folder_exists:
            self.folder_button.config(bg='lightgreen')
        else:
            self.folder_button.config(bg='pink')

    def update_key_label(self):
        self.key_label.config(text=self.key)
        self.key_exists_in_file = False
        if os.path.isfile(self.file_path):
            for line in open(self.file_path, 'r'):
                if re.search(self.key, line):
                    self.key_exists_in_file = True
                    break
        if self.key_exists_in_file:
            self.key_label.config(bg='lightgreen')
        else:
            self.key_label.config(bg='pink')
        test_filename.set(putty_connection.get(Test.unit))

    def update_unit_button(self):
        self.unit_button.config(text=self.unit)

    def update_version_button(self):
        self.version_button.config(text=self.version)


# Global methods
def add_to_clip_board(text):
    pyperclip.copy(text)


# Compare run driver
def clear_data_silent(nowait=True):
    clear_data(silent=True, nowait=nowait)


def clear_data_verbose():
    clear_data(silent=False)


def clear_data(silent=False, nowait=False):
    if os.path.isfile(putty_test_csv_path.get()):
        enter_size = putty_size()  # bytes
        time.sleep(1.)
        wait_size = putty_size()  # bytes
    else:
        enter_size = 0
        wait_size = 0
    if enter_size > 64:  # bytes
        if wait_size > enter_size and not nowait:
            if silent is False:
                print('stop data first')
            tkinter.messagebox.showwarning(message="stop data first")
        else:
            # create empty file
            if not save_putty():
                if silent is False:
                    tkinter.messagebox.showwarning(message="putty may be open already")
                else:
                    update_data_buttons()
    else:
        if silent is False:
            print('putty test file non-existent or too small (<64 bytes) probably already done')
            tkinter.messagebox.showwarning(message="Nothing to clear")


# Choose file to perform compare_hist_hist on
def compare_hist_hist_choose():
    # Select file
    print('compare_hist_hist_choose')
    testpaths = filedialog.askopenfilenames(title='Choose test file(s)', filetypes=[('csv', '.csv')],
                                            initialdir=Test.dataReduction_folder)
    if testpaths is None or testpaths == '':
        print("No file chosen")
    else:
        for testpath in testpaths:
            test_folder_path, test_parent, test_basename, test_txt, test_key = contain_all(testpath)
            if test_key != '':
                ref_path = filedialog.askopenfilename(title='Choose reference file', filetypes=[('csv', '.csv')],
                                                      initialdir=Ref.dataReduction_folder)
                ref_folder_path, ref_parent, ref_basename, ref_txt, ref_key = contain_all(ref_path)
                print('GUI_TestSOC compare_hist_hist_choose:  Ref', ref_basename, ref_key)
                print('GUI_TestSOC compare_hist_hist_choose:  Test', test_basename, test_key)
                # keys = [(ref_basename, ref_key), (test_basename, test_key)]
                # master.withdraw()
                compare_hist_hist(data_file_ref=ref_path, unit_key_ref=ref_key,
                                  data_file_tst=testpath, unit_key_tst=test_key,
                                  dt_resample=30.)
                # master.deiconify()
            else:
                tk.messagebox.showerror(message='key not found in' + testpath)
        update_data_buttons()


# Choose file to perform compare_run_sim on
def compare_hist_sim_choose():
    # Select file
    print('compare_hist_sim_choose')
    testpaths = filedialog.askopenfilenames(title='Please select files', filetypes=[('csv', '.csv')],
                                            initialdir=Test.dataReduction_folder)
    if testpaths is None or testpaths == '':
        print("No file chosen")
    else:
        update_data_buttons()
        for testpath in testpaths:
            test_folder_path, test_parent, basename, test_txt, key = contain_all(testpath)
            if key != '':
                answer = tk.simpledialog.askinteger(title=__file__,
                                                    prompt="Simulation re-construction sample time in seconds",
                                                    initialvalue=900)
                if answer is None:
                    print('enter operation cancelled')
                    return
                compare_hist_sim(data_file=testpath, unit_key=key, dt_resample=answer)
            else:
                tk.messagebox.showerror(message='key not found in' + testpath)


def compare_hist_to_sim():
    if modeling.get():
        update_data_buttons()
        print('compare_hist_to_sim.  save_pdf_path', os.path.join(Test.version_path, './figures'))
        # master.withdraw()
        answer = tk.simpledialog.askinteger(title=__file__, prompt="Simulation re-construction sample time in seconds",
                                            initialvalue=10)
        if answer is None:
            print('enter operation cancelled')
            return
        compare_hist_sim(data_file=Test.file_path, unit_key=Test.key, mon_t=True, dt_resample=answer)
        # master.deiconify()
    else:
        print('not possible')


def compare_run():
    if not Test.key_exists_in_file:
        tkinter.messagebox.showwarning(message="Test Key '" + Test.key + "' does not exist in " + Test.file_txt)
        return
    update_data_buttons()
    if modeling.get():
        print('compare_run_sim.  save_pdf_path', os.path.join(Test.version_path, './figures'))
        # master.withdraw()
        compare_run_sim(data_file=Test.file_path, unit_key=Test.key)
        # master.deiconify()
    else:
        if not Ref.key_exists_in_file:
            tkinter.messagebox.showwarning(message="Ref Key '" + Ref.key + "' does not exist in " + Ref.file_txt)
            return
        print('GUI_TestSOC compare_run:  Ref', Ref.file_path, Ref.key)
        print('GUI_TestSOC compare_run:  Test', Test.file_path, Test.key)
        keys = [(Ref.file_txt, Ref.key), (Test.file_txt, Test.key)]
        # master.withdraw()
        compare_run_run(keys=keys, data_file_folder_ref=Ref.version_path, data_file_folder_test=Test.version_path)

        # master.deiconify()


def compare_run_to_hist():
    if not Test.key_exists_in_file:
        tkinter.messagebox.showwarning(message="Test Key '" + Test.key + "' does not exist in " + Test.file_txt)
        return
    update_data_buttons()
    if modeling.get():
        print('compare_hist_to_sim.  save_pdf_path', os.path.join(Test.version_path, './figures'))
        compare_run_hist(data_file_=Test.file_path, unit_key_=Test.key)
    else:
        print('not possible')


# Choose file to perform compare_run_run on
def compare_run_run_choose():
    # Select file
    print('compare_run_run_choose')
    testpaths = filedialog.askopenfilenames(title='Choose test file(s)', filetypes=[('csv', '.csv')],
                                            initialdir=Test.dataReduction_folder)
    if testpaths is None or testpaths == '':
        print("No file chosen")
    else:
        for testpath in testpaths:
            test_folder_path, test_parent, test_basename, test_txt, test_key = contain_all(testpath)
            if test_key != '':
                ref_path = filedialog.askopenfilename(title='Choose reference file', filetypes=[('csv', '.csv')],
                                                      initialdir=Ref.dataReduction_folder)
                ref_folder_path, ref_parent, ref_basename, ref_txt, ref_key = contain_all(ref_path)
                print('GUI_TestSOC compare_run_run_choose:  Ref', ref_basename, ref_key)
                print('GUI_TestSOC compare_run_run_choose:  Test', test_basename, test_key)
                keys = [(ref_basename, ref_key), (test_basename, test_key)]
                # master.withdraw()
                compare_run_run(keys=keys, data_file_folder_ref=ref_folder_path, data_file_folder_test=test_folder_path,
                                sync_to_ctime=True)
                # master.deiconify()
            else:
                tk.messagebox.showerror(message='key not found in' + testpath)
        update_data_buttons()


# Choose file to perform compare_run_sim on
def compare_run_sim_choose():
    # Select file
    print('compare_run_sim_choose')
    testpaths = filedialog.askopenfilenames(title='Please select files', filetypes=[('csv', '.csv')],
                                            initialdir=Test.dataReduction_folder)
    if testpaths is None or testpaths == '':
        print("No file chosen")
    else:
        for testpath in testpaths:
            test_folder_path, test_parent, basename, test_txt, key = contain_all(testpath)
            if key != '':
                compare_run_sim(data_file=testpath, unit_key=key)
            else:
                tk.messagebox.showerror(message='key not found in' + testpath)
        update_data_buttons()


# Split all information contained in file path
def contain_all(testpath):
    folder_path, basename = os.path.split(testpath)
    parent, txt = os.path.split(folder_path)
    # get key
    key = ''
    with open(testpath, 'r') as file:
        for line in file:
            if line.__contains__(txt):
                shorter = line[line.find(txt):]
                end_key = shorter.find(',')
                key = shorter[:end_key].strip()
                break
    return folder_path, parent, basename, txt, key


# puTTY generates '\0' characters
def copy_clean(src, dst):
    with open(src, 'r') as file_in:
        data = file_in.read()
    with open(dst, 'w') as file_out:
        file_out.write(data.replace('\0', ''))


def create_file_key(version_, unit_, battery_):
    return version_ + '_' + unit_ + '_' + battery_


def create_file_txt(option_, unit_, battery_):
    return option_ + '_' + unit_ + '_' + battery_ + '.csv'


def empty_file(target):
    # create empty file
    try:
        open(empty_csv_path.get(), 'x')
    except FileExistsError:
        pass
    shutil.copyfile(empty_csv_path.get(), target)
    print('emptied', putty_test_csv_path.get())
    try:
        os.remove(empty_csv_path.get())
    except OSError:
        pass


def enter_mod_in_app():
    answer = tk.simpledialog.askinteger(title=__file__, prompt="enter the value of Modeling in app to assume", initialvalue=mod_in_app.get())
    if answer is None:
        print('enter operation cancelled')
        return
    mod_in_app.set(answer)
    cf['others']['mod_in_app'] = str(mod_in_app.get())
    cf.save_to_file()
    mod_in_app_button.config(text=mod_in_app.get())


def grab_macro():
    add_to_clip_board(macro.get())
    macro_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    init_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    get_time_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def grab_init():
    add_to_clip_board(init.get())
    grab_all_nominal()
    init_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    clear_data_silent()
    print('cleared putty data file')
    Test.create_file_path_and_key()
    Test.update_key_label()
    start_putty()


def grab_start():
    add_to_clip_board(start.get())
    grab_all_nominal()
    save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                            text='save data')
    save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                               text='save data as')
    grab_all_nominal()
    start_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    start_timer()


def grab_all_nominal():
    macro_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black')
    init_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    get_time_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black')


def grab_time():
    current_ut = 'UT' + str(int(time.time())) + ';'
    add_to_clip_board(current_ut)
    grab_all_nominal()
    get_time_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black',
                           text=current_ut)
    print('UT in paste buffer')


def handle_modeling(*_args):
    cf['others']['modeling'] = str(modeling.get())
    cf.save_to_file()
    if modeling.get() is True:
        ref_remove()
    else:
        ref_restore()


def handle_macro(*_args):
    lookup_macro()
    macro_option_ = macro_option.get()

    # Check if this is what you want to do
    if macro_option_.__contains__('CH'):
        if Test.battery == 'bb' or Ref.battery == 'bb':
            confirmation = tk.messagebox.askyesno('query sensical', 'Test/Ref are "bb." Continue?')
            if confirmation is False:
                print('start over')
                tkinter.messagebox.showwarning(message='try again')
                option.set('try again')
                return
    elif macro_option_.__contains__('BB'):
        if Test.battery == 'ch' or Ref.battery == 'ch' or Test.battery == 'chg' or Ref.battery == 'chg':
            confirmation = tk.messagebox.askyesno('query sensical', 'Test/Ref are "ch." Continue?')
            if confirmation is False:
                print('start over')
                tkinter.messagebox.showwarning(message='try again')
                option.set('try again')
                return

    macro_option_show.set(macro_option_)
    cf['others']['macro'] = macro_option_
    cf.save_to_file()
    macro_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def handle_option(*_args):
    lookup_start()
    option_ = option.get()

    # Check if this is what you want to do
    if option_.__contains__('CH'):
        if Test.battery == 'bb' or Ref.battery == 'bb':
            confirmation = tk.messagebox.askyesno('query sensical', 'Test/Ref are "bb." Continue?')
            if confirmation is False:
                print('start over')
                tkinter.messagebox.showwarning(message='try again')
                option.set('try again')
                return
    elif option_.__contains__('BB'):
        if Test.battery == 'ch' or Ref.battery == 'ch' or Test.battery == 'chg' or Ref.battery == 'chg':
            confirmation = tk.messagebox.askyesno('query sensical', 'Test/Ref are "cc." Continue?')
            if confirmation is False:
                print('start over')
                tkinter.messagebox.showwarning(message='try again')
                option.set('try again')
                return

    option_show.set(option_)
    cf['others']['option'] = option_
    cf.save_to_file()
    Test.create_file_path_and_key()
    Ref.create_file_path_and_key()
    save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                            text='save data')
    save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                               text='save data as')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    update_data_buttons()


def handle_ref_battery(*_args):
    Ref.battery = ref_battery.get()
    Ref.update_battery_stuff()
    update_data_buttons()


def handle_ref_unit(*_args):
    Ref.unit = ref_unit.get()
    Ref.update_battery_stuff()
    update_data_buttons()


def handle_test_battery(*_args):
    Test.battery = test_battery.get()
    Test.update_battery_stuff()
    update_data_buttons()


def handle_test_unit(*_args):
    Test.unit = test_unit.get()
    Test.update_battery_stuff()
    update_data_buttons()


def kill_putty(sys_=None, silent=True):
    command = ''
    if sys_ == 'Linux':
        command = 'pkill -e putty'
    elif sys_ == 'Windows':
        command = 'taskkill /f /im putty.exe'
    elif sys_ == 'Darwin':
        command = 'pkill putty'
    else:
        print(f"kill_putty: SYS = {sys_} unknown")
    if silent is False:
        print(command + '\n')
        print(Colors.bg.brightblack, Colors.fg.wheat)
        result = run_shell_cmd(command, silent=silent)
        print(Colors.reset)
        print(command + '\n')
        if result == -1:
            print(Colors.fg.blue, 'failed.', Colors.reset)
            return None, False
    else:
        result = run_shell_cmd(command, silent=silent)
    return result


def look_putty(sys_=None, silent=True):
    command = ''
    if sys_ == 'Linux':
        return subprocess.check_output(['ps']).decode('ascii').__contains__('putty')
    elif sys_ == 'Windows':
        return subprocess.check_output(['tasklist']).decode('ascii').__contains__('putty.exe')
    elif sys_ == 'Darwin':
        command = 'tbd'
    else:
        print(f"kill_putty: SYS = {sys_} unknown")
    if silent is False:
        print(command + '\n')
        print(Colors.bg.brightblack, Colors.fg.wheat)
        result = run_shell_cmd(command, silent=silent)
        print(Colors.reset)
        print(command + '\n')
        if result == -1:
            print(Colors.fg.blue, 'failed.', Colors.reset)
            return None, False
    else:
        result = run_shell_cmd(command, silent=False, save_stdout=True)
        print(f"run_shell_cmd {result=}")
    return result


def lookup_macro():
    dum_, macro_val, ev_val = macro_lookup.get(macro_option.get())
    macro.set(macro_val)
    macro_button.config(text=macro.get())
    while len(ev_val) < 4:
        ev_val = ev_val + ('',)
    if ev_val[0]:
        ev1_label.config(text='-' + ev_val[0])
    else:
        ev1_label.config(text='')
    if ev_val[1]:
        ev2_label.config(text='-' + ev_val[1])
    else:
        ev2_label.config(text='')
    if ev_val[2]:
        ev3_label.config(text='-' + ev_val[2])
    else:
        ev3_label.config(text='')
    if ev_val[3]:
        ev4_label.config(text='-' + ev_val[3])
    else:
        ev4_label.config(text='')


def lookup_start():
    dawdle_val_, start_val, ev_val = lookup.get(option.get())
    start.set(start_val)
    start_button.config(text=start.get())
    while len(ev_val) < 4:
        ev_val = ev_val + ('',)
    if ev_val[0]:
        ev1_label.config(text='-' + ev_val[0])
    else:
        ev1_label.config(text='')
    if ev_val[1]:
        ev2_label.config(text='-' + ev_val[1])
    else:
        ev2_label.config(text='')
    if ev_val[2]:
        ev3_label.config(text='-' + ev_val[2])
    else:
        ev3_label.config(text='')
    if ev_val[3]:
        ev4_label.config(text='-' + ev_val[3])
    else:
        ev4_label.config(text='')
    timer_val.set(dawdle_val_)


def lookup_test():
    test_filename.set(putty_connection.get(Test.unit))


def putty_size():
    if os.path.isfile(putty_test_csv_path.get()):
        enter_size = os.path.getsize(putty_test_csv_path.get())  # bytes
    else:
        enter_size = 0
    return enter_size


def ref_remove():
    top_panel_right.pack_forget()
    run_button.config(text='Compare Run Sim')
    run_hist_button.config(text='Compare Run Sim Hist')
    hist_sim_button.config(text='Compare Hist Sim')
    hist_sim_button.pack(side=tk.RIGHT, padx=5, pady=5)
    run_hist_button.pack(side=tk.RIGHT, padx=5, pady=5)
    Ref.label.forget()


def ref_restore():
    top_panel_right.pack(expand=True, fill='both')
    run_button.config(text='Compare Run Run')
    run_hist_button.forget()
    hist_sim_button.forget()
    Ref.label.pack(padx=5, pady=5)


def stay_awake(up_set_min=3.):
    """Keep computer awake using shift key when recording then return to previous state"""

    # Timer starts
    start_time = float(time.time())
    up_time_min = 0.0
    # FAILSAFE to FALSE feature is enabled by default so that you can easily stop execution of
    # your pyautogui program by manually moving the mouse to the upper left corner of the screen.
    # Once the mouse is in this location, pyautogui will throw an exception and exit.
    pyautogui.FAILSAFE = False
    putty_running = True
    while putty_running > 0 and (up_time_min < up_set_min):
        time.sleep(30.)
        for i in range(0, 3):
            pyautogui.press('shift')  # Shift key does not disturb fullscreen
        up_time_min = (time.time() - start_time) / 60.
        print(f"stay_awake: {up_time_min=} out of {up_set_min}")
        # Check putty running
        putty_running = look_putty(platform.system())

    print(f"stay_awake: ending")


def save_data():
    print(f"save_data: {putty_test_csv_path.get()=}")
    if size_of(putty_test_csv_path.get()) > 64:  # bytes
        # For custom option, redefine Test.file_path if requested
        new_file_txt = None
        if option.get() == 'custom':
            new_file_txt = tk.simpledialog.askstring(title=__file__, prompt="custom file name string:")
            if new_file_txt is not None:
                Test.create_file_path_and_key(name_override=new_file_txt)
                Test.label.config(text=Test.file_txt)
                print('Test.file_path', Test.file_path)
        if os.path.isfile(Test.file_path) and os.path.getsize(Test.file_path) > 0:  # bytes
            confirmation = tk.messagebox.askyesno('query overwrite', 'File exists:  overwrite?')
            if confirmation is False:
                print('skipped overwrite')
                tkinter.messagebox.showwarning(message='retained ' + Test.file_path)
                return
        save_data_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black',
                                text='data saving')
        tksleep(0.1)
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        save_data_button.config(bg='green', activebackground='green', fg='red', activeforeground='red',
                                text='data saved')
        empty_file(putty_test_csv_path.get())
        print('updating Test file label')
        Test.create_file_path_and_key(name_override=new_file_txt)
    else:
        print('putty test file non-existent or too small (<64 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to save")
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def save_data_as():
    if size_of(putty_test_csv_path.get()) > 512:  # bytes
        # For custom option, redefine Test.file_path if requested
        if option.get() == 'custom':
            new_file_txt = tk.simpledialog.askstring(title=__file__, prompt="custom file name string:")
            if new_file_txt is not None:
                Test.create_file_path_and_key(name_override=new_file_txt)
                Test.label.config(text=Test.file_txt)
                print('Test.file_path', Test.file_path)
        else:
            new_file_txt = tk.simpledialog.askstring(title=__file__, prompt="custom file name string:",
                                                     initialvalue=Test.file_txt)
            if new_file_txt is not None:
                Test.create_file_path_and_key(name_override=new_file_txt)
                Test.label.config(text=Test.file_txt)
                print('Test.file_path', Test.file_path)
        if os.path.isfile(Test.file_path) and os.path.getsize(Test.file_path) > 0:  # bytes
            confirmation = tk.messagebox.askyesno('query overwrite', 'File exists:  overwrite?')
            if confirmation is False:
                print('reset and use clear')
                tkinter.messagebox.showwarning(message='reset and use clear')
                return
        save_data_as_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black',
                                   text='data saving')
        tksleep(0.1)
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        save_data_as_button.config(bg='green', activebackground='green', fg='red', activeforeground='red',
                                   text='data saved as')
        empty_file(putty_test_csv_path.get())
        print('updating Test file label')
        Test.create_file_path_and_key(name_override=new_file_txt)
    else:
        print('putty test file is too small (<512 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to save")
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def save_progress():
    if size_of(putty_test_csv_path.get()) > 64:  # bytes
        # For custom option, redefine Test.file_path if requested
        new_file_txt = None
        if option.get() == 'custom':
            new_file_txt = tk.simpledialog.askstring(title=__file__, prompt="custom file name string:")
            if new_file_txt is not None:
                Test.create_file_path_and_key(name_override=new_file_txt)
                Test.label.config(text=Test.file_txt)
                print('Test.file_path', Test.file_path)
        if os.path.isfile(Test.file_path) and os.path.getsize(Test.file_path) > 0:  # bytes
            confirmation = tk.messagebox.askyesno('query overwrite', 'File exists:  overwrite?')
            if confirmation is False:
                print('skipped overwrite')
                tkinter.messagebox.showwarning(message='Nothing changed')
                return
        save_progress_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black',
                                    text='data saving')
        tksleep(0.1)
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        save_progress_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                                    text='save_progress')
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        print('updating Test file label')
        Test.create_file_path_and_key(name_override=new_file_txt)
        tkinter.messagebox.showwarning(message="Progress saved")
        update_data_buttons()
    else:
        print('putty test file non-existent or too small (<64 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to save")


def save_putty():
    m_str = datetime.datetime.fromtimestamp(os.path.getmtime(putty_test_csv_path.get())).strftime("%Y-%m-%dT%H-%M-%S").replace(' ', 'T')
    putty_test_sav_path = tk.StringVar(master, os.path.join(path_to_temp.get(), 'putty_' + m_str + '.csv'))
    print(f"GUI_TestSOC(save_putty):\n{putty_test_csv_path.get()=}\n{putty_test_sav_path.get()=}\n")
    try:
        shutil.copyfile(putty_test_csv_path.get(), putty_test_sav_path.get())
        print('wrote', putty_test_sav_path.get())
        empty_file(putty_test_csv_path.get())
        return True
    except PermissionError:
        print('putty holding file open')
        return False


def size_of(path):
    if os.path.isfile(path) and (size := os.path.getsize(path)) > 0:  # bytes
        return size
    else:
        return 0


def start_putty():
    lookup_test()
    enter_size = putty_size()
    if enter_size >= 64:
        if not save_putty():
            tkinter.messagebox.showwarning(message="putty may be open already")
        enter_size = putty_size()
    if enter_size < 64:
        kill_putty(platform.system())
        print(f'restarting putty   putty -load {test_filename.get()}')
        subprocess.Popen(['putty', '-load', test_filename.get()], stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)
    thread = Thread(target=stay_awake, kwargs={'up_set_min': putty_timeout.get()})
    thread.start()


def start_timer():
    CountdownTimer(master, timer_val.get(), max_flash=60, exit_function=None, trigger=True)


def swap_ref_test():
    """Swap and save Test and Ref choices"""
    global Test, Ref
    swap = Test.__copy__()
    Test.super_shallow_copy(Ref)
    Ref.super_shallow_copy(swap)
    test_unit.set(Test.unit)  # does Test update
    ref_unit.set(Ref.unit)  # does Ref update
    test_battery.set(Test.battery)  # does Test update
    ref_battery.set(Ref.battery)  # does Ref update


def tksleep(t):
    """emulating time.sleep(seconds)"""
    ms = int(t*1000)
    root = tk._get_default_root()
    var = tk.IntVar(root)
    root.after(ms, lambda: var.set(1))
    root.wait_variable(var)


def update_data_buttons():
    save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                            text='save data')
    save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                               text='save data as')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


if __name__ == '__main__':
    import os
    import tkinter as tk
    from tkinter import ttk
    result_ready = 0
    thread_active = 0

    ex_root = ExRoot()

    cf = Begini(__file__, def_dict)

    # Define frames
    min_width = 800
    main_height = 500
    folder_reveal = 25
    wrap_length = 500
    wrap_length_note = 700
    note_font = ("Arial bold", 10)
    label_font = ("Arial bold", 12)
    label_font_gentle = ("Arial", 10)
    butt_font = ("Arial", 8)
    butt_font_large = ("Arial bold", 10)
    bg_color = "lightgray"

    # Master and header
    master = tk.Tk()
    master.title('State of Charge')
    master.wm_minsize(width=min_width, height=main_height)
    master.iconphoto(False, tk.PhotoImage(file='./GUI_TestSOC.png'))
    # master.geometry('%dx%d' % (master.winfo_screenwidth(), master.winfo_screenheight()))
    Ref = Exec(cf, 'ref', path_disp_len_=folder_reveal)
    Test = Exec(cf, 'test', path_disp_len_=folder_reveal)
    if platform.system() == 'Linux':
        putty_test_csv_path = tk.StringVar(master, '/home/daveg/.local/putty_test.csv')
        path_to_temp = tk.StringVar(master, '/home/daveg/.local')
    elif platform.system() == 'Darwin':
        putty_test_csv_path = tk.StringVar(master, '/Users/daveg/.local/putty_test.csv')
        path_to_temp = tk.StringVar(master, '/Users/daveg/.local')
    else:
        putty_test_csv_path = tk.StringVar(master, os.path.join(os.getenv('LOCALAPPDATA'), 'Temp', 'putty_test.csv'))
        path_to_temp = tk.StringVar(master, os.path.join(os.getenv('LOCALAPPDATA'), 'Temp'))
    print(f"{putty_test_csv_path.get()=}")
    icon_path = os.path.join(ex_root.script_loc, 'GUI_TestSOC.png')
    master.iconphoto(False, tk.PhotoImage(file=icon_path))
    top_panel = tk.Frame(master)
    top_panel.pack(expand=True, fill='both')
    top_panel_left = tk.Frame(top_panel)
    top_panel_left.pack(side='left', expand=True, fill='both')
    top_panel_left_ctr = tk.Frame(top_panel)
    top_panel_left_ctr.pack(side='left', expand=True, fill='both')
    top_panel_right_ctr = tk.Frame(top_panel)
    top_panel_right_ctr.pack(side='left', expand=True, fill='both')
    top_panel_right = tk.Frame(top_panel)
    top_panel_right.pack(side='left', expand=True, fill='both')

    tk.Label(top_panel_left, text="Item", fg="blue", font=label_font).pack(pady=2)
    tk.Label(top_panel_left_ctr, text="Test", fg="blue", font=label_font).pack(pady=2)
    model_str = cf['others']['modeling']
    if model_str == 'True':
        modeling = tk.BooleanVar(master, True)
    else:
        modeling = tk.BooleanVar(master, False)
    modeling_button = tk.Checkbutton(top_panel_right_ctr, text='modeling', bg=bg_color, variable=modeling,
                                     onvalue=True, offvalue=False)
    modeling_button.pack(pady=2, fill='x')
    modeling.trace_add('write', handle_modeling)
    ref_label = tk.Label(top_panel_right, text="Ref", fg="blue", font=label_font)
    ref_label.pack(pady=2, expand=True, fill='both')

    # Folder row
    working_label = tk.Label(top_panel_left, text="dataReduction Folder", font=label_font)
    Test.folder_button = myButton(top_panel_left_ctr, text=Test.dataReduction_folder[-folder_reveal:],
                                  command=Test.enter_data_reduction_folder,
                                  fg="blue", bg=bg_color)
    Ref.folder_button = myButton(top_panel_right, text=Ref.dataReduction_folder[-folder_reveal:],
                                 command=Ref.enter_data_reduction_folder,
                                 fg="blue", bg=bg_color)
    working_label.pack(padx=5, pady=5)
    Test.folder_button.pack(padx=5, pady=5, anchor=tk.W)
    Ref.folder_button.pack(padx=5, pady=5, anchor=tk.E)

    # Version row
    tk.Label(top_panel_left, text="Version", font=label_font).pack(pady=2)
    Test.version_button = myButton(top_panel_left_ctr, text=Test.version, command=Test.enter_version, fg="blue", bg=bg_color)
    Test.version_button.pack(pady=2)
    Ref.version_button = myButton(top_panel_right, text=Ref.version, command=Ref.enter_version, fg="blue", bg=bg_color)
    Ref.version_button.pack(pady=2)

    # Unit row
    tk.Label(top_panel_left, text="Unit", font=label_font).pack(pady=2, expand=True, fill='both')
    test_unit = tk.StringVar(master, Test.unit)
    Test.unit_button = tk.OptionMenu(top_panel_left_ctr, test_unit, *unit_list)
    test_unit.trace_add('write', handle_test_unit)
    Test.unit_button.pack(pady=2)
    ref_unit = tk.StringVar(master, Ref.unit)
    Ref.unit_button = tk.OptionMenu(top_panel_right, ref_unit, *unit_list)
    ref_unit.trace_add('write', handle_ref_unit)
    Ref.unit_button.pack(pady=2)
    
    test_filename = tk.StringVar(master, putty_connection.get(Test.unit))

    # Battery row
    tk.Label(top_panel_left, text="Battery", font=label_font).pack(pady=2, expand=True, fill='both')
    test_battery = tk.StringVar(master, Test.battery)
    Test.battery_button = tk.OptionMenu(top_panel_left_ctr, test_battery, *battery_list)
    test_battery.trace_add('write', handle_test_battery)
    Test.battery_button.pack(pady=2)
    ref_battery = tk.StringVar(master, Ref.battery)
    Ref.battery_button = tk.OptionMenu(top_panel_right, ref_battery, *battery_list)
    ref_battery.trace_add('write', handle_ref_battery)
    Ref.battery_button.pack(pady=2)

    # Key row
    tk.Label(top_panel_left, text="Key", font=label_font).pack(pady=2, expand=True, fill='both')
    Test.key_label = tk.Label(top_panel_left_ctr, text=Test.key)
    Test.key_label.pack(padx=5, pady=5)
    Ref.key_label = tk.Label(top_panel_right, text=Ref.key)
    Ref.key_label.pack(padx=5, pady=5)

    # Swap row
    tk.Label(top_panel_left, text="", font=label_font).pack(pady=2, expand=True, fill='both')
    tk.Label(top_panel_left_ctr, text="", font=label_font).pack(pady=2, expand=True, fill='both')
    swap_button = myButton(top_panel_right, text="swap Ref<-->Test", command=swap_ref_test, bg=bg_color)
    swap_button.pack(side=tk.RIGHT, padx=5, pady=5)

    # Image
    pic_path = os.path.join(ex_root.script_loc, 'GUI_TestSOC.png')
    picture = tk.PhotoImage(file=pic_path).subsample(5, 5)
    label = tk.Label(top_panel_right_ctr, image=picture)
    label.pack(padx=5, pady=5, expand=True, fill='both')

    # Option panel
    option_sep_panel = tk.Frame(master)
    option_sep_panel.pack(expand=True, fill='x')
    tk.Label(option_sep_panel, text=' ', font=("Courier", 2), bg='darkgray').pack(expand=True, fill='x')
    option_panel = tk.Frame(master)
    option_panel.pack(expand=True, fill='both')
    option_panel_left = tk.Frame(option_panel)
    option_panel_left.pack(side='left', fill='x')
    option_panel_ctr = tk.Frame(option_panel)
    option_panel_ctr.pack(side='left', expand=True, fill='both')
    option_panel_right = tk.Frame(option_panel)
    option_panel_right.pack(side='left', expand=True, fill='both')
    putty_timeout = tk.DoubleVar(master, 720.)  # minutes maximum putty run without going to sleep

    # Option row
    option = tk.StringVar(master, str(cf['others']['option']))
    option_show = tk.StringVar(master, str(cf['others']['option']))
    sel = tk.OptionMenu(option_panel_left, option, *sel_list)
    sel.config(width=20, font=butt_font)
    sel.pack(padx=5, pady=5)
    sel1 = tk.OptionMenu(option_panel_left, option, *sel_list1)
    sel1.config(width=20, font=butt_font)
    sel1.pack(padx=5, pady=5)
    option.trace_add('write', handle_option)
    Test.label = tk.Label(option_panel_ctr, text=Test.file_txt)
    Test.label.pack(padx=5, pady=5, anchor=tk.W)
    Ref.label = tk.Label(option_panel_right, text=Ref.file_txt)
    Ref.label.pack(padx=5, pady=5, anchor=tk.E)
    Test.create_file_path_and_key(cf['others']['option'])
    Ref.create_file_path_and_key(cf['others']['option'])

    # init row
    empty_csv_path = tk.StringVar(master, os.path.join(Test.dataReduction_folder, 'empty.csv'))
    dum1, init_val, dum2 = lookup.get('satInit')
    init = tk.StringVar(master, init_val)
    init_label = tk.Label(option_panel_left, text='init & clear:', font=label_font_gentle)
    init_label.pack(padx=5, pady=5)
    if platform.system() == 'Darwin':
        init_button = myButton(option_panel_ctr, text=init.get(), command=grab_init, fg="purple", bg=bg_color,
                               justify=tk.LEFT, font=("Arial", 8))
    else:
        init_button = myButton(option_panel_ctr, text=init.get(), command=grab_init, fg="purple", bg=bg_color,
                               wraplength=wrap_length, justify=tk.LEFT, font=("Arial", 8))
    if platform.system() == 'Linux':
        paste_label = tk.Label(option_panel_right, text='ctrl-shift-ins to paste', font=label_font_gentle)
    elif platform.system() == 'Darwin':
        paste_label = tk.Label(option_panel_right, text='ctrl-shift-V to paste', font=label_font_gentle)
    else:
        paste_label = tk.Label(option_panel_right, text='right-click to paste', font=label_font_gentle)
    init_button.pack(padx=5, pady=5)
    paste_label.pack(padx=5, pady=5)

    # start row
    start = tk.StringVar(master, '')
    start_label = tk.Label(option_panel_left, text='copy start:', font=label_font_gentle)
    start_label.pack(padx=5, pady=5, expand=True, fill='x')
    if platform.system() == 'Darwin':
        start_button = myButton(option_panel_ctr, text='', command=grab_start, fg="purple", bg=bg_color,
                                justify=tk.LEFT, font=butt_font)
    else:
        start_button = myButton(option_panel_ctr, text='', command=grab_start, fg="purple", bg=bg_color, wraplength=wrap_length,
                                justify=tk.LEFT, font=butt_font)
    start_button.pack(padx=5, pady=5, expand=True, fill='both')
    timer_val = tk.IntVar(master, 0)

    # macro panel
    macro_sep_panel = tk.Frame(master)
    macro_sep_panel.pack(expand=True, fill='x')
    tk.Label(macro_sep_panel, text=' ', font=("Courier", 2), bg='darkgray').pack(expand=True, fill='x')
    macro_panel = tk.Frame(master)
    macro_panel.pack(expand=True, fill='both')
    macro_panel_left = tk.Frame(macro_panel)
    macro_panel_left.pack(side='left', fill='x')
    macro_panel_ctr = tk.Frame(macro_panel)
    macro_panel_ctr.pack(side='left', expand=True, fill='both')
    macro_panel_right = tk.Frame(macro_panel)
    macro_panel_right.pack(side='left', expand=True, fill='both')

    macro_option = tk.StringVar(master, str(cf['others']['macro']))
    macro_option_show = tk.StringVar(master, str(cf['others']['macro']))

    macro_sel = tk.OptionMenu(macro_panel_left, macro_option, *macro_sel_list)
    macro_sel.config(width=20, font=butt_font)
    macro_sel.pack(padx=5, pady=5)
    macro_option.trace_add('write', handle_macro)
    macro = tk.StringVar(master, '')
    if platform.system() == 'Darwin':
        macro_button = myButton(macro_panel_ctr, text=macro.get(), command=grab_macro, fg="purple", bg=bg_color,
                                justify=tk.LEFT, font=butt_font)
    else:
        macro_button = myButton(macro_panel_ctr, text=macro.get(), command=grab_macro, fg="purple", bg=bg_color, wraplength=wrap_length,
                                justify=tk.LEFT, font=butt_font)
    macro_button.pack(padx=5, pady=5)
    get_time_button = myButton(macro_panel_right, text='grab time copy/paste buffer', command=grab_time,
                               fg="blue", bg=bg_color)
    get_time_button.pack(pady=2)

    # Note panel
    note_sep_panel = tk.Frame(master)
    note_sep_panel.pack(expand=True, fill='x')
    tk.Label(note_sep_panel, text=' ', font=("Courier", 2), bg='darkgray').pack(expand=True, fill='x')
    note_panel = tk.Frame(master)
    note_panel.pack(expand=True, fill='both')
    note_panel_left = tk.Frame(note_panel)
    note_panel_left.pack(side='left', fill='x')
    note_panel_ctr = tk.Frame(note_panel)
    note_panel_ctr.pack(side='left', expand=True, fill='both')
    note_panel_right = tk.Frame(note_panel)
    note_panel_right.pack(side='left', expand=True, fill='both')
    ev1_label = tk.Label(note_panel_ctr, text='', wraplength=wrap_length_note, justify=tk.LEFT, font=note_font)
    ev1_label.pack(padx=5, pady=5, anchor=tk.W)
    ev2_label = tk.Label(note_panel_ctr, text='', wraplength=wrap_length_note, justify=tk.LEFT, font=note_font)
    ev2_label.pack(padx=5, pady=5, anchor=tk.W)
    ev3_label = tk.Label(note_panel_ctr, text='', wraplength=wrap_length_note, justify=tk.LEFT, font=note_font)
    ev3_label.pack(padx=5, pady=5, anchor=tk.W)
    ev4_label = tk.Label(note_panel_ctr, text='', wraplength=wrap_length_note, justify=tk.LEFT, font=note_font)
    ev4_label.pack(padx=5, pady=5, anchor=tk.W)

    # Save row
    sav_panel = tk.Frame(master)
    sav_panel.pack(expand=True, fill='both')
    save_data_label = tk.Label(sav_panel, text='save data:', font=label_font_gentle)
    save_data_label.pack(side=tk.LEFT, padx=5, pady=5)
    save_data_button = myButton(sav_panel, text='save data', command=save_data, fg="red", bg=bg_color,
                                wraplength=wrap_length, justify=tk.LEFT, font=butt_font_large)
    save_data_button.pack(side=tk.LEFT, padx=5, pady=5)

    save_progress_label = tk.Label(sav_panel, text='          ', font=label_font_gentle)
    save_progress_label.pack(side=tk.LEFT, padx=5, pady=5)
    save_progress_button = myButton(sav_panel, text='save progress', command=save_progress, fg="black", bg=bg_color,
                                    wraplength=wrap_length, justify=tk.LEFT)
    save_progress_button.pack(side=tk.LEFT, padx=5, pady=5)

    clear_data_button = myButton(sav_panel, text='clear', command=clear_data_verbose, fg="red", bg=bg_color,
                                 wraplength=wrap_length, justify=tk.RIGHT)
    clear_data_button.pack(side=tk.RIGHT, padx=5, pady=5)
    save_data_as_button = myButton(sav_panel, text='save as', command=save_data_as, fg="red", bg=bg_color,
                                   wraplength=wrap_length, justify=tk.LEFT)
    save_data_as_button.pack(side=tk.RIGHT, padx=5, pady=5)

    # Run panel
    mod_in_app = tk.IntVar(master, int(cf['others']['mod_in_app']))
    run_sep_panel = tk.Frame(master)
    run_sep_panel.pack(expand=True, fill='x')
    tk.Label(run_sep_panel, text=' ', font=("Courier", 2), bg='darkgray').pack(expand=True, fill='x')
    run_panel = tk.Frame(master)
    run_panel.pack(expand=True, fill='x')
    tk.Label(run_panel, text='------->', font=("Courier", 8), bg='lightgreen').pack(side=tk.LEFT)
    if platform.system() == 'Darwin':
        run_button = myButton(run_panel, text=' Compare ', command=compare_run, fg="green", bg=bg_color,
                              justify=tk.LEFT, font=butt_font_large)
        hist_sim_button = myButton(run_panel, text=' Compare ', command=compare_hist_to_sim, fg="green", bg=bg_color,
                                   justify=tk.LEFT, font=butt_font_large)
        run_hist_button = myButton(run_panel, text=' Compare ', command=compare_run_to_hist, fg="green", bg=bg_color,
                                   justify=tk.LEFT, font=butt_font_large)
    else:
        run_button = myButton(run_panel, text=' Compare ', command=compare_run, fg="green", bg=bg_color,
                              wraplength=wrap_length, justify=tk.LEFT, font=butt_font_large)
        hist_sim_button = myButton(run_panel, text=' Compare ', command=compare_hist_to_sim, fg="green", bg=bg_color,
                                   justify=tk.LEFT, font=butt_font_large)
        run_hist_button = myButton(run_panel, text=' Compare ', command=compare_run_to_hist, fg="green", bg=bg_color,
                                   justify=tk.LEFT, font=butt_font_large)
    mod_in_app_button = myButton(run_panel, text=mod_in_app.get(), command=enter_mod_in_app, fg="green", bg=bg_color)
    run_button.pack(side=tk.LEFT, padx=5, pady=5)
    mod_in_app_button.pack(side=tk.RIGHT, padx=5, pady=5)
    hist_sim_button.pack(side=tk.RIGHT, padx=5, pady=5)
    run_hist_button.pack(side=tk.RIGHT, padx=5, pady=5)

    # Compare panel
    compare_sep_panel = tk.Frame(master)
    compare_sep_panel.pack(expand=True, fill='x')
    tk.Label(compare_sep_panel, text=' ', font=("Courier", 2), bg='darkgray').pack(expand=True, fill='x')
    tk.ttk.Separator(compare_sep_panel, orient='horizontal').pack(pady=5, side=tk.TOP)
    compare_panel = tk.Frame(master)
    compare_panel.pack(expand=True, fill='x')
    choose_label = tk.Label(compare_panel, text='choose existing files:')
    choose_label.pack(side=tk.LEFT, padx=5, pady=5)
    run_sim_choose_button = myButton(compare_panel, text='Compare Run Sim Choose', command=compare_run_sim_choose,
                                     fg="blue", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT, font=butt_font)
    run_sim_choose_button.pack(side=tk.LEFT, padx=5, pady=5)
    run_run_choose_button = myButton(compare_panel, text='Compare Run Run Choose', command=compare_run_run_choose,
                                     fg="blue", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT, font=butt_font)
    run_run_choose_button.pack(side=tk.LEFT, padx=5, pady=5)
    run_sim_choose_button = myButton(compare_panel, text='Compare Hist Sim Choose', command=compare_hist_sim_choose,
                                     fg="blue", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT, font=butt_font)
    run_sim_choose_button.pack(side=tk.LEFT, padx=5, pady=5)
    hist_hist_choose_button = myButton(compare_panel, text='Compare Hist Hist Choose', command=compare_hist_hist_choose,
                                       fg="blue", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT, font=butt_font)
    hist_hist_choose_button.pack(side=tk.LEFT, padx=5, pady=5)

    # Begin
    handle_test_unit()
    handle_ref_unit()
    handle_test_battery()
    handle_ref_battery()
    handle_modeling()
    handle_macro()
    handle_option()
    master.mainloop()
