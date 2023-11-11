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
from CompareRunSim import compare_run_sim
from CompareRunRun import compare_run_run
import shutil
import pyperclip
import subprocess
import datetime
# import sys
import platform
if platform.system() == 'Darwin':
    from ttwidgets import TTButton as myButton
else:
    from tkinter import Button as myButton
global putty_shell

# sys.stdout = open('logs.txt', 'w')
# sys.stderr = open('logse.txt', 'w')

# Transient string
sel_list = ['custom', 'ampHiFail', 'rapidTweakRegression', 'offSitHysBmsBB', 'offSitHysBmsCH', 'triTweakDisch', 'coldStart', 'ampHiFailFf',
            'ampLoFail', 'ampHiFailNoise', 'rapidTweakRegression40C', 'slowTweakRegression', 'satSitBB', 'satSitCH', 'flatSitHys',
            'offSitHysBmsNoiseBB', 'offSitHysBmsNoiseCH', 'ampHiFailSlow', 'vHiFail', 'vHiFailFf', 'pulseEKF', 'pulseSS', 'tbFailMod',
            'tbFailHdwe', 'DvMon', 'DvSim']
lookup = {'custom': ('', '', ("For general purpose running", "'save data' will present a choice of file name", "")),
          'ampHiFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect and switch amp current failure (reset when current display changes from '50/diff' back to normal '0' and wait for CoolTerm to stop streaming.)", "'diff' will be displayed. After a bit more, current display will change to 0.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).")),
          'rapidTweakRegression': ('Ff0;HR;Xp10;', 'self terminates', ('Should run three very large current discharge/recharge cycles without fault', 'Best test for seeing time skews and checking fault logic for false trips')),
          'offSitHysBmsBB': ('Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;', ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',)),
          'offSitHysBmsCH': ('Ff0;D^0;Xp0;Xm247;Ca0.103;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.103;W5;Pf;Rf;Pf;v0;DP4;', ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',)),
          'triTweakDisch': ('Ff0;D^0;Xp0;v0;Xm15;Xtt;Ca1.;Ri;Mw0;Nw0;MC0.004;Mx0.04;NC0.004;Nx0.04;Mk1;Nk1;-Dm1;-Dn1;DP1;Rb;Pa;Xf0.02;Xa-29500;XW5;XT5;XC3;W2;HR;Pf;v2;W2;Fi1000;Fo1000;Fc1000;Fd1000;FV1;FI1;FT1;XR;', 'v0;Hd;XS;Dm0;Dn0;Fi1;Fo1;Fc1;Fd1;FV0;FI0;FT0;Xp0;Ca1.;Pf;DP4;', ("Should run three very large triangle wave current discharge/recharge cycles without fault",)),
          'coldStart': ('Ff0;D^-18;Xp0;Xm247;Fi1000;Fo1000;Ca0.93;Ds0.06;Sk0.5;Rb;Rf;Dr100;DP1;v2;W100;DI40;Fi1;Fo1;', 'DI0;W10;v0;W5;Pf;Rf;Pf;v0;DP4;D^0;Ds0;Sk1;Fi1;Fo1;Ca0.93;', ("Should charge for a bit after hitting cutback on BMS.   Should see current gradually reduce.   Run until 'SAT' is displayed.   Could take ½ hour.", "The Ds term biases voc(soc) by delta x and makes a different type of saturation experience, to accelerate the test.", "Look at chart 'DOM 1' and verify that e_wrap misses ewlo_thr (thresholds moved as result of previous failures in this transient)", "Don't remember why this problem on cold day only.")),
          'ampHiFailFf': ('Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;Hd;Ff0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect but not switch amp current failure. (See 'diff' and current!=0 on OLED).", "Run about 60s. Start by looking at 'DOM 1' fig 3. No fault record (keeps recording).  Verify that on Fig 3 the e_wrap goes through a threshold ~0.4 without tripping faults.")),
          'ampLoFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm-50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect and switch amp current failure.", "Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).")),
          'ampHiFailNoise': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;DT.05;DV0.05;DM.2;DN2;W50;Dm50;Dn0.0001;Ff0;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Noisy ampHiFail.  Should detect and switch amp current failure.", "Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).")),
          'rapidTweakRegression40C': ('Ff0;HR;D^15;Xp10;', 'D^0;', ("Should run three very large current discharge/recharge cycles without fault", "Self-terminates")),
          'slowTweakRegression': ('Ff0;HR;Xp11;', 'v0;', ("Should run one very large slow (~15 min) current discharge/recharge cycle without fault.   It will take 60 seconds to start changing current.",)),
          'satSitBB': ('Ff0;D^0;Xp0;Xm247;Ca0.9962;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10;XT10;XC1;W2;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.9962;W5;Pf;Rf;Pf;v0;DP4;', ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",)),
          'satSitCH': ('Ff0;D^0;Xp0;Xm247;Ca0.992;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10;XT10;XC1;W2;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.992;W5;Pf;Rf;Pf;v0;DP4;', ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat",)),
          'flatSitHys': ('Ff0;D^0;Xp0;Xm247;Ca0.9;Rb;Rf;Dr100;DP1;Xts;Xa-81;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.9;W5;Pf;Rf;Pf;v0;DP4;', ("Operate around 0.9.  For CHINS, will check EKF with flat voc(soc).   Takes about 10 minutes.", "Make sure EKF soc (soc_ekf) tracks actual soc without wandering.")),
          'offSitHysBmsNoiseBB': ('Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;DT0;DV0;DM0;DN0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;', ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff.")),
          'offSitHysBmsNoiseCH': ('Ff0;D^0;Xp0;Xm247;Ca0.103;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;DT0;DV0;DM0;DN0;Ca.103;W5;Pf;Rf;Pf;v0;DP4;', ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shutoff.")),
          'ampHiFailSlow': ('Ff0;D^0;Xm247;Ca0.5;Pf;v2;W2;Dr100;DP1;HR;Dm6;Dn0.0001;Fc0.02;Fd0.5;', 'Hd;Xp0;Pf;Rf;W2;+v0;Dr100;DE20;Fc1;Fd1;Rf;Pf;', ("Should detect and switch amp current failure. Will be slow (~6 min) detection as it waits for the EKF to wind up to produce a cc_diff fault.", "Will display “diff” on OLED due to 6 A difference before switch (not cc_diff).", "EKF should tend to follow voltage while soc wanders away.", "Run for 6  minutes to see cc_diff_fa")),
          'vHiFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dm0.001;Dv0.82;', 'Dv0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;', ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with DOM 1 fig. 2 or 3.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.", "There MUST be no SATURATION")),
          'vHiFailFf': ('Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;', 'Dv0;Ff0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;', ("Run for about 1 minute.", "Should detect voltage failure (see DOM1 fig 2 or 3) but not display anything on OLED.")),
          'pulseEKF': ("Xp6; doesn't work", 'n/a', ("Xp6 # TODO: doesn't work now.",)),
          'pulseSS': ("Xp7;", 'n/a', ("Should generate a very short <10 sec data burst with a pulse.  Look at plots for good overlay. e_wrap will have a delay.", "This is the shortest of all tests.  Useful for quick checks.")),
          'tbFailMod': ('Ff0;D^0;Ca.5;Xp0;W4;Xm247;DP1;Dr100;W2;HR;Pf;v2;Xv.002;Xu1;W200;Xu0;Xv1;W100;v0;Pf;', 'Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;', ("Run for 60 sec.   Plots DOM 1 Fig 2 or 3 should show Tb was detected as fault but not failed.",)),
          'tbFailHdwe': ('Ff0;D^0;Ca.5;Xp0;W4;Xm246;DP1;Dr100;W2;HR;Pf;v2;Xv.002;W50;Xu1;W200;Xu0;Xv1;W100;v0;Pf;', 'Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;', ("Run for 60 sec.   Plots DOM 1 Fig 2 or 3 should show Tb was detected as fault but not failed.", "'Xp0' in reset puts Xm back to 247.")),
          'DvMon': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm0.001;Dw-0.8;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;Dw0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect and switch voltage failure and use vb_model", "'*fail' will be displayed.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'redl' flashing on OLED even after fault cleared automatically (lost redundancy).", "Run for 15  minutes to see cc_diff_fa")),
          'DvSim': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm0.001;Dy-0.8;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;Dy0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect and switch voltage failure and use vb_model", "'*fail' will be displayed.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'redl' flashing on OLED even after fault cleared automatically (lost redundancy).", "Run for 15  minutes to see cc_diff_fa")),
          }


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
    def __init__(self, cf_, ind, level=None):
        self.cf = cf_
        self.ind = ind
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.dataReduction_folder = cf[self.ind]['dataReduction_folder']
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
        self.folder_butt = myButton(master, text=self.dataReduction_folder, command=self.enter_dataReduction_folder,
                                    fg="blue", bg=bg_color)
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

    def create_file_path_and_key(self, name_override=None):
        if name_override is None:
            self.file_txt = create_file_txt(cf['others']['option'], self.unit, self.battery)
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
        cf['others']['dataReduction_folder'] = self.dataReduction_folder
        cf.save_to_file()
        self.folder_butt.config(text=self.dataReduction_folder)
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
        cf[self.ind]['version'] = self.version
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
        self.folder_butt.config(text=self.dataReduction_folder)
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


# Global methods
def add_to_clip_board(text):
    pyperclip.copy(text)


# Compare run driver
def clear_data():
    if os.path.isfile(putty_test_csv_path.get()):
        enter_size = putty_size()  # bytes
        time.sleep(1.)
        wait_size = putty_size()  # bytes
    else:
        enter_size = 0
        wait_size = 0
    if enter_size > 512:
        if wait_size > enter_size:
            print('stop data first')
            tkinter.messagebox.showwarning(message="stop data first")
        else:
            # create empty file
            if not save_putty():
                tkinter.messagebox.showwarning(message="putty may be open already")
            else:
                open(empty_csv_path.get(), 'x')
                shutil.copyfile(empty_csv_path.get(), putty_test_csv_path.get())
                print('emptied', putty_test_csv_path.get())
            try:
                os.remove(empty_csv_path.get())
            except OSError:
                pass
            reset_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    else:
        print('putty test file is too small (<512 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to clear")


def compare_run():
    if not Test.key_exists_in_file:
        tkinter.messagebox.showwarning(message="Test Key '" + Test.key + "' does not exist in " + Test.file_txt)
        return
    if modeling.get():
        print('compare_run_sim')
        master.withdraw()
        compare_run_sim(data_file_path=Test.file_path, unit_key=Test.key,
                        save_pdf_path=os.path.join(Test.version_path, './figures'),
                        path_to_temp=os.path.join(Test.version_path, './temp'))
        master.deiconify()
    else:
        if not Ref.key_exists_in_file:
            tkinter.messagebox.showwarning(message="Ref Key '" + Ref.key + "' does not exist in " + Ref.file_txt)
            return
        print('GUI_TestSOC compare_run:  Ref', Ref.file_path, Ref.key)
        print('GUI_TestSOC compare_run:  Test', Test.file_path, Test.key)
        keys = [(Ref.file_txt, Ref.key), (Test.file_txt, Test.key)]
        master.withdraw()
        compare_run_run(keys=keys, dir_data_ref_path=Ref.version_path, dir_data_test_path=Test.version_path,
                        save_pdf_path=os.path.join(Test.version_path, './figures'),
                        path_to_temp=os.path.join(Test.version_path, './temp'))
        master.deiconify()


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
                master.withdraw()
                compare_run_run(keys=keys, dir_data_ref_path=ref_folder_path,
                                dir_data_test_path=test_folder_path,
                                save_pdf_path=test_folder_path + './figures',
                                path_to_temp=test_folder_path + './temp')
                master.deiconify()
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


def grab_reset():
    add_to_clip_board(reset.get())
    reset_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def grab_start():
    add_to_clip_board(start.get())
    save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black', text='save data')
    save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black', text='save data as')
    start_button.config(bg='yellow', activebackground='yellow', fg='black', activeforeground='black')
    reset_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


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
    save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black', text='save data')
    save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black', text='save data as')
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    reset_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def lookup_start():
    start_val, reset_val, ev_val = lookup.get(option.get())
    start.set(start_val)
    start_button.config(text=start.get())
    reset.set(reset_val)
    reset_button.config(text=reset.get())
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


def putty_size():
    if os.path.isfile(putty_test_csv_path.get()):
        enter_size = os.path.getsize(putty_test_csv_path.get())  # bytes
    else:
        enter_size = 0
    return enter_size


def ref_remove():
    ref_label.grid_remove()
    Ref.version_button.grid_remove()
    Ref.unit_button.grid_remove()
    Ref.battery_button.grid_remove()
    Ref.key_label.grid_remove()
    Ref.label.grid_remove()
    Ref.folder_butt.grid_remove()
    run_button.config(text='Compare Run Sim')


def ref_restore():
    ref_label.grid()
    Ref.version_button.grid()
    Ref.unit_button.grid()
    Ref.battery_button.grid()
    Ref.key_label.grid()
    Ref.label.grid()
    Ref.folder_butt.grid()
    run_button.config(text='Compare Run Run')


def save_data():
    if size_of(putty_test_csv_path.get()) > 512:  # bytes
        # create empty file
        try:
            open(empty_csv_path.get(), 'x')
        except FileExistsError:
            pass
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
                print('reset and use clear')
                tkinter.messagebox.showwarning(message='reset and use clear')
                return
        copy_clean(putty_test_csv_path.get(), Test.file_path)
        print('copied ', putty_test_csv_path.get(), '\nto\n', Test.file_path)
        save_data_button.config(bg='green', activebackground='green', fg='red', activeforeground='red', text='data saved')
        save_data_as_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black', text='data saved')
        shutil.copyfile(empty_csv_path.get(), putty_test_csv_path.get())
        print('emptied', putty_test_csv_path.get())
        try:
            os.remove(empty_csv_path.get())
        except OSError:
            pass
        print('updating Test file label')
        Test.create_file_path_and_key(name_override=new_file_txt)
    else:
        print('putty test file non-existent or too small (<512 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to save")
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    reset_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def save_data_as():
    if size_of(putty_test_csv_path.get()) > 512:  # bytes
        # create empty file
        try:
            open(empty_csv_path.get(), 'x')
        except FileExistsError:
            pass
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
        save_data_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='black', text='data saved')
        save_data_as_button.config(bg='green', activebackground='green', fg='red', activeforeground='red', text='data saved as')
        shutil.copyfile(empty_csv_path.get(), putty_test_csv_path.get())
        print('emptied', putty_test_csv_path.get())
        try:
            os.remove(empty_csv_path.get())
        except OSError:
            pass
        print('updating Test file label')
        Test.create_file_path_and_key(name_override=new_file_txt)
    else:
        print('putty test file is too small (<512 bytes) probably already done')
        tkinter.messagebox.showwarning(message="Nothing to save")
    start_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')
    reset_button.config(bg=bg_color, activebackground=bg_color, fg='black', activeforeground='purple')


def save_putty():
    m_str = datetime.datetime.fromtimestamp(os.path.getmtime(putty_test_csv_path.get())).strftime("%Y-%m-%dT%H-%M-%S").replace(' ', 'T')
    putty_test_sav = 'putty_' + m_str + '.csv'
    putty_test_sav_path = tk.StringVar(master)
    putty_test_sav_path.set(os.path.join(Test.dataReduction_folder, putty_test_sav))
    try:
        shutil.move(putty_test_csv_path.get(), putty_test_sav_path.get())
        print('wrote', putty_test_sav_path.get())
        return True
    except OSError:
        print('putty already open?')
        return False


def size_of(path):
    if os.path.isfile(path) and (size := os.path.getsize(path)) > 0:  # bytes
        return size
    else:
        return 0


def start_putty():
    global putty_shell
    enter_size = putty_size()
    if enter_size > 512:
        if not save_putty():
            tkinter.messagebox.showwarning(message="putty may be open already")
        enter_size = putty_size()
    if enter_size <= 512:
        putty_shell = subprocess.Popen(['putty', '-load', 'test'], stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)
        print('starting putty   putty -load test')


if __name__ == '__main__':
    import os
    import tkinter as tk
    from tkinter import ttk
    result_ready = 0
    thread_active = 0

    ex_root = ExRoot()

    # Configuration for entire folder selection read with filepaths
    def_dict = {'test': {"version": "g20230530",
                         "unit": "pro1a",
                         "battery": "bb",
                         'dataReduction_folder': '<enter data dataReduction_folder>'},
                'ref':  {"version": "v20230403",
                         "unit": "pro1a",
                         "battery": "bb",
                         'dataReduction_folder': '<enter data dataReduction_folder>'},
                'others': {"option": "custom",
                           'modeling': True}
              }

    cf = Begini(__file__, def_dict)

    # Define frames
    min_width = 800
    main_height = 500
    wrap_length = 800
    bg_color = "lightgray"
    row = -1

    # Master and header
    row += 1
    master = tk.Tk()
    master.title('State of Charge')
    master.wm_minsize(width=min_width, height=main_height)
    # master.geometry('%dx%d' % (master.winfo_screenwidth(), master.winfo_screenheight()))
    Ref = Exec(cf, 'ref')
    Test = Exec(cf, 'test')
    putty_test_csv_path = tk.StringVar(master, os.path.join(ex_root.script_loc, '../dataReduction/putty_test.csv'))
    icon_path = os.path.join(ex_root.script_loc, 'GUI_TestSOC_Icon.png')
    master.iconphoto(False, tk.PhotoImage(file=icon_path))
    tk.Label(master, text="Item", fg="blue").grid(row=row, column=0, sticky=tk.N, pady=2)
    tk.Label(master, text="Test", fg="blue").grid(row=row, column=1, sticky=tk.N, pady=2)
    model_str = cf['others']['modeling']
    if model_str == 'True':
        modeling = tk.BooleanVar(master, True)
    else:
        modeling = tk.BooleanVar(master, False)
    modeling_button = tk.Checkbutton(master, text='modeling', bg=bg_color, variable=modeling,
                                     onvalue=True, offvalue=False)
    modeling_button.grid(row=row, column=3, pady=2, sticky=tk.N)
    modeling.trace_add('write', handle_modeling)
    ref_label = tk.Label(master, text="Ref", fg="blue")
    ref_label.grid(row=row, column=4, sticky=tk.N, pady=2)

    # Folder row
    row += 1
    working_label = tk.Label(master, text="dataReduction folder=")
    Test.folder_butt = myButton(master, text=Test.dataReduction_folder, command=Test.enter_dataReduction_folder, fg="blue", bg=bg_color)
    Ref.folder_butt = myButton(master, text=Ref.dataReduction_folder, command=Ref.enter_dataReduction_folder, fg="blue", bg=bg_color)
    working_label.grid(row=row, column=0, padx=5, pady=5)
    Test.folder_butt.grid(row=row, column=1, padx=5, pady=5)
    Ref.folder_butt.grid(row=row, column=4, padx=5, pady=5)

    # Version row
    row += 1
    tk.Label(master, text="Version").grid(row=row, column=0, pady=2)
    Test.version_button = myButton(master, text=Test.version, command=Test.enter_version, fg="blue", bg=bg_color)
    Test.version_button.grid(row=row, column=1, pady=2)
    Ref.version_button = myButton(master, text=Ref.version, command=Ref.enter_version, fg="blue", bg=bg_color)
    Ref.version_button.grid(row=row, column=4, pady=2)

    # Unit row
    row += 1
    tk.Label(master, text="Unit").grid(row=row, column=0, pady=2)
    Test.unit_button = myButton(master, text=Test.unit, command=Test.enter_unit, fg="purple", bg=bg_color)
    Test.unit_button.grid(row=row, column=1, pady=2)
    Ref.unit_button = myButton(master, text=Ref.unit, command=Ref.enter_unit, fg="purple", bg=bg_color)
    Ref.unit_button.grid(row=row, column=4, pady=2)

    # Battery row
    row += 1
    tk.Label(master, text="Battery").grid(row=row, column=0, pady=2)
    Test.battery_button = myButton(master, text=Test.battery, command=Test.enter_battery, fg="green", bg=bg_color)
    Test.battery_button.grid(row=row, column=1, pady=2)
    Ref.battery_button = myButton(master, text=Ref.battery, command=Ref.enter_battery, fg="green", bg=bg_color)
    Ref.battery_button.grid(row=row, column=4, pady=2)

    # Key row
    row += 1
    tk.Label(master, text="Key").grid(row=row, column=0, pady=2)
    Test.key_label = tk.Label(master, text=Test.key)
    Test.key_label.grid(row=row, column=1,  padx=5, pady=5)
    Ref.key_label = tk.Label(master, text=Ref.key)
    Ref.key_label.grid(row=row, column=4, padx=5, pady=5)

    # Image
    pic_path = os.path.join(ex_root.script_loc, 'GUI_TestSOC.png')
    picture = tk.PhotoImage(file=pic_path).subsample(5, 5)
    label = tk.Label(master, image=picture)
    label.grid(row=1, column=2, columnspan=2, rowspan=3, padx=5, pady=5)

    # Option
    row += 1
    tk.ttk.Separator(master, orient='horizontal').grid(row=row, columnspan=5, pady=5, sticky='ew')
    row += 1
    option = tk.StringVar(master)
    option.set(str(cf['others']['option']))
    option_show = tk.StringVar(master)
    option_show.set(str(cf['others']['option']))
    sel = tk.OptionMenu(master, option, *sel_list)
    sel.config(width=20)
    sel.grid(row=row, padx=5, pady=5, sticky=tk.W)
    option.trace_add('write', handle_option)
    Test.label = tk.Label(master, text=Test.file_txt)
    Test.label.grid(row=row, column=1, padx=5, pady=5)
    Ref.label = tk.Label(master, text=Ref.file_txt)
    Ref.label.grid(row=row, column=4, padx=5, pady=5)
    Test.create_file_path_and_key(cf['others']['option'])
    Ref.create_file_path_and_key(cf['others']['option'])

    row += 1
    putty_shell = None
    putty_label = tk.Label(master, text='start putty:')
    putty_label.grid(row=row, column=0, padx=5, pady=5)
    putty_button = myButton(master, text='putty -load test', command=start_putty, fg="green", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
    putty_button.grid(sticky="W", row=row, column=1, columnspan=2, rowspan=1, padx=5, pady=5)
    empty_csv_path = tk.StringVar(master)
    empty_csv_path.set(os.path.join(Test.dataReduction_folder, 'empty.csv'))

    row += 1
    start = tk.StringVar(master)
    start.set('')
    start_label = tk.Label(master, text='copy start:')
    start_label.grid(row=row, column=0, padx=5, pady=5)
    start_button = myButton(master, text='', command=grab_start, fg="purple", bg=bg_color, wraplength=wrap_length,
                            justify=tk.LEFT, font=("Arial", 8))
    start_button.grid(sticky="W", row=row, column=1, columnspan=4, rowspan=1, padx=5, pady=5)

    row += 1
    reset = tk.StringVar(master)
    reset.set('')
    reset_label = tk.Label(master, text='copy reset:')
    reset_label.grid(row=row, column=0, padx=5, pady=5)
    reset_button = myButton(master, text='', command=grab_reset, fg="purple", bg=bg_color, wraplength=wrap_length,
                             justify=tk.LEFT, font=("Arial", 8))
    reset_button.grid(sticky="W", row=row, column=1, columnspan=4, rowspan=1, padx=5, pady=5)

    row += 1
    ev1_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
    ev1_label.grid(sticky="W", row=row, column=1, columnspan=4, padx=5, pady=5)

    row += 1
    ev2_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
    ev2_label.grid(sticky="W", row=row, column=1, columnspan=4, padx=5, pady=5)

    row += 1
    ev3_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
    ev3_label.grid(sticky="W", row=row, column=1, columnspan=4, padx=5, pady=5)

    row += 1
    ev4_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
    ev4_label.grid(sticky="W", row=row, column=1, columnspan=4, padx=5, pady=5)

    row += 1
    save_data_label = tk.Label(master, text='save data:')
    save_data_label.grid(row=row, column=0, padx=5, pady=5)
    save_data_button = myButton(master, text='save data', command=save_data, fg="red", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
    save_data_button.grid(sticky="W", row=row, column=1, padx=5, pady=5)
    save_data_as_button = myButton(master, text='save as', command=save_data_as, fg="red", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
    save_data_as_button.grid(sticky="W", row=row, column=2, padx=5, pady=5)
    clear_data_button = myButton(master, text='clear', command=clear_data, fg="red", bg=bg_color, wraplength=wrap_length, justify=tk.RIGHT)
    clear_data_button.grid(sticky="W", row=row, column=3, padx=5, pady=5)

    row += 1
    tk.ttk.Separator(master, orient='horizontal').grid(row=row, columnspan=5, pady=5, sticky='ew')
    row += 1
    run_button = myButton(master, text='Compare', command=compare_run, fg="green", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
    run_button.grid(row=row, column=0, padx=5, pady=5)

    row += 1
    tk.ttk.Separator(master, orient='horizontal').grid(row=row, columnspan=5, pady=5, sticky='ew')
    row += 1
    choose_label = tk.Label(master, text='choose existing files:')
    choose_label.grid(row=row, column=0, padx=5, pady=5)
    run_sim_choose_button = myButton(master, text='Compare Run Sim Choose', command=compare_run_sim_choose, fg="blue", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
    run_sim_choose_button.grid(sticky="W", row=row, column=1, padx=5, pady=5)
    run_run_choose_button = myButton(master, text='Compare Run Run Choose', command=compare_run_run_choose, fg="blue", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
    run_run_choose_button.grid(sticky="W", row=row, column=2, padx=5, pady=5)

    # Begin
    handle_modeling()
    handle_option()
    master.mainloop()
