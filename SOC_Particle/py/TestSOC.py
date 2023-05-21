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
import pyperclip
import configparser
import platform
import shelve
import atexit
if platform.system() == 'Darwin':
    import ttwidgets as tktt
else:
    import tkinter as tk
import tkinter.simpledialog
result_ready = 0
thread_active = 0


def default_cf(cf_):
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
          'triTweakDisch': ('Ff0;D^0;Xp0;v0;Xm15;Xtt;Ca1.;Ri;Mw0;Nw0;MC0.004;Mx0.04;NC0.004;Nx0.04;Mk1;Nk1;-Dm1;-Dn1;DP1;Rb;Pa;Xf0.02;Xa-29500;XW5;XT5;XC3;W2;HR;Pf;v2;W2;Fi1000;Fo1000;Fc1000;Fd1000;FV1;FI1;FT1;XR;', 'v0;Hd;XS;Dm0;Dn0;Fi1;Fo1;Fc1;Fd1;FV0;FI0;FT0;Xp0;Ca1.;Pf;DP4;', ('',)),
          'coldStart': ('Ff0;D^-18;Xp0;Xm247;Fi1000;Fo1000;Ca0.93;Ds0.06;Sk0.5;Rb;Rf;Dr100;DP1;v2;W100;DI40;Fi1;Fo1;', 'DI0;W10;v0;W5;Pf;Rf;Pf;v0;DP4;D^0;Ds0;Sk1;Fi1;Fo1;Ca0.93;', ('',)),
          'ampHiFailFf': ('Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;Hd;Ff0;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ('',)),
          'ampLoFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;Dm-50;Dn0.0001;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ('',)),
          'ampHiFailNoise': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W30;DT.05;DV0.05;DM.2;DN2;W50;Dm50;Dn0.0001;Ff0;', 'Hs;Hs;Hs;Hs;Pf;DT0;DV0;DM0;DN0;Xp0;Rf;W200;+v0;Ca.5;Dr100;Rf;Pf;DP4;', ('',)),
          'rapidTweakRegression40C': ('Ff0;HR;D^15;Xp10;', 'D^0;', ('',)),
          'slowTweakRegression': ('Ff0;HR;Xp11;', 'self terminates', ('',)),
          'satSit': ('Ff0;D^0;Xp0;Xm247;Ca0.993;Rb;Rf;Dr100;DP1;Xts;Xa17;Xf0.002;XW10;XT10;XC1;W2;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.9962;W5;Pf;Rf;Pf;v0;DP4;', ('',)),
          'flatSitHys': ('Ff0;D^0;Xp0;Xm247;Ca0.9;Rb;Rf;Dr100;DP1;Xts;Xa-81;Xf0.004;XW10;XT10;XC2;W2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;Ca.9;W5;Pf;Rf;Pf;v0;DP4;', ('',)),
          'offSitHysBmsNoise': ('Ff0;D^0;Xp0;Xm247;Ca0.05;Rb;Rf;Dr100;DP1;Xts;Xa-162;Xf0.004;XW10;XT10;XC2;W2;DT.05;DV0.10;DM.2;DN2;Ph;HR;Pf;v2;W5;XR;', 'XS;v0;Hd;Xp0;DT0;DV0;DM0;DN0;Ca.05;W5;Pf;Rf;Pf;v0;DP4;', ('',)),
          'ampHiFailSlow': ('Ff0;D^0;Xm247;Ca0.5;Pf;v2;W2;Dr100;DP1;HR;Dm6;Dn0.0001;Fc0.02;Fd0.5;', 'Hd;Xp0;Pf;Rf;W2;+v0;Dr100;DE20;Fc1;Fd1;Rf;Pf;', ('',)),
          'vHiFail': ('Ff0;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;', 'Dv0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;', ('',)),
          'vHiFailFf': ('Ff1;D^0;Xm247;Ca0.5;Dr100;DP1;HR;Pf;v2;W50;Dv0.8;', 'Dv0;Ff0;Hd;Xp0;Rf;W50;+v0;Dr100;Rf;Pf;DP4;', ('',)),
          'pulseEKF': ("Xp6; doesn't work now", 'n/a', ('',)),
          'pulseSS': ("Xp7 doesn't work now", 'n/a', ('',)),
          'tbFailMod': ('Ff0;D^0;Ca.5;Xp0;W4;Xm247;DP1;Dr100;W2;HR;Pf;v2;Xv.002;Xu1;W200;Xu0;Xv1;W100;v0;Pf;', 'Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;', ('',)),
          'tbFailHdwe': ('Ff0;D^0;Ca.5;Xp0;W4;Xm246;DP1;Dr100;W2;HR;Pf;v2;Xv.002;W50;Xu1;W200;Xu0;Xv1;W100;v0;Pf;', 'Hd;Xp0;Xu0;Xv1;Ca.5;v0;Rf;Pf;DP4;', ('',)),
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


def save_cf():
    print('saved:', list(cf.items()))
    cf.close()


def show_option(*args):
    print('option', option.get())
    lookup_start()
    option_ = option.get()
    option_show.set(option_)
    cf['option'] = option_
    print(list(cf.items()))


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
tk.Label(master, text="Ref", fg="blue").grid(row=0, column=4, sticky=tk.N, pady=2)
tk.Label(master, text="Test", fg="blue").grid(row=0, column=1, sticky=tk.N, pady=2)

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
label.grid(row=0, column=2, columnspan=2, rowspan=4, padx=5, pady=5)

option = tk.StringVar(master)
option.set(cf['option'])
option_show = tk.StringVar(master)
option_show.set(cf['option'])
sel = tk.OptionMenu(master, option, *sel_list)
sel.config(width=25)
sel.grid(row=5, column=0, columnspan=3, padx=5, pady=5, sticky=tk.W)
option.trace_add('write', show_option)

start = tk.StringVar(master)
start.set('')
start_label = tk.Label(master, text='grab start:')
start_label.grid(row=6, column=0, padx=5, pady=5)
start_button = tk.Button(master, text='', command=grab_start, fg="purple", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
start_button.grid(row=6, column=1, columnspan=4, rowspan=2, padx=5, pady=5)

reset = tk.StringVar(master)
reset.set('')
reset_label = tk.Label(master, text='grab reset:')
reset_label.grid(row=8, column=0, padx=5, pady=5)
reset_button = tk.Button(master, text='', command=grab_reset, fg="purple", bg=bg_color, wraplength=wrap_length, justify=tk.LEFT)
reset_button.grid(row=8, column=1, columnspan=4, rowspan=2, padx=5, pady=5)

ev1_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev1_label.grid(row=10, column=1, columnspan=4, padx=5, pady=5)

ev2_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev2_label.grid(row=11, column=1, columnspan=4, padx=5, pady=5)

ev3_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev3_label.grid(row=12, column=1, columnspan=4, padx=5, pady=5)

ev4_label = tk.Label(master, text='', wraplength=wrap_length, justify=tk.LEFT)
ev4_label.grid(row=13, column=1, columnspan=4, padx=5, pady=5)

# Begin
atexit.register(save_cf)  # shelve needs to be handled
show_option()
master.mainloop()

