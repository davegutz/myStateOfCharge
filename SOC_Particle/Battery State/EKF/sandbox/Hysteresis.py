# Hysteresis class to model battery charging / discharge hysteresis
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

__author__ = 'Dave Gutz <davegutz@alum.mit.edu>'
__version__ = '$Revision: 1.1 $'
__date__ = '$Date: 2022/05/30 13:15:02 $'

import numpy as np
from pyDAGx.lookup_table import LookupTable
from unite_pictures import cleanup_fig_files

class Hysteresis:
    # Use variable resistor to create hysteresis from an RC circuit

    def __init__(self, t_dv=None, t_soc=None, t_r=None, cap=3.6e5, scale=1.):
        # Defaults
        self.reverse = scale<0.0
        if self.reverse:
            scale = -scale
        if t_dv is None:
            t_dv = [-0.9, -0.7,     -0.5,   -0.3,   0.0,    0.3,    0.5,    0.7,    0.9]
        if t_soc is None:
            t_soc = [0, .5, 1]
        if t_r is None:
            t_r = [ 1e-6, 0.064,    0.050,  0.036,  0.015,  0.024,  0.030,  0.046,  1e-6,
                    1e-6, 1e-6,     0.050,  0.036,  0.015,  0.024,  0.030,  1e-6,   1e-6,
                    1e-6, 1e-6,     1e-6,   0.036,  0.015,  0.024,  1e-6,   1e-6,   1e-6]
        for i in range(len(t_dv)):
            t_dv[i] *= scale
            t_r[i] *= scale
        self.disabled = scale<1e-5
        self.lut = LookupTable()
        self.lut.addAxis('x', t_dv)
        self.lut.addAxis('y', t_soc)
        self.lut.setValueTable(t_r)
        if self.disabled:
            self.cap = cap
        else:
            self.cap = cap / scale  # maintain time constant = R*C
        self.res = 0.
        self.soc = 0.
        self.ib = 0.
        self.ioc = 0.
        self.voc_stat = 0.
        self.voc = 0.
        self.dv_hys = 0.
        self.dv_dot = 0.
        self.saved = Saved()

    def __str__(self, prefix=''):
        s = prefix + "Hysteresis:\n"
        res = self.look_hys(dv=0., soc=0.8)
        s += "  res(median) =  {:6.4f}  // Null resistance, Ohms\n".format(res)
        s += "  cap      = {:10.1f}  // Capacitance, Farads\n".format(self.cap)
        s += "  tau      = {:10.1f}  // Null time constant, sec\n".format(res*self.cap)
        s += "  ib       =    {:7.3f}  // Current in, A\n".format(self.ib)
        s += "  ioc      =    {:7.3f}  // Current out, A\n".format(self.ioc)
        s += "  voc_stat =    {:7.3f}  // Battery model voltage input, V\n".format(self.voc_stat)
        s += "  voc      =    {:7.3f}  // Discharge voltage output, V\n".format(self.voc)
        s += "  soc      =   {:8.4f}  // State of charge input, dimensionless\n".format(self.soc)
        s += "  res      =    {:7.3f}  // Variable resistance value, ohms\n".format(self.res)
        s += "  dv_dot   =    {:7.3f}  // Calculated voltage rate, V/s\n".format(self.dv_dot)
        s += "  dv_hys   =    {:7.3f}  // Delta voltage state, V\n".format(self.dv_hys)
        s += "  disabled =     {:2.0f}      // Hysteresis disabled by low scale input < 1e-5, T=disabled\n".format(self.disabled)
        s += "  reverse  =     {:2.0f}      // If hysteresis hooked up backwards, T=reversed\n".format(self.reverse)
        return s

    def calculate_hys(self, ib, voc_stat, soc):
        self.ib = ib
        self.voc_stat = voc_stat
        self.soc = soc
        if self.disabled:
            self.res = 0.
            self.ioc = ib
            self.dv_dot = 0.
        else:
            self.res = self.look_hys(self.dv_hys, self.soc)
            self.ioc = self.dv_hys / self.res
            self.dv_dot = (self.ib - self.dv_hys/self.res) / self.cap

    def init(self, dv_init):
        self.dv_hys = dv_init

    def look_hys(self, dv, soc):
        if self.disabled:
            self.res = 0.
        else:
            self.res = self.lut.lookup(x=dv, y=soc)
        return self.res

    def save(self, time):
        self.saved.time.append(time)
        self.saved.soc.append(self.soc)
        self.saved.res.append(self.res)
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dot.append(self.dv_dot)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.voc_stat.append(self.voc_stat)
        self.saved.voc.append(self.voc)

    def update(self, dt):
        self.dv_hys += self.dv_dot * dt
        if self.reverse:
            self.voc = self.voc_stat - self.dv_hys
        else:
            self.voc = self.voc_stat + self.dv_hys
        return self.voc


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.dv_hys = []
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
        plt.legend(loc=3)
        plt.subplot(222)
        plt.plot(hys.time, hys.res, color='black', label='res, Ohm')
        plt.legend(loc=3)
        plt.subplot(223)
        plt.plot(hys.time, hys.ib, color='blue', label='ib, A')
        plt.plot(hys.time, hys.ioc, color='green', label='ioc, A')
        plt.legend(loc=2)
        plt.subplot(224)
        plt.plot(hys.time, hys.dv_hys, color='red', label='dv_hys, V')
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
        plt.subplot(111)
        plt.title(plot_title)
        plt.plot(hys.soc, hys.voc, color='red', label='voc vs soc')
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files


    class Pulsar:
        def __init__(self):
            self.time_last_hold = 0.
            self.time_last_rest = -100000.
            self.holding = False
            self.resting = True
            self.index = -1
            self.amp = [100., 0., -100., -100., -100., -100., -100., -100., -100., -100., -100., -100.,
                        100., 100., 100., 100., 100., 100., 100., 100., 100., 100.]
            self.dur = [16000., 0., 600., 600., 600., 600., 600., 600., 600., 600., 600., 600.,
                        600., 600., 600., 600., 600., 600., 600., 600., 600., 600.]
            self.rst = [600., 7200., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 7200.,
                        3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 46800.]
            self.pulse_value = self.amp[0]
            self.end_time = self.time_end()

        def calculate(self, time):
            if self.resting and time >= self.time_last_rest + self.rst[self.index]:
                if time < self.end_time:
                    self.index += 1
                self.resting = False
                self.holding = True
                self.time_last_hold = time
                self.pulse_value = self.amp[self.index]
            elif self.holding and time >= self.time_last_hold + self.dur[self.index]:
                self.index += 0  # only advance after resting
                self.resting = True
                self.holding = False
                self.time_last_rest = time
                self.pulse_value = 0.
            return self.pulse_value

        def time_end(self):
            time = 0
            for du in self.dur:
                time += du
            for rs in self.rst:
                time += rs
            return time


    def main():
        # Setup to run the transients
        dt = 10
        # time_end = 2
        # time_end = 500000
        pull = Pulsar()
        time_end = pull.time_end()

        hys = Hysteresis()

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        soc = 0.2
        current_in_s = []

        # time loop
        for i in range(len(t)):
            if t[i] < 10000:
                current_in = 0
            elif t[i] < 20000:
                current_in = 40
            elif t[i] < 30000:
                current_in = -40
            elif t[i] < 80000:
                current_in = 8
            elif t[i] < 130000:
                current_in = -8
            elif t[i] < 330000:
                current_in = 2
            elif t[i] < 440000:
                current_in = -2
            else:
                current_in = 0

            current_in = pull.calculate(t[i])

            init_ekf = (t[i] <= 1)
            if init_ekf:
                hys.init(0.0)

            # Models
            soc = min(max(soc + current_in / 100. * dt / 20000., 0.), 1.)
            voc_stat = 13. + (soc - 0.5)
            hys.calculate_hys(ib=current_in, voc_stat=voc_stat, soc=soc)
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

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')
        cleanup_fig_files(fig_files)
        plt.show()

    main()
