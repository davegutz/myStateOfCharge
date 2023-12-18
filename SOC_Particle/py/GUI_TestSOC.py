#  Graphical interface to Test State of Charge application
#  Run in PyCharm
#     or
#  'python3 GUI_TestSOC.py
#
#  2023-Jun-15  Dave Gutz   Create
# Copyright (C) 2023 Dave Gutz
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
import time
from configparser import ConfigParser
import re
from tkinter import ttk, filedialog
import tkinter.simpledialog
import tkinter.messagebox
from CompareHistSim import compare_hist_sim
# from CompareHistRun import compare_hist_run
from CompareRunSim import compare_run_sim
from CompareRunRun import compare_run_run
from CountdownTimer import CountdownTimer
import shutil
import pyperclip
import subprocess
import datetime
import platform
import Colors
from test_soc_util import run_shell_cmd
if platform.system() == 'Darwin':
    from ttwidgets import TTButton as myButton
else:
    from tkinter import Button as myButton
bg_color = 'lightgray'

# sys.stdout = open('logs.txt', 'w')
# sys.stderr = open('logse.txt', 'w')

# Configuration for entire folder selection read with filepaths
def_dict = {'test': {"version": "g20230530",
                     "unit": "pro1a",
                     "battery": "bb",
                     'dataReduction_folder': '<enter data dataReduction_folder>'},
            'ref': {"version": "v20230403",
                    "unit": "pro1a",
                    "battery": "bb",
                    'dataReduction_folder': '<enter data dataReduction_folder>'},
            'others': {"option": "custom",
                       'mod_in_app': "247",
                       'modeling': True}
            }

# Transient string
unit_list = ['pro0p', 'pro1a', 'pro3p2', 'pro2p2', 'soc0p', 'soc1a']
batt_list = ['bb', 'ch']
sel_list = ['custom', 'init1', 'saveAdjusts', 'ampHiFail', 'rapidTweakRegression', 'pulseSS', 'rapidTweakRegressionH0', 'offSitHysBmsBB',
            'offSitHysBmsCH', 'triTweakDisch', 'coldStart', 'ampHiFailFf', 'ampLoFail', 'ampHiFailNoise',
            'rapidTweakRegression40C', 'slowTweakRegression', 'satSitBB', 'satSitCH', 'flatSitHys',
            'offSitHysBmsNoiseBB', 'offSitHysBmsNoiseCH', 'ampHiFailSlow', 'vHiFail', 'vHiFailH', 'vHiFailFf',
            'pulseEKF', 'pulseSSH', 'tbFailMod', 'tbFailHdwe', 'DvMon', 'DvSim', 'faultParade']
