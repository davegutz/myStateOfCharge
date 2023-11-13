# MonSim:  Monitor and Simulator replication of Particle Photon Application
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


class PlotKiller(tk.Toplevel):
    def __init__(self, message, caller, fig_list_=None):
        """Recursively keep asking to close plots until positive"""
        self.fig_list = fig_list_
        tk.Toplevel.__init__(self)
        self.label = tk.Label(self, text=caller + ' ' + message)
        self.label.grid(row=0, column=0)
        tk.Button(self, command=self.close_all, text="Close Figs " + caller).grid(row=2, column=0)
        self.lift()
        self.mainloop()
        # self.grab_set()  # Prevents other Tkinter windows from being used

    def close_all(self):
        if self.fig_list is None:
            plt.close('all')
        else:
            for fig in self.fig_list:
                plt.close(fig)
        # self.grab_release()
        self.destroy()


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
    plt.show()
    show_killer('close plots?', 'sp1', fig_list)


def simple_plot2():
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
    plt.show()
    show_killer('close plots?', 'sp2', fig_list)


if __name__ == '__main__':
    root = tk.Tk()
    tk.Label(root, text="Try opening multiple plots then killing").pack()
    tk.Button(root, text="plot 1", command=simple_plot1).pack()
    tk.Button(root, text="plot 2", command=simple_plot2).pack()
    root.mainloop()
