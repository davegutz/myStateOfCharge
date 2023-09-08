#  Utilities for TestSOC
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
import sys
import json
import inspect
import subprocess
import configparser
import pkg_resources
from Colors import Colors
from mbox import MessageBox
from typing import Callable, TextIO


def str2bool(string):
    str2val = {"True": True, "False": False}
    if string in str2val:
        return str2val[string]
    else:
        raise ValueError(f"Expected one of {set(str2val.keys())}, got {string}")


def optional_int(string):
    return None if string == "None" else int(string)


def optional_float(string):
    return None if string == "None" else float(string)


system_encoding = sys.getdefaultencoding()
if system_encoding != "utf-8":
    def make_safe(string):
        # replaces any character not representable using the system default encoding with an '?',
        # avoiding UnicodeEncodeError (https://github.com/openai/whisper/discussions/729).
        return string.encode(system_encoding, errors="replace").decode(system_encoding)
else:
    def make_safe(string):
        # utf-8 can encode any Unicode code point, so no need to do the round-trip encoding
        return string


# Make the installation as easy as possible
# Assume that we're either starting with a good python installation
#   or
# a working PyCharm installation
def check_install(platform, pure_python=True):
    print("checking for dependencies...", end='')

    # Check status
    if platform == 'Darwin' or platform == 'Windows' or platform == 'Linux':
        (have_python, have_pip) = check_install_python(platform)
        print('')

        # python help
        if not have_python:
            python_help(platform)

        # pip help
        if not have_pip:
            pip_help(platform)

        # All good
        # #########Interim don't worry about macOS
        # If we have_python and have_pip:
        if have_python and have_pip:
            return 0
        else:
            return -1
    else:
        print(Colors.fg.red, "platform '", platform, "' unknown.   Contact your administrator", Colors.reset)
        return -1


def check_install_pkg(pkg, verbose=False):
    if verbose:
        print("checking for {:s}...".format(pkg), end='')
    installed_packages = pkg_resources.working_set
    installed_packages_list = sorted(["%s" % i.key for i in installed_packages])
    return installed_packages_list.__contains__(pkg)


# Check installation status of python
def check_install_python(platform, verbose=False):
    have_python = False
    have_pip = False
    if platform == 'Darwin':
        test_cmd_python = 'python3 --version'
        test_cmd_pip = 'python3 -m pip --version'
    elif platform == 'Windows':
        test_cmd_python = 'python --version'
        test_cmd_pip = 'python -m pip --version'
    elif platform == 'Linux':
        test_cmd_python = 'python3 --version'
        test_cmd_pip = 'python3 -m pip --version'
    else:
        raise Exception('platform unknown.   Contact your administrator')
    if verbose:
        print('')
        print("checking for {:s}...".format(test_cmd_python), end='')
    result = run_shell_cmd(test_cmd_python, silent=True, save_stdout=True)
    if result == -1:
        print('failed')
    else:
        ver = result[0].split('\n')[0].split(' ')[1]
        ver_no = int(ver.split('.')[0])
        rel_no = int(ver.split('.')[1])
        if result == -1 or ver_no < 3 or rel_no < 6:
            print(Colors.fg.red, 'failed')
            if ver_no < 3:
                print("System '", test_cmd_python, "' command points to version<3.  whisper needs 3", Colors.reset)
            if rel_no < 6:
                print("System '", test_cmd_python, "' command points to release<6.  whisper needs >=6", Colors.reset)
        else:
            have_python = True
            print('success')
    if verbose:
        print("checking for {:s}...".format(test_cmd_pip), end='')
    result = run_shell_cmd(test_cmd_pip, silent=True, save_stdout=True)
    if result == -1:
        print(Colors.fg.red, 'failed', Colors.reset)
    else:
        have_pip = True
        if verbose:
            print('success')
    return have_python, have_pip


# Config helper function
def config_section_map(config, section):
    dict1 = {}
    options = config.options(section)
    for option in options:
        try:
            dict1[option] = config.get(section, option)
            if dict1[option] == -1:
                print("skip: %s" % option)
        except AttributeError:
            print("exception on %s!" % option)
            dict1[option] = None
    return dict1


# Work out all the paths
def configurator(filepath):
    (config_path, config_basename) = os.path.split(filepath)
    config_file_path = os.path.join(config_path, 'TestSOC.ini')
    config = load_config(config_file_path)
    return config_path, config_basename, config_file_path, config


# Open text file in editor
def display_result(txt_path, platform, silent):
    if silent is False:
        if platform == 'Darwin':
            subprocess.Popen(['open', '-a', 'TextEdit', txt_path])

        if platform == 'Linux':
            subprocess.Popen(['gedit', txt_path])

        elif platform == 'Windows':
            subprocess.Popen(['notepad', txt_path])
    else:
        print('Results in', txt_path)