lookup = {'init': ('Y;c;Dh1800000;*W;*vv0;*XS;*Ca1;<HR;<Pf;', ('',), 10, 12),
          'initMid': ('Y;c;Dh1800000;*W;*vv0;*XS;*Ca.5;<HR;<Pf;', ('',), 10, 12),
          'saveAdjusts': ('Dr100;DP1;vv4;PR;PV;Bm1;Pr;Bm0;Pr;BP2;Pr;BP1;Pr;BS2;Pr;BS1;Pr;Bs1;Pr;Bs0;Pr;DA5;Pr;DB-5;Pr;RS;Pr;Dc0.2;Pr;Dc0;DI-10;Pr;DI0;Pr;Dt5;Pr;Dt0;Pr;SA2;Pr;SA1;Pr;SB2;Pr;SB1;Pr;si-1;Pr;RS;Pr;Sk2;Pr;Sk1;Pr;SQ2;Pr;SQ1;Pr;Sq3;Pr;Sq1;Pr;SV1.1;Pr;SV1;Pr;Xb10;Pr;Xb0;Pr;Xa1000;Pr;Xa0;Pr;Xf1;Pr;RS;Pr;Xm10;Pr;RS;Pr;W3;vv0;XQ3;PR;PV;', ("For testing out the adjustments and memory", "Read through output and witness set and reset of all", "The DS2482 moderate headroom should not exceed limit printed.  EG 11 of 12 is ok."), 60, 0),
          'custom': ('', ("For general purpose data collection", "'save data' will present a choice of file name", ""), 60, 12),
          'ampHiFail': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;Dh1000;W20;Dm50;Dn0.0001;', ("Should detect and switch amp current failure (reset when current display changes from '50/diff' back to normal '0' and wait for CoolTerm to stop streaming.)", "'diff' will be displayed. After a bit more, current display will change to 0.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change"),20, 12),
          'rapidTweakRegression': ('Ff0;HR;Xp10;Dh1000;', ('Should run three very large current discharge/recharge cycles without fault', 'Best test for seeing time skews and checking fault logic for false trips'), 180, 12),
          'pulseSS': ("Xp7;", ("Should generate a very short <10 sec data burst with a sw pulse.  Look at plots for good overlay. e_wrap will have a delay.", "This is the shortest of all tests.  Useful for quick checks."), 15, 1),
          'rapidTweakRegressionH0': ('Sh0;SH0;Ff0;HR;Xp10;', ('Should run three very large current discharge/recharge cycles without fault', 'No hysteresis. Best test for seeing time skews and checking fault logic for false trips', 'Tease out cause of e_wrap faults.  e_wrap MUST be flat!'), 180, 12),
          'offSitHysBmsBB': ('Ff0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10000;XT10;XC2;W1;Ph;HR;Pf;vv2;W1;XR;', ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',), 568, 12),
          'offSitHysBmsCH': ('Ff0;Xp0;Xm247;Ca0.103;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10000;XT10;XC2;W1;Ph;HR;Pf;vv2;W1;XR;', ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',), 568, 12),
          'triTweakDisch': ('Ff0;HR;Xp13;', ('Should run three very large current discharge/recharge cycles without fault', 'Best test for seeing time skews and checking fault logic for false trips'), 180, 12),
          'coldStart': ('Ff0;D^-18;Xp0;Xm247;Fi1000;Fo1000;Ca0.93;Ds0.06;Sk0.5;Rb;Rf;Dr100;DP1;vv2;W100;DI40;Fi1;Fo1;', ("Should charge for a bit after hitting cutback on BMS.   Should see current gradually reduce.   Run until 'SAT' is displayed.   Could take ½ hour.", "The Ds term biases voc(soc) by delta x and makes a different type of saturation experience, to accelerate the test.", "Look at chart 'DOM 1' and verify that e_wrap misses ewlo_thr (thresholds moved as result of previous failures in this transient)", "Don't remember why this problem on cold day only."), 220, 12),
          'ampHiFailFf': ('Ff1;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;Dh1000;W20;Dm50;Dn0.0001;', ("Should detect but not switch amp current failure. (See 'diff' and current!=0 on OLED).", "Run about 60s. Start by looking at 'DOM 1' fig 3. No fault record (keeps recording).  Verify that on Fig 3 the e_wrap goes through a threshold ~0.4 without tripping faults.", "This show when deploy with Fake Faults (Ff) don't throw false trips (it happened)", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change"), 60, 12),
          'ampLoFail': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;Dh1000;W20;Dm-50;Dn0.0001;', ("Should detect and switch amp current failure.", "Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change"), 30, 12),
          'ampHiFailNoise': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;DT.05;DV0.05;DM.2;DN2;Dm50;Dn0.0001;Ff0;', ("Noisy ampHiFail.  Should detect and switch amp current failure.", "Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).", "ib_diff_fa will set red_loss but wait for wrap_fa to isolate and make selection change"), 30, 12),
          'rapidTweakRegression40C': ('Ff0;HR;D^15;Xp10;', ("Should run three very large current discharge/recharge cycles without fault", "Self-terminates"), 180, 12),
          'slowTweakRegression': ('Ff0;HR;Xp11;', ("Should run one very large slow (~15 min) current discharge/recharge cycle without fault.   It will take 60 seconds to start changing current.",), 622, 12),
          'satSitBB': ('Ff0;Xp0;Xm247;Ca0.9962;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10000;XT10;XC1;W1;HR;Pf;vv2;W1;XR;', ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",), 600, 12),
          'satSitCH': ('Ff0;Xp0;Xm247;Ca0.992;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10000;XT10;XC1;W1;HR;Pf;vv2;W1;XR;', ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",), 600, 12),
          'flatSitHys': ('Ff0;Xp0;Xm247;Ca0.9;Rb;Rf;Dr100;DP1;Xts;Xa-81;Xf0.004;XW10000;XT10;XC2;W1;Ph;HR;Pf;vv2;W1;XR;', ("Operate around 0.9.  For CHINS, will check EKF with flat voc(soc).   Takes about 10 minutes.", "Make sure EKF soc (soc_ekf) tracks actual soc without wandering."), 568, 12),
          'offSitHysBmsNoiseBB': ('Ff0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10000;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;vv2;W1;XR;', ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff."), 568, 12),
          'offSitHysBmsNoiseCH': ('Ff0;Xp0;Xm247;Ca0.103;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10000;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;vv2;W1;XR;', ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff."), 568, 12),
          'ampHiFailSlow': ('Ff0;Xm247;Ca0.5;Pf;vv2;W2;Dr100;DP1;HR;Dm6;Dn0.0001;Fc0.02;Fd0.5;', ("Should detect and switch amp current failure. Will be slow (~6 min) detection as it waits for the EKF to wind up to produce a cc_diff fault.", "Will display “diff” on OLED due to 6 A difference before switch (not cc_diff).", "EKF should tend to follow voltage while soc wanders away.", "Run for 6  minutes to see cc_diff_fa"), 400, 12),
          'vHiFail': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;W50;Dm0.001;Dv0.82;', ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with DOM 1 fig. 2 or 3.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION"), 60, 12),
          'vHiFailH': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;W50;SH.3;W10;Dm0.001;Dv0.82;', ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with DOM 1 fig. 2 or 3.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION.  Initial BB shift will be limited by hys table"), 30, 12),
          'vHiFailFf': ('Ff1;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;W10;Dv0.8;', ("Run for about 1 minute.", "Should detect voltage failure (see DOM1 fig 2 or 3) but not display anything on OLED.", "Usually shows SAT."), 60, 12),
          'pulseEKF': ("Xp6; doesn't work", ("Xp6 # TODO: doesn't work now.",), 0, 0),
          'pulseSSH': ("Xp8;", ("Should generate a very short <10 sec data burst with a hw pulse.  Look at plots for good overlay. e_wrap should be flat.", "This is the shortest of all tests.  Useful for quick checks."), 15, 1),
          'tbFailMod': ('Ff0;Ca.5;Xp0;Xm247;DP1;Dr100;W2;HR;Pf;vv2;Xv.002;Xu1;W4;Xu0;Xv1;W20;vv0;Pf;', ("Run for 60 sec.   Plots DOM 1 Fig 2 or 3 should show Tb was detected as fault but not failed.",), 60, 12),
          'tbFailHdwe': ('Ff0;Ca.5;Xp0;Xm246;DP1;Dr100;W2;HR;Pf;vv2;Xv.002;W10;Xu1;W20;Xu0;Xv1;W20;vv0;Pf;', ("Run for 60 sec.   Plots DOM 1 Fig 2 or 3 should show Tb was detected as fault but not failed.", "'Xp0' in reset puts Xm back to 247."), 60, 12),
          'DvMon': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;W30;Dm0.001;Dw-0.8;Dn0.0001;', ("Should detect and switch voltage failure and use vb_model", "'*fail' will be displayed.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'redl' flashing on OLED even after fault cleared automatically (lost redundancy).", "Run for 2 min to confirm no cc_diff_fa"), 120, 12),
          'DvSim': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;vv2;W30;Dm0.001;Dy-0.8;Dn0.0001;', ("Should detect and switch voltage failure and use vb_model", "'*fail' will be displayed.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'redl' flashing on OLED even after fault cleared automatically (lost redundancy).", "Run for 2 min to confirm no cc_diff_fa"), 120, 12),
          'faultParade': ('Ff0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;Dh1000;vv2;Dm50;Dn0.0001;W200;Dm0;Dn0;W20;Rf;', ("Check fault, history, and summary logging", "Should flag faults but take no action", "", "", ""), 240, 80),
          }
putty_connection = {'': 'test',
                    'pro0p': 'testpro0p',
                    'pro1a': 'testpro1a',
                    'pro3p2': 'testpro3p2'}


# Begini - configuration class using .ini files
class Begini(ConfigParser):

    def __init__(self, name, def_dict_):
        ConfigParser.__init__(self)

        (config_path, config_basename) = os.path.split(name)
        config_txt = os.path.splitext(config_basename)[0] + '.ini'
        self.config_file_path = os.path.join(config_path, config_txt)
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
    def __init__(self, cf_, ind, level=None, path_disp_len_=25):
        self.cf = cf_
        self.ind = ind
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.dataReduction_folder = self.cf[self.ind]['dataReduction_folder']
        self.version = self.cf[self.ind]['version']
        self.version_button = None
        if self.version is None:
            self.version = 'undefined'
        self.version_path = os.path.join(self.dataReduction_folder, self.version)
        os.makedirs(self.version_path, exist_ok=True)
        self.battery = self.cf[self.ind]['battery']
        self.battery_button = None
        self.level = level
        self.level_button = None
        self.folder_butt = myButton(master, text=self.dataReduction_folder[-20:],
                                    command=self.enter_dataReduction_folder, fg="blue", bg=bg_color)
        self.unit = self.cf[self.ind]['unit']
        self.unit_button = None
        self.key_label = None
        self.root_config = None
        self.load_root_config(self.config_path)
        self.file_txt = None
        self.file_path = None
        self.file_exists = None
        self.dataReduction_folder_exists = None
        self.key_exists_in_file = None
        self.label = None
        self.key = None
        self.path_disp_len = path_disp_len_

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
        self.update_folder_butt()

    def enter_battery(self):
        self.battery = tk.simpledialog.askstring(title=self.level,
                                                 prompt="Enter battery e.g. 'bb for Battleborn', 'ch' for CHINS:")
        self.cf[self.ind]['battery'] = self.battery
        self.cf.save_to_file()
        self.battery_button.config(text=self.battery)
        self.create_file_path_and_key()
        self.update_key_label()

    def enter_dataReduction_folder(self):
        answer = tk.filedialog.askdirectory(title="Select a destination (i.e. Library) dataReduction folder",
                                            initialdir=self.dataReduction_folder)
        if answer is not None and answer != '':
            self.dataReduction_folder = answer
        self.cf[self.ind]['dataReduction_folder'] = self.dataReduction_folder
        self.cf.save_to_file()
        self.folder_butt.config(text=self.dataReduction_folder[self.path_disp_len:])
        self.update_folder_butt()

    def enter_unit(self):
        self.unit = tk.simpledialog.askstring(title=self.level, initialvalue=self.unit,
                                              prompt="Enter unit e.g. 'pro0p', 'pro1a', 'soc0p', 'soc1a':")
        self.cf[self.ind]['unit'] = self.unit
        self.cf.save_to_file()
        self.unit_button.config(text=self.unit)
        self.create_file_path_and_key()
        self.update_key_label()
        self.update_file_label()

    def enter_version(self):
        self.version = tk.simpledialog.askstring(title=__file__, prompt="Enter version <vYYYYMMDD>:",
                                                 initialvalue=self.version)
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

    def update_battery_stuff(self):
        self.cf[self.ind]['unit'] = self.unit
        self.cf[self.ind]['battery'] = self.battery
        self.cf.save_to_file()
        self.create_file_path_and_key()
        self.update_key_label()
        self.update_file_label()

    def update_file_label(self):
        self.label.config(text=self.file_txt)
        if self.file_exists:
            self.label.config(bg='lightgreen')
        else:
            self.label.config(bg='pink')

    def update_folder_butt(self):
        if os.path.exists(self.dataReduction_folder):
            self.dataReduction_folder_exists = True
        else:
            self.dataReduction_folder_exists = False
        self.folder_butt.config(text=self.dataReduction_folder[-self.path_disp_len:])
        if self.dataReduction_folder_exists:
            self.folder_butt.config(bg='lightgreen')
        else:
            self.folder_butt.config(bg='pink')

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
        if silent is False:
            print('putty test file non-existent or too small (<64 bytes) probably already done')
            tkinter.messagebox.showwarning(message="Nothing to clear")


def compare_hist():
    if not Test.key_exists_in_file:
        tkinter.messagebox.showwarning(message="Test Key '" + Test.key + "' does not exist in " + Test.file_txt)
        return
    if modeling.get():
        print('compare_hist.  save_pdf_path', os.path.join(Test.version_path, './figures'))
        # master.withdraw()
        chm = None
        if Test.battery == 'bb':
            chm = 0
        elif Test.battery == 'ch':
            chm = 1
        compare_hist_sim(data_file=Test.file_path,
                         rel_path_to_save_pdf=os.path.join(Test.version_path, './figures'),
                         rel_path_to_temp=os.path.join(Test.version_path, './temp'),
                         chm_in=chm, mod_in=mod_in_app.get())
        # master.deiconify()
    else:
        if not Ref.key_exists_in_file:
            tkinter.messagebox.showwarning(message="Ref Key '" + Ref.key + "' does not exist in " + Ref.file_txt)
            return
        print('GUI_TestSOC compare_hist:  Ref', Ref.file_path, Ref.key)
        print('GUI_TestSOC compare_hist:  Test', Test.file_path, Test.key)
        keys = [(Ref.file_txt, Ref.key), (Test.file_txt, Test.key)]
        # master.withdraw()
        chm = None
        if Test.battery == 'bb':
            chm = 0
        elif Test.battery == 'ch':
            chm = 1
        print(f"make compare_hist_run.py")
        # compare_hist_run(keys=keys, dir_data_ref_path=Ref.version_path, dir_data_test_path=Test.version_path,
        #                 save_pdf_path=os.path.join(Test.version_path, './figures'),
        #                 path_to_temp=os.path.join(Test.version_path, './temp'))
        # master.deiconify()


def compare_run():
    if not Test.key_exists_in_file:
        tkinter.messagebox.showwarning(message="Test Key '" + Test.key + "' does not exist in " + Test.file_txt)
        return
    if modeling.get():
        print('compare_run_sim.  save_pdf_path', os.path.join(Test.version_path, './figures'))
        # master.withdraw()
        compare_run_sim(data_file=Test.file_path, unit_key=Test.key,
                        rel_path_to_save_pdf=os.path.join(Test.version_path, './figures'),
                        rel_path_to_temp=os.path.join(Test.version_path, './temp'))
        # master.deiconify()
    else:
        if not Ref.key_exists_in_file:
            tkinter.messagebox.showwarning(message="Ref Key '" + Ref.key + "' does not exist in " + Ref.file_txt)
            return
        print('GUI_TestSOC compare_run:  Ref', Ref.file_path, Ref.key)
        print('GUI_TestSOC compare_run:  Test', Test.file_path, Test.key)
        keys = [(Ref.file_txt, Ref.key), (Test.file_txt, Test.key)]
        # master.withdraw()
        compare_run_run(keys=keys, data_file_folder_ref=Ref.version_path, data_file_folder_test=Test.version_path,
                        rel_path_to_save_pdf=os.path.join(Test.version_path, './figures'),
                        rel_path_to_temp=os.path.join(Test.version_path, './temp'))
        # master.deiconify()


# Choose file to perform compare_run_run on
def compare_run_run_choose():
    # Select file
    print('compare_run_run_choose')
    testpaths = filedialog.askopenfilenames(title='Choose test file(s)', filetypes=[('csv', '.csv')])
    if testpaths is None or testpaths == '':
        print("No file chosen")
    else:
        for testpath in testpaths:
            test_folder_path, test_parent, test_basename, test_txt, test_key = contain_all(testpath)
            if test_key != '':
                ref_path = filedialog.askopenfilename(title='Choose reference file', filetypes=[('csv', '.csv')])
                ref_folder_path, ref_parent, ref_basename, ref_txt, ref_key = contain_all(ref_path)
                keys = [ref_key, test_key]
                # master.withdraw()
                compare_run_run(keys=keys, dir_data_ref_path=ref_folder_path,
                                dir_data_test_path=test_folder_path,
                                save_pdf_path=test_folder_path + './figures',
                                path_to_temp=test_folder_path + './temp')
                # master.deiconify()
            else:
                tk.messagebox.showerror(message='key not found in' + testpath)


# Choose file to perform compare_run_sim on
def compare_run_sim_choose():
    # Select file
    print('compare_run_sim_choose')
    testpaths = filedialog.askopenfilenames(title='Please select files', filetypes=[('csv', '.csv')])
    if testpaths is None or testpaths == '':
        print("No file chosen")
    else:
        for testpath in testpaths:
            test_folder_path, test_parent, basename, test_txt, key = contain_all(testpath)
            if key != '':
                compare_run_sim(data_file_path=testpath, unit_key=key,
                                save_pdf_path=os.path.join(test_folder_path, './figures'),
                                path_to_temp=os.path.join(test_folder_path, './temp'))
            else:
                tk.messagebox.showerror(message='key not found in' + testpath)


# Split all information contained in file path
def contain_all(testpath):
    folder_path, basename = os.path.split(testpath)
    parent, txt = os.path.split(folder_path)
    # get key
    key = ''
    with open(testpath, 'r') as file:
        for line in file:
            if line.__contains__(txt):
                us_loc = line.find('_' + txt)
                key = (basename, line[:us_loc])
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
    mod_in_app.set(tk.simpledialog.askinteger(title=__file__, prompt="enter the value of Modeling in app to assume", initialvalue=mod_in_app.get()))
    cf['others']['mod_in_app'] = str(mod_in_app.get())
    cf.save_to_file()
    mod_in_app_button.config(text=mod_in_app.get())


def end_early():
    add_to_clip_board(init.get())
    end_early_butt.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    init_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def grab_init():
    add_to_clip_board(init.get())
    end_early_butt.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black')
    init_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    clear_data_silent()
    print('cleared putty data file')
    Test.create_file_path_and_key()
    Test.update_key_label()
    start_putty()


def grab_start():
    add_to_clip_board(start.get())
    save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                            text='save data')
    save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                               text='save data as')
    end_early_butt.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black')
    init_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    start_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    start_timer()


def handle_modeling(*args):
    cf['others']['modeling'] = str(modeling.get())
    cf.save_to_file()
    if modeling.get() is True:
        ref_remove()
    else:
        ref_restore()


def handle_option(*args):
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
        if Test.battery == 'ch' or Ref.battery == 'ch':
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


def handle_ref_batt(*args):
    Ref.battery = ref_batt.get()
    Ref.update_battery_stuff()
    update_data_buttons()


def handle_ref_unit(*args):
    Ref.unit = ref_unit.get()
    Ref.update_battery_stuff()
    update_data_buttons()


def handle_test_batt(*args):
    Test.battery = test_batt.get()
    Test.update_battery_stuff()
    update_data_buttons()


def handle_test_unit(*args):
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


def lookup_start():
    start_val, ev_val, timer_val_, dawdle_val_ = lookup.get(option.get())
    start.set(start_val+'XQ'+str(timer_val_*1000)+';')
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
    timer_val.set(timer_val_ + dawdle_val_)


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
    comp_button.config(text='Compare Hist Sim')
    Ref.label.forget()


def ref_restore():
    top_panel_right.pack(expand=True, fill='both')
    run_button.config(text='Compare Run Run')
    comp_button.config(text='Compare Hist Run')
    Ref.label.pack(padx=5, pady=5)


def save_data():
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
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        save_data_button.config(bg='green', activebackground='green', fg='red', activeforeground='red',
                                text='data saved')
        save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
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
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black',
                                text='data saved')
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
                tkinter.messagebox.showwarning(message='retained ' + Test.file_path)
                return
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        print('updating Test file label')
        Test.create_file_path_and_key(name_override=new_file_txt)
    else:
        print('putty test file non-existent or too small (<64 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to save")


def save_putty():
    m_str = datetime.datetime.fromtimestamp(os.path.getmtime(putty_test_csv_path.get())).strftime("%Y-%m-%dT%H-%M-%S").replace(' ', 'T')
    putty_test_sav_path = tk.StringVar(master, os.path.join(Test.dataReduction_folder, 'putty_' + m_str + '.csv'))
    print(f"{putty_test_csv_path.get()=} {putty_test_sav_path.get()=}")
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
        subprocess.Popen(['putty', '-load', test_filename.get()], stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)
        print('restarting putty   putty -load test')


def start_timer():
    CountdownTimer(master, timer_val.get(), max_flash=60, exit_function=None, trigger=True)


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
    # master.geometry('%dx%d' % (master.winfo_screenwidth(), master.winfo_screenheight()))
    Ref = Exec(cf, 'ref', path_disp_len_=folder_reveal)
    Test = Exec(cf, 'test', path_disp_len_=folder_reveal)
    putty_test_csv_path = tk.StringVar(master, os.path.join(ex_root.script_loc, '../dataReduction/putty_test.csv'))
    icon_path = os.path.join(ex_root.script_loc, 'GUI_TestSOC_Icon.png')
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
    Test.folder_butt = myButton(top_panel_left_ctr, text=Test.dataReduction_folder[-folder_reveal:], command=Test.enter_dataReduction_folder,
                                fg="blue", bg=bg_color)
    Ref.folder_butt = myButton(top_panel_right, text=Ref.dataReduction_folder[-folder_reveal:], command=Ref.enter_dataReduction_folder,
                               fg="blue", bg=bg_color)
    working_label.pack(padx=5, pady=5)
    Test.folder_butt.pack(padx=5, pady=5, anchor=tk.W)
    Ref.folder_butt.pack(padx=5, pady=5, anchor=tk.E)

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
    test_batt = tk.StringVar(master, Test.battery)
    Test.battery_button = tk.OptionMenu(top_panel_left_ctr, test_batt, *batt_list)
    test_batt.trace_add('write', handle_test_batt)
    Test.battery_button.pack(pady=2)
    ref_batt = tk.StringVar(master, Ref.battery)
    Ref.battery_button = tk.OptionMenu(top_panel_right, ref_batt, *batt_list)
    ref_batt.trace_add('write', handle_ref_batt)
    Ref.battery_button.pack(pady=2)

    # Key row
    tk.Label(top_panel_left, text="Key", font=label_font).pack(pady=2, expand=True, fill='both')
    Test.key_label = tk.Label(top_panel_left_ctr, text=Test.key)
    Test.key_label.pack(padx=5, pady=5)
    Ref.key_label = tk.Label(top_panel_right, text=Ref.key)
    Ref.key_label.pack(padx=5, pady=5)

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

    # Option row
    option = tk.StringVar(master, str(cf['others']['option']))
    option_show = tk.StringVar(master, str(cf['others']['option']))
    sel = tk.OptionMenu(option_panel_left, option, *sel_list)
    sel.config(width=20, font=butt_font)
    sel.pack(padx=5, pady=5)
    option.trace_add('write', handle_option)
    Test.label = tk.Label(option_panel_ctr, text=Test.file_txt)
    Test.label.pack(padx=5, pady=5, anchor=tk.W)
    Ref.label = tk.Label(option_panel_right, text=Ref.file_txt)
    Ref.label.pack(padx=5, pady=5, anchor=tk.E)
    Test.create_file_path_and_key(cf['others']['option'])
    Ref.create_file_path_and_key(cf['others']['option'])

    empty_csv_path = tk.StringVar(master, os.path.join(Test.dataReduction_folder, 'empty.csv'))
    init_val, dum1, dum2, dum3 = lookup.get('init')
    init = tk.StringVar(master, init_val)
    init_label = tk.Label(option_panel_left, text='init & clear:', font=label_font_gentle)
    init_label.pack(padx=5, pady=5)
    if platform.system() == 'Darwin':
        init_button = myButton(option_panel_ctr, text=init.get(), command=grab_init, fg="purple", bg=bg_color,
                               justify=tk.LEFT, font=("Arial", 8))
    else:
        init_button = myButton(option_panel_ctr, text=init.get(), command=grab_init, fg="purple", bg=bg_color,
                               wraplength=wrap_length, justify=tk.LEFT, font=("Arial", 8))
    init_button.pack(padx=5, pady=5)

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
    end_early_butt = myButton(option_panel_right, text='END EARLY', command=end_early, fg="black", bg=bg_color,
                              justify=tk.RIGHT, font=butt_font)
    end_early_butt.pack(padx=5, pady=5, side=tk.BOTTOM)

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
        comp_button = myButton(run_panel, text=' Compare ', command=compare_hist, fg="green", bg=bg_color,
                              justify=tk.LEFT, font=butt_font_large)
    else:
        run_button = myButton(run_panel, text=' Compare ', command=compare_run, fg="green", bg=bg_color,
                              wraplength=wrap_length, justify=tk.LEFT, font=butt_font_large)
        comp_button = myButton(run_panel, text=' Compare ', command=compare_hist, fg="green", bg=bg_color,
                               justify=tk.LEFT, font=butt_font_large)
    mod_in_app_button = myButton(run_panel, text=mod_in_app.get(), command=enter_mod_in_app, fg="green", bg=bg_color)
    run_button.pack(side=tk.LEFT, padx=5, pady=5)
    mod_in_app_button.pack(side=tk.RIGHT, padx=5, pady=5)
    comp_button.pack(side=tk.RIGHT, padx=5, pady=5)

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


    # Begin
    handle_test_unit()
    handle_ref_unit()
    handle_test_batt()
    handle_ref_batt()
    handle_modeling()
    handle_option()
    master.mainloop()
