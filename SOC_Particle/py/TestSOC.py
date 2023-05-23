#  Graphical interface to Test State of Charge application
#  Run in PyCharm
#     or
#  'python3 TestSOC.py
#
#  2023-May-15  Dave Gutz   Create
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

import os
import shelve
import atexit
import platform
import subprocess
import pyperclip
import configparser
if platform.system() == 'Darwin':
    import ttwidgets as tktt
else:
    import tkinter as tk
import tkinter.simpledialog
result_ready = 0
thread_active = 0
global putty_shell


def default_cf(cf_):
    cf_['modeling'] = True
    cf_['option'] = 'ampHiFail'
    cf_['cf_test'] = {"version": "v20230520", "processor": "A", "battery": "CH", "key": "pro1a"}
    cf_['cf_ref'] = {"version": "v20230510", "processor": "A", "battery": "CH", "key": "pro1a"}
    return cf_


# Transient string
sel_list = ['ampHiFail', 'rapidTweakRegression', 'offSitHysBms', 'triTweakDisch', 'coldStart', 'ampHiFailFf',
            'ampLoFail', 'ampHiFailNoise', 'rapidTweakRegression40C', 'slowTweakRegression', 'satSit', 'flatSitHys',
            'offSitHysBmsNoise', 'ampHiFailSlow', 'vHiFail', 'vHiFailFf', 'pulseEKF', 'pulseSS', 'tbFailMod',
            'tbFailHdwe']
