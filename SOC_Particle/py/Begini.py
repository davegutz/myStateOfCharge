# Begini - configuration class using .ini files
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

from configparser import ConfigParser
import re
from tkinter import ttk
import tkinter.simpledialog


# Organize configuration
class Begini(ConfigParser):

    def __init__(self, name, def_dict_):
        self.name = name
        print(def_dict_)
        ConfigParser.__init__(self)
        self.read_dict(def_dict_)
        print(self.sections())

    # Config file
    def load_from_file_else_default(self, path):
        config = ConfigParser()
        if os.path.isfile(path):
            config.read(path)
        else:
            cfg_file = open(path, 'w')
            config.write(cfg_file)
            cfg_file.close()
        return config


# Executive class to control the global variables
class ExRoot:
    def __init__(self):
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.version = None
        self.root_config = None
        self.load_root_config(self.config_path)

    def enter_version(self):
        self.version = tk.simpledialog.askstring(title=__file__, prompt="Enter version <vYYYYMMDD>:")

    def load_root_config(self, config_file_path):
        self.root_config = ConfigParser()
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
        self.dataReduction_path = os.path.join(self.script_loc, '../dataReduction')
        self.version = self.cf['version']
        self.version_button = None
        if self.version is None:
            self.version = 'undefined'
        self.version_path = os.path.join(self.dataReduction_path, self.version)
        os.makedirs(self.version_path, exist_ok=True)
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
        self.file_txt = None
        self.file_path = None
        self.file_exists = None
        self.key_exists_in_file = None
        self.label = None
        print('ExTarget:  version', self.version, 'proc', self.proc, 'battery', self.battery, 'key', self.key)

    def create_file_path(self, name_override=None):
        if name_override is None:
            self.file_txt = create_file_txt(cf['others']['option'], self.proc, self.battery)
        else:
            self.file_txt = create_file_txt(name_override, self.proc, self.battery)
            self.update_file_label()
        self.file_path = os.path.join(self.version_path, self.file_txt)
        self.file_exists = os.path.isfile(self.file_path)
        self.update_file_label()

    def enter_battery(self):
        self.battery = tk.simpledialog.askstring(title=self.level,
                                                 prompt="Enter battery e.g. 'BB for Battleborn', 'CH' for CHINS:")
        self.cf['battery'] = self.battery
        self.battery_button.config(text=self.battery)
        self.create_file_path()

    def enter_key(self):
        self.key = tk.simpledialog.askstring(title=self.level, initialvalue=self.key,
                                             prompt="Enter key e.g. 'pro0p', 'pro1a', 'soc0p', 'soc1a':")
        self.cf['key'] = self.key
        self.key_button.config(text=self.key)
        self.update_file_label()

    def enter_proc(self):
        self.proc = tk.simpledialog.askstring(title=self.level, prompt="Enter Processor e.g. 'A', 'P', 'P2':")
        self.cf['processor'] = self.proc
        self.proc_button.config(text=self.proc)
        self.create_file_path()
        self.label.config(text=self.file_path)

    def enter_version(self):
        self.version = tk.simpledialog.askstring(title=self.level, prompt="Enter version <vYYYYMMDD>:")
        self.cf['version'] = self.version
        self.version_button.config(text=self.version)
        self.version_path = os.path.join(self.dataReduction_path, self.version)
        os.makedirs(self.version_path, exist_ok=True)
        self.create_file_path()
        self.label.config(text=self.file_path)

    def load_root_config(self, config_file_path):
        self.root_config = ConfigParser()
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

    def update_file_label(self):
        self.label.config(text=self.file_path)
        if self.file_exists:
            self.label.config(bg='lightgreen')
        else:
            self.label.config(bg='pink')
        self.key_exists_in_file = False
        if os.path.isfile(self.file_path):
            for line in open(self.file_path, 'r'):
                if re.search(self.key, line):
                    self.key_exists_in_file = True
                    break
        if self.key_exists_in_file:
            self.key_button.config(bg='lightgreen')
        else:
            self.key_button.config(bg='pink')


def create_file_txt(option_, proc_, battery_):
    return option_ + '_' + proc_ + '_' + battery_ + '.csv'


if __name__ == '__main__':
    import os
    import tkinter as tk
    from tkinter import ttk
    from TestSOC import modeling_handler, option_handler, sel_list
    result_ready = 0
    thread_active = 0
    global putty_shell

    ex_root = ExRoot()

    # Configuration for entire folder selection read with filepaths
    defaults = ConfigParser()
    def_dict = {'test': {"version": "g20230530",
                         "processor": "A",
                         "battery": "BB",
                         "key": "pro1"},
                'ref':  {"version": "v20230403",
                         "processor": "A",
                         "battery": "BB",
                         "key": "pro1"},
                'others': {"option": "custom",
                           'modeling': True}
                }

    cf = Begini(__file__, def_dict)

    cf_test = cf['test']
    cf_ref = cf['ref']
    Ref = ExTarget(cf_=cf_ref)
    Test = ExTarget(cf_=cf_test)

    # Define frames
    min_width = 800
    main_height = 500
    wrap_length = 800
    bg_color = "lightgray"

    # Master and header
    master = tk.Tk()
    master.title('State of Charge')
    master.wm_minsize(width=min_width, height=main_height)
    # master.geometry('%dx%d' % (master.winfo_screenwidth(), master.winfo_screenheight()))
    pwd_path = tk.StringVar(master)
    pwd_path.set(os.getcwd())
    path_to_data = os.path.join(pwd_path.get(), '../dataReduction')
    print(path_to_data)
    icon_path = os.path.join(ex_root.script_loc, 'TestSOC_Icon.png')
    master.iconphoto(False, tk.PhotoImage(file=icon_path))
    tk.Label(master, text="Item", fg="blue").grid(row=0, column=0, sticky=tk.N, pady=2)
    tk.Label(master, text="Test", fg="blue").grid(row=0, column=1, sticky=tk.N, pady=2)
    modeling = tk.BooleanVar(master)
    modeling.set(bool(cf['others']['modeling']))
    modeling_button = tk.Checkbutton(master, text='modeling', bg=bg_color, variable=modeling,
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

    # Option
    tk.ttk.Separator(master, orient='horizontal').grid(row=5, columnspan=5, pady=5, sticky='ew')
    option = tk.StringVar(master)
    option.set(str(cf['others']['option']))
    option_show = tk.StringVar(master)
    option_show.set(str(cf['others']['option']))
    sel = tk.OptionMenu(master, option, *sel_list)
    sel.config(width=20)
    sel.grid(row=6, padx=5, pady=5, sticky=tk.W)
    option.trace_add('write', option_handler)
    Test.label = tk.Label(master, text=Test.file_path, wraplength=wrap_length)
    Test.label.grid(row=6, column=1, columnspan=4, padx=5, pady=5)
    Ref.label = tk.Label(master, text=Ref.file_path, wraplength=wrap_length)
    Ref.label.grid(row=7, column=1, columnspan=4, padx=5, pady=5)
    Test.create_file_path(cf['others']['option'])
    Ref.create_file_path(cf['others']['option'])

    # Begin
    master.mainloop()
