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
    def __init__(self, message, caller):
        """Recursively keep asking to close plots until positive"""
        tk.Toplevel.__init__(self)
        self.label = tk.Label(self, text=caller + message)
        self.label.grid(row=0, column=0)
        self.lift()  # Puts Window on top
        tk.Button(self, command=self.close_all, text="Close All").grid(row=1, column=0)
        self.grab_set()  # Prevents other Tkinter windows from being used

    def close_all(self):
        plt.close('all')
        self.destroy()


def show_killer(string, caller):
    PlotKiller(string, caller)
