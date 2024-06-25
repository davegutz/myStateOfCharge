#!/usr/bin/env python

"""
Python-Tail - Unix tail follow implementation in Python. 

python-tail can be used to monitor changes to a file.

Example:
    from MyTail import Tail

    # Create a tail instance
    t = tail.Tail('file-to-be-followed')

    # Register a callback function to be called when a new line is found in the followed file. 
    # If no callback function is registered, new lines would be printed to standard out.
    t.register_callback(callback_function)

    # Follow the file with 5 seconds as sleep time between iterations. 
    # If sleep time is not provided 1 second is used as the default time.
    t.follow(s=5) """

# Author - Kasun Herath <kasunh01 at gmail.com>
# Source - https://github.com/kasun/python-tail

import os
import sys
import tkinter.filedialog
from threading import Thread
from tkinter import simpledialog


class Tail(object):
    """Represents a tail command."""
    def __init__(self, tailed_file):
        """Initiate a Tail instance.
            Check for file validity, assigns callback function to standard out.
            
            Arguments:
                tailed_file - File to be followed. """

        self.check_file_validity(tailed_file)
        self.tailed_file = tailed_file
        self.callback = sys.stdout.write

    def follow(self, s=1, _str=None):
        """Do a tail follow. If a callback function is registered it is called with every new line. 
        Else printed to standard out.
    
        Arguments:
            s - Number of seconds to wait between each iteration; Defaults to 1. """

        with open(self.tailed_file) as file_:
            # Go to the end of file
            file_.seek(0, 2)
            while True:
                curr_position = file_.tell()
                line = file_.readline()
                if not line:
                    file_.seek(curr_position)
                    time.sleep(s)
                else:
                    if str is None:
                        self.callback(line)
                    else:
                        if line.__contains__(_str):
                            print(line)
                            # self.callback(line)

    def register_callback(self, func):
        """ Overrides default callback function to provided function. """
        self.callback = func

    @staticmethod
    def check_file_validity(_file):
        """ Check whether a given file exists, readable and is a file """
        if not os.access(_file, os.F_OK):
            raise TailError("File '%s' does not exist" % _file)
        if not os.access(_file, os.R_OK):
            raise TailError("File '%s' not readable" % _file)
        if os.path.isdir(_file):
            raise TailError("File '%s' is a directory" % _file)


class TailError(Exception):
    def __init__(self, msg):
        self.message = msg

    def __str__(self):
        return self.message


def enter_string():
    answer = tk.simpledialog.askstring(title=__file__, prompt="enter tail-f destination file",
                                       initialvalue=string.get())
    string.set(answer)
    string_butt.config(text=string.get())


def enter_tail_file_name():
    answer = tk.simpledialog.askstring(title=__file__, prompt="enter tail-f destination file",
                                       initialvalue=tail_file.get())
    tail_file.set(answer)
    tail_file_butt.config(text=tail_file.get())


def enter_folder():
    answer = tk.filedialog.askdirectory(title="Select a tail destination folder", initialdir=tail_path.get())
    if answer is not None and answer != '':
        tail_folder.set(answer)
    tail_path.set(os.path.join(tail_folder.get(), tail_file.get()))
    tail_path_butt.config(text=tail_path.get())
    out_path.set(tail_file.get() + '.tail')


def gen_data():
    # gen_temp_file(path_=path.get(), max_=max_len.get())
    thread = Thread(target=gen_temp_file, kwargs={'path_': path.get(), 'max_': max_len.get()})
    thread.start()


def gen_temp_file(path_=None, max_=100, trigger=10):
    if path_ is None:
        path_to_data = os.path.join(os.getcwd(), 'MyTail.txt')
    else:
        path_to_data = path_
    line_cnt = 0
    with open(path_to_data, "w") as output:
        while line_cnt < max_:
            time.sleep(2.)
            line_cnt += 1
            if line_cnt < trigger:
                output.write("{:d},  {:6.3f},\n".format(line_cnt, float(line_cnt)))
            else:
                output.write("{:s},  {:d},  {:6.3f},\n".format('STOP', line_cnt, float(line_cnt)))
            output.flush()


