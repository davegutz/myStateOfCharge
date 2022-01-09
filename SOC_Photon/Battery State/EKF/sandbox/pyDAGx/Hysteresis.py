# Hysteresis class to model battery charging / discharge hysteresis
# Copyright (C) 2022 Dave Gutz
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

__author__ = 'Dave Gutz <davegutz@alum.mit.edu>'
__version__ = '$Revision: 1.1 $'
__date__ = '$Date: 2022/01/08 13:15:02 $'

import numpy as np
from pyDAGx.lookup_table import LookupTable

class Hysteresis():
    # Use variable resistor to create hysteresis from an RC circuit

    def __init__(self, t_dv=None, t_soc=None, t_r=None, cap=1.8e6):
        # Defaults
        if t_dv is None:
            t_dv = [-10, -0.08, -1e-6, 0., 0.08, 10]
        if t_soc is None:
            t_soc = [-10, .1, .5, 1, 10]
        if t_r is None:
            t_r = [1e-7, 1e-7, 0.006, 0.003, 1e-7, 1e-7,
                   1e-7, 1e-7, 0.006, 0.003, 1e-7, 1e-7,
                   1e-7, 1e-7, 0.006, 0.003, 1e-7, 1e-7,
                   1e-7, 1e-7, 0.006, 0.003, 1e-7, 1e-7,
                   1e-7, 1e-7, 0.006, 0.003, 1e-7, 1e-7]
        self.lut = LookupTable()
        self.lut.addAxis('x', t_dv)
        self.lut.addAxis('y', t_soc)
        self.lut.setValueTable(t_r)
        self.cap = cap
        self.res = 0.
        self.soc = 0.
        self.ib = 0.
        self.ioc = 0.
        self.voc_stat = 0.
        self.voc = 0.
        self.dv = 0.
        self.dv_dot = 0.
        self.saved = Saved()

    def __str__(self):
        s =  "Hysteresis:\n"
        s += "  ib       =    {:7.3f}  // Current in, A\n".format(self.ib)
        s += "  ioc      =    {:7.3f}  // Current out, A\n".format(self.ioc)
        s += "  voc_stat =    {:7.3f}  // Battery model voltage input, V\n".format(self.voc_stat)
        s += "  voc      =    {:7.3f}  // Discharge voltage output, V\n".format(self.voc)
        s += "  soc      =    {:7.3f}  // State of charge input, dimensionless\n".format(self.soc)
        s += "  res =    {:7.3f}  // Variable resistance value, ohms\n".format(self.res)
        s += "  dv_dot = {:7.3f}  // Calculated voltage rate, V/s\n".format(self.dv_dot)
        s += "  dv =     {:7.3f}  // Delta voltage state, V\n".format(self.dv)
        s += "\n"
        return s

    def calculate(self, ib, voc, soc):
        self.ib = ib
        self.voc_stat = voc
        self.soc = soc
        self.res = self.look(self.dv, self.soc)
        self.ioc = self.dv / self.res
        self.dv_dot = -self.dv / self.res / self.cap + (self.ib - self.ioc) / self.cap

    def update(self, dt):
        self.dv += self.dv_dot * dt
        self.voc = self.voc_stat + self.dv

    def init(self, dv_init):
        self.dv = dv_init

    def look(self, dv, soc):
        self.res = self.lut.lookup(x=dv, y=soc)
        return self.res

    def save(self, time):
        self.saved.time.append(time)
        self.saved.soc.append(self.soc)
        self.saved.res.append(self.res)
        self.saved.dv.append(self.dv)
        self.saved.dv_dot.append(self.dv_dot)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.voc_stat.append(self.voc_stat)
        self.saved.voc.append(self.voc)


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.dv = []
        self.dv_dot = []
        self.res = []
        self.soc = []
        self.ib = []
        self.ioc = []
        self.voc = []
        self.voc_stat = []


if __name__ == '__main__':
    import sys
    import doctest
    from datetime import datetime
    from unite_pictures import unite_pictures_into_pdf
    import os

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt

    def overall(hys=Hysteresis().saved, filename='', fig_files=None, plot_title=None, n_fig=None, ref=None):
        if fig_files is None:
            fig_files = []
        if ref is None:
            ref = []

        plt.figure()
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title)
        plt.plot(hys.time, hys.soc, color='red', label='soc')
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(hys.time, hys.res, color='blue', label='res, Ohm')
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(hys.time, hys.ib, color='blue', label='ib, A')
        plt.plot(hys.time, hys.ioc, color='red', label='ioc, A')
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(hys.time, hys.dv, color='blue', label='dv, V')
        plt.legend(loc=1)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files


    def main():
        # Setup to run the transients
        dt = 0.1
        # time_end = 2
        time_end = 35000

        hys = Hysteresis()

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        current_in = 0
        current_in_s = []

        # time loop
        for i in range(len(t)):
            if t[i] < 10000:
                current_in = 20
            elif t[i] < 20000:
                current_in = -20
            init_ekf = (t[i] <= 1)

            if init_ekf:
                hys.init(0.0)

            # Models
            hys.calculate(ib=current_in, voc=0., soc=0.5)
            hys.update(dt=dt)

            # Plot stuff
            current_in_s.append(current_in)
            hys.save(t[i])

        # Data
        print('hys:  ', str(hys))

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        n_fig, fig_files = overall(hys.saved, filename, fig_files, plot_title=plot_title, n_fig=n_fig, ref=current_in_s)
        plt.show()

    main()
