# PlotKiller: class called after plt.show to block and take input from user to then close all plots.  Viewable
# on taskbar.
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

"""
Raise a window visible at task bar to close all plots.
  *** Call ion before show in caller, or use 'show_and_kill()' instead of 'plt.show(); show_killer();
  Sometimes works without ion.  (Race condition in IPC)
  https://stackoverflow.com/questions/28269157/plotting-in-a-non-blocking-way-with-matplotlib
  *** sometimes need to send list of figures to prevent close('all') from closing all.  (Again, race condition in IPC)
"""

import matplotlib.pyplot as plt
import time
import platform
if platform.system() == 'Darwin':
    from ttwidgets import TTButton as myButton
else:
    import tkinter as tk
    from tkinter import Button as myButton
bg_color = "lightgray"


class PlotKiller(tk.Toplevel):
    def __init__(self, message, caller, fig_list_=None):
        """Block caller task asking to close all plots then doing so"""
        self.fig_list = fig_list_
        tk.Toplevel.__init__(self)
        tk.Button(self, command=self.close_figs, text=caller + ": close " + message, font=("Courier", 12)).grid(row=0, column=0, padx=15, pady=15)
        self.lift()
        self.mainloop()
        # self.grab_set()  # Prevents other Tkinter windows from being used

    def close_figs(self):
        if self.fig_list is None:
            plt.close('all')
        else:
            for fig in self.fig_list:
                plt.close(fig)
        # self.grab_release()
        self.destroy()


def show_and_kill(string, caller, fig_list=None):
    plt.show()
    time.sleep(1)
    PlotKiller(string, caller, fig_list)


def show_killer(string, caller, fig_list=None):
    PlotKiller(string, caller, fig_list)


def simple_plot1():
    import numpy as np
    fig_list = []
    t = np.arange(0.0, 2.0, 0.01)
    s = 1 + np.sin(2 * np.pi * t)
    fig, ax = plt.subplots()
    fig_list.append(fig)
    ax.plot(t, s)
    ax.set(xlabel='time (s)', ylabel='voltage (mV)',
           title='Sine wave1')
    ax.grid()
    fig, ax = plt.subplots()
    fig_list.append(fig)
    ax.plot(t, s)
    ax.set(xlabel='time (s)', ylabel='voltage (mV)',
           title='Sine wave2')
    ax.grid()
    plt.ion()
    plt.show()
    show_killer('close plots?', 'sp1', fig_list)


def simple_plot2():
    import numpy as np
    t = np.arange(0.0, 2.0, 0.01)
    s = 1 + np.sin(2 * np.pi * t)
    fig, ax = plt.subplots()
    ax.plot(t, s)
    ax.set(xlabel='time (s)', ylabel='voltage (mV)',
           title='Sine wave1')
    ax.grid()
    fig, ax = plt.subplots()
    ax.plot(t, s)
    ax.set(xlabel='time (s)', ylabel='voltage (mV)',
           title='Sine wave2')
    ax.grid()
    plt.ion()
    # plt.show()
    show_and_kill('close plots?', 'sp2')


if __name__ == '__main__':
    root = tk.Tk()
    tk.Label(root, text="Try opening multiple plots then killing").pack()
    tk.Button(root, text="plot 1", command=simple_plot1).pack()
    tk.Button(root, text="plot 2", command=simple_plot2).pack()
    root.mainloop()
