# query_close_plots: run after plt.show to hold off return to tk mainloop and close all plots at once
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

""" Python model of what's installed on the Particle Photon.  Includes
a monitor object (MON) and a simulation object (SIM).   The monitor is
the EKF and Coulomb Counter.   The SIM is a battery model, that also has a
Coulomb Counter built in."""

import matplotlib.pyplot as plt
import tkinter as tk
import tkinter.messagebox


def query_close_plots(caller=''):
    """Recursively keep asking to close plots until positive"""
    confirmation = tk.messagebox.askyesno(caller + 'query close plots', 'close plots?')
    if confirmation is True:
        plt.close('all')
    else:
        query_close_plots()
