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
from threading import Thread


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

    def follow(self, s=1):
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
                    self.callback(line)

    def register_callback(self, func):
        """ Overrides default callback function to provided function. """
        self.callback = func

    def check_file_validity(self, file_):
        """ Check whether a given file exists, readable and is a file """
        if not os.access(file_, os.F_OK):
            raise TailError("File '%s' does not exist" % file_)
        if not os.access(file_, os.R_OK):
            raise TailError("File '%s' not readable" % file_)
        if os.path.isdir(file_):
            raise TailError("File '%s' is a directory" % file_)


class TailError(Exception):
    def __init__(self, msg):
        self.message = msg

    def __str__(self):
        return self.message


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


if __name__ == '__main__':
    import tkinter as tk
    import time
    import platform

    if platform.system() == 'Darwin':
        from ttwidgets import TTButton as myButton
    else:
        import tkinter as tk
        from tkinter import Button as myButton
    bg_color = "lightgray"

    root = tk.Tk()
    path = tk.StringVar(root, os.path.join(os.getcwd(), 'MyTail.txt'))
    max_len = tk.IntVar(root, 1000)
    tk.Label(root, text="tail -f").pack()
    myButton(root, text="start monitoring", command=watch_data).pack()
    myButton(root, text="generate MyTail.txt", command=gen_data).pack()
    root.mainloop()