lookup = {'ampHiFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect and switch amp current failure (reset when current display changes from '50/diff' back to normal '0' and wait for CoolTerm to stop streaming.)", "'diff' will be displayed. After a bit more, current display will change to 0.", "To evaluate plots, start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).")),
          'rapidTweakRegression': ('Ff0;HR;Xp10;', 'self terminates', ('Should run three very large current discharge/recharge cycles without fault', 'Best test for seeing time skews and checking fault logic for false trips')),
          'offSitHysBms': ('Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;', ('for CompareRunRun.py Argon vs Photon builds. This is the only test for that.',)),
          'triTweakDisch': ('Ff0;D^0;Xp0;v0;Xm15;Xtt;Ca1.;Ri;Mw0;Nw0;MC0.004;Mx0.04;NC0.004;Nx0.04;Mk1;Nk1;-Dm1;-Dn1;DP1;Rb;Pa;Xf0.02;Xa-29500;XW5;XT5;XC3;W2;HR;Pf;v2;W2;Fi1000;Fo1000;Fc1000;Fd1000;FV1;FI1;FT1;XR;', 'v0;Hd;XS;Dm0;Dn0;Fi1;Fo1;Fc1;Fd1;FV0;FI0;FT0;Xp0;Ca1.;Pf;DP4;', ("Should run three very large triangle wave current discharge/recharge cycles without fault",)),
          'coldStart': ('Ff0;D^-18;Xp0;Xm247;Fi1000;Fo1000;Ca0.93;Ds0.06;Sk0.5;Rb;Rf;Dr100;DP1;v2;W100;DI40;Fi1;Fo1;', 'DI0;W10;v0;W5;Pf;Rf;Pf;v0;DP4;D^0;Ds0;Sk1;Fi1;Fo1;Ca0.93;', ("Should charge for a bit after hitting cutback on BMS.   Should see current gradually reduce.   Run until 'SAT' is displayed.   Could take ½ hour.", "The Ds term biases voc(soc) by delta x and makes a different type of saturation experience, to accelerate the test.", "Look at chart 'DOM 1' and verify that e_wrap misses ewlo_thr (thresholds moved as result of previous failures in this transient)", "Don't remember why this problem on cold day only.")),
          'ampHiFailFf': ('Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;Hd;Ff0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect but not switch amp current failure. (See 'diff' and current!=0 on OLED).", "Run about 60s. Start by looking at 'DOM 1' fig 3. No fault record (keeps recording).  Verify that on Fig 3 the e_wrap goes through a threshold ~0.4 without tripping faults.")),
          'ampLoFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm-50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Should detect and switch amp current failure.", "Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).")),
          'ampHiFailNoise': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;DT.05;DV0.05;DM.2;DN2;W50;Dm50;Dn0.0001;Ff0;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ("Noisy ampHiFail.  Should detect and switch amp current failure.", "Start looking at 'DOM 1' fig 3. Fault record (frozen). Will see 'diff' flashing on OLED even after fault cleared automatically (lost redundancy).")),
          'rapidTweakRegression40C': ('Ff0;HR;D^15;Xp10;', 'D^0;', ("Should run three very large current discharge/recharge cycles without fault",)),
          'slowTweakRegression': ('Ff0;HR;Xp11;', 'self terminates', ("Should run one very large slow (~15 min) current discharge/recharge cycle without fault.   It will take 60 seconds to start changing current.",)),
          'satSit': ('Ff0;D^0;Xp0;Xm247;Ca0.993;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10;XT10;XC1;W2;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.9962;W5;Pf;Rf;Pf;v0;DP4;', ("Should run one saturation and de-saturation event without fault.   Takes about 15 minutes.", "operate around saturation, starting below, go above, come back down. Tune Ca to start just below vsat"), "for CH set Ca0.993;"),
          'flatSitHys': ('Ff0;D^0;Xp0;Xm247;Ca0.9;Rb;Rf;Dr100;DP1;Xts;Xa-81;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.9;W5;Pf;Rf;Pf;v0;DP4;', ("Operate around 0.9.  For CHINS, will check EKF with flat voc(soc).   Takes about 10 minutes.", "Make sure EKF soc (soc_ekf) tracks actual soc without wandering.")),
          'offSitHysBmsNoise': ('Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;DT0;DV0;DM0;DN0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;', ("Stress test with 2x normal Vb noise DV0.10.  Takes about 10 minutes.", "operate around saturation, starting above, go below, come back up. Tune Ca to start just above vsat. Go low enough to exercise hys reset ", "Make sure comes back on.", "It will show one shutoff only since becomes biased with pure sine input with half of down current ignored on first cycle during the shuttoff.")),
          'ampHiFailSlow': ('Ff0;D^0;Xm247;Ca0.5;Pf;v2;W2;Dr100;DP1;HR;Dm6;Dn0.0001;Fc0.02;Fd0.5;', 'Hd;Xp0;Pf;Rf;W2;+v0;Dr100;DE20;Fc1;Fd1;Rf;Pf;', ("Should detect and switch amp current failure. Will be slow (~6 min) detection as it waits for the EKF to wind up to produce a cc_diff fault.", "Will display “diff” on OLED due to 6 A difference before switch (not cc_diff).", "EKF should tend to follow voltage while soc wanders away.")),
          'vHiFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;', 'Dv0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;', ("Should detect voltage failure and display '*fail' and 'redl' within 60 seconds.", "To diagnose, begin with DOM 1 fig. 2 or 3.   Look for e_wrap to go through ewl_thr.", "You may have to increase magnitude of injection (Dv).  The threshold is 32 * r_ss.")),
          'vHiFailFf': ('Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;', 'Dv0;Ff0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;', ("Run for about 1 minute.", "Should detect voltage failure (see DOM1 fig 2 or 3) but not display anything on OLED.")),
          'pulseEKF': ("Xp6; doesn't work now", 'n/a', ("Xp6 # TODO: doesn't work now.",)),
          'pulseSS': ("Xp7 doesn't work now", 'n/a', ("Should generate a very short <10 sec data burst with a pulse.  Look at plots for good overlay. e_wrap will have a delay.", "This is the shortest of all tests.  Useful for quick checks.")),
          'tbFailMod': ('Ff0;D^0;Ca.5;Xp0;W4;Xm247;DP1;Dr100;W2;HR;Pf;v2;Xv.002;Xu1;W200;Xu0;Xv1;W100;v0;Pf;', 'Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;', ("Run for 60 sec.   Plots DOM 1 Fig 2 or 3 should show Tb was detected as fault but not failed.",)),
          'tbFailHdwe': ('Ff0;D^0;Ca.5;Xp0;W4;Xm246;DP1;Dr100;W2;HR;Pf;v2;Xv.002;W50;Xu1;W200;Xu0;Xv1;W100;v0;Pf;', 'Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;', ("Should momentary flash '***' then clear itself.  All within 60 seconds.", "'Xp0' in reset puts Xm back to 247.")),
          }


# Executive class to control the global variables
class ExRoot:
    def __init__(self):
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.version = None
        self.root_config = None
        self.load_root_config(self.config_path)

    def enter_version(self):
        self.version = tk.simpledialog.askstring(title=self.level, prompt="Enter version <vYYYYMMDD>:")

    def load_root_config(self, config_file_path):
        self.root_config = configparser.ConfigParser()
        if os.path.isfile(config_file_path):
            self.root_config.read(config_file_path)
        else:
            cfg_file = open(config_file_path, 'w')
            self.root_config.add_section('Root Preferences')
            rec_folder_path = os.path.expanduser('~') + '/Documents/Recordings'
            if not os.path.exists(rec_folder_path):
                os.makedirs(rec_folder_path)
            self.root_config.set('Root Preferences', 'recordings path', rec_folder_path)
            self.root_config.write(cfg_file)
            cfg_file.close()
        return self.root_config

    def save_root_config(self, config_path_):
        if os.path.isfile(config_path_):
            cfg_file = open(config_path_, 'w')
            self.root_config.write(cfg_file)
            cfg_file.close()
            print('Saved', config_path_)
        return self.root_config