def watch_data():
    t = Tail(path.get())

    # Register a callback function to be called when a new line is found in the followed file.
    # If no callback function is registered, new lines would be printed to standard out.
    # t.register_callback(callback_function)

    # Follow the file with 5 seconds as sleep time between iterations.
    # If sleep time is not provided 1 second is used as the default time.
    thread = Thread(target=t.follow, kwargs={'s': 5})
    thread.start()


def watch_target():
    global of
    t = Tail(tail_path.get())
    t.register_callback(write_to_out_path)
    try:
        os.remove(out_path.get())
    except IOError:
        pass
    of = open(out_path.get(), 'a')

    # Register a callback function to be called when a new line is found in the followed file.
    # If no callback function is registered, new lines would be printed to standard out.
    # t.register_callback(callback_function)

    # Follow the file with 5 seconds as sleep time between iterations.
    # If sleep time is not provided 1 second is used as the default time.
    thread = Thread(target=t.follow, kwargs={'s': 1})
    thread.start()


def watch_target_string():
    global of
    t = Tail(tail_path.get())
    t.register_callback(write_to_out_path)
    try:
        os.remove(out_path.get())
    except IOError:
        pass
    of = open(out_path.get(), 'a')

    # Register a callback function to be called when a new line is found in the followed file.
    # If no callback function is registered, new lines would be printed to standard out.
    # t.register_callback(callback_function)

    # Follow the file with 5 seconds as sleep time between iterations.
    # If sleep time is not provided 1 second is used as the default time.
    thread = Thread(target=t.follow, kwargs={'s': 1, 'str': string.get()})
    thread.start()


def write_to_out_path(_input):
    global of
    of.write(_input)


if __name__ == '__main__':
    import tkinter as tk
    import time
    import platform

    if platform.system() == 'Darwin':
        # noinspection PyUnresolvedReferences
        from ttwidgets import TTButton as myButton
    else:
        import tkinter as tk
        from tkinter import Button as myButton
    bg_color = "lightgray"

    global of

    root = tk.Tk()
    root.title('tail -f')
    path = tk.StringVar(root, os.path.join(os.getcwd(), 'MyTail.txt'))
    max_len = tk.IntVar(root, 1000)
    tail_folder = tk.StringVar(root, os.getcwd())
    tail_file = tk.StringVar(root, 'putty_test.csv')
    tail_path = tk.StringVar(root, os.path.join(tail_folder.get(), tail_file.get()))
    out_path = tk.StringVar(root, tail_file.get() + '.tail')
    string = tk.StringVar(root, '<enter string to monitor>')
    myButton(root, text="monitor MyTail.txt", command=watch_data).grid(row=0, column=1)
    myButton(root, text="generate MyTail.txt", command=gen_data).grid(row=1, column=1)
    tk.Label(root, text='file name target of tail -f').grid(row=2, column=0)
    tail_file_butt = myButton(root, text=tail_file.get(), command=enter_tail_file_name)
    tail_file_butt.grid(row=2, column=1)
    tk.Label(root, text='change folder: ').grid(row=3, column=0)
    tail_path_butt = myButton(root, text=tail_path.get(), command=enter_folder, fg="blue", bg=bg_color)
    tail_path_butt.grid(row=3, column=1, columnspan=3)
    myButton(root, text="monitor target", command=watch_target).grid(row=5, column=1)
    tk.Label(root, text='string to monitor in target: ').grid(row=6, column=0)
    string_butt = myButton(root, text=string.get(), command=enter_string, fg="blue", bg=bg_color)
    string_butt.grid(row=6, column=1, columnspan=3)
    myButton(root, text="monitor target for string", command=watch_target_string).grid(row=7, column=1)
    root.mainloop()
