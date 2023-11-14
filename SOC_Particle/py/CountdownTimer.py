# CountdownTimer: class to display a non-blocking countdown timer
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

"""Raise a window visible at task bar to close all plots"""

import matplotlib.pyplot as plt
import tkinter as tk
from threading import Thread
import pyautogui
import time
import platform
if platform.system() == 'Darwin':
    from ttwidgets import TTButton as myButton
else:
    import tkinter as tk
    from tkinter import Button as myButton
bg_color = "lightgray"


class CountdownTimer(tk.Toplevel):
    def __init__(self,  time_, message, caller, exit_function=None):
        """Block caller task asking to close all plots then doing so"""
        tk.Toplevel.__init__(self)
        self.attributes('-topmost', True)
        self.initial_time = time_
        self.time = tk.IntVar(self, time_)
        self.label = tk.Label(self, text=caller + ' ' + message)
        self.label.grid(row=0, column=0)
        self.lift()
        self.button = myButton(self, command=self.begin, text="START " + str(time_) + " sec timer")
        self.button.grid(row=2, column=0)
        self.center()
        self.mainloop()
        # self.grab_set()  # Prevents other Tkinter windows from being used

    def center(self, width=200, height=100):
        # get screen width and height
        screen_width = root.winfo_screenwidth()
        screen_height = root.winfo_screenheight()

        # calculate position x and y coordinates
        x = (screen_width / 2) - (width / 2)
        y = (screen_height / 2) - (height / 2)
        self.geometry('%dx%d+%d+%d' % (width, height, x, y))

    def close_all(self):
        plt.close('all')
        # self.grab_release()
        self.destroy()

    def begin(self):
        """Countdown in seconds then exit"""
        thread = Thread(target=stay_awake, kwargs={'up_set_min': float(self.time.get()) / 60.})
        thread.start()
        self.lift()
        self.center()
        self.countdown()

    def countdown(self):
        """Countdown in seconds then exit"""
        msg = 'Counting down'
        self.time.set(self.time.get() - 1)
        self.label.config(text=f'{msg} ({self.time.get()}sec)')
        self.button.config(text='', fg=bg_color, bg=bg_color)
        if self.time.get() > 0:
            self.lift()
            self.center()
            self.after(1000, self.countdown)
        else:
            self.time.set(self.initial_time)
            self.label.config(text=f'{msg} ({self.time.get()}sec)')
            self.button.config(text='', fg=bg_color, bg=bg_color)
            if message_window is None:  # window is not busy
                message_start('flash')  # display message

    def flash_bang(self):
        while True:
            print("before flash")
            flash()
            print("after flash")
            self.after(10000)
            print("before bell")
            self.bell()
            print("end")


def flash():
    flasher = tk.Toplevel()
    flasher.geometry("300x250")
    flasher.configure(bg='red')
    flasher.deiconify()
    flasher.after(2000, flasher.destroy())  # set the flash time to 100 milliseconds
    flasher.mainloop()


# function which creates window with message
def message_start(text):
    global repeates
    global message_window
    global message_label

    repeates = 15  # how many times change background color

    # create window with messages
    message_window = tk.Toplevel()
    message_window.geometry("300x250")
    message_label = tk.Label(message_window, text=text, bg='red', fg='black')
    message_label.pack()

    # update window after 500ms
    root.after(500, message_update)


# function which changes background in displayed window
def message_update():
    global message_window
    global repeates

    if repeates > 0:
        repeates -= 1

        message_label['text'] = str(repeates)
        if message_label['bg'] == 'red':
            message_label['bg'] = 'white'
            message_window.configure(bg='white')
        else:
            message_label['bg'] = 'red'
            message_window.configure(bg='red')
            message_label['bg'] = 'red'

        # update window after 1000ms
        root.after(1000, message_update)
    else:
        # close window
        message_window.destroy()

        # inform `check_time` that window is not busy
        message_window = None


def show_countdown_timer(time_, message, caller, exit_function=None):
    CountdownTimer(time_, message, caller, exit_function)


def start():
    show_countdown_timer(5., "5 second test", 'show_countdown_timer')


def stay_awake(up_set_min=3.):
    """Keep computer awake using shift key when recording then return to previous state"""

    # Timer starts
    start_time = float(time.time())
    up_time_min = 0.0
    # FAILSAFE to FALSE feature is enabled by default so that you can easily stop execution of
    # your pyautogui program by manually moving the mouse to the upper left corner of the screen.
    # Once the mouse is in this location, pyautogui will throw an exception and exit.
    pyautogui.FAILSAFE = False
    while True and (up_time_min < up_set_min):
        time.sleep(30.)
        for i in range(0, 3):
            pyautogui.press('shift')  # Shift key does not disturb fullscreen
        up_time_min = (time.time() - start_time) / 60.
        print(f"stay_awake: {up_time_min=}")
    print(f"stay_awake: ending\n")


if __name__ == '__main__':
    message_window = None
    repeates = 3
    root = tk.Tk()
    tk.Label(root, text="Try timer variations").pack()
    tk.Button(root, text="Timer", command=start).pack()
    root.mainloop()