# Executive class to control the global variables
class ExTarget:
    def __init__(self, cf_=None, level=None):
        self.cf = cf_
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.version = self.cf['version']
        self.version_button = None
        self.battery = self.cf['battery']
        self.battery_button = None
        self.level = level
        self.level_button = None
        self.proc = self.cf['processor']
        self.proc_button = None
        self.key = self.cf['key']
        self.key_button = None
        self.root_config = None
        self.load_root_config(self.config_path)
        print('ExTarget:  version', self.version, 'proc', self.proc, 'battery', self.battery, 'key', self.key )

    def enter_battery(self):
        self.battery = tk.simpledialog.askstring(title=self.level, prompt="Enter battery e.g. 'BB for Battleborn', 'CH' for CHINS:")
        self.cf['battery'] = self.battery
        self.battery_button.config(text=self.battery)

    def enter_key(self):
        self.key = tk.simpledialog.askstring(title=self.level, prompt="Enter key e.g. 'pro0p', 'pro1a', 'soc0p', 'soc1a':")
        self.cf['key'] = self.key
        self.key_button.config(text=self.key)

    def enter_proc(self):
        self.proc = tk.simpledialog.askstring(title=self.level, prompt="Enter Processor e.g. 'A', 'P', 'P2':")
        self.cf['processor'] = self.proc
        self.proc_button.config(text=self.proc)

    def enter_version(self):
        self.version = tk.simpledialog.askstring(title=self.level, prompt="Enter version <vYYYYMMDD>:")
        self.cf['version'] = self.version
        self.version_button.config(text=self.version)

    def load_root_config(self, config_file_path):
        self.root_config = configparser.ConfigParser()
        if os.path.isfile(config_file_path):
            self.root_config.read(config_file_path)
        else:
            cfg_file = open(config_file_path, 'w')
            self.root_config.add_section('Root Preferences')
            rec_folder_path = os.path.expanduser('~') + '/Documents/Recordings'
            if not os.path.exists(rec_folder_path):
                os.makedirs(rec_folder_path)
            self.root_config.set('Root Preferences', 'recordings path', rec_folder_path)
            self.root_config.write(cfg_file)
            cfg_file.close()
        return self.root_config

    def save_root_config(self, config_path_):
        if os.path.isfile(config_path_):
            cfg_file = open(config_path_, 'w')
            self.root_config.write(cfg_file)
            cfg_file.close()
            print('Saved', config_path_)
        return self.root_config


# Global methods
def addToClipBoard(text):
    pyperclip.copy(text)


def grab_start():
    addToClipBoard(start.get())


def grab_reset():
    addToClipBoard(reset.get())


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


def modeling_handler(*args):
    cf['modeling'] = modeling.get()
    if modeling.get() is True:
        ref_remove()
    else:
        ref_restore()


def option_handler(*args):
    print('option', option.get())
    lookup_start()
    option_ = option.get()
    option_show.set(option_)
    cf['option'] = option_
    print(list(cf.items()))


def ref_remove():
    ref_label.grid_remove()
    Ref.version_button.grid_remove()
    Ref.proc_button.grid_remove()
    Ref.battery_button.grid_remove()
    Ref.key_button.grid_remove()


def ref_restore():
    ref_label.grid()
    Ref.version_button.grid()
    Ref.proc_button.grid()
    Ref.battery_button.grid()
    Ref.key_button.grid()


def save_cf():
    print('saved:', list(cf.items()))
    cf.close()


def start_putty():
    global putty_shell
    putty_shell = subprocess.Popen(['putty', '-load', 'test'], stdin=subprocess.PIPE, bufsize=1, universal_newlines=True)
    print('starting putty   putty -load test')


# --- main ---
# Configuration for entire folder selection read with filepaths
cwd_path = os.getcwd()
ex_root = ExRoot()
cf = shelve.open("TestSOC", writeback=True)
if len(cf.keys()) == 0:
    cf = default_cf(cf)
print(list(cf.items()))
cf_test = cf['cf_test']
cf_ref = cf['cf_ref']
Ref = ExTarget(cf_=cf_ref)
Test = ExTarget(cf_=cf_test)

# Define frames
min_width = 300
main_height = 500
wrap_length = 300
bg_color = "lightgray"