# Config file
def load_config(path):
    config = configparser.ConfigParser()
    if os.path.isfile(path):
        config.read(path)
    else:
        with open(path, 'w') as cfg_file:
            config.add_section('Base')
            config.set('Base', 'version remark',
                       'Format:  vYYYYMMDD')
            config.set('Base', 'version', 'v20230305')
            config.set('Base', 'processor remark',
                       'Possible values:  P, A, P2')
            config.set('Base', 'processor', 'A')
            config.set('Base', 'key remark',
                       'Format:  unit#proc.  The leading key in v1 output set by Particle Workbench config.h')
            config.set('Base', 'key', 'pro1a')
            config.set('Base', 'battery remark',
                       'Possible values: BB, CH')
            config.set('Base', 'battery', 'CH')
            config.add_section('Test')
            config.set('Test', 'version remark',
                       'Format:  vYYYYMMDD')
            config.set('Test', 'version', 'v20230515')
            config.set('Test', 'processor remark',
                       'Possible values:  P, A, P2')
            config.set('Test', 'processor', 'A')
            config.set('Test', 'key remark',
                       'Format:  unit#proc.  The leading key in v1 output set by Particle Workbench config.h')
            config.set('Test', 'key', 'pro1a')
            config.set('Test', 'battery remark',
                       'Possible values: BB, CH')
            config.set('Test', 'battery', 'CH')
            config.write(cfg_file)
    return config


# Help for pip install
def pip_help(platform):
    # windows
    if platform == 'Windows':
        print(inspect.cleandoc("""
            #############  Once python installed:
            python -m pip install --upgrade pip
            pip install configparser
            """), Colors.reset, sep=os.linesep)
    elif platform == 'Linux':
        python_help(platform)


# Help for python install
def python_help(platform):
    # Windows
    if platform == 'Windows':
        print(Colors.fg.green, inspect.cleandoc("""
            #############  Install python3.6+ and check path 'python --version' points to it")
            # go to:  python.org/download"
            python -m pip install --upgrade pip
            python -m pip install configparser
            """), Colors.reset, sep=os.linesep)
    # macOS
    if platform == 'Darwin':
        print(Colors.fg.green, inspect.cleandoc("""
            #############  Install python3.10.10 and check path 'python3 --version' points to it")
            # go to:  python.org/download"
            python3 -m pip install --upgrade pip
            python3 -m pip install configparser
            python3 certifi_glob.py
            """), Colors.reset, sep=os.linesep)
    # Linux
    elif platform == 'Linux':
        print(Colors.fg.green, inspect.cleandoc("""
            #############  Install python3.6+ and check path 'python --version' points to it")
            sudo apt update && sudo apt upgrade
            sudo apt install python3.10
            sudo python3.10 -m pip install upgrade
            pip3 install configparser
            pip3 install shortcuts
            """), Colors.reset, sep=os.linesep)


# Run shell command showing stdout progress (special logic for Windows)
# Hope it works on Mac and Linux
def run_shell_cmd(cmd, silent=False, save_stdout=False, colorize=False):
    stdout_line = None
    if save_stdout:
        stdout_line = []
    if colorize:
        print(Colors.bg.brightblack, Colors.fg.wheat)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            bufsize=1, universal_newlines=True)
    # Poll process for new output until finished
    while True:
        try:
            nextline = proc.stdout.readline()
        except AttributeError:
            nextline = ''
        if nextline == '' and proc.poll() is not None:
            break
        if save_stdout:
            stdout_line.append(nextline)
        if not silent:
            sys.stdout.write(nextline)
            sys.stdout.flush()
    if colorize:
        print(Colors.reset)
    if save_stdout and not silent:
        print('stdout', stdout_line)
    output = proc.communicate()[0]
    exit_code = proc.returncode
    if exit_code == 0:
        if save_stdout:
            return stdout_line
        else:
            return output
    else:
        return -1


def overwrite_query(msg, b1=('yes', 'yes'), b2=('no', 'no'), b3=('all', 'all'), b4=('none', 'none'),
                    b5=('exit', 'exit'), frame_=True, t=False, entry=False):
    """Create an instance of MessageBox, and get data back from the user.
    msg = string to be displayed
    b1 = text for left button, or a tuple (<text for button>, <to return on press>)
    b2 = text for right button, or a tuple (<text for button>, <to return on press>)
    frame = include a standard outerframe: True or False
    t = time in seconds (int or float) until the msgbox automatically closes
    entry = include an entry widget that will have its contents returned: True or False
    """
    msgbox = MessageBox(msg, b1, b2, b3, b4, b5, frame_, t, entry)
    msgbox.root.mainloop()
    # the function pauses here until the mainloop is quit
    msgbox.root.withdraw()
    msgbox.root.destroy()
    return msgbox.returning
