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

import tkinter as tk
from threading import Thread
from pynput.keyboard import Key, Controller
import time
import platform
if platform.system() == 'Darwin':
    # noinspection PyUnresolvedReferences
    from ttwidgets import TTButton as myButton  # Need for macOS - ignore warning
else:
    import tkinter as tk
    from tkinter import Button as myButton
bg_color = "lightgray"


class CountdownTimer(tk.Toplevel):
    def __init__(self,  root_, time_, max_flash=30, exit_function=None, trigger=False):
        """Block caller task asking to close all plots then doing so"""
        tk.Toplevel.__init__(self)
        self.root = root_
        self.flasher_window = None
        self.flasher_label = None
        self.max_flashes = int(max_flash)
        self.flashes = 0
        self.attributes('-topmost', True)
        self.initial_time = time_
        self.time = tk.IntVar(self, time_ + 1)
        self.lift()
        self.button = myButton(self, command=self.begin, text="START " + str(time_) + " sec timer")
        self.button.pack(side='top', fill='x')
        # self.center()
        self.exit_function = exit_function
        self.trigger = trigger
        if self.trigger:
            self.begin()
        self.mainloop()
        # self.grab_set()  # Prevents other Tkinter windows from being used

    def center(self, width=200, height=150):
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()

        # calculate position x and y coordinates
        x = (screen_width / 2) - (width / 2)
        y = (screen_height / 2) - (height / 2)
        self.geometry('%dx%d+%d+%d' % (width, height, x, y))

    def close(self):
        self.destroy()

    def begin(self):
        """Countdown in seconds then exit"""
        if self.trigger:
            self.trigger = False
            self.after(1000, self.begin)
            self.button.config(text='wait')
            return
        thread = Thread(target=stay_awake, kwargs={'up_set_min': float(self.time.get()) / 60.})
        thread.start()
        self.lift()
        # self.center()
        self.countdown()

    def countdown(self):
        """Countdown in seconds then exit"""
        self.time.set(self.time.get() - 1)
        self.button.config(text=str(self.time.get()), fg='black', bg=bg_color, font=("Courier", 96))
        if self.time.get() > 0:
            self.lift()
            # self.center()
            self.after(1000, self.countdown)
        else:
            self.time.set(self.initial_time)
            self.button.config(text=str(self.initial_time), fg='white', bg=bg_color, font=("Courier", 96))
            if self.exit_function is not None:
                self.exit_function()
            if self.flasher_window is None:  # window is not busy
                self.flasher_start('0')  # display message

    def flasher_start(self, text):
        """function which creates window with message"""
        # create window with messages
        self.flasher_window = tk.Toplevel()
        self.flasher_window.geometry("300x200")
        self.flasher_label = tk.Label(self.flasher_window, text=text, bg='red', fg='black', font=("Courier", 96))
        self.flasher_label.pack(side='bottom')
        self.flasher_window.configure(bg='red')
        self.bell()

        # update window after 500ms
        self.after(500, self.flasher_update)

    def flasher_update(self):
        """function which changes background in displayed window"""
        if self.flashes < self.max_flashes:
            if self.flasher_label['bg'] == 'red':
                self.flasher_label['bg'] = 'white'
                self.flasher_window.configure(bg='white')
            else:
                self.flashes += 1
                self.flasher_label['text'] = str(self.flashes)
                self.flasher_label['bg'] = 'red'
                self.flasher_window.configure(bg='red')

            # update window
            self.after(500, self.flasher_update)
        else:
            self.flasher_window.destroy()
            self.destroy()


def start_timer():
    CountdownTimer(root, 5, max_flash=5, exit_function=None, trigger=True)


def stay_awake(up_set_min=3.):
    """Keep computer awake using shift key when recording then return to previous state"""

    # Timer starts
    start_time = float(time.time())
    up_time_min = 0.0
    while True and (up_time_min < up_set_min):
        time.sleep(30.)
        keyboard = Controller()
        for i in range(0, 3):
            keyboard.press(Key.shift)  # Shift key does not disturb fullscreen
        up_time_min = (time.time() - start_time) / 60.
        print(f"stay_awake: {up_time_min=}")
    print(f"stay_awake: ending\n")


if __name__ == '__main__':
    root = tk.Tk()
    tk.Label(root, text="Try timer variations").pack()
    tk.Button(root, text="Timer", command=start_timer).pack()
    root.mainloop()