# Master and header
master = tk.Tk()
master.title('State of Charge')
master.wm_minsize(width=min_width, height=main_height)
icon_path = os.path.join(ex_root.script_loc, 'TestSOC_Icon.png')
master.iconphoto(False, tk.PhotoImage(file=icon_path))
tk.Label(master, text="Item", fg="blue").grid(row=0, column=0, sticky=tk.N, pady=2)
tk.Label(master, text="Test", fg="blue").grid(row=0, column=1, sticky=tk.N, pady=2)
modeling = tk.BooleanVar(master)
modeling.set(cf['modeling'])
modeling_button = tk.Checkbutton(master, text='Ref is Model', bg=bg_color, variable=modeling,
                                 onvalue=True, offvalue=False)
modeling_button.grid(row=0, column=3, pady=2, sticky=tk.N)
modeling.trace_add('write', modeling_handler)
ref_label = tk.Label(master, text="Ref", fg="blue")
ref_label.grid(row=0, column=4, sticky=tk.N, pady=2)


# Version row
tk.Label(master, text="Version").grid(row=1, column=0, pady=2)
Test.version_button = tk.Button(master, text=Test.version, command=Test.enter_version, fg="blue", bg=bg_color)
Test.version_button.grid(row=1, column=1, pady=2)
Ref.version_button = tk.Button(master, text=Ref.version, command=Ref.enter_version, fg="blue", bg=bg_color)
Ref.version_button.grid(row=1, column=4, pady=2)

# Processor row
tk.Label(master, text="Processor").grid(row=2, column=0, pady=2)
Test.proc_button = tk.Button(master, text=Test.proc, command=Test.enter_proc, fg="red", bg=bg_color)
Test.proc_button.grid(row=2, column=1, pady=2)
Ref.proc_button = tk.Button(master, text=Ref.proc, command=Ref.enter_proc, fg="red", bg=bg_color)
Ref.proc_button.grid(row=2, column=4, pady=2)

# Battery row
tk.Label(master, text="Battery").grid(row=3, column=0, pady=2)
Test.battery_button = tk.Button(master, text=Test.battery, command=Test.enter_battery, fg="green", bg=bg_color)
Test.battery_button.grid(row=3, column=1, pady=2)
Ref.battery_button = tk.Button(master, text=Ref.battery, command=Ref.enter_battery, fg="green", bg=bg_color)
Ref.battery_button.grid(row=3, column=4, pady=2)

# Key row
tk.Label(master, text="Key").grid(row=4, column=0, pady=2)
Test.key_button = tk.Button(master, text=Test.key, command=Test.enter_key, fg="purple", bg=bg_color)
Test.key_button.grid(row=4, column=1, pady=2)
Ref.key_button = tk.Button(master, text=Ref.key, command=Ref.enter_key, fg="purple", bg=bg_color)
Ref.key_button.grid(row=4, column=4, pady=2)

# Image
pic_path = os.path.join(ex_root.script_loc, 'TestSOC.png')
picture = tk.PhotoImage(file=pic_path).subsample(5, 5)
label = tk.Label(master, image=picture)
label.grid(row=1, column=2, columnspan=2, rowspan=3, padx=5, pady=5)

option = tk.StringVar(master)
option.set(cf['option'])
option_show = tk.StringVar(master)
option_show.set(cf['option'])
sel = tk.OptionMenu(master, option, *sel_list)
sel.config(width=25)
sel.grid(row=5, column=0, columnspan=3, padx=5, pady=5, sticky=tk.W)
option.trace_add('write', option_handler)

putty_shell = None
putty_label = tk.Label(master, text='start putty:')
putty_label.grid(row=6, column=0, padx=5, pady=5)
putty_button = tk.Button(master, text='putty -load test', command=start_putty, fg="red", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
putty_button.grid(row=6, column=1, columnspan=2, rowspan=1, padx=5, pady=5)

start = tk.StringVar(master)
start.set('')
start_label = tk.Label(master, text='copy start:')
start_label.grid(row=7, column=0, padx=5, pady=5)
start_button = tk.Button(master, text='', command=grab_start, fg="purple", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
start_button.grid(row=7, column=1, columnspan=4, rowspan=2, padx=5, pady=5)

reset = tk.StringVar(master)
reset.set('')
reset_label = tk.Label(master, text='copy reset:')
reset_label.grid(row=9, column=0, padx=5, pady=5)
reset_button = tk.Button(master, text='', command=grab_reset, fg="purple", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
reset_button.grid(row=9, column=1, columnspan=4, rowspan=2, padx=5, pady=5)

ev1_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev1_label.grid(row=11, column=1, columnspan=4, padx=5, pady=5)

ev2_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev2_label.grid(row=12, column=1, columnspan=4, padx=5, pady=5)

ev3_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev3_label.grid(row=13, column=1, columnspan=4, padx=5, pady=5)

ev4_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev4_label.grid(row=14, column=1, columnspan=4, padx=5, pady=5)

# Begin
atexit.register(save_cf)  # shelve needs to be handled
modeling_handler()
option_handler()
master.mainloop()

