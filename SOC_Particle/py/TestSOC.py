#  Graphical interface to test State of Charge application
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
from tkinter import Tk
import tkinter.filedialog
import tkinter.messagebox
from threading import Thread
from datetime import datetime
import platform
result_ready = 0
thread_active = 0
if platform.system() == 'Darwin':
    import ttwidgets as tktt
else:
    import tkinter as tk


def addToClipBoard(text):
    pyperclip.copy(text)


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
    def __init__(self, level=None):
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.version = None
        self.level = level
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


# r = Tk()
# r.withdraw()
# r.clipboard_clear()
# r.clipboard_append('Has clipboard?')
# r.update()  # now it stays on the clipboard after the window is closed
# print('on clipboard:', r.clipboard_get())
# result = r.selection_get(selection="CLIPBOARD")
# print('result:', result)
# addToClipBoard(result)
# r.destroy()


# --- main ---
# Configuration for entire folder selection read with filepaths
cwd_path = os.getcwd()
ex_root = ExRoot()
ex_base = ExTarget('base')
ex_test = ExTarget('test')

# Define frames
window_width = 500
root = tk.Tk()
root.maxsize(window_width, 800)
root.title('State of Charge')
icon_path = os.path.join(ex_root.script_loc, 'TestSOC_Icon.png')
root.iconphoto(False, tk.PhotoImage(file=icon_path))

# Checks
bg_color = "lightgray"
box_color = "lightgray"
relief = tk.FLAT

outer_frame = tk.Frame(root, bd=5, bg=bg_color)
outer_frame.pack(fill='x')

pic_frame = tk.Frame(root, bd=5, bg=bg_color)
pic_frame.pack(fill='x')

pad_x_frames = 1
pad_y_frames = 2

config_frame = tk.Frame(outer_frame, width=window_width, height=200, bg=box_color, bd=4)
config_frame.pack(side=tk.TOP)

config_header = tk.Label(config_frame, text='        Item    Base      Test', bg=box_color, fg="blue")
config_header.pack(side=tk.TOP)

version_desc = tk.Label(config_frame, text='Version', bg=box_color, fg="blue")
version_desc.pack(side=tk.LEFT)

base_version_button = tk.Button(config_frame, text=ex_base.version, command=ex_base.enter_version,
                                fg="blue", bg=bg_color)
base_version_button.pack(ipadx=5, pady=5, side=tk.LEFT)

test_version_button = tk.Button(config_frame, text=ex_test.version, command=ex_test.enter_version,
                                fg="blue", bg=bg_color)
test_version_button.pack(ipadx=5, pady=5, side=tk.LEFT)

#
# if platform.system() == 'Darwin':
#     folder_button = tktt.TTButton(recordings_frame, text=ex_root.rec_folder, command=select_re,
#                                   fg="blue", bg=bg_color)
# else:
#     folder_button = tk.Button(recordings_frame, text=ex_root.rec_folder, command=select_recordings_folder,
#                               fg="blue", bg=bg_color)
# folder_button.pack(ipadx=5, pady=5)
#


pic_path = os.path.join(ex_root.script_loc, 'TestSOC.png')
image = tk.Frame(pic_frame, borderwidth=2, bg=box_color)
image.pack(side=tk.TOP, fill="x")
image.picture = tk.PhotoImage(file=pic_path)
image.label = tk.Label(image, image=image.picture)
image.label.pack()

# Begin
root.mainloop()

