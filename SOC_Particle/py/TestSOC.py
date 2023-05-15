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
    def __init__(self, level=None, proc=None):
        self.script_loc = os.path.dirname(os.path.abspath(__file__))
        self.config_path = os.path.join(self.script_loc, 'root_config.ini')
        self.version = None
        self.level = level
        self.proc = proc
        self.root_config = None
        self.load_root_config(self.config_path)

    def enter_version(self):
        self.version = tk.simpledialog.askstring(title=self.level, prompt="Enter version <vYYYYMMDD>:")

    def enter_proc(self):
        self.proc = tk.simpledialog.askstring(title=self.level, prompt="Enter Processor e.g. 'A', 'P', 'P2':")

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
ex_base = ExTarget('base', 'A')
ex_test = ExTarget('test', 'A')

# Define frames
window_width = 500
item_width = 150
base_width = 175
test_width = 175
pad_x_frames = 1
pad_y_frames = 2

# root = tk.Tk()
# # root.maxsize(window_width, 800)
# root.title('State of Charge')
# icon_path = os.path.join(ex_root.script_loc, 'TestSOC_Icon.png')
# root.iconphoto(False, tk.PhotoImage(file=icon_path))
#
# # Checks
# bg_color = "lightgray"
# box_color = "lightgray"
# relief = tk.FLAT
#
# config_header0 = tk.Label(root, text='Item', bg=box_color, fg="blue", width=item_width)
# config_header0.grid(row=0, column=0, pady=2)
# config_header1 = tk.Label(root, text='Base', bg=box_color, fg="blue", width=base_width)
# config_header1.grid(row=0, column=1, pady=2)
# config_header2 = tk.Label(root, text='Test', bg=box_color, fg="blue", width=test_width)
# config_header2.grid(row=0, column=2, pady=2)
#
#
# # Version row
# version_desc = tk.Label(root, text='Version', bg=box_color, fg="blue", width=item_width)
# config_header0.grid(row=1, column=0, pady=2)
# base_version_button = tk.Button(root, text=ex_base.version, command=ex_base.enter_version,
#                                 fg="blue", bg=bg_color)
# config_header0.grid(row=1, column=1, pady=2)
# test_version_button = tk.Button(root, text=ex_test.version, command=ex_test.enter_version,
#                                 fg="blue", bg=bg_color)
# config_header0.grid(row=1, column=2, pady=2)
#
# # Processor row
# proc_desc = tk.Label(root, text='Processor', bg=box_color, fg="red", width=item_width)
# base_proc_button = tk.Button(root, text=ex_base.proc, command=ex_base.enter_proc,
#                                 fg="red", bg=bg_color)
# test_proc_button = tk.Button(root, text=ex_test.proc, command=ex_test.enter_proc,
#                                 fg="red", bg=bg_color)
#
#
#
# #
# # if platform.system() == 'Darwin':
# #     folder_button = tktt.TTButton(recordings_frame, text=ex_root.rec_folder, command=select_re,
# #                                   fg="blue", bg=bg_color)
# # else:
# #     folder_button = tk.Button(recordings_frame, text=ex_root.rec_folder, command=select_recordings_folder,
# #                               fg="blue", bg=bg_color)
# # folder_button.pack(ipadx=5, pady=5)
# #
#
#
# pic_path = os.path.join(ex_root.script_loc, 'TestSOC.png')
# image = tk.Frame(root, borderwidth=2, bg=box_color)
# # image.pack(side=tk.TOP, fill="x")
# image.picture = tk.PhotoImage(file=pic_path)
# image.label = tk.Label(image, image=image.picture)
# # image.label.pack()


# creating main tkinter window/toplevel
master = tk.Tk()
l1 = tk.Label(master, text="Height")
l2 = tk.Label(master, text="Width")
l1.grid(row=0, column=0, sticky=tk.W, pady=2)
l2.grid(row=1, column=0, sticky=tk.W, pady=2)
e1 = tk.Entry(master)
e2 = tk.Entry(master)
e1.grid(row=0, column=1, pady=2)
e2.grid(row=1, column=1, pady=2)
c1 = tk.Checkbutton(master, text="Preserve")
c1.grid(row=2, column=0, sticky=tk.W, columnspan=2)

# adding image (remember image should be PNG and not JPG)
pic_path = os.path.join(ex_root.script_loc, 'capture1.png')
picture = tk.PhotoImage(file=pic_path)
label = tk.Label(master, image=picture)
label.grid(row=0, column=2, columnspan=2, rowspan=2, padx=5, pady=5)

b1 = tk.Button(master, text="Zoom in")
b2 = tk.Button(master, text="Zoom out")
b1.grid(row=2, column=2, sticky=tk.E)
b2.grid(row=2, column=3, sticky=tk.E)

# Begin
# root.mainloop()
master.mainloop()

